CC := g++
ROOT_DIR := .
BUILD_DIR := Build
INCLUDE_DIR := Include
SRC_DIR := Source
TEST_DIR := Test
GTEST_DIR := gtest/googletest
GTEST_LIB := gtest_main.a
GMOCK_DIR := gtest/googlemock
GMOCK_LIB := gmock_main.a
HIREDIS_DIR := hiredis
HIREDIS_LIB := libhiredis.a
LIB_REDIS = libredis.a
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
# LIBS := $(HIREDIS_DIR)/$(HIREDIS_LIB) $(GTEST_DIR)/make/$(GTEST_LIB)
LIBS := $(BUILD_DIR)/$(HIREDIS_LIB)
TESTLIBS := $(BUILD_DIR)/$(HIREDIS_LIB) $(BUILD_DIR)/$(GTEST_LIB) $(BUILD_DIR)/$(GMOCK_LIB)
TESTS := $(patsubst $(TEST_DIR)/%.cpp, %, $(wildcard $(TEST_DIR)/*Test.cpp))
CPPS := $(patsubst $(SRC_DIR)/%.cpp, %.cpp, $(SRCS))
OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRCS))
CFLAGS := -g -Wall -I$(ROOT_DIR)/$(INCLUDE_DIR) -I$(HIREDIS_DIR)/..
TEST_CFLAGS := -g -Wall -I$(ROOT_DIR)/$(INCLUDE_DIR) -I$(GTEST_DIR)/include -I$(GMOCK_DIR)/include -I$(HIREDIS_DIR)/..
LDFLAGS := -lstdc++ -lpthread

all: $(LIB_REDIS) $(TESTS)

$(LIB_REDIS): $(HIREDIS_LIB) $(OBJS)
	ar -r $(BUILD_DIR)/$@ $(OBJS)

$(OBJS):
	mkdir -p $(BUILD_DIR)
	@echo 'Building target: $@'
	$(CC) -c $(CFLAGS) $(LDFLAGS) -o $@ $(patsubst $(BUILD_DIR)/%.o, $(SRC_DIR)/%.cpp, $@)

$(TESTS): $(GTEST_LIB) $(GMOCK_LIB) $(LIB_REDIS)
	mkdir -p $(BUILD_DIR)
	@echo 'Building test: $@'
	$(CC) $(TEST_CFLAGS) $(LDFLAGS) -o $(BUILD_DIR)/$@ $(TEST_DIR)/$@.cpp $(BUILD_DIR)/$(LIB_REDIS) $(TESTLIBS)
	cd $(BUILD_DIR) && ./$@
	rm -f $(BUILD_DIR)/core.*

$(HIREDIS_LIB):
	mkdir -p $(BUILD_DIR)
	@echo 'Building hiredis library'
	cd $(HIREDIS_DIR) && $(MAKE)
	cp -f $(HIREDIS_DIR)/$(HIREDIS_LIB) $(BUILD_DIR)

$(GTEST_LIB): 
	mkdir -p $(BUILD_DIR)
	@echo 'Building googletest'
	cd $(GTEST_DIR)/make && $(MAKE)
	cp -f $(GTEST_DIR)/make/$(GTEST_LIB) $(BUILD_DIR)

$(GMOCK_LIB): $(GTEST_LIB)
	mkdir -p $(BUILD_DIR)
	@echo 'Building googlemock'
	cd $(GMOCK_DIR)/make && $(MAKE)
	cp -f $(GMOCK_DIR)/make/$(GMOCK_LIB) $(BUILD_DIR)

test: $(LIB_REDIS) $(TESTS)

clean:
	cd $(GTEST_DIR)/make && $(MAKE) clean
	cd $(GMOCK_DIR)/make && $(MAKE) clean
	cd $(HIREDIS_DIR) && $(MAKE) clean
	rm -rf $(BUILD_DIR)

.PHONY: all
