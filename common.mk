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
