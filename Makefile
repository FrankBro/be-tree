################################################################################
# Variables
################################################################################

UNAME := $(shell uname)

CFLAGS := -O3 -g -std=gnu11 -Wall -Wextra -Wshadow -Wfloat-equal -Wundef -Wcast-align \
	-Wwrite-strings -Wunreachable-code -Wformat=2 -Wswitch-enum \
	-Wswitch-default -Winit-self -Wno-strict-aliasing

LDFLAGS := -lm -fPIC
LDFLAGS_TESTS := $(LDFLAGS) -lgsl -lgslcblas

LEX_SOURCES = $(wildcard src/*.l)
LEX_INTERMEDIATES = \
	$(patsubst %.l,%.c,${LEX_SOURCES}) \
	$(patsubst %.l,%.h,${LEX_SOURCES})

YACC_SOURCES = $(wildcard src/*.y)
YACC_INTERMEDIATES = \
	$(patsubst %.y,%.c,${YACC_SOURCES}) \
	$(patsubst %.y,%.h,${YACC_SOURCES})

INTERMEDIATES = $(LEX_INTERMEDIATES) $(YACC_INTERMEDIATES)
GENERATED_OBJECTS = \
	$(patsubst %.l,%.o,$(LEX_SOURCES)) \
	$(patsubst %.y,%.o,$(YACC_SOURCES))

SOURCES = $(filter-out $(INTERMEDIATES),$(wildcard src/*.c))
OBJECTS = \
	$(patsubst %.c,%.o,$(SOURCES)) \
	$(GENERATED_OBJECTS)

TEST_SOURCES=$(wildcard tests/*_tests.c)
TEST_BINARIES=$(patsubst %.c,%,${TEST_SOURCES})

LEX?=flex
YACC?=bison
YFLAGS?=-dv

VALGRIND=valgrind --tool=memcheck --leak-check=full --track-origins=yes --suppressions=valgrind.supp --error-exitcode=1
CALLGRIND=valgrind --tool=callgrind --instr-atstart=no
CACHEGRIND=valgrind --tool=cachegrind
MASSIF=valgrind --tool=massif
TIDY=clang-tidy

ERTS_INCLUDE_DIR ?= $(shell erl -noshell -eval "io:format(\"~ts/erts-~ts/include/\", [code:root_dir(), erlang:system_info(version)])." -s init stop)
ERL_INTERFACE_INCLUDE_DIR ?= $(shell erl -noshell -eval "io:format(\"~ts\", [code:lib_dir(erl_interface, include)])." -s init stop)
ERL_INTERFACE_LIB_DIR ?= $(shell erl -noshell -eval "io:format(\"~ts\", [code:lib_dir(erl_interface, lib)])." -s init stop)

ifdef NIF
	DEFINES += -DNIF
	CFLAGS += -I $(ERTS_INCLUDE_DIR) -I $(ERL_INTERFACE_INCLUDE_DIR)
	LDFLAGS += -L $(ERL_INTERFACE_LIB_DIR) -lei
endif

################################################################################
# Default Target
################################################################################

#all: build/libbetree.so build/libbetree.a
#dev: build/libbetree.so build/libbetree.a test valgrind
.DEFAULT_GOAL := build/libbetree.a
all: build/libbetree.a
dev: $(GENERATED_OBJECTS) build/libbetree.a test valgrind

dot:
	# dot -Tpng data/betree.dot -o data/betree.png
	dot -Tsvg data/betree.dot -o data/betree.svg

neato:
	neato -Tsvg data/betree.dot -o data/betree.svg

################################################################################
# Binaries
################################################################################

#build/libbetree.so: build $(OBJECTS)
	#$(CC) -shared $(OBJECTS) -o $@

build/libbetree.a: build $(OBJECTS)
	ar rcs $@ $(OBJECTS)

build:
	mkdir -p build

################################################################################
# Bison / Flex
################################################################################

%.c %.h: | %.l
	$(LEX) --header-file=$*.h -o $@ $<
%.c: | %.y
	$(YACC) $(YFLAGS) -o $@ $<

################################################################################
# BETree
################################################################################

src/%.o: src/%.c
	$(CC) $(DEFINES) $(CFLAGS) -c -o $@ $^ $(LDFLAGS)

################################################################################
# Tests
################################################################################

test: $(TEST_BINARIES)
	@bash ./tests/runtests.sh

build/tests:
	mkdir -p build/tests

#$(TEST_BINARIES): %: %.c build/tests build/libbetree.so
$(TEST_BINARIES): %: %.c build/tests build/libbetree.a
	$(CC) $(CFLAGS) -Isrc -o build/$@ $< build/libbetree.a $(LDFLAGS_TESTS)

clean:
	$(RM) $(OBJECTS)
	$(RM) -rf build

realclean: clean
	$(RM) $(INTERMEDIATES)

valgrind: $(TEST_BINARIES)
	$(VALGRIND) build/tests/betree_tests
	$(VALGRIND) build/tests/bound_tests
	$(VALGRIND) build/tests/change_boundaries_tests
	$(VALGRIND) build/tests/eq_expr_tests
	$(VALGRIND) build/tests/event_parser_tests
	$(VALGRIND) build/tests/memoize_tests
	$(VALGRIND) build/tests/parser_tests
	$(VALGRIND) build/tests/performance_tests
	$(VALGRIND) build/tests/printer_tests
	$(VALGRIND) build/tests/report_tests
	$(VALGRIND) build/tests/special_tests
	$(VALGRIND) build/tests/valid_tests
	#$(VALGRIND) build/tests/real_tests 1

callgrind:
	$(CALLGRIND) build/tests/real_tests 1

cachegrind:
	$(CACHEGRIND) build/tests/real_tests 1

massif:
	$(MASSIF) build/tests/real_tests 1

tidy:
	#$(TIDY) src/alloc.c -checks='*' -- -Isrc
	#$(TIDY) src/ast.c -checks='*' -- -Isrc
	#$(TIDY) src/ast_compare.c -checks='*' -- -Isrc
	#$(TIDY) src/betree.c -checks='*' -- -Isrc
	#$(TIDY) src/clone.c -checks='*' -- -Isrc
	#$(TIDY) src/config.c -checks='*' -- -Isrc
	#$(TIDY) src/debug.c -checks='*' -- -Isrc
	#$(TIDY) src/hashmap.c -checks='*' -- -Isrc
	#$(TIDY) src/helper.c -checks='*' -- -Isrc
	#$(TIDY) src/jsw_rbtree.c -checks='*' -- -Isrc
	#$(TIDY) src/map.c -checks='*' -- -Isrc
	#$(TIDY) src/memoize.c -checks='*' -- -Isrc
	#$(TIDY) src/printer.c -checks='*' -- -Isrc
	#$(TIDY) src/special.c -checks='*' -- -Isrc
	#$(TIDY) src/tree.c -checks='*' -- -Isrc
	#$(TIDY) src/utils.c -checks='*' -- -Isrc
	#$(TIDY) src/value.c -checks='*' -- -Isrc
	#$(TIDY) src/var.c -checks='*' -- -Isrc

build-test-benchmark: build/libbetree.a
	gcc -o testbenchmark tests/real_tests.c -Isrc -I/usr/include -I/usr/local/include  build/libbetree.a  $(LDFLAGS_TESTS)

.PHONY: clean realclean test valgrind
