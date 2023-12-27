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
#include <vector>

extern "C" JNIIMPORT VMStructEntry *gHotSpotVMStructs;
extern "C" JNIIMPORT VMTypeEntry *gHotSpotVMTypes;
extern "C" void jnihook_gateway();

typedef struct {
	// NOTE: you have to dereference the 'jclass' to get a constant result for the method, hence why 'clazz oop' instead of just 'clazz'
	void *clazzOop;
	jnihook_callback_t callback;
	std::vector<uint8_t> cachedMethod; // Copy of the original method (before the hook)
	std::vector<uint8_t> callableMethod; // Modified version from the original method, callable from the hook callback
	void *arg; // Argument passed to the callback
} jniHookInfo;

static std::unordered_map<void *, jniHookInfo> jniHookTable;
static JavaVM *jvm = nullptr;
static jvmtiEnv *jvmti = nullptr;

extern "C" jvalue JNIHOOK_CALL JNIHook_CallHandler(void *methodAddr, void *senderSP)
{
	auto hkEntry = jniHookTable[methodAddr];

	auto method = VMType::from_instance("Method", methodAddr).value();
	void **_constMethod = method.get_field<void *>("_constMethod").value();

	// Retrieve _size_of_parameters from ConstMethod
	auto constMethod = VMType::from_instance("ConstMethod", *_constMethod).value();
	uint16_t *_size_of_parameters = constMethod.get_field<uint16_t>("_size_of_parameters").value();
	size_t nparams = (size_t)*_size_of_parameters;

	// Sort arguments to easier handling during the hook
	std::vector<jvalue> args;
	for (size_t i = 0; i < nparams; ++i) {
		// stack layout for nparams == 3:
		//   - senderSP[2] is arg0
		//   - senderSP[1] is arg1
		//   - senderSP[0] is arg2
		args.push_back(((jvalue *)senderSP)[nparams - 1 - i]);
	}

	// Generate jmethodID from callableMethod
	uint8_t *callableMethodData = hkEntry.callableMethod.data();
	jmethodID callableMethod = (jmethodID)&callableMethodData;

	// Retrieve JNIEnv as a facility for the callback
	JNIEnv *jni;
	jvm->GetEnv((void **)&jni, JNI_VERSION_1_6);

	// Call the callback and store its return value, which will also be passed back to the interpreter
	jvalue call_result = hkEntry.callback(jni, callableMethod, args.data(), nparams, hkEntry.arg);

	// Return the callback retval to the interpreter
	return call_result;
}

extern "C" JNIHOOK_API jint JNIHOOK_CALL JNIHook_Init(JavaVM *vm)
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

bool IsClassRetransformed(void *oop)
{
	for (const auto &[key, value] : jniHookTable) {
		if (value.clazzOop == oop)
			return true;
	}

	return false;
}

void *RetransformOwnerClass(jmethodID mID)
{
	jclass klass;
	JNIEnv *jni;
	void *oop;

	// Get method owner class
	if (jvm->GetEnv((void **)&jni, JNI_VERSION_1_6) != JNI_OK)
		return NULL;
	
	if (jvmti->GetMethodDeclaringClass(mID, &klass) != JVMTI_ERROR_NONE)
		return NULL;

	// 'jclass'es are unreliable, so get the 'oop' of the class
	// example: 1 class 'oop' might have many different 'jclass'es in memory
	// but all the 'jclass'es point to this 'oop'
	oop = *(void **)klass;

	// Only retransform if class has not been retransformed yet
	// (retransforming again will cause the previous hooked to be reset)
	if (IsClassRetransformed(oop))
		return oop;

	if (jvmti->RetransformClasses(1, &klass) != JVMTI_ERROR_NONE)
		return NULL;

	// Get new oop (post retransform)
	oop = *(void **)klass;

	// Clean up
	jni->DeleteLocalRef(klass);

	return oop;
}

extern "C" JNIHOOK_API jint JNIHOOK_CALL JNIHook_Attach(jmethodID mID, jnihook_callback_t callback, void *arg)
{
	jint result;
	
	if (!jvm || !jvmti)
		return JNI_ERR;

	// TODO: Don't hook if is already hooked, or replace the hook properly

	// Force class methods to be interpreted through RetransformClasses
	void *clazzOop = RetransformOwnerClass(mID);
	if (!clazzOop)
		return JNI_ERR;

	// Retrieve method AFTER RetransformClasses (doing so before might not work!)
	auto method = VMType::from_instance("Method", *(void **)mID).value();
	uint64_t method_size = method.size();
	auto cachedMethod = std::vector<uint8_t>((uint8_t *)method.get_instance(), &((uint8_t *)method.get_instance())[method_size]);
	auto callableMethod = cachedMethod;

	// Patch callableMethod so that it becomes callable.
	// JVM does lookups for non-static methods instead of
	// calling the 'jmethodID' you give it. This can be
	// bypassed by setting '_vtable_index' to 'nonvirtual_vtable_index',
	// where it will call whatever 'Method' you give it.
	auto callable = VMType::from_instance("Method", (void *)callableMethod.data()).value();
	int *callable_vtable_index = callable.get_field<int>("_vtable_index").value();
	*callable_vtable_index = nonvirtual_vtable_index;

	// Retrieve important Method fields
	jint *_access_flags = method.get_field<jint>("_access_flags").value();
	void **_i2i_entry = method.get_field<void *>("_i2i_entry").value();
	void **_from_interpreted_entry = method.get_field<void *>("_from_interpreted_entry").value();

	// Store hook information
	jniHookInfo hkInfo = {
		.clazzOop = clazzOop,
		.callback = callback,
		.cachedMethod = cachedMethod,
		.callableMethod = callableMethod,
		.arg = arg
	};

	jniHookTable[method.get_instance()] = hkInfo;

	// Prevent method from being compiled
	*_access_flags = *_access_flags | JVM_ACC_NOT_C2_COMPILABLE | JVM_ACC_NOT_C1_COMPILABLE | JVM_ACC_NOT_C2_OSR_COMPILABLE;

	// Place interpreted hooks
	*_i2i_entry = (void *)jnihook_gateway;
	*_from_interpreted_entry = (void *)jnihook_gateway;

	return JNI_OK;
}

extern "C" JNIHOOK_API jint JNIHOOK_CALL JNIHook_Detach(jmethodID mID)
{
	void *methodAddr = *(void **)mID;

	auto hkEntry = jniHookTable.find(methodAddr);
	if (hkEntry == jniHookTable.end())
		return JNI_ERR;

	auto method = VMType::from_instance("Method", methodAddr).value();

	// Copy cached original method back to the 'Method' instance
	size_t method_size = method.size();
	memcpy(method.get_instance(), (void *)hkEntry->second.cachedMethod.data(), method_size);
	jniHookTable.erase(methodAddr);

	return JNI_OK;
}

extern "C" JNIHOOK_API jint JNIHOOK_CALL JNIHook_Shutdown()
{
	for (auto it = jniHookTable.cbegin(); it != jniHookTable.cend();) {
		void *method = it->first;
		it++;
		JNIHook_Detach((jmethodID)&method); // WARN: This call will erase the table entry, hence why we increment the iterator beforehand
	}

	return JNI_OK;
}
