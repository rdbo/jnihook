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

#ifndef _CLASSFILE_HPP_
#define _CLASSFILE_HPP_

#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

typedef uint8_t u1;
typedef uint16_t u2;
typedef uint32_t u4;

/* cp_info tag values */
enum {
	CONSTANT_Class = 7,
	CONSTANT_Fieldref = 9,
	CONSTANT_Methodref = 10,
	CONSTANT_InterfaceMethodref = 11,
	CONSTANT_String = 8,
	CONSTANT_Integer = 3,
	CONSTANT_Float = 4,
	CONSTANT_Long = 5,
	CONSTANT_Double = 6,
	CONSTANT_NameAndType = 12,
	CONSTANT_Utf8 = 1,
	CONSTANT_MethodHandle = 15,
	CONSTANT_MethodType = 16,
	CONSTANT_InvokeDynamic = 18,
};

/* access flags */
enum {
	ACC_PUBLIC = 0x0001,
	ACC_PRIVATE = 0x0002,
	ACC_PROTECTED = 0x0004,
	ACC_STATIC = 0x0008,
	ACC_FINAL = 0x0010,
	ACC_SUPER = 0x0020,
	ACC_VOLATILE = 0x0040,
	ACC_TRANSIENT = 0x0080,
	ACC_INTERFACE = 0x0200,
	ACC_ABSTRACT = 0x0400,
	ACC_SYNTHETIC = 0x1000,
	ACC_ANNOTATION = 0x2000,
	ACC_ENUM = 0x4000
};

/********************************/

/* cp_info variants */
typedef struct {
    u1 tag;
    u2 name_index;
} CONSTANT_Class_info;

typedef struct {
    u1 tag;
    u2 class_index;
    u2 name_and_type_index;
} CONSTANT_Fieldref_info;

typedef struct {
    u1 tag;
    u2 class_index;
    u2 name_and_type_index;
} CONSTANT_Methodref_info;

typedef struct {
    u1 tag;
    u2 class_index;
    u2 name_and_type_index;
} CONSTANT_InterfaceMethodref_info;

typedef struct {
    u1 tag;
    u2 string_index;
} CONSTANT_String_info;

typedef struct {
    u1 tag;
    u4 bytes;
} CONSTANT_Integer_info;

typedef struct {
    u1 tag;
    u4 bytes;
} CONSTANT_Float_info;

typedef struct {
    u1 tag;
    u4 high_bytes;
    u4 low_bytes;
} CONSTANT_Long_info;

typedef struct {
    u1 tag;
    u4 high_bytes;
    u4 low_bytes;
} CONSTANT_Double_info;

typedef struct {
    u1 tag;
    u2 name_index;
    u2 descriptor_index;
} CONSTANT_NameAndType_info;

typedef struct {
    u1 tag;
    u2 length;
    u1 bytes[];
} CONSTANT_Utf8_info;

typedef struct {
    u1 tag;
    u1 reference_kind;
    u2 reference_index;
} CONSTANT_MethodHandle_info;

typedef struct {
    u1 tag;
    u2 descriptor_index;
} CONSTANT_MethodType_info;

typedef struct {
    u1 tag;
    u2 bootstrap_method_attr_index;
    u2 name_and_type_index;
} CONSTANT_InvokeDynamic_info;

/********************************/

typedef struct {
	u2 attribute_name_index;
	std::vector<u1> info;
} attribute_info;

typedef struct {
	u2 access_flags;
	u2 name_index;
	u2 descriptor_index;
	std::vector<attribute_info> attributes;
} field_info;

typedef struct {
	u2 access_flags;
	u2 name_index;
	u2 descriptor_index;
	std::vector<attribute_info> attributes;
} method_info;

typedef struct {
	std::vector<u1> bytes; // The cp_info variants can have size-varying fields,
			       // so we just store the bytes
} cp_info;

/********************************/

class ClassFile {
private:
	u4 magic;
	u2 minor;
	u2 major;
	std::vector<cp_info> constant_pool;
	u2 access_flags;
	u2 this_class;
	u2 super_class;
	std::vector<u2> interfaces;
	std::vector<field_info> fields;
	std::vector<method_info> methods;
	std::vector<attribute_info> attributes;
public:
	static std::unique_ptr<ClassFile>
	load(const uint8_t *classfile_bytes);

	inline std::string str()
	{
		std::stringstream ss;

		ss << "ClassFile {" << std::endl;
		ss << "\tmagic: " << std::hex << magic << std::dec << std::endl;
		ss << "\tminor: " << minor << std::endl;
		ss << "\tmajor: " << minor << std::endl;
		ss << "}";

		return ss.str();
	}
public:
	inline ClassFile(u4 magic, u2 minor, u2 major, std::vector<cp_info> constant_pool,
	                 u2 access_flags, u2 this_class, u2 super_class,
	                 std::vector<u2> interfaces, std::vector<field_info> fields,
	                 std::vector<method_info> methods, std::vector<attribute_info> attributes)
		: magic(magic), minor(minor), major(major), constant_pool(constant_pool),
		access_flags(access_flags), this_class(this_class), super_class(super_class),
		interfaces(interfaces), fields(fields), methods(methods), attributes(attributes)
	{}
};

#endif
