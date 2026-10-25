CC ?= cc
PYTHON ?= python3
CFLAGS ?= -std=c17 -O2 -g -Wall -Wextra -Wpedantic -Werror
CPPFLAGS += -Isubject/include
PRODUCTION := $(wildcard subject/src/core*.c subject/src/shell*.c)
HEADERS := $(wildcard subject/include/*.h)
.PHONY: all clean test-fast test-full
all: build/aurum build/libaurum.so
build:
	mkdir -p build
build/aurum: $(PRODUCTION) subject/src/main.c $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(PRODUCTION) subject/src/main.c -o $@
# Legacy ABI exists only for independent acceptance, outside the CLI product.
build/libaurum.so: $(PRODUCTION) tests/legacy_adapter.c $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -shared $(PRODUCTION) tests/legacy_adapter.c -o $@
test-fast: all
	$(PYTHON) tests/qualify.py --fast
test-full: all
	$(PYTHON) tests/qualify.py
clean:
	rm -rf build
