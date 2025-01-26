TESTS = $(patsubst %.c,%.o,$(wildcard test/*.c))
CFLAGS = -I. -Og -ggdb

.PHONY: test

%.o: %.c arena.h
	$(CC) $(CFLAGS) -o $@ $<

test: $(TESTS)
	echo $^ | xargs -Ixx bash -c "xx"
