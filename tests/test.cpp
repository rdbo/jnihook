#include <jnihook.h>
#include <jvmti.h>
#include <iostream>

static JavaVM *jvm;
static jvmtiEnv *jvmti;
static int callCounter = 0;

jvalue hkMyFunction(jmethodID mID, jvalue *args, size_t nargs, void *thread, void *arg)
{
	std::cout << "hkMyFunction called!" << std::endl;

	std::cout << "[*] method ID: " << mID << std::endl;
	std::cout << "[*] args: " << args << std::endl;
	std::cout << "[*] nargs: " << nargs << std::endl;
	std::cout << "[*] thread: " << thread << std::endl;
	std::cout << "[*] arg: " << arg << std::endl;

	if (++callCounter >= 3) {
		JNIHook_Detach(mID);
		std::cout << "[*] Unhooked method" << std::endl;
	}

	std::cout << "[*] call counter: " << callCounter << std::endl;

	JNIEnv *jni;
	jvm->GetEnv((void **)&jni, JNI_VERSION_1_6);
	std::cout << "[*] JNI: " << jni << std::endl;

	jint *mynumber = (jint *)&((void **)args)[1];
	void *name = (void *)&((void **)args)[0];
	std::cout << "[*] mynumber address: " << mynumber << std::endl;
	std::cout << "[*] mynumber value: " << *mynumber << std::endl;
	std::cout << "[*] name: " << name << std::endl;
	*mynumber = 1337;

	// Force call the original function
	jclass clazz;
	jvmti->GetMethodDeclaringClass(mID, &clazz);
	std::cout << "[*] clazz: " << clazz << std::endl;
	
	// jint result = jni->CallStaticIntMethod(clazz, mID, 1000, name);
	// std::cout << "[*] Forced call result: " << result << std::endl;

	return jvalue { .i = 6969 };
}

static void start(JavaVM *jvm, JNIEnv *jni)
{
	jclass dummyClass;
	jmethodID myFunctionID;
	
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

	if (jvm->GetEnv((void **)&jvmti, JVMTI_VERSION_1_0) != JNI_OK) {
		std::cout << "[!] Failed to retrieve JVMTI environment" << std::endl;
		return;
	}

	start(jvm, jni);
}
