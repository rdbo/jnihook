#include <jnihook.h>
#include "../src/classfile.hpp"
#include <iostream>

void JNICALL hk_Dummy_sayHello(JNIEnv *env)
{
	std::cout << "Dummy.sayHello hook called!" << std::endl;
	std::cout << "JNIEnv: " << env << std::endl;
}

void JNICALL hk_Dummy_sayHi(JNIEnv *env)
{
	std::cout << "Dummy.sayHi hook called!" << std::endl;
	std::cout << "JNIEnv: " << env << std::endl;
}

void
start()
{
	JavaVM *jvm;
	JNIEnv *env;
	jsize jvm_count;
	jnihook_t jnihook;
	jvmtiEventCallbacks callbacks = {};
	jclass clazz;
	jmethodID sayHello_mid;
	jmethodID sayHi_mid;

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

	if (auto result = JNIHook_Init(env, &jnihook); result != JNIHOOK_OK) {
		std::cerr << "[!] Failed to initialize JNIHook: " << result << std::endl;
		goto DETACH;
	}

	std::cout << "[*] Helper: jnihook_t { jvm: " <<
		jnihook.jvm << ", env: " << jnihook.env <<
		", jvmti: " << jnihook.jvmti << " }" <<
	std::endl;

	clazz = env->FindClass("dummy/Dummy");
	std::cout << "[*] Class Dummy.dummy: " << clazz << std::endl;

	sayHello_mid = env->GetStaticMethodID(clazz, "sayHello", "()V");
	std::cout << "[*] Dummy.sayHello: " << sayHello_mid << std::endl;

	sayHi_mid = env->GetStaticMethodID(clazz, "sayHi", "()V");
	std::cout << "[*] Dummy.sayHi: " << sayHi_mid << std::endl;

	if (auto result = JNIHook_Attach(&jnihook, sayHello_mid, reinterpret_cast<void *>(hk_Dummy_sayHello)); result != JNIHOOK_OK) {
		std::cerr << "[!] Failed to attach hook: " << result << std::endl;
		goto DETACH;
	}

	if (auto result = JNIHook_Attach(&jnihook, sayHi_mid, reinterpret_cast<void *>(hk_Dummy_sayHi)); result != JNIHOOK_OK) {
		std::cerr << "[!] Failed to attach hook: " << result << std::endl;
		goto DETACH;
	}

	JNIHook_Shutdown(&jnihook);

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
