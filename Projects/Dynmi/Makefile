SRCS := $(wildcard $(SRC_DIR)/*.cpp)
TESTS := $(patsubst $(TEST_DIR)/%.cpp, %, $(wildcard $(TEST_DIR)/*Test.cpp))
CPPS := $(patsubst $(SRC_DIR)/%.cpp, %.cpp, $(SRCS))
OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(OBJS_DIR)/%.o, $(SRCS))

all: $(LIB_DYNMI) $(TESTS)

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

test: $(LIB_DYNMI) $(TESTS)

cleanlib:

clean:
	cd $(GTEST_DIR)/make && $(MAKE) clean
	cd $(GMOCK_DIR)/make && $(MAKE) clean
	cd $(HIREDIS_DIR) && $(MAKE) clean
	rm -rf $(BUILD_DIR)

.PHONY: all cleanlib
