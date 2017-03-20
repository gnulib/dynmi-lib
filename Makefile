TARGETS := Dynmi TestApp
APPTESTS := $(patsubst %, %Test, $(TARGETS))
#mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
#ROOT_DIR := $(notdir $(patsubst %/,%,$(dir $(mkfile_path))))
export ROOT_DIR := $(shell pwd)
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
export LIBS_DIR := $(BUILD_DIR)/Libs
export OBJS_DIR := $(BUILD_DIR)/Objs
export BINS_DIR := $(BUILD_DIR)/Bins
export TESTS_DIR := $(BUILD_DIR)/Tests
export LIBS := $(LIBS_DIR)/$(HIREDIS_LIB)
export TESTLIBS := $(LIBS_DIR)/$(HIREDIS_LIB) $(LIBS_DIR)/$(GTEST_LIB) $(LIBS_DIR)/$(GMOCK_LIB)
export CFLAGS := -g -Wall -I$(INCLUDE_DIR) -I$(HIREDIS_DIR)/.. -I$(COMMON_INCLUDE_DIR)
export TEST_CFLAGS := $(CFLAGS) -I$(GTEST_DIR)/include -I$(GMOCK_DIR)/include
export LDFLAGS := -lstdc++ -lpthread -pthread

all: $(TARGETS)

$(HIREDIS_LIB):
	@echo 'Building hiredis library'
	mkdir -p $(LIBS_DIR)
	cd $(HIREDIS_DIR) && $(MAKE)
	cp -f $(HIREDIS_DIR)/$(HIREDIS_LIB) $(LIBS_DIR)

$(GTEST_LIB): 
	@echo 'Building googletest'
	mkdir -p $(LIBS_DIR)
	cd $(GTEST_DIR)/make && $(MAKE)
	cp -f $(GTEST_DIR)/make/$(GTEST_LIB) $(LIBS_DIR)

$(GMOCK_LIB): $(GTEST_LIB)
	@echo 'Building googlemock'
	mkdir -p $(LIBS_DIR)
	cd $(GMOCK_DIR)/make && $(MAKE)
	cp -f $(GMOCK_DIR)/make/$(GMOCK_LIB) $(LIBS_DIR)

$(TARGETS): $(HIREDIS_LIB) $(GTEST_LIB) $(GMOCK_LIB)
	@echo 'Building target: $@'
	cd Projects/$@ && $(MAKE)

$(APPTESTS): $(HIREDIS_LIB) $(GTEST_LIB) $(GMOCK_LIB)
	@echo 'Building test: $@'
	cd Projects/$(patsubst %Test,%,$@) && $(MAKE) test

test: $(APPTESTS)

clean:
	cd $(GTEST_DIR)/make && $(MAKE) clean
	cd $(GMOCK_DIR)/make && $(MAKE) clean
	cd $(HIREDIS_DIR) && $(MAKE) clean
	rm -rf $(BUILD_DIR)

.PHONY: all cleanlib
