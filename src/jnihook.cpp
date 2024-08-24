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
#include <unordered_map>
#include <string>
#include <vector>
#include <cstdint>
#include "classfile.hpp"

#include <iostream> // TODO: Remove

typedef struct method_info_t {
	std::string name;
	std::string signature;
} method_info_t;

typedef struct hook_info_t {
	method_info_t method_info;
	void *native_hook_method;
} hook_info_t;

// TODO: Make the following globals specific to a jnihook_t without breaking C compat
static std::unordered_map<std::string, std::vector<hook_info_t>> g_hooks;
static std::unordered_map<std::string, std::unique_ptr<ClassFile>> g_class_file_cache;

static std::string
get_class_signature(jvmtiEnv *jvmti, jclass clazz)
{
	char *sig;
	
	if (jvmti->GetClassSignature(clazz, &sig, NULL) != JVMTI_ERROR_NONE) {
		return "";
	}

	std::string signature = std::string(sig, &sig[strlen(sig)]);

	jvmti->Deallocate(reinterpret_cast<unsigned char *>(sig));

	return signature;
}

static std::unique_ptr<method_info_t>
get_method_info(jvmtiEnv *jvmti, jmethodID method)
{
	char *name;
	char *sig;
	
	if (jvmti->GetMethodName(method, &name, &sig, NULL) != JVMTI_ERROR_NONE)
		return nullptr;

	std::string name_str(name, &name[strlen(name)]);
	std::string signature_str(sig, &sig[strlen(sig)]);

	jvmti->Deallocate(reinterpret_cast<unsigned char *>(name));
	jvmti->Deallocate(reinterpret_cast<unsigned char *>(sig));

	return std::make_unique<method_info_t>(method_info_t { name_str, signature_str });
}

void JNICALL JNIHook_ClassFileLoadHook(jvmtiEnv *jvmti_env,
				       JNIEnv* jni_env,
				       jclass class_being_redefined,
				       jobject loader,
				       const char* name,
				       jobject protection_domain,
				       jint class_data_len,
				       const unsigned char* class_data,
				       jint* new_class_data_len,
				       unsigned char** new_class_data)
{
	auto class_signature = get_class_signature(jvmti_env, class_being_redefined);

	// If the cache is not hooked, or it is already cached, no point in caching again
	if (class_signature == "" || g_hooks.find(class_signature) == g_hooks.end() ||
	    g_class_file_cache.find(class_signature) != g_class_file_cache.end()) {
		return;
	}

	// Cache parsed classfile
	auto cf = ClassFile::load(class_data);
	if (!cf)
		return;

	g_class_file_cache[class_signature] = std::move(cf);

	return;
}

JNIHOOK_API jnihook_result_t JNIHOOK_CALL
JNIHook_Init(JNIEnv *env, jnihook_t *jnihook)
{
	JavaVM *jvm;
	jvmtiEnv *jvmti;
	jvmtiCapabilities capabilities = { 0 };
	jvmtiEventCallbacks callbacks = {};

	if (env->GetJavaVM(&jvm) != JNI_OK) {
		return JNIHOOK_ERR_GET_JVM;
	}

	if (jvm->GetEnv(reinterpret_cast<void **>(&jvmti), JVMTI_VERSION_1_2) != JNI_OK) {
		return JNIHOOK_ERR_GET_JVMTI;
	}

	capabilities.can_redefine_classes = 1;
	capabilities.can_redefine_any_class = 1;
	capabilities.can_retransform_classes = 1;
	capabilities.can_retransform_any_class = 1;
	if (jvmti->GetPotentialCapabilities(&capabilities) != JVMTI_ERROR_NONE) {
		return JNIHOOK_ERR_ADD_JVMTI_CAPS;
	}

	if (jvmti->AddCapabilities(&capabilities) != JVMTI_ERROR_NONE) {
		return JNIHOOK_ERR_ADD_JVMTI_CAPS;
	}

	callbacks.ClassFileLoadHook = JNIHook_ClassFileLoadHook;
	if (jvmti->SetEventCallbacks(&callbacks, sizeof(callbacks)) != JVMTI_ERROR_NONE) {
		return JNIHOOK_ERR_SETUP_CLASS_FILE_LOAD_HOOK;
	}

	if (jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_CLASS_FILE_LOAD_HOOK, NULL) != JVMTI_ERROR_NONE) {
		return JNIHOOK_ERR_SETUP_CLASS_FILE_LOAD_HOOK;
	}

	jnihook->jvm = jvm;
	jnihook->env = env;
	jnihook->jvmti = jvmti;

	return JNIHOOK_OK;
}

