CFLAGS = -g -std=c11 -Wall -Wextra -Wshadow -Wfloat-equal -Wundef -Wcast-align \
	-Wwrite-strings -Wunreachable-code -Wformat=2 -Wswitch-enum \
	-Wswitch-default -Winit-self -Wno-strict-aliasing

OBJECTS = ast.o betree.o

all: test

betree: $(OBJECTS)
	cc $(CFLAGS) -o betree $(OBJECTS)

betree.o: betree.c
	cc $(CFLAGS) -c betree.c

ast.o: ast.c
	cc $(CFLAGS) -c ast.c

TEST_OBJECTS = betree_tests.o

test: $(OBJECTS) $(TEST_OBJECTS)
	cc $(CFLAGS) -o betree_tests $(OBJECTS) $(TEST_OBJECTS)
	@sh ./runtests.sh

betree_tests.o: betree_tests.c
	cc $(CFLAGS) -c betree_tests.c

.PHONY: clean
clean:
	rm $(OBJECTS) $(TEST_OBJECTS)
