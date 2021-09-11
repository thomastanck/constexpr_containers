OUT ?= build

LIBSRCS := \
#

MAINSRCS := \
	test/main.cc \
	test/vector.cc \
	test/container_algorithm.cc \
#

ALLSRCS := ${MAINSRCS} ${LIBSRCS}

CXX ?= g++
CXXFLAGS ?= -Irange-v3/include -Iinclude -std=c++20 -Wall -Wextra -g
LDFLAGS ?=
LDLIBS ?=

ifeq ($(SANITIZE),1)
	CXXFLAGS += -fsanitize=address,undefined
	LDFLAGS += -fsanitize=address,undefined
endif

all: $(OUT)/main

$(OUT)/main: $(patsubst %,$(OUT)/%.o,$(MAINSRCS)) $(patsubst %,$(OUT)/%.o,$(LIBSRCS))
	@mkdir -p $(@D)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(OUT)/%.cc.o: %.cc
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ -c $<

$(OUT)/%.cc.d: %.cc
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -MM -MT "$(patsubst %,$(OUT)/%.o,$<) $(patsubst %,$(OUT)/%.d,$<)" -o $@ $<

include $(patsubst %,$(OUT)/%.d,$(ALLSRCS))

.PHONY: clean
clean:
	rm -rf $(OUT)