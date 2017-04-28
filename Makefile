TARGET_EXEC ?= lp-solver.out

BUILD_DIR ?= ./build

SRCS := $(wildcard *.cc)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

CXX := g++
CXXFLAGS := -MMD -MP

release: CXXFLAGS += -O3
release: $(BUILD_DIR)/$(TARGET_EXEC)

debug: CXXFLAGS += -DDEBUG -g
debug: $(BUILD_DIR)/$(TARGET_EXEC)

# linking the executable
$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

# compiling the c++ source files
$(BUILD_DIR)/%.cc.o: %.cc
	$(MKDIR_P) $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean cleanall

clean:
	$(RM) -r $(BUILD_DIR)/*

cleanall:
	$(RM) -r $(BUILD_DIR)

# including the header files
-include $(DEPS)

MKDIR_P ?= mkdir -p

# used to print makefile variables for testing purposes
print-%:
	@echo '$*=$($*)'
