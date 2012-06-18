$(BINARY_ROOT)/%.o: $(PROJECT_ROOT)/%.cpp
	@[ -d $(@D) ] || mkdir -p $(@D)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

$(BINARY_ROOT)/%.dep: $(PROJECT_ROOT)/%.cpp
	@[ -d $(@D) ] || mkdir -p $(@D)
	$(CXX) -MM -MT $(patsubst %.dep,%.o,$@) $< $(CXXFLAGS) > $@

%.dep: %.cpp
	$(CXX) -MM -MT $(patsubst %.dep,%.o,$@) $< $(CXXFLAGS) > $@

build-required: $(addsuffix -build_submake_phony,$(REQUIRE))

%-build_submake_phony:
	$(MAKE) -C $* $(subst $(*)-,,$(subst _submake_phony,,$@))

$(BINARY_DIRECTORY)/%.cpp: %.py
	@[ -d $(@D) ] || mkdir -p $(@D)
	$(PYTHON3) $< $(ARGUMENT) > $@

clean:
	@echo '************************************************************'
	@echo '* Command "make clean" is not implemented.'
	@echo '* Use the following command to remove the entire build tree:'
	@echo '*   rm -rf $(BINARY_ROOT)'
	@echo '************************************************************'
