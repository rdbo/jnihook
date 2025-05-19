#include "classfile.hpp"
#include <cstring>

template <typename T>
void cf_read(T *dest, uint8_t *raw, size_t &index)
{
        memcpy(reinterpret_cast<void *>(dest), reinterpret_cast<void *>(&raw[index]), sizeof(T));
        index += sizeof(T);
}

template <typename T>
void cf_read(T *dest, uint8_t *raw, size_t &index, size_t size)
{
        memcpy(reinterpret_cast<void *>(dest), reinterpret_cast<void *>(&raw[index]), size);
        index += size;
}

/// Read big-endian bytes
template <typename T>
void cf_read_be(T *dest, uint8_t *raw, size_t &index)
{
        for (size_t i = 0; i < sizeof(T); ++i) {
                reinterpret_cast<uint8_t *>(dest)[i] = raw[index + sizeof(T) - i - 1];
        }
        index += sizeof(T);
}

template <typename T>
void cf_push(std::vector<uint8_t> &dest, T *source)
{
        uint8_t *ptr = reinterpret_cast<uint8_t *>(source);

        for (int i = 0; i < sizeof(T); ++i) {
                dest.push_back(ptr[i]);
        }
}

template <typename T>
void cf_push(std::vector<uint8_t> &dest, T *source, size_t size)
{
        uint8_t *ptr = reinterpret_cast<uint8_t *>(source);

        for (size_t i = 0; i < size; ++i) {
                dest.push_back(ptr[i]);
        }
}

template <typename T>
void cf_push_be(std::vector<uint8_t> &dest, T *source)
{
        uint8_t *ptr = reinterpret_cast<uint8_t *>(source);

        for (int i = sizeof(T) - 1; i >= 0; --i) {
                dest.push_back(ptr[i]);
        }
}

