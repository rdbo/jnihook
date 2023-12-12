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

#define JNIHOOK_API
#define JNIHOOK_CALLBACK

typedef jvalue(JNIHOOK_CALLBACK *jnihook_callback_t)(jvalue *args, size_t nargs, void *thread, void *arg);

#ifdef __cplusplus
extern "C" {
#endif

JNIHOOK_API jint JNIHook_Init(JavaVM *jvm);
JNIHOOK_API jint JNIHook_Attach(jmethodID mID, jnihook_callback_t callback, void *arg);
JNIHOOK_API jint JNIHook_Detach(jmethodID mID);

#ifdef __cplusplus
}
#endif
#endif
