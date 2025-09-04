#include <jnihook.h>
#include <iostream>

static JavaVM *jvm;
static jclass perfTestClass;
static jmethodID doSomethingMethod;

jvalue hkDoSomething(JNIEnv *jni, jmethodID callableMethod, jvalue *args, size_t nargs, void *arg)
{
	std::cout << " <HOOK CALLED> " << std::flush;

	jni->CallStaticVoidMethod(perfTestClass, callableMethod, args[0].i);

	return jvalue {};
}

jvalue hkPrintName(JNIEnv *jni, jmethodID callableMethod, jvalue *args, size_t nargs, void *arg)
{
	std::cout << "[*] hkPrintName called!" << std::endl;

	std::cout << "[*] Number of Args: " << nargs << std::endl;

	std::cout << "[*] Args: " << std::endl;
	std::cout << " - thisptr: " << (void *)(args[0].l) << std::endl;
	std::cout << " - number: " << args[1].i << std::endl;

	std::cout << "[*] JNI: " << jni << std::endl;

	std::cout << "[*] Calling original..." << std::endl;

	jni->CallVoidMethod((jobject)&args[0].l, callableMethod, 2);

	std::cout << "[*] Original called" << std::endl;
	
	return jvalue { 0 };
}

static void start(JavaVM *jvm, JNIEnv *jni)
{
	perfTestClass = jni->FindClass("dummy/PerfTest");
	if (!perfTestClass) {
		std::cout << "[!] Failed to find class" << std::endl;
		return;
	}
	std::cout << "[*] PerfTest class: " << perfTestClass << std::endl;

	doSomethingMethod = jni->GetStaticMethodID(perfTestClass, "doSomething", "(I)V");
	if (!doSomethingMethod) {
		std::cout << "[!] Failed to find 'doSomething'" << std::endl;
		return;
	}
	std::cout << "[*] doSomething Method ID: " << doSomethingMethod << std::endl;

	JNIHook_Init(jvm);
	JNIHook_Attach(doSomethingMethod, hkDoSomething, (void *)0xdeadbeef);

	std::cout << "[*] Hooked doSomething" << std::endl;
}

void *main_thread(void *arg)
{
	JNIEnv *jni;

	std::cout << "[*] Library loaded!" << std::endl;

	if (JNI_GetCreatedJavaVMs(&jvm, 1, NULL) != JNI_OK) {
		std::cout << "[!] Failed to retrieve JavaVM pointer" << std::endl;
		return arg;
	}

	std::cout << "[*] JavaVM: " << jvm << std::endl;

	if (jvm->AttachCurrentThread((void **)&jni, NULL) != JNI_OK) {
		std::cout << "[!] Failed to retrieve JNI environment" << std::endl;
		return arg;
	}

	std::cout << "[*] JNIEnv: " << jni << std::endl;

	start(jvm, jni);

	return arg;
}

#ifdef _WIN32
#include <windows.h>
DWORD WINAPI WinThread(LPVOID lpParameter)
{
	main_thread(NULL);
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
void __attribute__((constructor))
dl_entry()
{
	pthread_t th;
	pthread_create(&th, NULL, main_thread, NULL);
}
#endif
