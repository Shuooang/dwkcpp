# orshrd/UcDB — Linux CMake placeholder
# Windows: UcDB.sln / UcDB.vcxproj (그대로)
#
# 활성화: cmake -DNGS_BUILD_UCDB=ON ...
# 준비되면 add_library(ucdb ...) 와 target_link_libraries(… uctool) 추가.

cmake_minimum_required(VERSION 3.20)

message(FATAL_ERROR
	"UcDB Linux port is not started yet.\n"
	"  Keep NGS_BUILD_UCDB=OFF, or implement sources in this CMakeLists.txt.")
