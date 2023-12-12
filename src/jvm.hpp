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

#ifndef JVM_HPP
#define JVM_HPP

#include <unordered_map>
#include <optional>
#include <string>
#include <cstdint>

/* JVM definitions */
typedef struct {
	const char *typeName;
	const char *fieldName;
	const char *typeString;
	int32_t isStatic;
	uint64_t offset;
	void *address;
} VMStructEntry;

typedef struct {
	const char *typeName;
	const char *superclassName;
	int32_t isOopType;
	int32_t isIntegerType;
	int32_t isUnsigned;
	uint64_t size;
} VMTypeEntry;

/* Wrappers */
class VMTypes {
public:
	typedef std::unordered_map<std::string, VMStructEntry *> struct_entry_t;
	typedef std::unordered_map<std::string, struct_entry_t> struct_entries_t;
	typedef std::unordered_map<std::string, VMTypeEntry *> type_entries_t;
private:
	static struct_entries_t struct_entries;
	static type_entries_t type_entries;
public:
	static void init(VMStructEntry *vmstructs, VMTypeEntry *vmtypes);
	static std::optional<std::reference_wrapper<struct_entry_t>> findTypeFields(const char *typeName);
	static std::optional<VMTypeEntry *> findType(const char *typeName);
};

class VMType {
private:
	VMTypeEntry *type_entry;
	std::optional<std::reference_wrapper<VMTypes::struct_entry_t>> fields;
	void *instance; // pointer to instantiated VM type

	inline std::optional<void *> find_field_address(const char *fieldName)
	{
		auto tbl = fields.value().get();
		auto entry = tbl.find(fieldName);
		if (entry == tbl.end())
			return std::nullopt;

		auto field = entry->second;
		void *fieldAddress;
		if (field->isStatic)
			fieldAddress = (void *)field->address;
		else
			fieldAddress = (void *)((uint64_t)this->instance + field->offset);

		return fieldAddress;
	}

public:
	// the following function will lookup the type in the
	// VMTypes. If it is found, return successful std::optional
	static std::optional<VMType> from_instance(const char *typeName, void *instance);

	// helper functions for accessing fields
	template <typename T>
	std::optional<T> get_field(const char *fieldName)
	{
		auto fieldAddress = find_field_address(fieldName);
		if (!fieldAddress.has_value())
			return std::nullopt;

		return *(T *)(fieldAddress.value());
	}

	template <typename T>
	bool set_field(const char *fieldName, T value)
	{
		auto fieldAddress = find_field_address(fieldName);
		if (!fieldAddress.has_value())
			return false;

		*(T *)(fieldAddress.value()) = value;

		return true;
	}

	inline void *get_instance()
	{
		return this->instance;
	}
};

#endif
