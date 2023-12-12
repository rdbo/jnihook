#include <jnihook.h>
#include <jvmti.h>
#include <iostream>

jvalue hkMyFunction(jmethodID mID, void *params, size_t nparams, void *thread, void *arg)
{
	std::cout << "hkMyFunction called!" << std::endl;

	std::cout << "[*] method ID: " << mID << std::endl;
	std::cout << "[*] params: " << params << std::endl;
	std::cout << "[*] nparams: " << nparams << std::endl;
	std::cout << "[*] thread: " << thread << std::endl;
	
	JavaVM *jvm = (JavaVM *)arg;
	std::cout << "[*] Retrieved JVM: " << jvm << std::endl;

	JNIEnv *jni;
	jvmtiEnv *jvmti;
	jvm->GetEnv((void **)&jni, JNI_VERSION_1_6);
	std::cout << "[*] JNI: " << jni << std::endl;

	jvm->GetEnv((void **)&jvmti, JVMTI_VERSION_1_0);
	std::cout << "[*] JVMTI: " << jvmti << std::endl;

	jint *mynumber = (jint *)&((void **)params)[1];
	void *name = (void *)&((void **)params)[0];
	std::cout << "[*] mynumber address: " << mynumber << std::endl;
	std::cout << "[*] mynumber value: " << *mynumber << std::endl;
	std::cout << "[*] name: " << name << std::endl;
	*mynumber = 1337;

	// Force call the original function
	jclass clazz;
	jvmti->GetMethodDeclaringClass(mID, &clazz);
	std::cout << "[*] clazz: " << clazz << std::endl;
	
	jint result = jni->CallStaticIntMethod(clazz, mID, 69, name);
	std::cout << "[*] Forced call result: " << result << std::endl;

	return jvalue { 0 };
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
	JNIHook_Attach(myFunctionID, hkMyFunction, (void *)jvm);
}

void __attribute__((constructor))
dl_entry()
{
	JavaVM *jvm;
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
