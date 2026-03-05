CC := gcc
CFLAGS := -ggdb3 -O0
PATHBD := build
PATHBN := bin
EXE := i.sh
PATHS := $(PATHBD) $(PATHBN)

.PHONY: setup all clean

all: $(PATHBN)/$(EXE)

$(PATHBN)/$(EXE): main.c
	$(CC) $(CFLAGS) main.c -o $(PATHBN)/$(EXE)

setup:
	mkdir -p $(PATHS)

clean:
	rm -rf $(PATHBN)/* $(PATHBD)/*
