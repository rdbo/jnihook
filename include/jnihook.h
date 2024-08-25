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

typedef struct jnihook_t {
	JavaVM   *jvm;
	JNIEnv   *env;
	jvmtiEnv *jvmti;
} jnihook_t;

typedef enum {
	JNIHOOK_OK = 0,

	JNIHOOK_ERR_GET_JVM,
	JNIHOOK_ERR_GET_JVMTI,
	JNIHOOK_ERR_ADD_JVMTI_CAPS,
	JNIHOOK_ERR_SETUP_CLASS_FILE_LOAD_HOOK,

	JNIHOOK_ERR_JNI_OPERATION,
	JNIHOOK_ERR_JVMTI_OPERATION,
	JNIHOOK_ERR_CLASS_FILE_CACHE,
} jnihook_result_t;

/**
 * Initializes a JNIHook context
 *
 * @param env The JNI environment used to initialize the JNIHook context
 * @param jnihook Output variable that will receive the initialized JNIHook context
 * @return JNIHOOK_OK on success, JNIHOOK_ERR_* on failure.
 */
JNIHOOK_API jnihook_result_t JNIHOOK_CALL
JNIHook_Init(JNIEnv *env, jnihook_t *jnihook);

/**
 * Attaches a hook to a Java method
 * NOTE: Native method signatures are as follows:
 *           ReturnType (*fnPtr)(JNIEnv *env, jobject objectOrClass, ...);
 *
 * @param jnihook The JNIHook context
 * @param method The Java method being hooked
 * @param native_hook_method The native method that will be called by the JVM instead of `method`
 * @param original_class (optional) Output variable that will receive the original (unhooked) class
 * @return JNIHOOK_OK on success, JNIHOOK_ERR_* on failure.
 */
JNIHOOK_API jnihook_result_t JNIHOOK_CALL
JNIHook_Attach(jnihook_t *jnihook, jmethodID method, void *native_hook_method, jclass *original_class);

/**
 * Gets the original/unhooked class by its name.
 * You can use this to call the original methods of
 * a hooked class, just like you would call a regular
 * class method.
 *
 * @param class_name The name of the hooked class
 * @return A valid pointer on success, `nullptr` on error.
 */
JNIHOOK_API jclass JNIHOOK_CALL
JNIHook_GetOriginalClass(const char *class_name);

/**
 * Detaches a hook from a Java method
 *
 * @param jnihook The JNIHook context
 * @param method The method being unhooked
 * @return JNIHOOK_OK on success, JNIHOOK_ERR_* on failure.
 */
JNIHOOK_API jnihook_result_t JNIHOOK_CALL
JNIHook_Detach(jnihook_t *jnihook, jmethodID method);

/**
 * Detaches every hook and shuts down JNIHook
 *
 * @param jnihook The JNIHook context
 */
JNIHOOK_API void JNIHOOK_CALL
JNIHook_Shutdown(jnihook_t *jnihook);

#ifdef __cplusplus
}
#endif
#endif
