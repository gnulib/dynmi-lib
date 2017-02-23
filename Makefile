CC := g++
ROOT_DIR := .
BUILD_DIR := Build
INCLUDE_DIR := Include
SRC_DIR := Source
TEST_DIR := Test
GTEST_DIR := gtest/googletest
GTEST_LIB := gtest_main.a
HIREDIS_DIR := hiredis
HIREDIS_LIB := libhiredis.a
LIB_REDIS = libredis.a
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
LIBS := $(HIREDIS_DIR)/$(HIREDIS_LIB) $(GTEST_DIR)/make/$(GTEST_LIB)
TESTS := $(patsubst $(TEST_DIR)/%.cpp, %, $(wildcard $(TEST_DIR)/*Test.cpp))
CPPS := $(patsubst $(SRC_DIR)/%.cpp, %.cpp, $(SRCS))
OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRCS))
CFLAGS := -g -Wall -I$(ROOT_DIR)/$(INCLUDE_DIR) -I$(GTEST_DIR)/include -I$(HIREDIS_DIR)/..
LDFLAGS := -lstdc++ -lpthread

all: $(LIB_REDIS) $(TESTS)

$(LIB_REDIS): $(GTEST_LIB) $(HIREDIS_LIB) $(OBJS)
	ar -r $(BUILD_DIR)/$@ $(OBJS)

$(OBJS):
	mkdir -p $(BUILD_DIR)
	@echo 'Building target: $@'
	$(CC) -c $(CFLAGS) $(LDFLAGS) -o $@ $(patsubst $(BUILD_DIR)/%.o, $(SRC_DIR)/%.cpp, $@)

$(TESTS): $(LIB_REDIS)
	@echo 'Building test: $@'
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(BUILD_DIR)/$@ $(TEST_DIR)/$@.cpp $(BUILD_DIR)/$(LIB_REDIS) $(LIBS)
	cd $(BUILD_DIR) && ./$@
	rm -f $(BUILD_DIR)/core.*

$(HIREDIS_LIB):
	@echo 'Building hiredis library'
	cd $(HIREDIS_DIR) && $(MAKE)

$(GTEST_LIB): 
	@echo 'Building googletest'
	cd $(GTEST_DIR)/make && $(MAKE)

hiredis:
	@echo 'need to add rules to build hiredis lib'

test: $(LIB_REDIS) $(TESTS)

clean:
	cd $(GTEST_DIR)/make && $(MAKE) clean
	cd $(HIREDIS_DIR) && $(MAKE) clean
	rm -rf $(BUILD_DIR)

.PHONY: all
