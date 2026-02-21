JAVA_HOME := `java -XshowSettings:properties -version 2>&1 | awk -F '= ' '/java.home/ {print $2}'`
NTHREADS := `nproc`

test-dev: build-dev
    cd build && java dummy.Dummy "`pwd`/libtest.so"

build-dev:
    mkdir -p build
    cd build && \
        JAVA_HOME={{JAVA_HOME}} cmake .. -DCMAKE_BUILD_TYPE=Debug -DJNIHOOK_BUILD_TESTS=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && \
        make -j {{NTHREADS}}

build-release:
    mkdir -p build
    cd build && \
        JAVA_HOME={{JAVA_HOME}} cmake .. -DCMAKE_BUILD_TYPE=Release -DJNIHOOK_BUILD_TESTS=OFF -DCMAKE_EXPORT_COMPILE_COMMANDS=OFF && \
        make -j {{NTHREADS}}

clean:
    rm -rf build
