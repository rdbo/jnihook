/*
 *  -----------------------------------
 * |         JNIHook - by rdbo         |
 * |      Java VM Hooking Library      |
 *  -----------------------------------
 */

/*
 * Copyright (C) 2026    Rdbo
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

#include "jvm.hpp"

/* VMTypes */

VMTypes::struct_entries_t VMTypes::struct_entries;
VMTypes::type_entries_t VMTypes::type_entries;

void VMTypes::init(VMStructEntry *vmstructs, VMTypeEntry *vmtypes)
{
        for (int i = 0; vmstructs[i].typeName != NULL; ++i) {
                auto s = &vmstructs[i];
                VMTypes::struct_entries[s->typeName][s->fieldName] = s;
        }

        for (int i = 0; vmtypes[i].typeName != NULL; ++i) {
                auto t = &vmtypes[i];
                VMTypes::type_entries[t->typeName] = t;
        }
}

std::optional<std::reference_wrapper<VMTypes::struct_entry_t>> VMTypes::find_type_fields(const char *typeName)
{
        for (auto &m : VMTypes::struct_entries) {
                if (m.first == typeName)
                        return m.second;
        }

        return std::nullopt;
}

std::optional<VMTypeEntry *> VMTypes::find_type(const char *typeName)
{
        auto t = type_entries.find(typeName);
        if (t == type_entries.end())
                return std::nullopt;

        return t->second;
}

/* VMType */
std::optional<VMType> VMType::from_instance(const char *typeName, void *instance)
{
        return from_static(typeName)
                .and_then([instance](auto x){
                        x.instance = instance;
                        return std::optional(x);
                });
}

std::optional<VMType> VMType::from_static(const char *typeName)
{
        auto type = VMTypes::find_type(typeName);
        if (!type.has_value())
                return std::nullopt;

        auto fields = VMTypes::find_type_fields(typeName);
        if (!fields.has_value())
                return std::nullopt;
        
        VMType vmtype;
        vmtype.instance = NULL;
        vmtype.type_name = std::string(typeName);
        vmtype.type_entry = type.value();
        vmtype.fields = fields;

        return vmtype;
}
