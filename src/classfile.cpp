#include "classfile.hpp"
#include <cstring>
#include <iostream> // TODO: Remove

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

std::unique_ptr<ClassFile>
ClassFile::load(const uint8_t *classfile_bytes)
{
	uint8_t *raw = const_cast<uint8_t *>(classfile_bytes);
	size_t index = 0;
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

	cp_info cpi;
	for (u2 i = 1; i < constant_pool_count; ++i, constant_pool.push_back(cpi)) {
		cpi.bytes.clear();
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

				continue;
			}
		case CONSTANT_Fieldref:
			{
				CONSTANT_Fieldref_info ci;

				ci.tag = tag;
				cf_read_be(&ci.class_index, raw, index);
				cf_read_be(&ci.name_and_type_index, raw, index);

				cpi.bytes.resize(sizeof(ci));
				memcpy(cpi.bytes.data(), &ci, sizeof(ci));

				continue;
			}
		case CONSTANT_Methodref:
			{
				CONSTANT_Methodref_info ci;

				ci.tag = tag;
				cf_read_be(&ci.class_index, raw, index);
				cf_read_be(&ci.name_and_type_index, raw, index);

				cpi.bytes.resize(sizeof(ci));
				memcpy(cpi.bytes.data(), &ci, sizeof(ci));

				continue;
			}
		case CONSTANT_InterfaceMethodref:
			{
				CONSTANT_InterfaceMethodref_info ci;

				ci.tag = tag;
				cf_read_be(&ci.class_index, raw, index);
				cf_read_be(&ci.name_and_type_index, raw, index);

				cpi.bytes.resize(sizeof(ci));
				memcpy(cpi.bytes.data(), &ci, sizeof(ci));

				continue;
			}
		case CONSTANT_String:
			{
				CONSTANT_String_info ci;

				ci.tag = tag;
				cf_read_be(&ci.string_index, raw, index);

				cpi.bytes.resize(sizeof(ci));
				memcpy(cpi.bytes.data(), &ci, sizeof(ci));

				continue;
			}
		case CONSTANT_Integer:
			{
				CONSTANT_Integer_info ci;

				ci.tag = tag;
				cf_read_be(&ci.bytes, raw, index);

				cpi.bytes.resize(sizeof(ci));
				memcpy(cpi.bytes.data(), &ci, sizeof(ci));

				continue;
			}
		case CONSTANT_Float:
			{
				CONSTANT_Float_info ci;

				ci.tag = tag;
				cf_read_be(&ci.bytes, raw, index);

				cpi.bytes.resize(sizeof(ci));
				memcpy(cpi.bytes.data(), &ci, sizeof(ci));

				continue;
			}
		case CONSTANT_Long:
			{
				CONSTANT_Long_info ci;

				ci.tag = tag;
				cf_read_be(&ci.high_bytes, raw, index);
				cf_read_be(&ci.low_bytes, raw, index);

				cpi.bytes.resize(sizeof(ci));
				memcpy(cpi.bytes.data(), &ci, sizeof(ci));

				continue;
			}
		case CONSTANT_Double:
			{
				CONSTANT_Double_info ci;

				ci.tag = tag;
				cf_read_be(&ci.high_bytes, raw, index);
				cf_read_be(&ci.low_bytes, raw, index);

				cpi.bytes.resize(sizeof(ci));
				memcpy(cpi.bytes.data(), &ci, sizeof(ci));

				continue;
			}
		case CONSTANT_NameAndType:
			{
				CONSTANT_NameAndType_info ci;

				ci.tag = tag;
				cf_read_be(&ci.name_index, raw, index);
				cf_read_be(&ci.descriptor_index, raw, index);

				cpi.bytes.resize(sizeof(ci));
				memcpy(cpi.bytes.data(), &ci, sizeof(ci));

				continue;
			}
		case CONSTANT_Utf8:
			{
				CONSTANT_Utf8_info ci;
				size_t size;

				ci.tag = tag;
				cf_read_be(&ci.length, raw, index);

				cpi.bytes.resize(sizeof(ci) + ci.length);
				memcpy(cpi.bytes.data(), &ci, sizeof(ci));
				cf_read(&cpi.bytes.data()[sizeof(ci)], raw, index, ci.length);

				continue;
			}
		case CONSTANT_MethodHandle:
			{
				CONSTANT_MethodHandle_info ci;

				ci.tag = tag;
				cf_read_be(&ci.reference_kind, raw, index);
				cf_read_be(&ci.reference_index, raw, index);

				cpi.bytes.resize(sizeof(ci));
				memcpy(cpi.bytes.data(), &ci, sizeof(ci));

				continue;
			}
		case CONSTANT_MethodType:
			{
				CONSTANT_MethodType_info ci;

				ci.tag = tag;
				cf_read_be(&ci.descriptor_index, raw, index);

				cpi.bytes.resize(sizeof(ci));
				memcpy(cpi.bytes.data(), &ci, sizeof(ci));

				continue;
			}
		case CONSTANT_InvokeDynamic:
			{
				CONSTANT_InvokeDynamic_info ci;

				ci.tag = tag;
				cf_read_be(&ci.bootstrap_method_attr_index, raw, index);
				cf_read_be(&ci.name_and_type_index, raw, index);

				cpi.bytes.resize(sizeof(ci));
				memcpy(cpi.bytes.data(), &ci, sizeof(ci));

				continue;
			}
		default:
			// TODO: Log about unknown tag
			break;
		}

		break;
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

	return std::make_unique<ClassFile>(magic, minor, major, constant_pool, access_flags,
	                                   this_class, super_class, interfaces, fields,
	                                   methods, attributes);
}
