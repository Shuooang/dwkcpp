# orshrd/UcTool — Linux CMake (Windows는 UcTool.sln / .vcxproj)
#
# 루트에서:  add_subdirectory(orshrd/UcTool)
# 단독으로:  cmake -S orshrd/UcTool -B orshrd/UcTool/build

cmake_minimum_required(VERSION 3.20)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	project(UcTool LANGUAGES CXX)
	set(CMAKE_CXX_STANDARD 20)
	set(CMAKE_CXX_STANDARD_REQUIRED ON)
	set(CMAKE_CXX_EXTENSIONS OFF)
	if(WIN32)
		message(FATAL_ERROR "On Windows use UcTool.sln. This CMakeLists is for Linux.")
	endif()
endif()

option(UCTOOL_LINUX_CORE_ONLY "Build Linux-portable core only (no MFC/Win32 UI)" ON)

add_library(uctool STATIC)

target_compile_definitions(uctool PRIVATE
	PROJECT_NAME="UcTool"
	$<$<CONFIG:Debug>:_DEBUG>
	$<$<NOT:$<CONFIG:Debug>>:NDEBUG>
)

target_include_directories(uctool
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
)

if(UCTOOL_LINUX_CORE_ONLY)
	# Phase-1: expand as porting progresses (same .cpp as Windows, subset only)
	target_sources(uctool PRIVATE
		UcBaseTools.cpp
	)
else()
	message(FATAL_ERROR "Full UcTool Linux port not ready; keep UCTOOL_LINUX_CORE_ONLY=ON")
endif()

set_target_properties(uctool PROPERTIES OUTPUT_NAME uctool)

message(STATUS "UcTool: Linux core → target 'uctool' (libuctool.a)")
