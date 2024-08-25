cmake_minimum_required(VERSION 3.21)

project(jnihook LANGUAGES C CXX)

set(JAVA_HOME $ENV{JAVA_HOME} CACHE STRING "Set JAVA_HOME for dependency lookup")
option(JNIHOOK_BUILD_TESTS "Enable building of tests" OFF)

set(JNIHOOK_DIR "${PROJECT_SOURCE_DIR}/src")
set(JNIHOOK_INC "${PROJECT_SOURCE_DIR}/include")
set(JNIHOOK_SRC "${JNIHOOK_DIR}/jnihook.cpp" "${JNIHOOK_DIR}/classfile.cpp" "${JNIHOOK_DIR}/uuid.cpp")

set(JAVA_INCLUDES "${JAVA_HOME}/include")
if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
	set(JAVA_INCLUDES ${JAVA_INCLUDES} "${JAVA_HOME}/include/windows")
else()
	set(JAVA_INCLUDES ${JAVA_INCLUDES} "${JAVA_HOME}/include/linux")
endif()

add_library(jnihook STATIC ${JNIHOOK_SRC})
target_include_directories(jnihook PUBLIC ${JNIHOOK_INC} ${JAVA_INCLUDES})
set_target_properties(jnihook PROPERTIES POSITION_INDEPENDENT_CODE True)
target_link_directories(jnihook PUBLIC "${JAVA_HOME}/lib" "${JAVA_HOME}/lib/server")
target_link_libraries(jnihook PUBLIC jvm)

if(JNIHOOK_BUILD_TESTS)
	# Build Java classes
	file(COPY "${PROJECT_SOURCE_DIR}/tests/dummy" DESTINATION .)
	execute_process(COMMAND "${JAVA_HOME}/bin/javac${CMAKE_EXECUTABLE_SUFFIX}" "dummy/Dummy.java")
	
	# Build library to inject
	set(TESTS_DIR "${PROJECT_SOURCE_DIR}/tests")
	set(TESTS_SRC "${TESTS_DIR}/test.cpp")
	add_library(test SHARED ${TESTS_SRC})
	target_link_libraries(test PRIVATE jnihook)
	set_target_properties(test PROPERTIES POSITION_INDEPENDENT_CODE True)
endif()
