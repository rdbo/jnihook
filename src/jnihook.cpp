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

#include <algorithm>
#include <atomic>
#include <jnihook.h>
#include <map>
#include <sstream>
#include <unordered_map>
#include <string>
#include <vector>
#include <cstring>
#include <jnif.hpp>
#include "jvm.hpp"
#include "uuid.hpp"
#ifdef JNIHOOK_DEBUG
        #define LOG(...) {printf("[JNIHOOK] " __VA_ARGS__);fflush(stdout);}
#else
        #define LOG(...)
#endif

extern "C" JNIIMPORT VMStructEntry *gHotSpotVMStructs;
extern "C" JNIIMPORT VMTypeEntry *gHotSpotVMTypes;

using namespace jnif;

typedef struct jnihook_t {
        JavaVM   *jvm;
        jvmtiEnv *jvmti;
} jnihook_t;

typedef struct method_info_t {
        std::string name;
        std::string signature;
        jint access_flags;
} method_info_t;

typedef struct hook_info_t {
        method_info_t method_info;
        void *native_hook_method;
        size_t hook_offset = 0;
} hook_info_t;

enum hkType {
    hook_default = 0,
    hook_init,
    hook_clinit,
    hook_midfunction,
};

static std::unique_ptr<jnihook_t> g_jnihook = nullptr;
static std::unordered_map<std::string, std::vector<hook_info_t>> g_hooks;
static std::unordered_map<std::string, std::unique_ptr<ClassFile>> g_class_file_cache;
// static std::unordered_map<std::string, jclass> g_original_classes;
static std::atomic<bool> g_force_class_caching = false;

static std::string
get_class_signature(jvmtiEnv *jvmti, jclass clazz)
{
        char *sig;
        
        if (jvmti->GetClassSignature(clazz, &sig, NULL) != JVMTI_ERROR_NONE) {
                return "";
        }

        std::string signature = std::string(sig, &sig[strlen(sig)]);

        jvmti->Deallocate(reinterpret_cast<unsigned char *>(sig));

        return signature;
}

static std::string
get_class_name(JNIEnv *env, jclass clazz)
{
        jclass klass = env->FindClass("java/lang/Class");
        if (!klass)
                return "";

        jmethodID getName_method = env->GetMethodID(klass, "getName", "()Ljava/lang/String;");
        if (!getName_method)
                return "";

        jstring name_obj = reinterpret_cast<jstring>(env->CallObjectMethod(clazz, getName_method));
        if (!name_obj)
                return "";

        const char *c_name = env->GetStringUTFChars(name_obj, 0);
        if (!c_name)
                return "";

        std::string name = std::string(c_name, &c_name[strlen(c_name)]);

        env->ReleaseStringUTFChars(name_obj, c_name);

        // Replace dots with slashes to match contents of ClassFile
        for (size_t i = 0; i < name.length(); ++i) {
                if (name[i] == '.')
                        name[i] = '/';
        }
        
        return name;
}

static std::unique_ptr<method_info_t>
get_method_info(jvmtiEnv *jvmti, jmethodID method)
{
        char *name;
        char *sig;
        jint access_flags;
        
        if (jvmti->GetMethodName(method, &name, &sig, NULL) != JVMTI_ERROR_NONE)
                return nullptr;

        if (jvmti->GetMethodModifiers(method, &access_flags) != JVMTI_ERROR_NONE)
                return nullptr;

        std::string name_str(name, &name[strlen(name)]);
        std::string signature_str(sig, &sig[strlen(sig)]);

        jvmti->Deallocate(reinterpret_cast<unsigned char *>(name));
        jvmti->Deallocate(reinterpret_cast<unsigned char *>(sig));

        return std::make_unique<method_info_t>(method_info_t { name_str, signature_str, access_flags });
}
// generates name for new native method
static std::string
get_copy_method_name(const std::string &method_name, const std::string& class_name)
{
        static std::string uuid = GenerateUuid();
        std::string clazz = class_name + "_";
        std::replace(clazz.begin(), clazz.end(), '/', '_');
        // init methods have additional "constructor" to not overlap possible method named init or clinit
        if (method_name == "<init>") {
            return "init_____jnihook_constructor_" + clazz + uuid;
        }
        else if (method_name == "<clinit>") {
            return "clinit_____jnihook_constructor_" + clazz + uuid;
        }
        return method_name + "_____jnihook_" + clazz + uuid;
}
// generates name for new copy of original method
static std::string
get_copy_clone_name(const std::string& method_name, const std::string& class_name)
{
    static std::string uuid = GenerateUuid();
    std::string clazz = class_name + "_";
    std::replace(clazz.begin(), clazz.end(), '/', '_');
    // init methods have additional "constructor" to not overlap possible method named init or clinit
    if (method_name == "<init>") {
        return "init_clone_____jnihook_constructor_" + clazz + uuid;
    }
    else if (method_name == "<clinit>") {
        return "clinit_clone_____jnihook_constructor_" + clazz + uuid;
    }
    return method_name + "_clone_____jnihook_" + clazz + uuid;
}

