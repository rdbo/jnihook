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
	int _access_flags;
	void *_i2i_entry;
	void *_from_interpreted_entry;
	void *arg;
	int should_unhook;
} jniHookInfo;

static std::unordered_map<void *, jniHookInfo> jniHookTable;
static JavaVM *jvm = nullptr;
static jvmtiEnv *jvmti = nullptr;

extern "C" JNIHOOK_API jvalue JNIHook_CallHandler(void *methodAddr, void *senderSP, void *thread)
{
	auto hkEntry = jniHookTable.find(methodAddr);

	auto method = VMType::from_instance("Method", methodAddr).value();
	void **_i2i_entry = method.get_field<void *>("_i2i_entry").value();
	void **_from_interpreted_entry = method.get_field<void *>("_from_interpreted_entry").value();

	// Restore original Method to allow for a midhook call
	*_i2i_entry = hkEntry->second._i2i_entry;
	*_from_interpreted_entry = hkEntry->second._from_interpreted_entry;

	// Call the callback and store its return value, which will also be passed back to the interpreter
	jvalue call_result = hkEntry->second.callback((jmethodID)&methodAddr, senderSP, 0, thread, hkEntry->second.arg);

	// Handle scheduled unhook
	if (hkEntry->second.should_unhook) {
		// Restore access flags
		int *_access_flags = method.get_field<int>("_access_flags").value();
		*_access_flags = hkEntry->second._access_flags;
		jniHookTable.erase(methodAddr);
		return call_result;
	}

	// Rehook method, so that its next call will fall into the hook
	*_i2i_entry = (void *)jnihook_gateway;
	*_from_interpreted_entry = (void *)jnihook_gateway;

	// Return the callback retval to the interpreter
	return call_result;
}

extern "C" JNIHOOK_API jint JNIHook_Init(JavaVM *vm)
{
	jint result;

	jvm = vm; // Store JVM globally for future uses

	if ((result = jvm->GetEnv((void **)&jvmti, JVMTI_VERSION_1_0)) != JNI_OK)
		return result;

	// Setup JVMTI so that it can retransform classes (required for disabling compilation and inlining of methods)
	jvmtiCapabilities capabilities = { .can_retransform_classes = JVMTI_ENABLE };
	if (jvmti->AddCapabilities(&capabilities) != JVMTI_ERROR_NONE)
		return JNI_ERR;

	// Generate VM type hashmaps
	VMTypes::init(gHotSpotVMStructs, gHotSpotVMTypes);

	return JNI_OK;
}

extern "C" JNIHOOK_API jint JNIHook_Attach(jmethodID mID, jnihook_callback_t callback, void *arg)
{
	jint result;
	
	if (!jvm || !jvmti)
		return JNI_ERR;

	// Force class methods to be interpreted through RetransformClasses
	JNIEnv *jni;
	jclass klass;
	if ((result = jvm->GetEnv((void **)&jni, JNI_VERSION_1_6)) != JNI_OK)
		return result;
	
	if (jvmti->GetMethodDeclaringClass(mID, &klass) != JVMTI_ERROR_NONE)
		return JNI_ERR;

	if (jvmti->RetransformClasses(1, &klass) != JVMTI_ERROR_NONE)
		return JNI_ERR;

	jni->DeleteLocalRef(klass);

	// Retrieve method AFTER RetransformClasses (doing so before might not work!)
	auto method = VMType::from_instance("Method", *(void **)mID).value();

	// Store hook information
	int *_access_flags = method.get_field<int>("_access_flags").value();
	void **_i2i_entry = method.get_field<void *>("_i2i_entry").value();
	void **_from_interpreted_entry = method.get_field<void *>("_from_interpreted_entry").value();

	jniHookInfo hkInfo = {
		.callback = callback,
		._access_flags = *_access_flags,
		._i2i_entry = *_i2i_entry,
		._from_interpreted_entry = *_from_interpreted_entry,
		.arg = arg,
		.should_unhook = 0
	};

	jniHookTable[method.get_instance()] = hkInfo;

	// Prevent method from being compiled
	*_access_flags = *_access_flags | JVM_ACC_NOT_C2_COMPILABLE | JVM_ACC_NOT_C1_COMPILABLE | JVM_ACC_NOT_C2_OSR_COMPILABLE;

	// Place interpreted hooks
	*_i2i_entry = (void *)jnihook_gateway;
	*_from_interpreted_entry = (void *)jnihook_gateway;

	return JNI_OK;
}

JNIHOOK_API jint JNIHook_Detach(jmethodID mID)
{
	void *methodAddr = *(void **)mID;

	auto hkEntry = jniHookTable.find(methodAddr);
	if (hkEntry == jniHookTable.end())
		return JNI_ERR;

	hkEntry->second.should_unhook = 1; // Schedule unhook
	
	return JNI_OK;
}

