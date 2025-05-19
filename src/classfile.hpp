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

#define DEFINE_GETTER(field) inline auto &get_##field() { return this->field; }

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
        ACC_PUBLIC     = 0x0001,
        ACC_PRIVATE    = 0x0002,
        ACC_PROTECTED  = 0x0004,
        ACC_STATIC     = 0x0008,
        ACC_FINAL      = 0x0010,
        ACC_SUPER      = 0x0020,
        ACC_VOLATILE   = 0x0040,
        ACC_TRANSIENT  = 0x0080,
        ACC_NATIVE     = 0x0100,
        ACC_INTERFACE  = 0x0200,
        ACC_ABSTRACT   = 0x0400,
        ACC_STRICT     = 0x0800,
        ACC_SYNTHETIC  = 0x1000,
        ACC_ANNOTATION = 0x2000,
        ACC_ENUM       = 0x4000
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
        size_t constant_pool_count; // Don't trust this value. It is very tricky. Used only for bytes generation.
        std::vector<cp_info> constant_pool;
        u2 access_flags;
        u2 this_class;
        u2 super_class;
        std::vector<u2> interfaces;
        std::vector<field_info> fields;
        std::vector<method_info> methods;
        std::vector<attribute_info> attributes;

        std::vector<uint8_t> original_bytes; // The bytes that were passed to ClassFile::load