void JNICALL JNIHook_ClassFileLoadHook(jvmtiEnv *jvmti_env,
                                       JNIEnv* jni_env,
                                       jclass class_being_redefined,
                                       jobject loader,
                                       const char* name,
                                       jobject protection_domain,
                                       jint class_data_len,
                                       const unsigned char* class_data,
                                       jint* new_class_data_len,
                                       unsigned char** new_class_data)
{
        auto class_name = get_class_name(jni_env, class_being_redefined);

        // Don't do anything for unhooked classes
        // (unless g_force_class_caching is true)
        if (class_name == "" || (g_hooks.find(class_name) == g_hooks.end() || g_hooks[class_name].size() == 0) && !g_force_class_caching)
                return;

        // Cache parsed ClassFile if it's not cached yet
        if (g_class_file_cache.find(class_name) == g_class_file_cache.end()) {
                auto cf = ClassFile::parse((u1 *)class_data, class_data_len);
                if (!cf)
                        return;

#ifdef JNIHOOK_DEBUG
                // Assert that parsed class is the same as original class
                auto bytes = cf->toBytes();
                auto len = static_cast<jint>(bytes.size());
                bool check = true;
                if (len != class_data_len) {
                        LOG("WARN: The parsed classfile length is not the same as the original (expected: %d, found: %d)\n", class_data_len, len);
                        check = false;
                }
                len = std::min({ len, class_data_len });
                for (jint i = 0; i < len; ++i) {
                        auto byte = bytes[i];
                        auto expected = class_data[i];
                        if (byte != expected) {
                                LOG("WARN: Class file byte '%d' differs from original (expected: %d, found: %d)\n", i, byte, expected);
                                check = false;
                        }
                }
                LOG("Class file parse check: %s\n", check ? "OK" : "BAD");
                // cf->dump("/tmp/ORIG.class");
#endif
                g_class_file_cache[class_name] = std::move(cf);
        }

        return;
}

