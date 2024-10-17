#include <jnihook.h>
#include <jnihook.hpp>
#include <iostream>
#include <thread>
#include <chrono>

jclass Target_class;
jmethodID orig_Target_sayHello;
void JNICALL hk_Target_sayHello(JNIEnv *jni, jobject obj)
{
	std::cout << "Target::sayHello HOOK CALLED!" << std::endl;
	std::cout << "Calling original method..." << std::endl;
	jni->CallNonvirtualVoidMethod(obj, Target_class, orig_Target_sayHello);
}

void
start()
{
	JavaVM *jvm;
	JNIEnv *env;
	jsize jvm_count;
	jmethodID Target_sayHello_mid;

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

	// Place hooks
	if (auto result = JNIHook_Init(jvm); result != JNIHOOK_OK) {
		std::cerr << "[!] Failed to initialize JNIHook: " << result << std::endl;
		goto DETACH;
	}

	if (auto result = JNIHook_Attach(Target_sayHello_mid, reinterpret_cast<void *>(hk_Target_sayHello), &orig_Target_sayHello); result != JNIHOOK_OK) {
		std::cerr << "[!] Failed to attach hook: " << result << std::endl;
		goto DETACH;
	}

	// JNIHook_Shutdown();
	// std::cout << "[*] JNIHook has been shut down" << std::endl;

DETACH:
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
