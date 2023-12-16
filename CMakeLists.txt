cmake_minimum_required(VERSION 3.21)

project(jnihook LANGUAGES C CXX ASM)

set(JNIHOOK_DIR "${PROJECT_SOURCE_DIR}/src")
set(JNIHOOK_INC "${PROJECT_SOURCE_DIR}/include")
set(JNIHOOK_SRC "${JNIHOOK_DIR}/jnihook.cpp" "${JNIHOOK_DIR}/jvm.cpp")
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
	# TODO: Add Windows gateway
else()
	set(JNIHOOK_SRC ${JNIHOOK_SRC} "${JNIHOOK_DIR}/gateway-nix-x86_64.S")
	set(LINK_OPTS -z noexecstack)
endif()

set(TESTS_DIR "${PROJECT_SOURCE_DIR}/tests")
set(TESTS_SRC ${JNIHOOK_SRC} "${TESTS_DIR}/test.cpp")

add_library(test SHARED ${TESTS_SRC})
target_include_directories(test PUBLIC ${JNIHOOK_INC})
target_link_options(test PUBLIC ${LINK_OPTS})