// Patches up a class with the current hooks (if any)
// and redefines it using JVMTI
jnihook_result_t
ReapplyClass(jclass clazz, std::string clazz_name)
{
        jvmtiClassDefinition class_definition;
        jvmtiError err;
        auto cf = g_class_file_cache[clazz_name]->clone();

        // Patch class file
        // NOTE: The `methods` attribute only has the methods defined by the main class of this ClassFile
        //       Method references are not included here
        //       If the source file has more than one class, they are compiled as separate ClassFiles
        for (auto &method : cf->methods) {
                auto name = method.getName();
                auto descriptor = method.getDesc();

                // Check if the current method is a method that should be hooked
                // TODO: Use hashmap for faster lookup
                bool should_hook = false;
                size_t hook_offset = 0;
                for (auto &hk_info : g_hooks[clazz_name]) {
                        auto &minfo = hk_info.method_info;
                        if (minfo.name == name && minfo.signature == descriptor) {
                                should_hook = true;
                                hook_offset = hk_info.hook_offset;
                                break;
                        }
                }
                if (!should_hook)
                        continue;

                hkType hookType = hook_default;
                if (strcmp(name, "<init>") == 0) {
                    hookType = hook_init;
                }
                else if (strcmp(name, "<clinit>") == 0) {
                    hookType = hook_clinit;
                }
                else if (hook_offset != 0) {
                    hookType = hook_midfunction;
                }
                // New method
                std::string newName = get_copy_method_name(name, cf->getThisClassName());
                
                u2 copyflags = Method::PRIVATE | Method::FINAL;
                if (method.accessFlags & Method::STATIC) {
                    copyflags |= Method::STATIC;
                }
                // constructor hook
                if (hookType == hook_init or 
                    hookType == hook_clinit) {

                    std::string copyName = get_copy_clone_name(name, cf->getThisClassName());
                    auto& copyMethod = cf->addMethod(copyName.c_str(), descriptor, copyflags);
                    auto& nativeMethod = cf->addMethod(newName.c_str(), descriptor, copyflags);


                    for (size_t i = 0; i < method.attrs.size(); ++i) {
                        auto& attr = method.attrs[i];
                        if (attr.kind == ATTR_CODE) {
                            u2 code_nameindex = 0;
                            // get index of "Code" from ConstPool
                            for (ConstPool::Iterator it = attr.constPool->iterator(); it.hasNext(); it++) {
                                ConstPool::Index i = *it;
                                ConstPool::Tag tag = attr.constPool->getTag(i);
                                if (tag == 1) {
                                    std::string bytes = attr.constPool->getUtf8(i);
                                    if (bytes == "Code") {
                                        code_nameindex = i;
                                    }
                                }
                            }

                            CodeAttr* ca = cf->_arena.create<CodeAttr>(code_nameindex, cf.get());
                            CodeAttr* orig_ca = ((CodeAttr*)&attr);
                            InstList& instList = ca->instList;

                            ca->maxStack = orig_ca->maxStack;
                            ca->maxLocals = orig_ca->maxLocals;

                            auto& orig_instList = orig_ca->instList;

                            //copy constructor opcodes to copy method
                            for (auto it = orig_instList.begin(); it != orig_instList.end(); it.operator++()) {
                                if (hookType == hook_init and it->_offset <= 1) {
                                    /* 
                                    skip
                                     0: ( 42) aload_0 CS: PS:
                                     1: (183) invokespecial CS: PS: #1 java/lang/Object.<init>: ()V
                                    */
                                    continue;
                                }
                                instList.copy(*it);
                            }

                            ca->codeLen = instList.size();
                            ca->cfg = ((CodeAttr*)&attr)->cfg;
                            copyMethod.attrs.add(ca);

                            auto nativeMethodid = cf->addMethodRef(cf->thisClassIndex, newName.c_str(), descriptor);
                            auto iterator = orig_instList.begin();
                            if (hookType == hook_init) {
                                //patch original <init>
                                /*
                                skip
                                -1: empty
                                 0: ( 42) aload_0 CS: PS:
                                 1: (183) invokespecial CS: PS: #1 java/lang/Object.<init>: ()V
                                */
                                iterator.operator++();//-1
                                iterator.operator++();//0
                                iterator.operator++();//1
                                orig_instList.addZero(Opcode::aload_0, *iterator);                          // this = this
                                orig_instList.addInvoke(Opcode::invokespecial, nativeMethodid, *iterator); // this.nativeMethod()
                                orig_instList.addZero(Opcode::RETURN, *iterator);                         // return

                                // memory leak?
                                iterator->next = nullptr; // remove everything after

                                orig_ca->codeLen = orig_instList.size();
                            }
                            else if (hookType == hook_clinit) {
                                //patch original <clinit>
                                // i am sure this is memory leak
                                iterator->next = nullptr;
                                orig_instList.addInvoke(Opcode::invokestatic, nativeMethodid, *iterator); // nativeMethod()
                                orig_instList.addZero(Opcode::RETURN, *iterator);                        // return
                            }
                            // remove line number table
                            for (size_t i = 0; i < orig_ca->attrs.size(); i++) {
                                if (orig_ca->attrs.attrs[i]->kind == ATTR_LNT) {
                                    orig_ca->attrs.remove(i);
                                }
                            }
                        }
                        else {
                            copyMethod.attrs.add((Attr*)&attr);
                            nativeMethod.attrs.add((Attr*)&attr);
                        }
                    }
                    *(u2*)&nativeMethod.accessFlags |= Method::NATIVE;
                }
                else if (hookType == hook_midfunction) {
                    /*
                        if midfunction hook
                        make native method to store hook
                        invoke native method at specified offset
                        how offset works:
                        1-beginning of function
                        n-each instruction is +1
                    */
                    auto& nativeMethod = cf->addMethod(newName.c_str(), descriptor, copyflags);
                    
                    for (size_t i = 0; i < method.attrs.size(); ++i) {
                        auto& attr = method.attrs[i];
                        nativeMethod.attrs.add((Attr*)&attr); // native method should inherit all the attributes
                        if (attr.kind == ATTR_CODE) {
                            nativeMethod.attrs.remove(i);
                            u2 code_nameindex = 0;
                            // get index of "Code" from ConstPool
                            for (ConstPool::Iterator it = attr.constPool->iterator(); it.hasNext(); it++) {
                                ConstPool::Index i = *it;
                                ConstPool::Tag tag = attr.constPool->getTag(i);
                                if (tag == 1) {
                                    std::string bytes = attr.constPool->getUtf8(i);
                                    if (bytes == "Code") {
                                        code_nameindex = i;
                                    }
                                }
                            }

                            CodeAttr* orig_ca = ((CodeAttr*)&attr);
                            InstList& instList = orig_ca->instList;

                            auto nativeMethodid = cf->addMethodRef(cf->thisClassIndex, newName.c_str(), descriptor);
                            auto iterator = instList.begin();
                            
                            for (size_t i = 0; i < hook_offset; i++) {
                                if (iterator->next == nullptr) {
                                    break;
                                }
                                iterator.operator++();
                            }
                            if (copyflags & Method::STATIC) {
                                instList.addInvoke(Opcode::invokestatic, nativeMethodid, *iterator); // nativeMethod()
                            }else {
                                instList.addZero(Opcode::aload_0, *iterator);                          // this=this
                                instList.addInvoke(Opcode::invokespecial, nativeMethodid, *iterator); // this.nativeMethod()
                            }
                            // remove line number table
                            for (size_t i = 0; i < orig_ca->attrs.size(); i++) {
                                if (orig_ca->attrs.attrs[i]->kind == ATTR_LNT) {
                                    orig_ca->attrs.remove(i);
                                }
                            }
                        }
                    }

                    *(u2*)&nativeMethod.accessFlags |= Method::NATIVE;
                }
                else {

                    // method IS NOT <init>
                    auto& newMethod = cf->addMethod(newName.c_str(), descriptor, copyflags);

                    // Set method to native
                    *(u2*)&method.accessFlags |= Method::NATIVE;

                    // Remove "Code" attribute
                    for (size_t i = 0; i < method.attrs.size(); ++i) {
                        auto& attr = method.attrs[i];
                        newMethod.attrs.add((Attr*)&attr); // native method should inherit all the attributes
                                                          // from the original method
                        if (attr.kind == ATTR_CODE) {
                            method.attrs.remove(i);
                            break;
                        }
                    }
                }
        }

        // Redefine class with modified ClassFile
        auto cf_bytes = cf->toBytes();

        class_definition.klass = clazz;
        class_definition.class_byte_count = cf_bytes.size();
        class_definition.class_bytes = cf_bytes.data();
        err = g_jnihook->jvmti->RedefineClasses(1, &class_definition);
        std::stringstream ss;
        LOG("===== CLASS REAPPLIED =====\n");
        ss << *cf;
        LOG("%s\n", ss.str().c_str());
        LOG("===========================\n");
        if (err != JVMTI_ERROR_NONE) {
                LOG("ERR: JVMTI error in ReapplyClass: %d\n", err);
                // cf->dump("/tmp/DUMP.class");
                return JNIHOOK_ERR_JVMTI_OPERATION;
        }

        return JNIHOOK_OK;
}

