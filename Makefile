################################################################################
# Variables
################################################################################

# ERTS_INCLUDE_DIR ?= $(shell erl -noshell -s init stop -eval "io:format(\"~s/erts-~s/include/\", [code:root_dir(), erlang:system_info(version)]).")

CFLAGS := -g -std=gnu11 -D_GNU_SOURCE -Wall -Wextra -Wshadow -Wfloat-equal -Wundef -Wcast-align \
	-Wwrite-strings -Wunreachable-code -Wformat=2 -Wswitch-enum \
	-Wswitch-default -Winit-self -Wno-strict-aliasing \
	# -I$(ERTS_INCLUDE_DIR) \

LDFLAGS := -lm

# LDFLAGS := -shared

# UNAME_SYS := $(shell uname -s)
# ifeq ($(UNAME_SYS), Darwin)
# 	LDFLAGS += -flat_namespace -undefined suppress
# endif

LEX_SOURCES=$(wildcard src/*.l) 
LEX_OBJECTS=$(patsubst %.l,%.c,${LEX_SOURCES}) $(patsubst %.l,%.h,${LEX_SOURCES})

YACC_SOURCES=$(wildcard src/*.y) 
YACC_OBJECTS=$(patsubst %.y,%.c,${YACC_SOURCES}) $(patsubst %.y,%.h,${YACC_SOURCES})

SOURCES=$(wildcard src/*.c)
OBJECTS=$(patsubst %.c,%.o,${SOURCES}) $(patsubst %.l,%.o,${LEX_SOURCES}) $(patsubst %.y,%.o,${YACC_SOURCES})
LIB_SOURCES=$(filter-out src/erlang.c,${SOURCES})
LIB_OBJECTS=$(filter-out src/erlang.o,${OBJECTS})
TEST_SOURCES=$(wildcard tests/*_tests.c)
TEST_OBJECTS=$(patsubst %.c,%,${TEST_SOURCES})
TOOL_SOURCES=$(wildcard tools/*.c)
TOOL_OBJECTS=$(patsubst %.c,%,${TOOL_SOURCES})

LEX?=flex
YACC?=bison
YFLAGS?=-dv

VALGRIND=valgrind --leak-check=full --track-origins=yes --suppressions=valgrind.supp
CALLGRIND=valgrind --tool=callgrind

################################################################################
# Default Target
################################################################################

# all: build/betree.a build/betree.so $(OBJECTS) tool test dot
# all: build/betree.a build/betree.so $(OBJECTS) tool test
all: build/betree.a tool test

dot:
	# dot -Tpng betree.dot -o betree.png
	dot -Tsvg betree.dot -o betree.svg

################################################################################
# Binaries
################################################################################

build/betree.a: build $(LIB_OBJECTS)
	rm -f build/betree.a
	ar rcs $@ $(LIB_OBJECTS)
	ranlib $@

# src/erlang.c: $(LEX_OBJECTS)

# src/erlang.o: src/erlang.c
# 	$(CC) $(CFLAGS) -c -o $@ $^

# build/betree.so: $(OBJECTS)
# 	# -rdynamic before -o
# 	$(CC) $(LDFLAGS) -o $@ src/erlang.o build/betree.a

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

src.betree.o: src/betree.c
	$(CC) $(CFLAGS) -c -o $@ $^ $(LDFLAGS)

################################################################################
# Tools
################################################################################

tool: $(TOOL_OBJECTS)

build/tools:
	mkdir -p build/tools

$(TOOL_OBJECTS): %: %.c build/tools
	$(CC) $(CFLAGS) -Isrc -o build/$@ $< build/betree.a $(LDFLAGS)

################################################################################
# Tests
################################################################################

.PHONY: test
test: $(TEST_OBJECTS) build/tests/betree_tests
	@sh ./tests/runtests.sh

build/tests:
	mkdir -p build/tests

$(TEST_OBJECTS): %: %.c build/tests build/betree.a
	$(CC) $(CFLAGS) -Isrc -o build/$@ $< build/betree.a $(LDFLAGS)

clean:
	rm -rf build/betree.so build/betree.a $(OBJECTS) $(LEX_OBJECTS) $(YACC_OBJECTS)
	rm -rf build/tests

valgrind:
	$(VALGRIND) build/tests/betree_tests
	$(VALGRIND) build/tests/parser_tests
	$(VALGRIND) build/tests/event_parser_tests
	#$(VALGRIND) build/tests/performance_tests
	#$(VALGRIND) build/tools/gen_expr

callgrind:
	$(CALLGRIND) build/tests/performance_tests
