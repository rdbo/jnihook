#include <jnihook.h>
#include <iostream>

static JavaVM *jvm;
static int callCounter = 0;
static jclass dummyClass;
static jclass MyClass;
static jmethodID myFunctionID;

jvalue hkMyFunction(JNIEnv *jni, jmethodID cachedMethod, jvalue *args, size_t nargs, void *arg)
{
	std::cout << "hkMyFunction called!" << std::endl;

	std::cout << "[*] jni: " << jni << std::endl;
	std::cout << "[*] cachedMethod: " << cachedMethod << std::endl;
	std::cout << "[*] args: " << args << std::endl;
	std::cout << "[*] nargs: " << nargs << std::endl;
	std::cout << "[*] arg: " << arg << std::endl;

	if (++callCounter >= 3) {
		JNIHook_Detach(myFunctionID);
		std::cout << "[*] Unhooked method" << std::endl;
	}

	std::cout << "[*] call counter: " << callCounter << std::endl;

	jvalue mynumber = args[0];
	jvalue name = args[1];
	std::cout << "[*] mynumber value: " << mynumber.i << std::endl;
	std::cout << "[*] name object: " << name.l << std::endl;

	// Force call the original function
	mynumber.i = 1337;
	jstring newName = jni->NewStringUTF("root");
	// jstring newName = (jstring)&name;
	std::cout << "[*] newName: " << newName << std::endl;
	
	jint result = jni->CallStaticIntMethod(dummyClass, cachedMethod, mynumber, newName);
	std::cout << "[*] Forced call result: " << result << std::endl;

	return jvalue { .i = 6969 };
}

jvalue hkPrintName(JNIEnv *jni, jmethodID cachedMethod, jvalue *args, size_t nargs, void *arg)
{
	std::cout << "[*] hkPrintName called!" << std::endl;

	std::cout << "[*] Number of Args: " << nargs << std::endl;

	std::cout << "[*] Args: " << std::endl;
	std::cout << " - thisptr: " << (void *)(args[0].l) << std::endl;
	std::cout << " - number: " << args[1].i << std::endl;

	std::cout << "[*] JNI: " << jni << std::endl;

	std::cout << "[*] Calling original..." << std::endl;

	jni->CallVoidMethod((jobject)&args[0].l, cachedMethod, 2);

	std::cout << "[*] Original called" << std::endl;
	
	return jvalue { 0 };
}

static void start(JavaVM *jvm, JNIEnv *jni)
{
	dummyClass = jni->FindClass("dummy/Dummy");
	if (!dummyClass) {
		std::cout << "[!] Failed to find dummy class" << std::endl;
		return;
	}
	std::cout << "[*] Dummy class: " << dummyClass << std::endl;

	myFunctionID = jni->GetStaticMethodID(dummyClass, "myFunction", "(ILjava/lang/String;)I");
	if (!myFunctionID) {
		std::cout << "[!] Failed to find 'myFunction'" << std::endl;
		return;
	}
	std::cout << "[*] myFunction ID: " << myFunctionID << std::endl;

	MyClass = jni->FindClass("dummy/MyClass");
	if (!MyClass) {
		std::cout << "[!] Failed to find MyClass" << std::endl;
		return;
	}
	std::cout << "[*] MyClass: " << MyClass << std::endl;

	jmethodID printNameID = jni->GetMethodID(MyClass, "printName", "(I)V");
	if (!printNameID) {
		std::cout << "[!] Failed to find MyClass::printName" << std::endl;
		return;
	}
	std::cout << "[*] MyClass::printName method ID: " << printNameID << std::endl;

	JNIHook_Init(jvm);
	JNIHook_Attach(myFunctionID, hkMyFunction, (void *)0xdeadbeef);
	JNIHook_Attach(printNameID, hkPrintName, NULL);

	std::cout << "[*] Hooked myFunction" << std::endl;
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

void __attribute__((constructor))
dl_entry()
{
	pthread_t th;
	pthread_create(&th, NULL, main_thread, NULL);
}
