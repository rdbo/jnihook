cmake_minimum_required(VERSION 3.21)

include(ExternalProject)

project(jnihook LANGUAGES C CXX)

set (CMAKE_CXX_STANDARD 23) # Only necessary for <jnihook.hpp>. If you will use <jnihook.h> only, C++ 11 is fine.
set(JAVA_HOME $ENV{JAVA_HOME} CACHE STRING "Set JAVA_HOME for dependency lookup")
option(JNIHOOK_BUILD_TESTS "Enable building of tests" OFF)

# external dependencies
set(EXTERNAL_DEPENDENCIES_DIR "${PROJECT_SOURCE_DIR}/external")
set(JNIF_DIR "${EXTERNAL_DEPENDENCIES_DIR}/jnif")
set(JNIF_INC "${JNIF_DIR}/include")
set(JNIF_IMPORT_DIR "${PROJECT_BINARY_DIR}/external/jnif-lib-prefix/src/jnif-lib-build")

add_subdirectory(${JNIF_DIR})
# jnihook
set(JNIHOOK_DIR "${PROJECT_SOURCE_DIR}/src")
set(JNIHOOK_INC "${PROJECT_SOURCE_DIR}/include")
file(GLOB_RECURSE JNIHOOK_SRC "${JNIHOOK_DIR}/*.cpp")

set(JAVA_INCLUDES "${JAVA_HOME}/include")
if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
	set(JAVA_INCLUDES ${JAVA_INCLUDES} "${JAVA_HOME}/include/windows")
else()
	set(JAVA_INCLUDES ${JAVA_INCLUDES} "${JAVA_HOME}/include/linux")
endif()

add_library(jnihooksingle STATIC ${JNIHOOK_SRC})
target_include_directories(jnihooksingle PUBLIC ${JNIHOOK_INC} ${JAVA_INCLUDES})
set_target_properties(jnihooksingle PROPERTIES POSITION_INDEPENDENT_CODE True)
# TODO: Add other architectures for OpenJDK8 lookup path
#       since it uses a non-standard path across platforms ($JAVA_HOME/jre/lib/<ARCH>/server)
target_link_directories(jnihooksingle PUBLIC "${JAVA_HOME}/lib" "${JAVA_HOME}/lib/server" "${JAVA_HOME}/jre/lib/amd64/server/")
target_link_libraries(jnihooksingle PUBLIC jvm jnif)

# bundle static libraries
set(JNIHOOK_BUNDLE_LIB_PATH "${PROJECT_BINARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}jnihook${CMAKE_STATIC_LIBRARY_SUFFIX}")

if(MSVC)
	add_custom_command(
		TARGET jnihooksingle POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E remove -f ${JNIHOOK_BUNDLE_LIB_PATH}
		COMMAND lib /OUT:${JNIHOOK_BUNDLE_LIB_PATH} $<TARGET_FILE:jnif> $<TARGET_FILE:jnihook>
		COMMENT "Creating combined static library ${JNIHOOK_BUNDLE_LIB_PATH}"
	)
else()
	add_custom_command(
		TARGET jnihooksingle POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E remove -f ${JNIHOOK_BUNDLE_LIB_PATH}
		COMMAND ${CMAKE_AR} -x $<TARGET_FILE:jnif>
		COMMAND ${CMAKE_AR} -x $<TARGET_FILE:jnihooksingle>
		COMMAND ${CMAKE_AR} -rcs ${JNIHOOK_BUNDLE_LIB_PATH} *.o
		COMMAND ${CMAKE_COMMAND} -E remove -f *.o
		WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
		COMMENT "Creating combined static archive ${JNIHOOK_BUNDLE_LIB_PATH}"
	)
endif()

add_library(jnihook STATIC IMPORTED GLOBAL)
set_target_properties(jnihook PROPERTIES IMPORTED_LOCATION ${JNIHOOK_BUNDLE_LIB_PATH})
add_dependencies(jnihook jnihooksingle)

# tests
if(JNIHOOK_BUILD_TESTS)
	# Build Java classes
	file(COPY "${PROJECT_SOURCE_DIR}/tests/dummy" DESTINATION .)
	execute_process(COMMAND "${JAVA_HOME}/bin/javac${CMAKE_EXECUTABLE_SUFFIX}" "dummy/Dummy.java")
	
	# Build library to inject
	set(TESTS_DIR "${PROJECT_SOURCE_DIR}/tests")
	set(TESTS_SRC "${TESTS_DIR}/test.cpp")
	add_library(test SHARED ${TESTS_SRC})
	target_include_directories(test PUBLIC ${JNIHOOK_INC} ${JAVA_INCLUDES})
	target_link_libraries(test PRIVATE jnihook)
	set_target_properties(test PROPERTIES POSITION_INDEPENDENT_CODE True)
endif()
