# JNIHook
![jnihook-logo](https://raw.githubusercontent.com/rdbo/jnihook/master/LOGO.png)

Hook Java methods natively from C/C++

## License
This project is licensed under the `GNU AGPL-3.0`. No later version is allowed.

Read the file `LICENSE` for more information.

## Example
Hooking the function `static int myFunction(int mynumber, String name)` from a class `Dummy`:
```c++
jint hkMyFunction(JNIEnv *jni, jclass clazz, jint number, jstring name)
{
	// Print parameters
	std::cout << "[*] mynumber value: " << mynumber.i << std::endl;
	std::cout << "[*] name object: " << name.l << std::endl;

	// Get unhooked class for accessing original methods
	jclass originalClass = JNIHook_GetOriginalClass("dummy/Dummy");

	// Call the original method with 'mynumber' set to '1337'
	jni->CallNonvirtualIntMethod(dummyClass, callableMethod, 1337, name);

	return 42; // Modify the return value to '42'
}

void start(JavaVM *jvm, JNIEnv *env)
{
	jnihook_t jnihook;
	jclass dummyClass = env->FindClass("dummy/Dummy");
	jmethodID myFunctionID = env->GetStaticMethodID(dummyClass, "myFunction",
							"(ILjava/lang/String;)I");

	JNIHook_Init(env, &jnihook);
	JNIHook_Attach(&jnihook, myFunctionID, hkMyFunction, NULL);
}
```

## Building
To build this, you can either compile all the files in `src` into your project, or
use CMake to build a static library, which can be compiled into your project.

NOTE: This requires C++20, which is available on newer versions of Visual Studio and GCC.

The steps ahead are for building a static library with CMake.


1. Create a build directory inside the root directory of the repository and enter it:

NOTE: Use the `x64 Native Tools Command Prompt` on Windows.
```
mkdir build
cd build
```

2. Run CMake and set up the includes directories and additional OS-specific configuration.
Windows example (`%JAVA_HOME%` is `%ProgramFiles%\Java\jdk-<VERSION>`; might be necessary to copy `jvm.lib` into your `build` directory):
```
cmake .. -G "NMake Makefiles" -DCMAKE_CXX_FLAGS="/I \"%JAVA_HOME%\include\" /I \"%JAVA_HOME%\include\win32\"" -DCMAKE_CXX_STANDARD=20
```
Linux example (`$JAVA_HOME` is usually `/usr/lib/jvm/default-jvm` or `/usr/lib/jvm/default-java`):
```
cmake .. -DCMAKE_CXX_FLAGS="-I \"$JAVA_HOME/include\" -I \"$JAVA_HOME/include/linux\""
```

3. Build using `nmake` (Windows) or `make` (*nix):

Windows:
```
nmake
```
*nix:
```
make
```

After running these commands, you will have `jnihook.lib` or `libjnihook.a` in your `build` directory, which you can compile along with your project.

NOTE: Don't forget to include JNIHook's `include` dir in your project so that you can `#include <jnihook.h>`.

## Acknowledgements
Special thanks to [@Lefraudeur](https://github.com/Lefraudeur) for helping me with information about JVM functionality
