CC := g++
ROOT_DIR := .
BUILD_DIR := Build
INCLUDE_DIR := Include
SRC_DIR := Source
TEST_DIR := Test
LIBS := /usr/local/lib/libhiredis.a /usr/local/lib/gtest_main.a
LIB_REDIS = libredis.a
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
TESTS := $(patsubst $(TEST_DIR)/%.cpp, %, $(wildcard $(TEST_DIR)/*Test.cpp))
CPPS := $(patsubst $(SRC_DIR)/%.cpp, %.cpp, $(SRCS))
OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRCS))
CFLAGS := -g -Wall -I$(ROOT_DIR)/$(INCLUDE_DIR)
LFLAGS := -lstdc++

all: $(LIB_REDIS) $(TESTS)

$(LIB_REDIS): $(OBJS)
	ar -r $(BUILD_DIR)/$@ $(OBJS)

$(OBJS):
	mkdir -p $(BUILD_DIR)
	@echo 'Building target: $@'
	$(CC) -c $(CFLAGS) $(LDFLAGS) -o $@ $(patsubst $(BUILD_DIR)/%.o, $(SRC_DIR)/%.cpp, $@)

test: $(LIB_REDIS) $(TESTS)

$(TESTS): $(LIB_REDIS)
	@echo 'Building test: $@'
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(BUILD_DIR)/$@ $(TEST_DIR)/$@.cpp $(BUILD_DIR)/$(LIB_REDIS) $(LIBS)
	$(BUILD_DIR)/$@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all