public:
        static std::unique_ptr<ClassFile>
        load(const uint8_t *classfile_bytes);

        std::vector<uint8_t>
        bytes();

        inline std::string
        str()
        {
                std::stringstream ss;

                ss << "ClassFile {" << std::endl;
                ss << "\tmagic: " << std::hex << magic << std::dec << std::endl;
                ss << "\tminor: " << minor << std::endl;
                ss << "\tmajor: " << major << std::endl;
                ss << "\tconstant_pool_count: " << constant_pool_count << std::endl;
                ss << "\tconstant_pool: [" << std::endl;

                for (size_t i = 0; i < constant_pool.size(); ++i) {
                        auto &cpi = constant_pool[i];
                        u1 tag = cpi.bytes[0];

                        if (tag == 0)
                                continue;

                        ss << "\t\t" << i << ": {" << std::endl;

                        ss << "\t\t\ttag: " << static_cast<int>(tag) << std::endl;

                        switch (tag) {
                        case CONSTANT_Class:
                                {
                                        auto info = reinterpret_cast<CONSTANT_Class_info *>(cpi.bytes.data());
                                        ss << "\t\t\t_name_index: " << info->name_index << std::endl;
                                        break;
                                }
                        case CONSTANT_Fieldref:
                                {
                                        auto info = reinterpret_cast<CONSTANT_Fieldref_info *>(cpi.bytes.data());
                                        ss << "\t\t\t_class_index: " << info->class_index << std::endl;
                                        ss << "\t\t\t_name_and_type_index: " << info->name_and_type_index << std::endl;
                                        break;
                                }
                        case CONSTANT_Methodref:
                                {
                                        auto methodref = reinterpret_cast<CONSTANT_Methodref_info *>(cpi.bytes.data());
                                        ss << "\t\t\t_class_index: " << methodref->class_index << std::endl;
                                        ss << "\t\t\t_name_and_type_index: " << methodref->name_and_type_index << std::endl;
                                        break;
                                }
                        case CONSTANT_InterfaceMethodref:
                                {
                                        auto methodref = reinterpret_cast<CONSTANT_InterfaceMethodref_info *>(cpi.bytes.data());
                                        ss << "\t\t\t_class_index: " << methodref->class_index << std::endl;
                                        ss << "\t\t\t_name_and_type_index: " << methodref->name_and_type_index << std::endl;
                                        break;
                                }
                        case CONSTANT_String:
                                {
                                        auto info = reinterpret_cast<CONSTANT_String_info *>(cpi.bytes.data());
                                        ss << "\t\t\t_string_index: " << info->string_index << std::endl;
                                        break;
                                }
                        case CONSTANT_Integer:
                                {
                                        // uint32_t value;
                                        auto info = reinterpret_cast<CONSTANT_Integer_info *>(cpi.bytes.data());
                                        ss << "\t\t\t_bytes: " << info->bytes << std::endl;
                                        // ss << "\t\t\t_value: " << value << std::endl;
                                        break;
                                }
                        case CONSTANT_Long:
                                {
                                        uint64_t l;
                                        auto info = reinterpret_cast<CONSTANT_Long_info *>(cpi.bytes.data());

                                        ss << "\t\t\t_high_bytes: " << std::hex << info->high_bytes << std::dec << std::endl;
                                        ss << "\t\t\t_low_bytes: " << std::hex << info->low_bytes << std::dec << std::endl;
                                        *(&((uint32_t *)&l)[0]) = info->low_bytes;
                                        *(&((uint32_t *)&l)[1]) = info->high_bytes;
                                        ss << "\t\t\t_value: " << l << std::endl;
                                        break;
                                }
                        case CONSTANT_Double:
                                {
                                        double d;
                                        auto info = reinterpret_cast<CONSTANT_Double_info *>(cpi.bytes.data());

                                        ss << "\t\t\t_high_bytes: " << std::hex << info->high_bytes << std::dec << std::endl;
                                        ss << "\t\t\t_low_bytes: " << std::hex << info->low_bytes << std::dec << std::endl;
                                        // ss << "\t\t\t_value: " << l << std::endl;
                                        break;
                                }
                        case CONSTANT_NameAndType:
                                {
                                        auto info = reinterpret_cast<CONSTANT_NameAndType_info *>(cpi.bytes.data());
                                        ss << "\t\t\t_name_index: " << info->name_index << std::endl;
                                        ss << "\t\t\t_descriptor_index: " << info->descriptor_index << std::endl;
                                        break;
                                }
                        case CONSTANT_Utf8:
                                {
                                        auto info = reinterpret_cast<CONSTANT_Utf8_info *>(cpi.bytes.data());
                                        std::string utf8 = std::string(info->bytes, &info->bytes[info->length]);
                                        ss << "\t\t\t_length: " << info->length << std::endl;
                                        ss << "\t\t\t_bytes: " << utf8 << std::endl;
                                        break;
                                }
                        case CONSTANT_MethodHandle:
                                {
                                        auto info = reinterpret_cast<CONSTANT_MethodHandle_info *>(cpi.bytes.data());
                                        ss << "\t\t\t_reference_kind: " << info->reference_kind << std::endl;
                                        ss << "\t\t\t_reference_index: " << info->reference_kind << std::endl;
                                        break;
                                }
                        case CONSTANT_MethodType:
                                {
                                        auto info = reinterpret_cast<CONSTANT_MethodType_info *>(cpi.bytes.data());
                                        ss << "\t\t\t_descriptor_index: " << info->descriptor_index << std::endl;
                                        break;
                                }
                        case CONSTANT_InvokeDynamic:
                                {
                                        auto info = reinterpret_cast<CONSTANT_InvokeDynamic_info *>(cpi.bytes.data());
                                        ss << "\t\t\t_bootstrap_method_attr_index: " << info->bootstrap_method_attr_index << std::endl;
                                        ss << "\t\t\t_name_and_type_index: " << info->name_and_type_index << std::endl;
                                        break;
                                }
                        }

                        ss << "\t\t\t_size: " << cpi.bytes.size() << std::endl;

                        ss << "\t\t}," << std::endl;
                }
                
                ss << "\t]" << std::endl;

                ss << "\taccess_flags: " << access_flags << std::endl;
                ss << "\tthis_class: " << this_class << std::endl;
                ss << "\tsuper_class: " << super_class << std::endl;
                ss << "\tinterfaces_count: " << interfaces_count() << std::endl;
                ss << "\tinterfaces: [ ";

                for (size_t i = 0; i < interfaces.size(); ++i) {
                        ss << interfaces[i] << " ";
                }

                ss << "]" << std::endl;

                ss << "\tfields_count: " << fields_count() << std::endl;
                ss << "\tfields_info: [" << std::endl;

                for (size_t i = 0; i < fields.size(); ++i) {
                        auto &field = fields[i];
                        ss << "\t\t{" << std::endl;
                        ss << "\t\t\taccess_flags: " << field.access_flags << std::endl;
                        ss << "\t\t\tname_index: " << field.name_index << std::endl;
                        ss << "\t\t\tdescriptor_index: " << field.descriptor_index << std::endl;
                        ss << "\t\t\tattributes_count: " << field.attributes.size() << std::endl;
                        ss << "\t\t\tattributes: [" << std::endl;
                        for (size_t j = 0; j < field.attributes.size(); ++j) {
                                auto &attribute = field.attributes[j];
                                ss << "\t\t\t\t{" << std::endl;
                                ss << "\t\t\t\t\tattribute_name_index: " << attribute.attribute_name_index << std::endl;
                                ss << "\t\t\t\t\tattribute_length: " << attribute.info.size() << std::endl;
                                ss << "\t\t\t\t\tinfo: [ ";
                                for (size_t k = 0; k < attribute.info.size(); ++k) {
                                        ss << std::hex << static_cast<int>(attribute.info[k]) << std::dec << " ";
                                }
                                ss << "]" << std::endl;
                                ss << "\t\t\t\t}" << std::endl;
                        }
                        ss << "\t\t\t]" << std::endl;

                        ss << "\t\t}, " << std::endl;
                }

                ss << "\t]" << std::endl;

                ss << "\tmethods_count: " << methods_count() << std::endl;
                ss << "\tmethods_info: [" << std::endl;

                for (size_t i = 0; i < methods.size(); ++i) {
                        auto &method = methods[i];
                        ss << "\t\t{" << std::endl;
                        ss << "\t\t\taccess_flags: " << method.access_flags << std::endl;
                        ss << "\t\t\tname_index: " << method.name_index << std::endl;
                        ss << "\t\t\tdescriptor_index: " << method.descriptor_index << std::endl;
                        ss << "\t\t\tattributes_count: " << method.attributes.size() << std::endl;
                        // TODO: Show attributes
                        ss << "\t\t\tattributes: [" << std::endl;
                        for (size_t j = 0; j < method.attributes.size(); ++j) {
                                auto &attribute = method.attributes[j];
                                ss << "\t\t\t\t{" << std::endl;
                                ss << "\t\t\t\t\tattribute_name_index: " << attribute.attribute_name_index << std::endl;
                                ss << "\t\t\t\t\tattribute_length: " << attribute.info.size() << std::endl;
                                ss << "\t\t\t\t\tinfo: [ ";
                                for (size_t k = 0; k < attribute.info.size(); ++k) {
                                        ss << std::hex << static_cast<int>(attribute.info[k]) << std::dec << " ";
                                }
                                ss << "]" << std::endl;
                                ss << "\t\t\t\t}" << std::endl;
                        }
                        ss << "\t\t\t]" << std::endl;

                        ss << "\t\t}, " << std::endl;
                }

                ss << "\t]" << std::endl;

                ss << "\tattributes_count: " << attributes_count() << std::endl;
                ss << "\tattributes_info: [" << std::endl;

                for (size_t i = 0; i < attributes.size(); ++i) {
                        auto &attribute = attributes[i];
                        ss << "\t\t{" << std::endl;
                        ss << "\t\t\tattribute_name_index: " << attribute.attribute_name_index << std::endl;
                        ss << "\t\t\tattribute_length: " << attribute.info.size() << std::endl;
                        ss << "\t\t\tinfo: [ ";
                        for (size_t j = 0; j < attribute.info.size(); ++j) {
                                ss << std::hex << static_cast<int>(attribute.info[j]) << std::dec << " ";
                        }
                        ss << "]" << std::endl;
                        ss << "\t\t}, " << std::endl;
                }

                ss << "\t]" << std::endl;
                

                ss << "}";

                return ss.str();
        }
