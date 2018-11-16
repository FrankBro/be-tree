################################################################################
# Variables
################################################################################

CFLAGS := -O3 -std=gnu11 -D_GNU_SOURCE -Wall -Wextra -Wshadow -Wfloat-equal -Wundef -Wcast-align \
	-Wwrite-strings -Wunreachable-code -Wformat=2 -Wswitch-enum \
	-Wswitch-default -Winit-self -Wno-strict-aliasing

LDFLAGS := -lm -fPIC 
LDFLAGS_TESTS := $(LDFLAGS) -lgsl -lgslcblas 

LEX_SOURCES=$(wildcard src/*.l) 
LEX_OBJECTS=$(patsubst %.l,%.c,${LEX_SOURCES}) $(patsubst %.l,%.h,${LEX_SOURCES})

YACC_SOURCES=$(wildcard src/*.y) 
YACC_OBJECTS=$(patsubst %.y,%.c,${YACC_SOURCES}) $(patsubst %.y,%.h,${YACC_SOURCES})

SOURCES=$(filter-out ${YACC_OBJECTS},$(filter-out ${LEX_OBJECTS},$(wildcard src/*.c)))
OBJECTS=$(patsubst %.c,%.o,${SOURCES}) $(patsubst %.l,%.o,${LEX_SOURCES}) $(patsubst %.y,%.o,${YACC_SOURCES})
TEST_SOURCES=$(wildcard tests/*_tests.c)
TEST_OBJECTS=$(patsubst %.c,%,${TEST_SOURCES})

LEX?=flex
YACC?=bison
YFLAGS?=-dv

VALGRIND=valgrind --tool=memcheck --leak-check=full --track-origins=yes --suppressions=valgrind.supp
CALLGRIND=valgrind --tool=callgrind --instr-atstart=no
CACHEGRIND=valgrind --tool=cachegrind
MASSIF=valgrind --tool=massif

################################################################################
# Default Target
################################################################################

# all: build/betree.a build/betree.so $(OBJECTS) tool test dot
# all: build/betree.a build/betree.so $(OBJECTS) tool test
all: build/libbetree.so 
dev: build/libbetree.so test

dot:
	# dot -Tpng betree.dot -o betree.png
	dot -Tsvg betree.dot -o betree.svg

neato:
	neato -Tsvg betree.dot -o betree.svg

################################################################################
# Binaries
################################################################################

build/libbetree.so: build $(OBJECTS)
	$(CC) -shared $(OBJECTS) -o $@

build:
	mkdir -p build

################################################################################
# Bison / Flex
################################################################################

src/lexer.c: src/parser.c
	$(LEX) --header-file=src/lexer.h -o $@ src/lexer.l

src/parser.c: src/parser.y
	mkdir -p build/bison
	$(YACC) $(YFLAGS) -o $@ $^

src/event_lexer.c: src/event_parser.c
	$(LEX) --header-file=src/event_lexer.h -o $@ src/event_lexer.l

src/event_parser.c: src/event_parser.y
	mkdir -p build/bison
	$(YACC) $(YFLAGS) -o $@ $^

################################################################################
# BETree
################################################################################

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $^ $(LDFLAGS)

################################################################################
# Tests
################################################################################

.PHONY: clean test
test: $(TEST_OBJECTS)
	@bash ./tests/runtests.sh

build/tests:
	mkdir -p build/tests

$(TEST_OBJECTS): %: %.c build/tests build/libbetree.so
	$(CC) $(CFLAGS) -Isrc -o build/$@ $< build/libbetree.so $(LDFLAGS_TESTS)

clean:
	rm -rf build/libbetree.so $(OBJECTS) $(LEX_OBJECTS) $(YACC_OBJECTS)
	rm -rf build

valgrind:
	$(VALGRIND) build/tests/betree_tests
	$(VALGRIND) build/tests/bound_tests
	$(VALGRIND) build/tests/eq_expr_tests
	$(VALGRIND) build/tests/event_parser_tests
	$(VALGRIND) build/tests/memoize_tests
	$(VALGRIND) build/tests/parser_tests
	$(VALGRIND) build/tests/performance_tests
	$(VALGRIND) build/tests/printer_tests
	$(VALGRIND) build/tests/report_tests
	$(VALGRIND) build/tests/special_tests

callgrind:
	$(CALLGRIND) build/tests/real_tests 1

cachegrind:
	$(CACHEGRIND) build/tests/real_tests 1

massif:
	$(MASSIF) build/tests/real_tests

