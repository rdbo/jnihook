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

#ifndef JNIHOOK_H
#define JNIHOOK_H

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
	JNIHOOK_OK,
	JNIHOOK_ERR_JVMTI_CAP
} jnihook_result_t;

JNIHOOK_API jnihook_result_t JNIHOOK_CALL
JNIHook_Init(JavaVM *jvm, jnihook_t *jnihook);

JNIHOOK_API jint JNIHOOK_CALL
JNIHook_Attach(jnihook_t *jnihook, jmethodID method, void *native_hook_method);

JNIHOOK_API jint JNIHOOK_CALL JNIHook_Detach(jnihook_t *jnihook, jmethodID method);

JNIHOOK_API void JNIHOOK_CALL JNIHook_Shutdown(jnihook_t *jnihook);

#ifdef __cplusplus
}
#endif
#endif
