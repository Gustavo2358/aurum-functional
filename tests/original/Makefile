CC ?= cc
PYTHON ?= .venv/bin/python
CFLAGS ?= -std=c17 -O2 -g -Wall -Wextra -Wpedantic -Werror
CPPFLAGS += -Isubject/include
CORE := $(filter-out subject/src/main.c,$(wildcard subject/src/*.c))
.PHONY: all clean test-fast test-full
all: build/aurum build/libaurum.so
build:
	mkdir -p build
build/aurum: $(CORE) subject/src/main.c subject/include/aurum.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CORE) subject/src/main.c -o $@
build/libaurum.so: $(CORE) subject/include/aurum.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -shared $(CORE) -o $@
test-fast: all
	$(PYTHON) tools/check_spec.py --gherkin
	$(PYTHON) tools/check_oracle_math.py
	$(PYTHON) harness/run.py --self-test
	$(PYTHON) harness/protocol_adversarial.py
	$(PYTHON) tools/trace.py
clean:
	rm -rf build
test-full: all
	$(PYTHON) tools/full.py
