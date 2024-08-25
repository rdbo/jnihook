#include <jnihook.h>
#include <iostream>
#include <thread>
#include <chrono>

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

void JNICALL hk_Dummy_sayHello(JNIEnv *env, jclass clazz)
{
	std::cout << "Dummy.sayHello hook called!" << std::endl;
	std::cout << "JNIEnv: " << env << std::endl;
	std::cout << "Class: " << clazz << std::endl;

	std::cout << "Class name: " << get_class_name(env, clazz) << std::endl;

	jclass orig_Dummy = JNIHook_GetOriginalClass("dummy/Dummy");
	jmethodID orig_sayHello = env->GetStaticMethodID(orig_Dummy, "sayHello", "()V");
	std::cout << "Original class name (copy): " << get_class_name(env, orig_Dummy) << std::endl;
	std::cout << "Original 'sayHello': " << orig_sayHello << std::endl;

	std::cout << "Calling original sayHello..." << std::endl;
	env->CallStaticVoidMethod(orig_Dummy, orig_sayHello);
	std::cout << "Called original sayHello!" << std::endl;
}

void JNICALL hk_Dummy_sayHi(JNIEnv *env, jclass clazz)
{
	std::cout << "Dummy.sayHi hook called!" << std::endl;
	std::cout << "JNIEnv: " << env << std::endl;
	std::cout << "Class: " << clazz << std::endl;
}

void
start()
{
	JavaVM *jvm;
	JNIEnv *env;
	jsize jvm_count;
	jnihook_t jnihook;
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

	if (auto result = JNIHook_Attach(&jnihook, sayHello_mid, reinterpret_cast<void *>(hk_Dummy_sayHello), nullptr); result != JNIHOOK_OK) {
		std::cerr << "[!] Failed to attach hook: " << result << std::endl;
		goto DETACH;
	}

	if (auto result = JNIHook_Attach(&jnihook, sayHi_mid, reinterpret_cast<void *>(hk_Dummy_sayHi), nullptr); result != JNIHOOK_OK) {
		std::cerr << "[!] Failed to attach hook: " << result << std::endl;
		goto DETACH;
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(2000));

	if (auto result = JNIHook_Detach(&jnihook, sayHi_mid); result != JNIHOOK_OK) {
		std::cout << "[*] Failed to detach hook: " << result << std::endl;
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(2000));

	JNIHook_Shutdown(&jnihook);

	std::cout << "[*] JNIHook has been shut down" << std::endl;

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
