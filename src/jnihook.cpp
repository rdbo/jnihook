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
#include "jvm.hpp"
#include <iostream>

extern "C" VMStructEntry *gHotSpotVMStructs;
extern "C" VMTypeEntry *gHotSpotVMTypes;
extern "C" void jnihook_gateway();

typedef struct {
	jnihook_callback_t callback;
	void *orig_i2i;
	void *arg;
} jniHookInfo;

std::unordered_map<void *, jniHookInfo> jniHookTable;

extern "C" JNIHOOK_API void *JNIHook_CallHandler(void *method, void *senderSP, void *thread)
{
	std::cout << "CALL HANDLER CALLED!" << std::endl;

	jniHookTable[method].callback((jmethodID)&method, senderSP, 0, thread, jniHookTable[method].arg);
	
	return jniHookTable[method].orig_i2i;
}

extern "C" JNIHOOK_API jint JNIHook_Init(JavaVM *jvm)
{
	std::cout << "hello" << std::endl;
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

	return 0;
}

extern "C" JNIHOOK_API jint JNIHook_Attach(jmethodID mID, jnihook_callback_t callback, void *arg)
{
	std::cout << "method ID: " << mID << std::endl;

	auto method = VMType::from_instance("Method", *(void **)mID).value();
	std::cout << "Method: " << method.get_instance() << std::endl;

	void *original = method.get_field<void *>("_from_interpreted_entry").value();

	std::cout << "_from_interpreted_entry: " << original << std::endl;

	jniHookTable[method.get_instance()].callback = callback;
	jniHookTable[method.get_instance()].orig_i2i = original;
	jniHookTable[method.get_instance()].arg = arg;

	method.set_field<void *>("_from_interpreted_entry", (void *)jnihook_gateway);

	return JNI_OK;
}