// Stores a loaded class in the class cache
jnihook_result_t
CacheClass(JNIEnv *env, jclass clazz)
{
        std::string clazz_name = get_class_name(env, clazz);

        if (g_class_file_cache.find(clazz_name) == g_class_file_cache.end()) {
                if (g_jnihook->jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_CLASS_FILE_LOAD_HOOK, NULL) != JVMTI_ERROR_NONE) {
                        LOG("ERR: Failed to enable class file load hook\n");
                        return JNIHOOK_ERR_SETUP_CLASS_FILE_LOAD_HOOK;
                }

                // Enable forceful caching of classfiles
                // WARN: If something goes wrong, every class
                // that goes through the ClassFileLoadHook
                // would get cached! May waste a ton of memory.
                g_force_class_caching = true;
                auto result = g_jnihook->jvmti->RetransformClasses(1, &clazz);
                g_force_class_caching = false;

                // NOTE: We disable the ClassFileLoadHook here because it breaks
                //       any `env->DefineClass()` calls. Also, it's not necessary
                //       to keep it active at all times, we just have to use it for caching
                //       classes that havent been cached yet.
                // TODO: Investigate why it breaks it (possibly NullPointerException in
                //       JNIHook_ClassFileLoadHook)
                if (g_jnihook->jvmti->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_CLASS_FILE_LOAD_HOOK, NULL) != JVMTI_ERROR_NONE) {
                        LOG("ERR: Failed to disable class file load hook\n");
                        return JNIHOOK_ERR_SETUP_CLASS_FILE_LOAD_HOOK;
                }

                if (result != JVMTI_ERROR_NONE) {
                        LOG("ERR: Failed to cache classfile (JVMTI error)\n");
                        return JNIHOOK_ERR_CLASS_FILE_CACHE;
                }

                if (g_class_file_cache.find(clazz_name) == g_class_file_cache.end()) {
                        LOG("ERR: Failed to cache classfile\n");
                        return JNIHOOK_ERR_CLASS_FILE_CACHE;
                }
        }

        return JNIHOOK_OK;
}

