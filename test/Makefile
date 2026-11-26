CC ?= gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -I../main

test: test_riego_logic
	./test_riego_logic

test_riego_logic: test_riego_logic.c ../main/riego_logic.c ../main/riego_logic.h
	$(CC) $(CFLAGS) -o $@ test_riego_logic.c ../main/riego_logic.c

clean:
	rm -f test_riego_logic

.PHONY: test clean
