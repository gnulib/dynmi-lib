CC := g++
# ROOT_DIR := "$(dirname ${BASH_SOURCE[0]})"
ROOT_DIR := .
BUILD_DIR := Build
INCLUDE_DIR := Include
SRC_DIR := Source
TEST_DIR := Test
LIBS := /usr/local/lib/libhiredis.a
LIB_REDIS = libredis.a
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
CPPS := $(patsubst $(SRC_DIR)/%.cpp, %.cpp, $(SRCS))
OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRCS))
CFLAGS := -g -Wall -I$(ROOT_DIR)/$(INCLUDE_DIR) -Wc++11-extensions
LFLAGS := -Wall

all: $(LIB_REDIS) test

$(LIB_REDIS): $(OBJS)
	ar -r $(BUILD_DIR)/$@ $(OBJS)

$(OBJS):
	mkdir -p $(BUILD_DIR)
	@echo 'Building target: $@'
	$(CC) -c $(CFLAGS) $(LDFLAGS) -o $@ $(patsubst $(BUILD_DIR)/%.o, $(SRC_DIR)/%.cpp, $@)

test: $(LIB_REDIS)
	@echo 'Building target: $@'
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(BUILD_DIR)/test $(TEST_DIR)/*.cpp $(BUILD_DIR)/$(LIB_REDIS) $(LIBS)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all
