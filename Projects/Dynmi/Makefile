SRCS := $(wildcard $(SRC_DIR)/*.cpp)
TESTS := $(patsubst $(TEST_DIR)/%.cpp, %, $(wildcard $(TEST_DIR)/*Test.cpp))
CPPS := $(patsubst $(SRC_DIR)/%.cpp, %.cpp, $(SRCS))
OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(OBJS_DIR)/%.o, $(SRCS))
LDFLAGS := -lstdc++ -lpthread -pthread

all: $(LIB_DYNMI) $(TESTS)

include $(COMMON_MK)

$(LIB_DYNMI): $(HIREDIS_LIB) $(CPPS)
	@echo 'Building target: $@'
	ar -r $(LIBS_DIR)/$@ $(OBJS)

$(CPPS):
	@echo 'Building target: $@'
	mkdir -p $(OBJS_DIR)
	$(CC) -c $(CFLAGS) $(LDFLAGS) -o $(patsubst %.cpp, $(OBJS_DIR)/%.o, $@) $(SRC_DIR)/$@

$(TESTS): $(GTEST_LIB) $(GMOCK_LIB) $(LIB_DYNMI)
	@echo 'Building test: $@'
	mkdir -p $(TESTS_DIR)
	$(CC) $(TEST_CFLAGS) $(LDFLAGS) -o $(TESTS_DIR)/$@ $(TEST_DIR)/$@.cpp $(LIBS_DIR)/$(LIB_DYNMI) $(TESTLIBS)
	cd $(TESTS_DIR) && ./$@
	rm -f $(TESTS_DIR)/core.*

test: $(LIB_DYNMI) $(TESTS)

clean:
	cd $(GTEST_DIR)/make && $(MAKE) clean
	cd $(GMOCK_DIR)/make && $(MAKE) clean
	cd $(HIREDIS_DIR) && $(MAKE) clean
	rm -rf $(BUILD_DIR)
