#include <jnihook.h>
#include <iostream>

void JNICALL ClassFileLoadHook(jvmtiEnv *jvmti_env,
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
	std::cout << "[*] ClassFileLoadHook: " << name << std::endl;
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
		jvm->DetachCurrentThread();
		return;
	}

	std::cout << "[*] Helper: jnihook_t { jvm: " <<
		jnihook.jvm << ", env: " << jnihook.env <<
		", jvmti: " << jnihook.jvmti << " }" <<
	std::endl;

	callbacks.ClassFileLoadHook = ClassFileLoadHook;
	std::cout << "[*] SetEventCallbacks result: " << jnihook.jvmti->SetEventCallbacks(&callbacks, sizeof(callbacks)) << std::endl;
	std::cout << "[*] SetEventNotificationMode result: " << jnihook.jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_CLASS_FILE_LOAD_HOOK, NULL) << std::endl;
	clazz = env->FindClass("dummy/Dummy");
	std::cout << "[*] Found dummy.Dummy class: " << clazz << std::endl;
	int retransform_result = jnihook.jvmti->RetransformClasses(1, &clazz);
	std::cout << "[*] RetransformClasses result: " << retransform_result << std::endl;
	std::cout << "[*] Class retransformed" << std::endl;

	JNIHook_Shutdown(&jnihook);

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
