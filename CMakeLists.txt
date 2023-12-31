cmake_minimum_required(VERSION 3.21)

project(jnihook LANGUAGES C CXX)

option(JNIHOOK_BUILD_TESTS "Enable building of tests" OFF)

set(JNIHOOK_DIR "${PROJECT_SOURCE_DIR}/src")
set(JNIHOOK_INC "${PROJECT_SOURCE_DIR}/include")
set(JNIHOOK_SRC "${JNIHOOK_DIR}/jnihook.cpp" "${JNIHOOK_DIR}/jvm.cpp")
if("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
	enable_language(ASM_MASM)
	set(JNIHOOK_SRC ${JNIHOOK_SRC} "${JNIHOOK_DIR}/gateway-win-x86_64.asm")
else()
	enable_language(ASM)
	set(JNIHOOK_SRC ${JNIHOOK_SRC} "${JNIHOOK_DIR}/gateway-nix-x86_64.S")
	set(JNIHOOK_LDFLAGS -z noexecstack)
endif()

add_library(jnihook STATIC ${JNIHOOK_SRC})
target_include_directories(jnihook PUBLIC ${JNIHOOK_INC})
target_link_options(jnihook PUBLIC ${JNIHOOK_LDFLAGS})
set_target_properties(jnihook PROPERTIES POSITION_INDEPENDENT_CODE True)
if("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
	target_link_libraries(jnihook PRIVATE jvm)
endif()

if(JNIHOOK_BUILD_TESTS)
	set(TESTS_DIR "${PROJECT_SOURCE_DIR}/tests")
	set(TESTS_SRC "${TESTS_DIR}/test.cpp")
	add_library(test SHARED ${TESTS_SRC})
	target_link_libraries(test PRIVATE jnihook)
	set_target_properties(test PROPERTIES POSITION_INDEPENDENT_CODE True)
endif()
