export ROOT_DIR := $(shell pwd)
export COMMON_MK = $(ROOT_DIR)/common.mk
export BUILD_DIR := $(ROOT_DIR)/Build
export COMMON_INCLUDE_DIR := $(ROOT_DIR)/Common/Include
export INCLUDE_DIR := Include
export SRC_DIR := Source
export TEST_DIR := Test
export GTEST_DIR := $(ROOT_DIR)/gtest/googletest
export GTEST_LIB := gtest_main.a
export GMOCK_DIR := $(ROOT_DIR)/gtest/googlemock
export GMOCK_LIB := gmock_main.a
export HIREDIS_DIR := $(ROOT_DIR)/hiredis
export HIREDIS_LIB := libhiredis.a
export LIB_DYNMI = libdynmi.a
export LIBS_DIR := $(BUILD_DIR)/Lib
export OBJS_DIR := $(BUILD_DIR)/Obj
export BINS_DIR := $(BUILD_DIR)/Bin
export TESTS_DIR := $(BUILD_DIR)/Test
export LIBS := $(LIBS_DIR)/$(HIREDIS_LIB)
export TESTLIBS := $(LIBS_DIR)/$(HIREDIS_LIB) $(LIBS_DIR)/$(GTEST_LIB) $(LIBS_DIR)/$(GMOCK_LIB)
export CFLAGS := -g -Wall -I$(INCLUDE_DIR) -I$(HIREDIS_DIR)/.. -I$(COMMON_INCLUDE_DIR)
export TEST_CFLAGS := $(CFLAGS) -I$(GTEST_DIR)/include -I$(GMOCK_DIR)/include

TARGETS := Dynmi TestApp
APPTESTS := $(patsubst %, %Test, $(TARGETS))


all: $(TARGETS)

include $(COMMON_MK)

$(TARGETS): $(HIREDIS_LIB) $(GTEST_LIB) $(GMOCK_LIB)
	@echo 'Building target: $@'
	cd Projects/$@ && $(MAKE)

$(APPTESTS): $(HIREDIS_LIB) $(GTEST_LIB) $(GMOCK_LIB)
	@echo 'Building test: $@'
	cd Projects/$(patsubst %Test,%,$@) && $(MAKE) test

test: $(APPTESTS)
