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

#ifndef _JNIHOOK_H_
#define _JNIHOOK_H_

#include <jni.h>
#include <jvmti.h>

#define JNIHOOK_API
#define JNIHOOK_CALL

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	JNIHOOK_OK = 0,

	JNIHOOK_ERR_GET_JNI,
	JNIHOOK_ERR_GET_JVMTI,
	JNIHOOK_ERR_ADD_JVMTI_CAPS,
	JNIHOOK_ERR_SETUP_CLASS_FILE_LOAD_HOOK,

	JNIHOOK_ERR_JNI_OPERATION,
	JNIHOOK_ERR_JVMTI_OPERATION,
	JNIHOOK_ERR_CLASS_FILE_CACHE,
	JNIHOOK_ERR_JAVA_EXCEPTION,
} jnihook_result_t;

/**
 * Initializes the JNIHook library
 *
 * @param jvm The Java Virtual Machine that will be instrumented by JNIHook
 * @return JNIHOOK_OK on success, JNIHOOK_ERR_* on failure.
 */
JNIHOOK_API jnihook_result_t JNIHOOK_CALL
JNIHook_Init(JavaVM *jvm);

/**
 * Attaches a hook to a Java method
 * NOTE: Native method signatures are as follows:
 *           ReturnType (*fnPtr)(JNIEnv *env, jobject objectOrClass, ...);
 *
 * @param method The Java method being hooked
 * @param native_hook_method The native method that will be called by the JVM instead of `method`
 * @param original_method (optional) Output variable that will receive a copy of the original (unhooked) method
 * @return JNIHOOK_OK on success, JNIHOOK_ERR_* on failure.
 */
JNIHOOK_API jnihook_result_t JNIHOOK_CALL
JNIHook_Attach(jmethodID method, void *native_hook_method, jmethodID *original_method);

/**
 * Detaches a hook from a Java method
 *
 * @param method The method being unhooked
 * @return JNIHOOK_OK on success, JNIHOOK_ERR_* on failure.
 */
JNIHOOK_API jnihook_result_t JNIHOOK_CALL
JNIHook_Detach(jmethodID method);

/**
 * Detaches every hook and shuts down JNIHook
 */
JNIHOOK_API jnihook_result_t JNIHOOK_CALL
JNIHook_Shutdown();

#ifdef __cplusplus
}
#endif
#endif
