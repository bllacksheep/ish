PATHBD := build
PATHBN := bin
PATHUN := unity/src
PATHSC := src
PATHRT := build/results
CFLAGS := -I$(PATHUN) -I$(PATHSC) -ggdb3 -O0
CC := gcc
EXE := i.sh
PATHS := $(PATHBD) $(PATHBN) $(PATHRT)

TESTS := $(wildcard tests/test_*.c)
RUNNERS := $(patsubst tests/%.c,$(PATHBN)/%,$(TESTS))

.PHONY: setup all clean check

all: $(PATHBN)/$(EXE)

check: $(RUNNERS)
	@for runner in $(RUNNERS); do ./$$runner; done

$(PATHBN)/$(EXE): src/main.c src/ish.h
	@mkdir -p $(PATHBN)
	$(CC) $(CFLAGS) $< -o $@

$(PATHBN)/test_%: tests/test_%.c $(PATHUN)/unity.c
	@mkdir -p $(PATHBN)
	$(CC) $(CFLAGS) -DTEST src/$(subst test_,,$(notdir $@)).c $^ -o $@

setup:
	mkdir -p $(PATHS)

clean:
	rm -rf $(PATHBN)/* $(PATHBD)/*.o $(PATHRT)/*
