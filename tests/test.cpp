#include <jnihook.h>
#include <jnihook.hpp>
#include <iostream>

jclass Target_class;
jmethodID Target_sayHello_mid;
jmethodID orig_Target_sayHello = NULL;
jmethodID Target_sayAnotherThing_mid;
jmethodID orig_Target_sayAnotherThing = NULL;

JNIEXPORT void JNICALL hk_Target_sayHello(JNIEnv *jni, jobject obj)
{
        std::cout << "Target::sayHello HOOK CALLED!" << std::endl;
        std::cout << "Calling original method..." << std::endl << std::endl;
        jni->CallNonvirtualVoidMethod(obj, Target_class, orig_Target_sayHello);

        std::cout << std::endl << "I called the original method Target::sayHello, now im gonna detach the hook" << std::endl;
        JNIHook_Detach(Target_sayHello_mid);
        std::cout << "Hook Target::sayHello detached. Next time the method is called, it should do its default behavior." << std::endl << std::endl;
}

JNIEXPORT void JNICALL hk_Target_sayAnotherThing(JNIEnv *jni, jclass clazz, jint number)
{
        std::cout << "Target::sayAnotherThing HOOK CALLED!" << std::endl;
        std::cout << "Original number: " << number << std::endl;
        number = 1337;
        std::cout << "New number: " << number << std::endl;

        std::cout << "Calling original method..." << std::endl << std::endl;
        jni->CallStaticVoidMethod(clazz, orig_Target_sayAnotherThing, number);

        std::cout << std::endl << "I called the original method Target::sayAnotherThing, now im gonna detach the hook" << std::endl;
        JNIHook_Detach(Target_sayAnotherThing_mid);
        std::cout << "Hook Target::sayAnotherThing detached. Next time the method is called, it should do its default behavior." << std::endl << std::endl;
}

void
start()
{
        JavaVM *jvm;
        JNIEnv *env;
        jsize jvm_count;

        // Setup JVM
        std::cout << "[*] Library loaded!" << std::endl;

        if (JNI_GetCreatedJavaVMs(&jvm, 1, &jvm_count) != JNI_OK) {
                std::cerr << "[!] Failed to get created Java VMs!" << std::endl;
                return;
        }

        std::cout << "[*] JavaVM: " << jvm << std::endl;

        if (jvm->AttachCurrentThread(reinterpret_cast<void **>(&env), NULL) != JNI_OK) {
                std::cerr << "[!] Failed to attach current thread to JVM!" << std::endl;
                return;
        }

        // Get classes and methods
        Target_class = env->FindClass("dummy/Target");
        std::cout << "[*] Class dummy.Target: " << Target_class << std::endl;

        Target_sayHello_mid = env->GetMethodID(Target_class, "sayHello", "()V");
        std::cout << "[*] Target::sayHello: " << Target_sayHello_mid << std::endl;

        Target_sayAnotherThing_mid = env->GetStaticMethodID(Target_class, "sayAnotherThing", "(I)V");
        std::cout << "[*] Target::sayAnotherThing: " << Target_sayAnotherThing_mid << std::endl;

        // Place hooks
        JNIHook_Init(jvm); // Test to make sure init and shutdown are clean
        JNIHook_Shutdown();
        if (auto result = JNIHook_Init(jvm); result != JNIHOOK_OK) {
                std::cerr << "[!] Failed to initialize JNIHook: " << result << std::endl;
                goto DETACH;
        }
        std::cout << "[*] JNIHook initialized successfully";

        if (auto result = JNIHook_Attach(Target_sayHello_mid, reinterpret_cast<void *>(hk_Target_sayHello), &orig_Target_sayHello); result != JNIHOOK_OK) {
                std::cerr << "[!] Failed to attach hook: " << result << std::endl;
                goto DETACH;
        }
        std::cout << "[*] Target::sayHello hooked successfully!" << std::endl;

        if (auto result = JNIHook_Attach(Target_sayAnotherThing_mid, reinterpret_cast<void *>(hk_Target_sayAnotherThing), &orig_Target_sayAnotherThing); result != JNIHOOK_OK) {
                std::cerr << "[!] Failed to attach hook: " << result << std::endl;
                goto DETACH;
        }
        std::cout << "[*] Target::sayAnotherThing hooked successfully!" << std::endl;

        std::cout << "[*] Hooks attached" << std::endl;

DETACH:
        // JNIHook_Shutdown();
        // std::cout << "[*] JNIHook has been shut down" << std::endl;
        jvm->DetachCurrentThread(); // NOTE: The JNIEnv must live until JNIHook_Shutdown() is called!
                                    //       (or if you won't call JNIHook again).
}

#ifdef _WIN32
#include <windows.h>
DWORD WINAPI WinThread(LPVOID lpParameter)
{
        start();
        return 0;
}

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
        switch (dwReason) {
        case DLL_PROCESS_ATTACH:
                CreateThread(nullptr, 0, WinThread, nullptr, 0, nullptr);
                break;
        }
        
        return TRUE;
}
#else
void *main_thread(void *arg)
{
        start();
        return NULL;
}

void __attribute__((constructor))
dl_entry()
{
        pthread_t th;
        pthread_create(&th, NULL, main_thread, NULL);
}
#endif