// Copy a class and its inner classes
// (no longer used)
/*
jnihook_result_t
CopyClass(JNIEnv *env, jclass clazz, const std::string &new_class_name, std::string nest_host="", std::string old_nest_host="")
{
        jnihook_result_t result;
        auto clazz_name = get_class_name(env, clazz);
        std::map<jclass, std::string> additional_classes_to_copy = {};
        jobject class_loader;

        LOG("Copying class '%s' to: %s\n", clazz_name.c_str(), new_class_name.c_str());

        // Cache class being copied
        result = CacheClass(env, clazz);
        if (result != JNIHOOK_OK)
                return result;

        // Find inner classes
        auto &cf = g_class_file_cache[clazz_name];
        for (auto &attr : cf->attrs) {
                if (attr->kind == jnif::model::ATTR_NESTMEMBERS) {
                        auto nested_members_attr = (NestMembersAttr *)attr;

                        for (auto &class_index : nested_members_attr->classes) {
                                auto inner_name = std::string(nested_members_attr->constPool->getClassName(class_index));
                                auto inner_clazz = env->FindClass(inner_name.c_str());

                                result = CacheClass(env, inner_clazz);
                                if (result != JNIHOOK_OK)
                                        return result;

                                // Add inner class for copy
                                // WARN: Assumes that the inner class name
                                //       starts with the original class name
                                inner_name.replace(0, clazz_name.length(), new_class_name);
                                additional_classes_to_copy.insert({ inner_clazz, inner_name });
                        }

                        // NestMembers can only happen once per class
                        break;
                }
        }

        // Make copy of the class
        LOG("Generating copy class...\n");
        if (g_original_classes.find(clazz_name) == g_original_classes.end()) {
                jclass class_copy;
                auto new_cf = cf->clone();

                // Rename the copy class
                new_cf->renameClass(new_class_name.c_str());

                // Make all methods final (may help with CallNonvirtual)
                // for (auto &method : new_cf->methods) {
                //         if (method.isInit())
                //                 continue;

                //         const_cast<u2 &>(method.accessFlags) |= Method::FINAL;
                // }

                // Replace NestHost class if needed
                if (nest_host.length() > 0) {
                        for (auto &attr : new_cf->attrs) {
                                if (attr->kind != jnif::model::ATTR_NESTHOST)
                                        continue;

                                auto nest_host_attr = (NestHostAttr *)attr;
                                auto host_index = nest_host_attr->hostClassIndex;
                                auto name_index = nest_host_attr->constPool->getClassNameIndex(host_index);
                                nest_host_attr->constPool->replaceUtf8(name_index, nest_host.c_str());
                        }
                }

                // Rename references of NestHost class
                if (nest_host.length() > 0 && old_nest_host.length() > 0) {
                        new_cf->renameClass(old_nest_host.c_str(), nest_host.c_str());
                }

#ifdef JNIHOOK_DEBUG
                LOG("===== COPY CLASS DUMP =====\n");
                std::stringstream ss;
                ss << *new_cf;
                LOG("%s\n", ss.str().c_str());
                LOG("======================\n");
#endif

                std::vector<u1> class_data;
                try {
                        class_data = new_cf->toBytes();
                } catch (const Exception &ex) {
                        LOG("ERR: Failed to convert classfile to bytes: %s\n", ex.message.c_str());
                        return JNIHOOK_ERR_CLASS_FILE_FORMAT;
                } catch (...) {
                        LOG("ERR: Failed to convert classfile to bytes\n");
                        return JNIHOOK_ERR_CLASS_FILE_FORMAT;
                }

                if (g_jnihook->jvmti->GetClassLoader(clazz, &class_loader) != JVMTI_ERROR_NONE) {
                        LOG("ERR: Failed to get class loader\n");
                        return JNIHOOK_ERR_JVMTI_OPERATION;
                }

                class_copy = env->DefineClass(NULL, class_loader,
                                              reinterpret_cast<const jbyte *>(class_data.data()),
                                              class_data.size());

                if (!class_copy) {
                        LOG("ERR: Failed to define class\n");
                        return JNIHOOK_ERR_JNI_OPERATION;
                }

                g_original_classes[clazz_name] = class_copy;
        }

        for (auto &[inner_clazz, inner_new_name] : additional_classes_to_copy) {
                result = CopyClass(env, inner_clazz, inner_new_name, new_class_name, clazz_name);
                if (result != JNIHOOK_OK) {
                        LOG("ERR: Failed to copy inner class '%s' of class: %s\n", inner_new_name.c_str(), clazz_name.c_str());
                        return result;
                }
        }

        LOG("Class '%s' copied to '%s' successfully\n", clazz_name.c_str(), new_class_name.c_str());

        return JNIHOOK_OK;
}
*/

