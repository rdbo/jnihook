# JNIHook
![jnihook-logo](https://raw.githubusercontent.com/rdbo/jnihook/master/LOGO.png)

Hook Java methods natively from C/C++

## License
This project is licensed under the `GNU AGPL-3.0`. No later version is allowed.

Read the file `LICENSE` for more information.

## Example
Hooking the function `static int myFunction(int mynumber, String name)` from a class `Dummy`:
```c++
jmethodID originalMethod;
jint hkMyFunction(JNIEnv *env, jclass clazz, jint number, jstring name)
{
	// Print parameters
	std::cout << "[*] mynumber value: " << mynumber << std::endl;
	std::cout << "[*] name object: " << name << std::endl;

	// Call the original method with 'mynumber' set to '1337'
	env->CallStaticIntMethod(dummyClass, originalMethod, 1337, name);

	return 42; // Modify the return value to '42'
}

void start(JavaVM *jvm)
{
	jclass dummyClass = env->FindClass("dummy/Dummy");
	jmethodID myFunctionID = env->GetStaticMethodID(dummyClass, "myFunction",
							"(ILjava/lang/String;)I");

	JNIHook_Init(jvm);
	JNIHook_Attach(myFunctionID, hkMyFunction, &originalMethod);
}
```

## Building
To build this, you can either compile all the files in `src` into your project, or
use CMake to build a static library, which can be compiled into your project.

NOTE: This requires C++17, which is available on newer versions of Visual Studio and GCC.

The steps ahead are for building a static library with CMake.


1. Setup your `JAVA_HOME` environment variable. It will be used for accessing `jni.h`, `jvmti.h`, and linking
the `jvm` library. On Linux, it's usually on `/usr/lib/jvm/<java release>`, and on Windows at `%ProgramFiles%\Java\jdk-<version>`.

---

2. Create a build directory inside the root directory of the repository and enter it:

NOTE: Use the `x64 Native Tools Command Prompt` on Windows.
```
mkdir build
cd build
```

---

3. Run CMake to setup the project

**Linux/\*nix**:
```
cmake ..
```

**Windows**:
```
cmake .. -G "NMake Makefiles" -DCMAKE_CXX_STANDARD=17
```

---

4. Build using `nmake` (Windows) or `make` (*nix):

**Linux/\*nix**:
```
make
```

**Windows**:
```
nmake
```

---

After running these commands, you will have `libjnihook.a` or `jnihook.lib` in your `build` directory, which you can compile along with your project.

NOTE: Don't forget to include JNIHook's `include` dir in your project so that you can `#include <jnihook.h>`.

## Acknowledgements
Special thanks to [@Lefraudeur](https://github.com/Lefraudeur) for helping me with information about JVM functionality