std::unique_ptr<ClassFile>
ClassFile::load(const uint8_t *classfile_bytes)
{
        uint8_t *raw = const_cast<uint8_t *>(classfile_bytes);
        size_t index = 0;
        u4 magic;
        u2 minor;
        u2 major;
        cp_info empty_cpi = {{ 0 }};
        std::vector<cp_info> constant_pool = { empty_cpi };
        u2 access_flags;
        u2 this_class;
        u2 super_class;
        std::vector<u2> interfaces;
        std::vector<field_info> fields;
        std::vector<method_info> methods;
        std::vector<attribute_info> attributes;
        u2 constant_pool_count;
        u2 interfaces_count;
        u2 fields_count;
        u2 methods_count;
        u2 attributes_count;

        // Magic
        cf_read_be(&magic, raw, index);

        // Version
        cf_read_be(&minor, raw, index);
        cf_read_be(&major, raw, index);

        // Constant Pool
        cf_read_be(&constant_pool_count, raw, index);

        for (u2 i = 1; i < constant_pool_count; ++i) {
                cp_info cpi;
                u1 tag;

                cf_read_be(&tag, raw, index);

                switch (tag) {
                case CONSTANT_Class:
                        {
                                CONSTANT_Class_info ci;

                                ci.tag = tag;
                                cf_read_be(&ci.name_index, raw, index);

                                cpi.bytes.resize(sizeof(ci));
                                memcpy(cpi.bytes.data(), &ci, sizeof(ci));

                                break;
                        }
                case CONSTANT_Fieldref:
                        {
                                CONSTANT_Fieldref_info ci;

                                ci.tag = tag;
                                cf_read_be(&ci.class_index, raw, index);
                                cf_read_be(&ci.name_and_type_index, raw, index);

                                cpi.bytes.resize(sizeof(ci));
                                memcpy(cpi.bytes.data(), &ci, sizeof(ci));

                                break;
                        }
                case CONSTANT_Methodref:
                        {
                                CONSTANT_Methodref_info ci;

                                ci.tag = tag;
                                cf_read_be(&ci.class_index, raw, index);
                                cf_read_be(&ci.name_and_type_index, raw, index);

                                cpi.bytes.resize(sizeof(ci));
                                memcpy(cpi.bytes.data(), &ci, sizeof(ci));

                                break;
                        }
                case CONSTANT_InterfaceMethodref:
                        {
                                CONSTANT_InterfaceMethodref_info ci;

                                ci.tag = tag;
                                cf_read_be(&ci.class_index, raw, index);
                                cf_read_be(&ci.name_and_type_index, raw, index);

                                cpi.bytes.resize(sizeof(ci));
                                memcpy(cpi.bytes.data(), &ci, sizeof(ci));

                                break;
                        }
                case CONSTANT_String:
                        {
                                CONSTANT_String_info ci;

                                ci.tag = tag;
                                cf_read_be(&ci.string_index, raw, index);

                                cpi.bytes.resize(sizeof(ci));
                                memcpy(cpi.bytes.data(), &ci, sizeof(ci));

                                break;
                        }
                case CONSTANT_Integer:
                        {
                                CONSTANT_Integer_info ci;

                                ci.tag = tag;
                                cf_read_be(&ci.bytes, raw, index);

                                cpi.bytes.resize(sizeof(ci));
                                memcpy(cpi.bytes.data(), &ci, sizeof(ci));

                                break;
                        }
                case CONSTANT_Float:
                        {
                                CONSTANT_Float_info ci;

                                ci.tag = tag;
                                cf_read_be(&ci.bytes, raw, index);

                                cpi.bytes.resize(sizeof(ci));
                                memcpy(cpi.bytes.data(), &ci, sizeof(ci));

                                break;
                        }
                case CONSTANT_Long:
                        {
                                CONSTANT_Long_info ci;

                                ci.tag = tag;
                                cf_read_be(&ci.high_bytes, raw, index);
                                cf_read_be(&ci.low_bytes, raw, index);

                                cpi.bytes.resize(sizeof(ci));
                                memcpy(cpi.bytes.data(), &ci, sizeof(ci));

                                break;
                        }
                case CONSTANT_Double:
                        {
                                CONSTANT_Double_info ci;

                                ci.tag = tag;
                                cf_read_be(&ci.high_bytes, raw, index);
                                cf_read_be(&ci.low_bytes, raw, index);

                                cpi.bytes.resize(sizeof(ci));
                                memcpy(cpi.bytes.data(), &ci, sizeof(ci));

                                break;
                        }
                case CONSTANT_NameAndType:
                        {
                                CONSTANT_NameAndType_info ci;

                                ci.tag = tag;
                                cf_read_be(&ci.name_index, raw, index);
                                cf_read_be(&ci.descriptor_index, raw, index);

                                cpi.bytes.resize(sizeof(ci));
                                memcpy(cpi.bytes.data(), &ci, sizeof(ci));

                                break;
                        }
                case CONSTANT_Utf8:
                        {
                                CONSTANT_Utf8_info ci;

                                ci.tag = tag;
                                cf_read_be(&ci.length, raw, index);

                                cpi.bytes.resize(sizeof(ci) + ci.length);
                                memcpy(cpi.bytes.data(), &ci, sizeof(ci));
                                cf_read(&cpi.bytes.data()[sizeof(ci)], raw, index, ci.length);

                                break;
                        }
                case CONSTANT_MethodHandle:
                        {
                                CONSTANT_MethodHandle_info ci;

                                ci.tag = tag;
                                cf_read_be(&ci.reference_kind, raw, index);
                                cf_read_be(&ci.reference_index, raw, index);

                                cpi.bytes.resize(sizeof(ci));
                                memcpy(cpi.bytes.data(), &ci, sizeof(ci));

                                break;
                        }
                case CONSTANT_MethodType:
                        {
                                CONSTANT_MethodType_info ci;

                                ci.tag = tag;
                                cf_read_be(&ci.descriptor_index, raw, index);

                                cpi.bytes.resize(sizeof(ci));
                                memcpy(cpi.bytes.data(), &ci, sizeof(ci));

                                break;
                        }
                case CONSTANT_InvokeDynamic:
                        {
                                CONSTANT_InvokeDynamic_info ci;

                                ci.tag = tag;
                                cf_read_be(&ci.bootstrap_method_attr_index, raw, index);
                                cf_read_be(&ci.name_and_type_index, raw, index);

                                cpi.bytes.resize(sizeof(ci));
                                memcpy(cpi.bytes.data(), &ci, sizeof(ci));

                                break;
                        }
                default:
                        return nullptr;
                }

                constant_pool.push_back(cpi);
                /*
                 * From Oracle: "All 8-byte constants take up two entries in the constant_pool table of the class file.
                 * If a CONSTANT_Long_info or CONSTANT_Double_info structure is the item in the constant_pool table at
                 * index n, then the next usable item in the pool is located at index n+2. The constant_pool index n+1
                 * must be valid but is considered unusable".
                 */
                if (tag == CONSTANT_Long || tag == CONSTANT_Double) {
                        constant_pool.push_back(empty_cpi);
                        ++i;
                }
        }

        // Access Flags
        cf_read_be(&access_flags, raw, index);

        // Classes
        cf_read_be(&this_class, raw, index);
        cf_read_be(&super_class, raw, index);

        // Interfaces
        cf_read_be(&interfaces_count, raw, index);

        for (size_t i = 0; i < interfaces_count; ++i) {
                u2 interface;

                cf_read_be(&interface, raw, index);
                interfaces.push_back(interface);
        }

        // Fields
        cf_read_be(&fields_count, raw, index);

        for (size_t i = 0; i < fields_count; ++i) {
                field_info fi;
                u2 attributes_count;

                cf_read_be(&fi.access_flags, raw, index);
                cf_read_be(&fi.name_index, raw, index);
                cf_read_be(&fi.descriptor_index, raw, index);
                cf_read_be(&attributes_count, raw, index);

                attribute_info ai;
                for (size_t j = 0; j < attributes_count; ++j) {
                        u4 attribute_length;

                        cf_read_be(&ai.attribute_name_index, raw, index);
                        cf_read_be(&attribute_length, raw, index);

                        ai.info.resize(attribute_length);
                        cf_read(ai.info.data(), raw, index, attribute_length);
                        fi.attributes.push_back(ai);
                }

                fields.push_back(fi);
        }

        // Methods
        cf_read_be(&methods_count, raw, index);

        for (size_t i = 0; i < methods_count; ++i) {
                method_info mi;
                u2 attributes_count;

                cf_read_be(&mi.access_flags, raw, index);
                cf_read_be(&mi.name_index, raw, index);
                cf_read_be(&mi.descriptor_index, raw, index);
                cf_read_be(&attributes_count, raw, index);

                attribute_info ai;
                for (size_t j = 0; j < attributes_count; ++j) {
                        u4 attribute_length;

                        cf_read_be(&ai.attribute_name_index, raw, index);
                        cf_read_be(&attribute_length, raw, index);

                        ai.info.resize(attribute_length);
                        cf_read(ai.info.data(), raw, index, attribute_length);
                        mi.attributes.push_back(ai);
                }

                methods.push_back(mi);
        }

        // Attributes
        cf_read_be(&attributes_count, raw, index);

        for (size_t i = 0; i < attributes_count; ++i) {
                attribute_info ai;
                u4 attribute_length;

                cf_read_be(&ai.attribute_name_index, raw, index);
                cf_read_be(&attribute_length, raw, index);

                ai.info.resize(attribute_length);
                cf_read(ai.info.data(), raw, index, attribute_length);
                attributes.push_back(ai);
        }

        auto original_bytes = std::vector<uint8_t>(classfile_bytes, &classfile_bytes[index]);

        return std::make_unique<ClassFile>(magic, minor, major, constant_pool_count, constant_pool,
                                           access_flags, this_class, super_class, interfaces, fields,
                                           methods, attributes, original_bytes);
}

