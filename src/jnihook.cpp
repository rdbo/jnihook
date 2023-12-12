/*
 *  -----------------------------------
 * |         JNIHook - by rdbo         |
 * |      Java VM Hooking Library      |
 *  -----------------------------------
 */

/*
 * Copyright (C) 2023    Rdbo
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License version 3
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <jnihook.h>
#include <jvmti.h>
#include "jvm.hpp"
#include <iostream>

extern "C" VMStructEntry *gHotSpotVMStructs;
extern "C" VMTypeEntry *gHotSpotVMTypes;
extern "C" void jnihook_gateway();

typedef struct {
	jnihook_callback_t callback;
	void *_i2i_entry;
	void *_from_interpreted_entry;
	// void *_from_compiled_entry;
	void *arg;
} jniHookInfo;

static std::unordered_map<void *, jniHookInfo> jniHookTable;
static JavaVM *jvm = nullptr;
static jvmtiEnv *jvmti = nullptr;

extern "C" JNIHOOK_API jvalue JNIHook_CallHandler(void *methodAddr, void *senderSP, void *thread)
{
	std::cout << "CALL HANDLER CALLED!" << std::endl;

	auto hkEntry = jniHookTable[methodAddr];

	std::cout << "hkEntry arg: " << hkEntry.arg << std::endl;

	auto method = VMType::from_instance("Method", methodAddr).value();
	void **_i2i_entry = method.get_field<void *>("_i2i_entry").value();
	void **_from_interpreted_entry = method.get_field<void *>("_from_interpreted_entry").value();

	std::cout << "restoring original Method to allow for hook call" << std::endl;
	*_i2i_entry = hkEntry._i2i_entry;
	*_from_interpreted_entry = hkEntry._from_interpreted_entry;

	std::cout << "calling callback..." << std::endl;
	jvalue call_result = hkEntry.callback((jmethodID)&methodAddr, senderSP, 0, thread, hkEntry.arg);

	std::cout << "rehooking method" << std::endl;
	*_i2i_entry = (void *)jnihook_gateway;
	*_from_interpreted_entry = (void *)jnihook_gateway;

	return call_result;
}

extern "C" JNIHOOK_API jint JNIHook_Init(JavaVM *vm)
{
	jint result;

	jvm = vm;

	std::cout << "JNIHook_Init called" << std::endl;
	if ((result = jvm->GetEnv((void **)&jvmti, JVMTI_VERSION_1_0)) != JNI_OK)
		return result;

	jvmtiCapabilities capabilities = { .can_retransform_classes = JVMTI_ENABLE };
	if (jvmti->AddCapabilities(&capabilities) != JVMTI_ERROR_NONE)
		return JNI_ERR;

	std::cout << "JVMTI initialized" << std::endl;
	
	VMTypes::init(gHotSpotVMStructs, gHotSpotVMTypes);

	auto fields = VMTypes::findTypeFields("Method").value();
	auto field = fields.get()["_from_interpreted_entry"];
	for (auto fieldEntry : fields.get()) {
		auto field = fieldEntry.second;
		std::cout << field->typeString << " " << field->fieldName << " @ " << field->offset << std::endl;
	}

	auto type = VMTypes::findType("Method").value();
	std::cout << "Method Type: " << type << std::endl;
	std::cout << "Method Size: " << type->size << std::endl;

	return JNI_OK;
}

extern "C" JNIHOOK_API jint JNIHook_Attach(jmethodID mID, jnihook_callback_t callback, void *arg)
{
	jint result;
	
	if (!jvm || !jvmti)
		return JNI_ERR;

	std::cout << "method ID: " << mID << std::endl;
	std::cout << "callback: " << (void *)callback << std::endl;
	std::cout << "arg: " << arg << std::endl;

	// Force class methods to be interpreted
	JNIEnv *jni;
	jclass klass;
	if ((result = jvm->GetEnv((void **)&jni, JNI_VERSION_1_6)) != JNI_OK)
		return result;
	
	if (jvmti->GetMethodDeclaringClass(mID, &klass) != JVMTI_ERROR_NONE)
		return JNI_ERR;

	if (jvmti->RetransformClasses(1, &klass) != JVMTI_ERROR_NONE)
		return JNI_ERR;

	jni->DeleteLocalRef(klass);

	auto method = VMType::from_instance("Method", *(void **)mID).value();
	std::cout << "Method: " << method.get_instance() << std::endl;

	// Prevent method from being compiled
	int *_access_flags = method.get_field<int>("_access_flags").value();
	*_access_flags = *_access_flags | JVM_ACC_NOT_C2_COMPILABLE | JVM_ACC_NOT_C1_COMPILABLE | JVM_ACC_NOT_C2_OSR_COMPILABLE;

	// Store hook information
	void **_i2i_entry = method.get_field<void *>("_i2i_entry").value();
	void **_from_interpreted_entry = method.get_field<void *>("_from_interpreted_entry").value();
	// void **_from_compiled_entry = method.get_field<void *>("_from_compiled_entry").value();

	std::cout << "_i2i_entry: " << *_i2i_entry << std::endl;
	std::cout << "_from_interpreted_entry: " << *_from_interpreted_entry << std::endl;
	// std::cout << "_from_compiled_entry: " << *_from_compiled_entry << std::endl;

	jniHookInfo hkInfo = {
		.callback = callback,
		._i2i_entry = *_i2i_entry,
		._from_interpreted_entry = *_from_interpreted_entry,
		// ._from_compiled_entry = *_from_compiled_entry,
		.arg = arg
	};

	jniHookTable[method.get_instance()] = hkInfo;

	// Place interpreted hooks
	*_i2i_entry = (void *)jnihook_gateway;
	*_from_interpreted_entry = (void *)jnihook_gateway;

	return JNI_OK;
}