JNIHOOK_API jnihook_result_t JNIHOOK_CALL
JNIHook_Init(JavaVM *jvm)
{
        jvmtiEnv *jvmti;
        jvmtiCapabilities capabilities = {};
        jvmtiEventCallbacks callbacks = {};

        if (jvm->GetEnv(reinterpret_cast<void **>(&jvmti), JVMTI_VERSION_1_2) != JNI_OK) {
                LOG("ERR: Failed to get JVMTI");
                return JNIHOOK_ERR_GET_JVMTI;
        }

        capabilities.can_redefine_classes = 1;
        capabilities.can_redefine_any_class = 1;
        capabilities.can_retransform_classes = 1;
        capabilities.can_retransform_any_class = 1;
        capabilities.can_suspend = 1;

        if (jvmti->AddCapabilities(&capabilities) != JVMTI_ERROR_NONE) {
                LOG("ERR: Failed to add capabilities");
                return JNIHOOK_ERR_ADD_JVMTI_CAPS;
        }

        callbacks.ClassFileLoadHook = JNIHook_ClassFileLoadHook;
        if (jvmti->SetEventCallbacks(&callbacks, sizeof(callbacks)) != JVMTI_ERROR_NONE) {
                LOG("ERR: Failed to setup class file load hook");
                return JNIHOOK_ERR_SETUP_CLASS_FILE_LOAD_HOOK;
        }

        g_jnihook = std::make_unique<jnihook_t>(jnihook_t { jvm, jvmti });

        // Generate VM type hashmaps
        LOG("Address of gHotspotVMStructs: %p\n", gHotSpotVMStructs);
        LOG("Address of gHotspotVMTypes: %p\n", gHotSpotVMTypes);
        VMTypes::init(gHotSpotVMStructs, gHotSpotVMTypes);

        // Force AllowRedefinitionToAddDeleteMethods
        auto jvm_flag_type_result = VMType::from_static("JVMFlag");
        if (!jvm_flag_type_result && !(jvm_flag_type_result = VMType::from_static("Flag"))) {
                LOG("Failed to parse VMStructs\n");
                return JNIHOOK_ERR_UNKNOWN;
        }

        LOG("VMStructs successfully parsed\n");
        auto jvm_flag_type = jvm_flag_type_result.value();
        auto jvm_flag_size = jvm_flag_type.size();
        LOG("JVM Flag Type Size: %lu\n", jvm_flag_size);
        auto flagsFieldResult = jvm_flag_type.get_field<void *>("flags");
        auto flagsField = flagsFieldResult.value();
        LOG("Flags field: %p\n", flagsField);
        auto numFlagsFieldResult = jvm_flag_type.get_field<size_t>("numFlags");
        auto numFlagsField = numFlagsFieldResult.value();
        LOG("NumFlags field: %p\n", numFlagsField);
        LOG("NumFlags: %llu\n", static_cast<unsigned long long>(*numFlagsField));

        auto flags_buf = *(unsigned char **)flagsField; // flagTable
        auto numFlags = *numFlagsField;
        for (size_t i = 0; i < numFlags; ++i) {
                auto flag = VMType::from_instance(jvm_flag_type.get_type_name().c_str(), &flags_buf[i * jvm_flag_size]);
                auto name_addr = flag->get_field<void *>("_name").value();
                auto name = (char *)*name_addr;
                LOG("FLAG: %s\n", name);

                if (!name || strcmp(name, "AllowRedefinitionToAddDeleteMethods"))
                        continue;

                auto addr = *flag->get_field<bool *>("_addr").value();
                LOG("ADDR: %p\n", addr);

                auto value = reinterpret_cast<bool *>(addr);
                LOG("VALUE: %d\n", (int)*value);

                *value = true;
                LOG("NEW VALUE: %d\n", (int)*value);

                break;
        }

        return JNIHOOK_OK;
}

