all:
	mkdir -p build
	javac tests/dummy/Dummy.java
	$(CXX) -o build/libtest.so -I ./include -I /usr/lib/jvm/default-jvm/include -I /usr/lib/jvm/default-jvm/include/linux -g -ggdb -shared -fPIC src/*.cpp src/*.S tests/test.cpp

clean:
	rm -rf build