std::vector<uint8_t>
ClassFile::bytes()
{
        std::vector<uint8_t> bytes = {};
        u2 constant_pool_count = this->constant_pool_count;
        u2 interfaces_count = this->interfaces_count();
        u2 fields_count = this->fields_count();
        u2 methods_count = this->methods_count();
        u2 attributes_count = this->attributes_count();

        cf_push_be(bytes, &this->magic);
        cf_push_be(bytes, &this->minor);
        cf_push_be(bytes, &this->major);
        cf_push_be(bytes, &constant_pool_count);

        for (size_t i = 0; i < this->constant_pool.size(); ++i) {
                auto &cpi = this->constant_pool[i];
                u1 tag = cpi.bytes[0];

                // Read comments on 'ClassFile::load' to find out why. Not my fault.
                if (tag == 0)
                        continue;

                cf_push_be(bytes, &tag);

                switch (tag) {
                case CONSTANT_Class:
                        {
                                auto ci = reinterpret_cast<CONSTANT_Class_info *>(cpi.bytes.data());

                                cf_push_be(bytes, &ci->name_index);
                                
                                break;
                        }
                case CONSTANT_Fieldref:
                        {
                                auto ci = reinterpret_cast<CONSTANT_Fieldref_info *>(cpi.bytes.data());

                                cf_push_be(bytes, &ci->class_index);
                                cf_push_be(bytes, &ci->name_and_type_index);

                                break;
                        }
                case CONSTANT_Methodref:
                        {
                                auto ci = reinterpret_cast<CONSTANT_Fieldref_info *>(cpi.bytes.data());

                                cf_push_be(bytes, &ci->class_index);
                                cf_push_be(bytes, &ci->name_and_type_index);

                                break;
                        }
                case CONSTANT_InterfaceMethodref:
                        {
                                auto ci = reinterpret_cast<CONSTANT_Fieldref_info *>(cpi.bytes.data());

                                cf_push_be(bytes, &ci->class_index);
                                cf_push_be(bytes, &ci->name_and_type_index);

                                break;
                        }
                case CONSTANT_String:
                        {
                                auto ci = reinterpret_cast<CONSTANT_String_info *>(cpi.bytes.data());

                                cf_push_be(bytes, &ci->string_index);

                                break;
                        }
                case CONSTANT_Integer:
                        {
                                auto ci = reinterpret_cast<CONSTANT_Integer_info *>(cpi.bytes.data());

                                cf_push_be(bytes, &ci->bytes);

                                break;
                        }
                case CONSTANT_Float:
                        {
                                auto ci = reinterpret_cast<CONSTANT_Float_info *>(cpi.bytes.data());

                                cf_push_be(bytes, &ci->bytes);

                                break;
                        }
                case CONSTANT_Long:
                        {
                                auto ci = reinterpret_cast<CONSTANT_Long_info *>(cpi.bytes.data());

                                cf_push_be(bytes, &ci->high_bytes);
                                cf_push_be(bytes, &ci->low_bytes);

                                break;
                        }
                case CONSTANT_Double:
                        {
                                auto ci = reinterpret_cast<CONSTANT_Double_info *>(cpi.bytes.data());

                                cf_push_be(bytes, &ci->high_bytes);
                                cf_push_be(bytes, &ci->low_bytes);

                                break;
                        }
                case CONSTANT_NameAndType:
                        {
                                auto ci = reinterpret_cast<CONSTANT_NameAndType_info *>(cpi.bytes.data());

                                cf_push_be(bytes, &ci->name_index);
                                cf_push_be(bytes, &ci->descriptor_index);

                                break;
                        }
                case CONSTANT_Utf8:
                        {
                                auto ci = reinterpret_cast<CONSTANT_Utf8_info *>(cpi.bytes.data());

                                cf_push_be(bytes, &ci->length);
                                cf_push(bytes, &ci->bytes, ci->length);

                                break;
                        }
                case CONSTANT_MethodHandle:
                        {
                                auto ci = reinterpret_cast<CONSTANT_MethodHandle_info *>(cpi.bytes.data());

                                cf_push_be(bytes, &ci->reference_kind);
                                cf_push_be(bytes, &ci->reference_index);

                                break;
                        }
                case CONSTANT_MethodType:
                        {
                                auto ci = reinterpret_cast<CONSTANT_MethodType_info *>(cpi.bytes.data());

                                cf_push_be(bytes, &ci->descriptor_index);

                                break;
                        }
                case CONSTANT_InvokeDynamic:
                        {
                                auto ci = reinterpret_cast<CONSTANT_InvokeDynamic_info *>(cpi.bytes.data());

                                cf_push_be(bytes, &ci->bootstrap_method_attr_index);
                                cf_push_be(bytes, &ci->name_and_type_index);

                                break;
                        }
                }
        }

        cf_push_be(bytes, &this->access_flags);
        cf_push_be(bytes, &this->this_class);
        cf_push_be(bytes, &this->super_class);
        cf_push_be(bytes, &interfaces_count);

        for (auto interface : this->interfaces) {
                cf_push_be(bytes, &interface);
        }

        cf_push_be(bytes, &fields_count);

        for (auto &field : this->fields) {
                u2 attributes_count = field.attributes.size();

                cf_push_be(bytes, &field.access_flags);
                cf_push_be(bytes, &field.name_index);
                cf_push_be(bytes, &field.descriptor_index);
                cf_push_be(bytes, &attributes_count);
                for (auto &attr : field.attributes) {
                        u4 attribute_length = attr.info.size();

                        cf_push_be(bytes, &attr.attribute_name_index);
                        cf_push_be(bytes, &attribute_length);
                        cf_push(bytes, attr.info.data(), attribute_length);
                }
        }

        cf_push_be(bytes, &methods_count);

        for (auto &method : this->methods) {
                u2 attributes_count = method.attributes.size();

                cf_push_be(bytes, &method.access_flags);
                cf_push_be(bytes, &method.name_index);
                cf_push_be(bytes, &method.descriptor_index);
                cf_push_be(bytes, &attributes_count);
                for (auto &attr : method.attributes) {
                        u4 attribute_length = attr.info.size();

                        cf_push_be(bytes, &attr.attribute_name_index);
                        cf_push_be(bytes, &attribute_length);
                        cf_push(bytes, attr.info.data(), attribute_length);
                }
        }

        cf_push_be(bytes, &attributes_count);

        for (auto &attr : this->attributes) {
                u4 attribute_length = attr.info.size();

                cf_push_be(bytes, &attr.attribute_name_index);
                cf_push_be(bytes, &attribute_length);
                cf_push(bytes, attr.info.data(), attribute_length);
        }

        return bytes;
}