JNIHOOK_API jint JNIHOOK_CALL
JNIHook_Attach(jnihook_t *jnihook, jmethodID method, void *native_hook_method)
{
	jclass clazz;
	std::string signature;
	hook_info_t hook_info;
	jvmtiClassDefinition class_definition;

	if (jnihook->jvmti->GetMethodDeclaringClass(method, &clazz) != JVMTI_ERROR_NONE) {
		return JNIHOOK_ERR_JVMTI_OPERATION;
	}

	signature = get_class_signature(jnihook->jvmti, clazz);
	if (signature.length() == 0) {
		return JNIHOOK_ERR_JVMTI_OPERATION;
	}

	auto method_info = get_method_info(jnihook->jvmti, method);
	if (!method_info) {
		return JNIHOOK_ERR_JVMTI_OPERATION;
	}

	hook_info.method_info = *method_info;
	hook_info.native_hook_method = native_hook_method;

	g_hooks[signature].push_back(hook_info);

	// Force caching of the class being hooked
	jnihook->jvmti->RetransformClasses(1, &clazz);

	if (g_class_file_cache.find(signature) == g_class_file_cache.end()) {
		return JNIHOOK_ERR_CLASSFILE_CACHE;
	}

	auto cf = *g_class_file_cache[signature];

	// Patch class file
	auto constant_pool = cf.get_constant_pool();
	for (size_t i = 1; i < constant_pool.size(); ++i) {
		auto &item = constant_pool[i];
		auto tag = item.bytes[0];

		if (tag != CONSTANT_Methodref)
			continue;

		auto methodref = reinterpret_cast<CONSTANT_Methodref_info *>(item.bytes.data());

		auto name_and_type = reinterpret_cast<CONSTANT_NameAndType_info *>(
			cf.get_constant_pool_item(methodref->name_and_type_index).bytes.data()
		);

		auto name = reinterpret_cast<CONSTANT_Utf8_info *>(
			cf.get_constant_pool_item(name_and_type->name_index).bytes.data()
		);

		auto descriptor = reinterpret_cast<CONSTANT_Utf8_info *>(
			cf.get_constant_pool_item(name_and_type->descriptor_index).bytes.data()
		);

		std::cout << "NAME: " << name->bytes << std::endl;
		std::cout << "DESCRIPTOR: " << descriptor->bytes << std::endl;
	}

	// Redefine class with modified ClassFile
	auto cf_bytes = cf.bytes();
	class_definition.klass = clazz;
	class_definition.class_byte_count = cf_bytes.size();
	class_definition.class_bytes = cf_bytes.data();
	jnihook->jvmti->RedefineClasses(1, &class_definition);

	return JNIHOOK_OK;
}

JNIHOOK_API jint JNIHOOK_CALL JNIHook_Detach(jnihook_t *jnihook, jmethodID method);


JNIHOOK_API void JNIHOOK_CALL JNIHook_Shutdown(jnihook_t *jnihook)
{
	jvmtiEventCallbacks callbacks = {};

	jnihook->jvmti->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_CLASS_FILE_LOAD_HOOK, NULL);
	jnihook->jvmti->SetEventCallbacks(&callbacks, sizeof(callbacks));

	memset(jnihook, 0x0, sizeof(*jnihook));

	return;
}