public:
        inline ClassFile(u4 magic, u2 minor, u2 major, u2 constant_pool_count, std::vector<cp_info> constant_pool,
                         u2 access_flags, u2 this_class, u2 super_class, std::vector<u2> interfaces,
                         std::vector<field_info> fields, std::vector<method_info> methods,
                         std::vector<attribute_info> attributes, std::vector<uint8_t> original_bytes)
                : magic(magic), minor(minor), major(major), constant_pool_count(constant_pool_count),
                constant_pool(constant_pool), access_flags(access_flags), this_class(this_class),
                super_class(super_class), interfaces(interfaces), fields(fields), methods(methods),
                attributes(attributes), original_bytes(original_bytes)
        {}

        DEFINE_GETTER(magic)
        DEFINE_GETTER(minor)
        DEFINE_GETTER(major)
        DEFINE_GETTER(constant_pool_count)
        DEFINE_GETTER(constant_pool)
        DEFINE_GETTER(access_flags)
        DEFINE_GETTER(this_class)
        DEFINE_GETTER(super_class)
        DEFINE_GETTER(interfaces)
        DEFINE_GETTER(fields)
        DEFINE_GETTER(methods)
        DEFINE_GETTER(attributes)
        DEFINE_GETTER(original_bytes)

        inline u2 interfaces_count()
        {
                return this->interfaces.size();
        }

        inline u2 fields_count()
        {
                return this->fields.size();
        }

        inline u2 methods_count()
        {
                return this->methods.size();
        }

        inline u2 attributes_count()
        {
                return this->attributes.size();
        }

        inline cp_info &get_constant_pool_item(u2 index)
        {
                return this->constant_pool[index];
        }

        // Big-endian indexing for ease-of-use
        inline cp_info &get_constant_pool_item_be(u2 index)
        {
                u1 lo = index >> 8;
                u1 hi = index & 0xff;
                u2 le = (hi << 8) | lo;
                return this->get_constant_pool_item(le);
        }

        inline void set_constant_pool_item(u2 index, cp_info value)
        {
                this->constant_pool[index] = value;
        }

        inline void set_constant_pool_item_be(u2 index, cp_info value)
        {
                u1 lo = index >> 8;
                u1 hi = index & 0xff;
                u2 le = (hi << 8) | lo;

                this->set_constant_pool_item(le, value);
        }
};

#endif
