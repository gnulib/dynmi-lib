SRCS := $(wildcard $(SRC_DIR)/*.cpp)
TESTS := $(patsubst $(TEST_DIR)/%.cpp, %, $(wildcard $(TEST_DIR)/*Test.cpp))
CPPS := $(patsubst $(SRC_DIR)/%.cpp, %.cpp, $(SRCS))
OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(OBJS_DIR)/%.o, $(SRCS))
LDFLAGS := -lstdc++ -lpthread -pthread

all: test-app

include $(COMMON_MK)

$(CPPS):
	@echo 'Building target: $@'
	$(CC) -c $(CFLAGS) $(LDFLAGS) -o $(patsubst %.cpp, $(OBJS_DIR)/%.o, $@) $(SRC_DIR)/$@

$(TESTS): $(GTEST_LIB) $(GMOCK_LIB)
	@echo 'Building test: $@'
	$(CC) $(TEST_CFLAGS) $(LDFLAGS) -o $(TESTS_DIR)/$@ $(TEST_DIR)/$@.cpp $(LIBS_DIR)/$(LIB_DYNMI) $(TESTLIBS)
	cd $(BUILD_DIR) && ./$@
	rm -f $(BUILD_DIR)/core.*

test: $(LIB_DYNMI) $(TESTS)

test-app: $(CPPS)
	@echo 'Building redis-cli'
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(BINS_DIR)/$@ $(OBJS) $(LIBS_DIR)/$(LIB_DYNMI) $(LIBS_DIR)/$(HIREDIS_LIB)
