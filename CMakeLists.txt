# Main CMakeLists.txt

cmake_minimum_required(VERSION 3.1.0)
project(holgol_server CXX)

include(CheckCXXSourceCompiles)
find_package(Threads)

find_package(Boost REQUIRED COMPONENTS system)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
	set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

if(MSVC)
	# MSVC tends to fake C++98 even with /std:c++17 on the command line.
	# This compiler sucks.
	# I hate it.
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:__cplusplus /D_CRT_SECURE_NO_WARNINGS")
endif()

# clang-cl optimizations
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" AND MSVC)
	if("${CMAKE_BUILD_TYPE}" MATCHES "Release")
		set(CMAKE_CXX_FLAGS_RELEASE "/MD /Oi /DNDEBUG /clang:-O3 /clang:-ffast-math")
		set(CMAKE_C_FLAGS_RELEASE "/MD /Oi /DNDEBUG /clang:-O3 /clang:-ffast-math")
	endif()
elseif(MSVC)
	# not clang-cl, but instead, it is *really* MSVC
	message(WARNING "While using MSVC is okay, I really don't reccomend it if you want fast & small binaries (especially with constexpr)")
endif()

# Detect the particular C++ standard library.
if(NOT DEFINED CACHE{PROJECT_STDLIB})
	message(STATUS "Detecting C++ standard library")
	set(CMAKE_REQUIRED_QUIET ON)

	check_cxx_source_compiles(
		"#include <ciso646>
		#if !defined(__GLIBCXX__)
		#error \"not libstdc++\"
		#endif
		int main() {}"
	__LIBRARY_LIBSTDCXX)

	check_cxx_source_compiles(
		"#include <ciso646>
		#if !defined(_LIBCPP_VERSION)
		#error \"not libc++\"
		#endif
		int main() {}"
	__LIBRARY_LIBCXX)

	check_cxx_source_compiles(
		"#include <ciso646>
		#if !defined(_MSVC_STL_VERSION)
		#error \"not MSVC\"
		#endif
		int main() {}"
	__LIBRARY_MSVC)

	if(__LIBRARY_LIBSTDCXX)
		set(STANDARD_LIBRARY "gnu")
	elseif(__LIBRARY_LIBCXX)
		set(STANDARD_LIBRARY "llvm")
	elseif(__LIBRARY_MSVC)
		set(STANDARD_LIBRARY "msvc")
	else()
		message(FATAL_ERROR "Can't find which C++ runtime is in use")
	endif()

	set(PROJECT_STDLIB ${STANDARD_LIBRARY} CACHE STRING "The C++ standard library in use; possible values: gnu, llvm, msvc")

	message(STATUS "Detected standard library ${PROJECT_STDLIB}")
endif()

add_subdirectory(vendor)
add_subdirectory(src)