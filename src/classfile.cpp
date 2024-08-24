#include "classfile.hpp"
#include <cstring>

template <typename T>
void cf_read(T *dest, uint8_t *raw, size_t &index)
{
	memcpy(reinterpret_cast<void *>(dest), reinterpret_cast<void *>(&raw[index]), sizeof(T));
	index += sizeof(T);
}

/// Read big-endian bytes
template <typename T>
void cf_read_be(T *dest, uint8_t *raw, size_t &index)
{
	for (size_t i = 0; i < sizeof(T); ++i) {
		reinterpret_cast<uint8_t *>(dest)[i] = raw[index + i - 1];
	}
	index += sizeof(T);
}

std::unique_ptr<ClassFile>
ClassFile::load(std::vector<uint8_t> &classfile_bytes)
{
	uint8_t *raw = classfile_bytes.data();
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

	// Magic
	cf_read(&magic, raw, index);

	// Version
	cf_read_be(&minor, raw, index);

	return std::make_unique<ClassFile>(magic, minor, major, constant_pool, access_flags,
	                                   this_class, super_class, interfaces, fields,
	                                   methods, attributes);
}
