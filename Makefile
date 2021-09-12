OUT ?= build

TARGETS := \
	test/algorithm \
	test/main \
	test/vector_base \
	test/vector \
#

CXX ?= g++
CXXFLAGS ?= -Iinclude -std=c++20 -Wall -Wextra -g
LDFLAGS ?=
LDLIBS ?=

ifeq ($(SANITIZE),1)
	CXXFLAGS += -fsanitize=address,undefined
	LDFLAGS += -fsanitize=address,undefined
endif

all: $(patsubst %,$(OUT)/%,$(TARGETS))

$(OUT)/%: $(patsubst %,$(OUT)/%.cc.o,%)
	@mkdir -p $(@D)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(OUT)/%.cc.o: %.cc
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ -c $<

$(OUT)/%.cc.d: %.cc
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -MM -MT "$(patsubst %,$(OUT)/%.o,$<) $(patsubst %,$(OUT)/%.d,$<)" -o $@ $<

include $(patsubst %,$(OUT)/%.cc.d,$(TARGETS))

.PHONY: clean
clean:
	rm -rf $(OUT)
