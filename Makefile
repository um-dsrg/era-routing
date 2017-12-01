TARGET_EXEC ?= lp-solver.out

BUILD_DIR ?= ./build

SRCS := $(wildcard *.cc)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

CXX := g++
CXXFLAGS := -MMD -MP -std=c++11

INCLUDES := -I $(HOME)/libraries/lemon/include
# Changing library directories based on OS
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
  LDFLAGS := -L $(HOME)/libraries/lemon/lib -L $(HOME)/libraries/ILOG/CPLEX_Studio1271/cplex/lib/x86-64_linux/static_pic -lemon -lcplex -lglpk -ltinyxml2 -lpthread
endif
ifeq ($(UNAME_S),Darwin)
  LDFLAGS := -L $(HOME)/libraries/lemon/lib -L $(HOME)/libraries/ILOG/CPLEX_Studio1271/cplex/lib/x86-64_osx/static_pic -lemon -lcplex -lglpk -ltinyxml2 -lpthread
endif

release: CXXFLAGS += -O3 -Werror -Wall
release: $(BUILD_DIR)/$(TARGET_EXEC)

debug: CXXFLAGS += -DDEBUG -g -Werror -Wall
debug: $(BUILD_DIR)/$(TARGET_EXEC)

# linking the executable
$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -o $@

# compiling the c++ source files
$(BUILD_DIR)/%.cc.o: %.cc
	$(MKDIR_P) $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

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
