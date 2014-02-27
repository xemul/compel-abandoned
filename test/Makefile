ifneq ($(MAKECMDGOALS),clean)
compel-cflags := $(shell ./compel cflags)
compel-cflags += -iquote devel/include/
endif

clean:
	$(Q) $(RM) test/sleeping
	$(Q) $(RM) test/plugin/simple.o
	$(Q) $(RM) test/simple.blob.o

test/plugin/simple.o: test/plugin/simple.c
	$(E) "  GEN     " $@
	$(Q) $(CC) $(compel-cflags) -c $^ -o $@

test/sleeping: test/sleeping.c
	$(E) "  GEN     " $@
	$(Q) $(CC) -o $@ $^

test/simple.blob.o: test/plugin/simple.o
	$(E) "  GEN     " $@
	$(Q) $(SH) -c "./compel pack $^ -lfds -L./devel/lib/compel -o $@"

all: test/sleeping test/simple.blob.o
	$(E) "  Run test with 'sudo bash test/run.sh'"
