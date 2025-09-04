cmake_minimum_required(VERSION 3.21)

project(jnihook LANGUAGES C CXX)

set(JAVA_HOME $ENV{JAVA_HOME} CACHE STRING "Set JAVA_HOME for dependency lookup")
option(JNIHOOK_BUILD_TESTS "Enable building of tests" OFF)
set(JAVA_INCLUDES "${JAVA_HOME}/include")

set(JNIHOOK_DIR "${PROJECT_SOURCE_DIR}/src")
set(JNIHOOK_INC "${PROJECT_SOURCE_DIR}/include")
file(GLOB_RECURSE JNIHOOK_SRC "${JNIHOOK_DIR}/*.cpp")

if("${JAVA_HOME}" STREQUAL "")
	message(WARNING "The 'JAVA_HOME' environment is not set! Please set it or make sure your Java development files are globally available.")
	unset(JAVA_HOME CACHE)
endif()

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
	set(JAVA_INCLUDES ${JAVA_INCLUDES} "${JAVA_HOME}/include/windows")

	enable_language(ASM_MASM)
	set(JNIHOOK_SRC ${JNIHOOK_SRC} "${JNIHOOK_DIR}/gateway-win-x86_64.asm")
else()
	set(JAVA_INCLUDES ${JAVA_INCLUDES} "${JAVA_HOME}/include/linux")

	enable_language(ASM)
	set(JNIHOOK_SRC ${JNIHOOK_SRC} "${JNIHOOK_DIR}/gateway-nix-x86_64.S")
	set(JNIHOOK_LDFLAGS -z noexecstack)
endif()

add_library(jnihook STATIC ${JNIHOOK_SRC})
target_include_directories(jnihook PUBLIC ${JNIHOOK_INC} ${JAVA_INCLUDES})
target_link_options(jnihook PUBLIC ${JNIHOOK_LDFLAGS})
set_target_properties(jnihook PROPERTIES POSITION_INDEPENDENT_CODE True)

# TODO: Add other architectures for OpenJDK8 lookup path
#       since it uses a non-standard path across platforms ($JAVA_HOME/jre/lib/<ARCH>/server)
target_link_directories(jnihook PUBLIC "${JAVA_HOME}/lib" "${JAVA_HOME}/lib/server" "${JAVA_HOME}/jre/lib/amd64/server/")
target_link_libraries(jnihook PUBLIC jvm)

if(JNIHOOK_BUILD_TESTS)
	set(TESTS_DIR "${PROJECT_SOURCE_DIR}/tests")
	set(TESTS_SRC "${TESTS_DIR}/test.cpp")
	add_library(test SHARED ${TESTS_SRC})
	target_link_libraries(test PRIVATE jnihook)
	set_target_properties(test PROPERTIES POSITION_INDEPENDENT_CODE True)
endif()
