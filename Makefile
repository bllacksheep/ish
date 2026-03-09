PATHBD := build
PATHBN := bin
PATHUN := unity/src
PATHSC := src
CFLAGS := -I$(PATHUN) -I$(PATHSC) -ggdb3 -O0
CC := gcc
EXE := i.sh
PATHS := $(PATHBD) $(PATHBN)

# build runner names
TESTS := $(wildcard tests/test_*.c)
RUNNERS := $(patsubst tests/%.c,$(PATHBN)/%,$(TESTS))

# build obj names
SRCS := $(wildcard $(PATHSC)/*.c)
OBJ := $(patsubst $(PATHSC)/%.c,$(PATHBD)/%.o,$(SRCS))

.PHONY: setup all clean check

all: $(PATHBN)/$(EXE)

check: $(RUNNERS)
	@for runner in $(RUNNERS); do ./$$runner; done

# test_x -> x.c + test_x.c unity.c
$(PATHBN)/test_%: tests/test_%.c $(PATHUN)/unity.c
	@mkdir -p $(PATHBN)
	$(CC) $(CFLAGS) -DTEST $(PATHSC)/$(subst test_,,$(notdir $@)).c $^ -o $@

$(PATHBN)/$(EXE): $(OBJ)
	@mkdir -p $(PATHBN)
	$(CC) $(CFLAGS) $^ -o $@

build/%.o: $(PATHSC)/%.c
	@mkdir -p $(PATHBD)
	$(CC) $(CFLAGS) -c $< -o $@

setup:
	mkdir -p $(PATHS)

clean:
	rm -rf $(PATHBN)/* $(PATHBD)/*.o
