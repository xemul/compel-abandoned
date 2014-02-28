ifeq ($(strip $(V)),)
	E = @echo
	Q = @
else
	E = @\#
	Q =
endif

COMP=../compel
COMP_DEV=../devel/
COMP_LIB=../src/lib/
COMP_PLUGIN=$(COMP_DEV)/lib/compel/
COMP_INC=$(COMP_DEV)/include/
CC=gcc

all: piggie parasite.co

test: all
	./piggie ./parasite.co > piggie.out

.PHONY: all

parasite.co: parasite.o
	$(Q) $(COMP) pack $< $(shell cat $(OBJ)/plugins) -o $@ -L $(COMP_PLUGIN)

parasite.o: $(OBJ)/parasite.c
	$(Q) $(CC) -c -o $@ $< $(shell $(COMP) cflags) -I$(COMP_INC)

piggie: $(OBJ)/piggie.c
	$(Q) $(CC) -o $@ $< -I$(COMP_INC) -lcompel -L$(COMP_LIB)

clean:
	rm -f parasite.o
	rm -f parasite.co
	rm -f piggie
