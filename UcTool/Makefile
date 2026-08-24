# UcTool Linux build (Makefile)
# Windows: UcTool.sln / .vcxproj (Visual Studio)
# Linux / Cursor: make  또는  CMake (CMakeLists.txt)
#
# Usage:
#   make            # Release static lib -> build/libuctool.a
#   make debug      # Debug static lib
#   make clean

CXX      ?= g++
AR       ?= ar
PROJECT  := UcTool

BUILD    ?= build
CFG      ?= release

ifeq ($(CFG),debug)
  CXXFLAGS_CFG := -O0 -g -D_DEBUG
  OBJDIR       := $(BUILD)/debug
else
  CXXFLAGS_CFG := -O2 -DNDEBUG
  OBJDIR       := $(BUILD)/release
endif

CXXFLAGS := -std=c++20 -Wall -Wextra -fPIC \
	-DPROJECT_NAME=\"$(PROJECT)\" \
	-I. \
	$(CXXFLAGS_CFG)

# Linux 1차: MFC/Win32 UI·네트워크 제외 코어만
SRCS := \
	UcBaseTools.cpp

OBJS := $(SRCS:%.cpp=$(OBJDIR)/%.o)
LIB  := $(OBJDIR)/libuctool.a

.PHONY: all release debug clean dirs

all: release

release:
	$(MAKE) CFG=release lib

debug:
	$(MAKE) CFG=debug lib

lib: dirs $(LIB)

dirs:
	@mkdir -p $(OBJDIR)

$(LIB): $(OBJS)
	$(AR) rcs $@ $^
	@echo "Built $@"

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD)