JNIHOOK_API jnihook_result_t JNIHOOK_CALL
_JNIHook_Attach(jmethodID method, void *native_hook_method, jmethodID *original_method,size_t offset)
{
        jclass clazz;
        std::string clazz_name;
        hook_info_t hook_info;
        JNIEnv *env;
        jnihook_result_t result;

        hkType hookType = hook_default;

        if (g_jnihook->jvm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_8)) {
                LOG("ERR: Failed to get JNI\n");
                return JNIHOOK_ERR_GET_JNI;
        }

        if (g_jnihook->jvmti->GetMethodDeclaringClass(method, &clazz) != JVMTI_ERROR_NONE) {
                LOG("ERR: Failed to get declaring class of method\n");
                return JNIHOOK_ERR_JVMTI_OPERATION;
        }

        clazz_name = get_class_name(env, clazz);
        if (clazz_name.length() == 0) {
                LOG("ERR: Failed to get class name\n");
                return JNIHOOK_ERR_JNI_OPERATION;
        }

        auto method_info = get_method_info(g_jnihook->jvmti, method);
        std::string native_name = method_info->name;
        if (method_info->name == "<init>") {
            hookType = hook_init;
            native_name = get_copy_method_name(method_info->name, clazz_name);
        }else if (method_info->name == "<clinit>") {
            hookType = hook_clinit;
            native_name = get_copy_method_name(method_info->name, clazz_name);
        }
        else if (offset != 0) {
            hookType = hook_midfunction;
            native_name = get_copy_method_name(method_info->name, clazz_name);
        }

        if (!method_info) {
                LOG("ERR: Failed to get method info\n");
                return JNIHOOK_ERR_JVMTI_OPERATION;
        }

        hook_info.method_info = *method_info;
        hook_info.native_hook_method = native_hook_method;
        hook_info.hook_offset = offset;

        // Force caching of the class being hooked
        result = CacheClass(env, clazz);
        if (result != JNIHOOK_OK)
                return result;

        // Suspend other threads while the hook is being set up
        jthread curthread;
        jthread *threads;
        jint thread_count;

        env->PushLocalFrame(16);

        if (g_jnihook->jvmti->GetCurrentThread(&curthread) != JVMTI_ERROR_NONE) {
                LOG("ERR: Failed to get current thread\n");
                return JNIHOOK_ERR_JVMTI_OPERATION;
        }

        if (g_jnihook->jvmti->GetAllThreads(&thread_count, &threads) != JVMTI_ERROR_NONE) {
                LOG("ERR: Failed to get all threads\n");
                return JNIHOOK_ERR_JVMTI_OPERATION;
        }

        // TODO: Only suspend/resume threads that are actually active
        for (jint i = 0; i < thread_count; ++i) {
                if (env->IsSameObject(threads[i], curthread))
                        continue;

                g_jnihook->jvmti->SuspendThread(threads[i]);
        }

        // Apply current hooks
        jnihook_result_t ret = JNIHOOK_ERR_JNI_OPERATION;
        g_hooks[clazz_name].push_back(hook_info);
        if (ret = ReapplyClass(clazz, clazz_name); ret != JNIHOOK_OK) {
                LOG("ERR: Failed to reapply class\n");
                g_hooks[clazz_name].pop_back();
                goto RESUME_THREADS;
        }
        
        // Register native method for JVM lookup
        JNINativeMethod native_method;
        native_method.name = const_cast<char *>(native_name.c_str());
        native_method.signature = const_cast<char *>(method_info->signature.c_str());
        native_method.fnPtr = native_hook_method;

        if (env->RegisterNatives(clazz, &native_method, 1) < 0) {
            LOG("ERR: Failed to register natives\n");
            g_hooks[clazz_name].pop_back();
            ReapplyClass(clazz, clazz_name); // Attempt to restore class to previous state
            goto RESUME_THREADS;
        }

RESUME_THREADS:
        // Resume other threads, hook already placed succesfully
        for (jint i = 0; i < thread_count; ++i) {
                if (env->IsSameObject(threads[i], curthread))
                        continue;

                g_jnihook->jvmti->ResumeThread(threads[i]);
        }

        g_jnihook->jvmti->Deallocate(reinterpret_cast<unsigned char *>(threads));
        env->PopLocalFrame(NULL);

        if (ret != JNIHOOK_OK)
                return ret;

        // Get original method
        ret = JNIHOOK_OK;
        if (original_method) {
                jclass orig_class = clazz;
                jmethodID orig;
                
                std::string original_name; 
                switch (hookType) {
                case hook_init:
                case hook_clinit:
                    original_name = get_copy_clone_name(method_info->name, clazz_name);
                    break;
                case hook_midfunction:
                    original_name = method_info->name;
                    break;
                case hook_default:
                    original_name = get_copy_method_name(method_info->name, clazz_name);
                    break;
                }

                if ((method_info->access_flags & Method::STATIC) == Method::STATIC) {
                        orig = env->GetStaticMethodID(orig_class, original_name.c_str(),
                                                      method_info->signature.c_str());
                } else {
                        orig = env->GetMethodID(orig_class, original_name.c_str(),
                                                method_info->signature.c_str());
                }

                if (!orig || env->ExceptionOccurred()) {
                        LOG("ERR: Exception while getting original method '%s -> %s'\n", original_name.c_str(), method_info->signature.c_str());
                        env->ExceptionDescribe();
                        env->ExceptionClear();
                        ret = JNIHOOK_ERR_JAVA_EXCEPTION;
                }

                *original_method = orig;
        }

        if (ret != JNIHOOK_OK) {
                JNIHook_Detach(method);
                return ret;
        }

        return JNIHOOK_OK;
}

