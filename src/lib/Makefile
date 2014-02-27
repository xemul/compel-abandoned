lib-so			+= libcompel

obj-y			+= parasite.o
obj-y			+= ptrace.o
obj-y			+= compel.o
obj-y			+= argv.o
obj-y			+= log.o
obj-y			+= fds.o
obj-y			+= shmem.o

obj-y			+= arch/$(ARCH)/ptrace.o
obj-y			+= arch/$(ARCH)/parasite.o
obj-y			+= arch/$(ARCH)/elf.o

cflags-y		+= -fPIC -Wa,--noexecstack -fno-stack-protector
cflags-y		+= -iquote plugins/std/arch/$(ARCH)/include
cflags-y		+= -iquote plugins/include
cflags-so		+= -rdynamic

.SECONDARY:

ifneq ($(MAKECMDGOALS),clean)
incdeps := y
endif
