#include <jnihook.h>
#include <iostream>

static JavaVM *jvm;
static int callCounter = 0;
static jclass dummyClass;
static jmethodID myFunctionID;

jvalue hkMyFunction(jvalue *args, size_t nargs, void *thread, void *arg)
{
	std::cout << "hkMyFunction called!" << std::endl;

	std::cout << "[*] args: " << args << std::endl;
	std::cout << "[*] nargs: " << nargs << std::endl;
	std::cout << "[*] thread: " << thread << std::endl;
	std::cout << "[*] arg: " << arg << std::endl;

	if (++callCounter >= 3) {
		JNIHook_Detach(myFunctionID);
		std::cout << "[*] Unhooked method" << std::endl;
	}

	std::cout << "[*] call counter: " << callCounter << std::endl;

	JNIEnv *jni;
	jvm->GetEnv((void **)&jni, JNI_VERSION_1_6);
	std::cout << "[*] JNI: " << jni << std::endl;

	jvalue mynumber = args[0];
	jvalue name = args[1];
	std::cout << "[*] mynumber value: " << mynumber.i << std::endl;
	std::cout << "[*] name object: " << name.l << std::endl;

	// Force call the original function
	mynumber.i = 1337;
	jstring newName = jni->NewStringUTF("root");
	// jstring newName = (jstring)&name;
	std::cout << "[*] newName: " << newName << std::endl;
	
	jint result = jni->CallStaticIntMethod(dummyClass, myFunctionID, mynumber, newName);
	std::cout << "[*] Forced call result: " << result << std::endl;

	return jvalue { .i = 6969 };
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

	JNIHook_Init(jvm);
	JNIHook_Attach(myFunctionID, hkMyFunction, (void *)0xdeadbeef);

	std::cout << "[*] Hooked myFunction" << std::endl;
}

void __attribute__((constructor))
dl_entry()
{
	JNIEnv *jni;

	std::cout << "[*] Library loaded!" << std::endl;

	if (JNI_GetCreatedJavaVMs(&jvm, 1, NULL) != JNI_OK) {
		std::cout << "[!] Failed to retrieve JavaVM pointer" << std::endl;
		return;
	}

	std::cout << "[*] JavaVM: " << jvm << std::endl;

	if (jvm->AttachCurrentThread((void **)&jni, NULL) != JNI_OK) {
		std::cout << "[!] Failed to retrieve JNI environment" << std::endl;
		return;
	}

	std::cout << "[*] JNIEnv: " << jni << std::endl;

	start(jvm, jni);
}