JNIHOOK_API jnihook_result_t JNIHOOK_CALL
JNIHook_Attach(jmethodID method, void *native_hook_method, jmethodID *original_method)
{
        try {
                return _JNIHook_Attach(method, native_hook_method, original_method,0);
        } catch (jnif::Exception ex) {
                LOG("ERR: JNIF exception thrown -> %s\n", ex.message.c_str());
                return JNIHOOK_ERR_CLASS_FILE_FORMAT;
        } catch (...) {
                LOG("ERR: Unhandled exception thrown\n");
        }
        return JNIHOOK_ERR_UNKNOWN;
}
JNIHOOK_API jnihook_result_t JNIHOOK_CALL
JNIHook_Attach_MID(jmethodID method, void* native_hook_method, jmethodID* original_method,size_t offset)
{
    try {
        return _JNIHook_Attach(method, native_hook_method, original_method,offset);
    }
    catch (jnif::Exception ex) {
        LOG("ERR: JNIF exception thrown -> %s\n", ex.message.c_str());
        return JNIHOOK_ERR_CLASS_FILE_FORMAT;
    }
    catch (...) {
        LOG("ERR: Unhandled exception thrown\n");
    }
    return JNIHOOK_ERR_UNKNOWN;
}

JNIHOOK_API jnihook_result_t JNIHOOK_CALL
JNIHook_Detach(jmethodID method)
{
        JNIEnv *env;
        jclass clazz;
        std::string clazz_name;
        hook_info_t hook_info;
        jvmtiClassDefinition class_definition;

        if (g_jnihook->jvm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_8)) {
                return JNIHOOK_ERR_GET_JNI;
        }

        if (g_jnihook->jvmti->GetMethodDeclaringClass(method, &clazz) != JVMTI_ERROR_NONE) {
                return JNIHOOK_ERR_JVMTI_OPERATION;
        }

        clazz_name = get_class_name(env, clazz);
        if (clazz_name.length() == 0) {
                return JNIHOOK_ERR_JNI_OPERATION;
        }

        if (g_hooks.find(clazz_name) == g_hooks.end() || g_hooks[clazz_name].size() == 0) {
                return JNIHOOK_OK;
        }

        auto method_info = get_method_info(g_jnihook->jvmti, method);
        if (!method_info) {
                return JNIHOOK_ERR_JVMTI_OPERATION;
        }

        for (size_t i = 0; i < g_hooks[clazz_name].size(); ++i) {
                auto &hook_info = g_hooks[clazz_name][i];
                if (hook_info.method_info.name != method_info->name ||
                    hook_info.method_info.signature != method_info->signature)
                        continue;

                g_hooks[clazz_name].erase(g_hooks[clazz_name].begin() + i);
        }

        return ReapplyClass(clazz, clazz_name);
}


JNIHOOK_API jnihook_result_t JNIHOOK_CALL
JNIHook_Shutdown()
{
        JNIEnv *env;
        jvmtiEventCallbacks callbacks = {};

        if (g_jnihook->jvm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_8)) {
                return JNIHOOK_ERR_GET_JNI;
        }

        for (auto &[key, _value] : g_class_file_cache) {
                jclass clazz = env->FindClass(key.c_str());

                g_hooks[key].clear();

                if (!clazz)
                        continue;

                // Reapplying the class with empty hooks will just restore the original one.
                ReapplyClass(clazz, key);
        }

        g_class_file_cache.clear();

        // TODO: Fully cleanup defined classes in `g_original_classes` by deleting them from the JVM memory
        //       (if possible without doing crazy hacks)
        // NOTE: The above is no longer needed due to changing the hooking method.
        // g_original_classes.clear();

        g_jnihook->jvmti->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_CLASS_FILE_LOAD_HOOK, NULL);
        g_jnihook->jvmti->SetEventCallbacks(&callbacks, sizeof(callbacks));

        jvmtiCapabilities caps{};
		caps.can_redefine_classes = 1;
		caps.can_redefine_any_class = 1;
		caps.can_retransform_classes = 1;
        caps.can_retransform_any_class = 1;
		caps.can_suspend = 1;
		jvmtiError err = g_jnihook->jvmti->RelinquishCapabilities(&caps);

        g_jnihook = nullptr;

        return JNIHOOK_OK;
}
