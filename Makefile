MAKEFLAGS 	:= -r -R --no-print-directory

ifeq ($(strip $(V)),)
	E = @echo
	Q = @
else
	E = @\#
	Q =
endif

FIND		:= find
CSCOPE		:= cscope
TAGS		:= ctags
RM		:= rm -f
CP		:= cp
LD		:= ld
CC		:= gcc
CD		:= cd
ECHO		:= echo
NM		:= nm
AWK		:= awk
SH		:= bash
MAKE		:= make
OBJCOPY		:= objcopy
MKDIR		:= mkdir
LN		:= ln
ESED		:= esed
SED		:= sed
CAT		:= cat

#
# Fetch ARCH from the uname if not yet set
#
ARCH ?= $(shell uname -m | sed		\
		-e s/i.86/i386/		\
		-e s/sun4u/sparc64/	\
		-e s/arm.*/arm/		\
		-e s/sa110/arm/		\
		-e s/s390x/s390/	\
		-e s/parisc64/parisc/	\
		-e s/ppc.*/powerpc/	\
		-e s/mips.*/mips/	\
		-e s/sh[234].*/sh/)

ifeq ($(ARCH),x86_64)
	ARCH         := x86
	DEFINES      := -DCONFIG_X86_64 -DARCH="\"$(ARCH)\""
	LDARCH       := i386:x86-64
endif

ifneq ($(ARCH),x86)
$(error "The architecture $(ARCH) isn't supported"))
endif

cflags-y	+= -iquote src/include
cflags-y	+= -fno-strict-aliasing
cflags-y	+= -I/usr/include
export cflags-y

VERSION_MAJOR		:= 0
VERSION_MINOR		:= 1
VERSION_SUBLEVEL	:= 0
VERSION_EXTRA		:=
VERSION_NAME		:=

export VERSION_MAJOR VERSION_MINOR VERSION_SUBLEVEL VERSION_EXTRA VERSION_NAME

include scripts/Makefile.version
include scripts/Makefile.config

LIBS		:= -lrt

DEFINES		+= -D_FILE_OFFSET_BITS=64
DEFINES		+= -D_GNU_SOURCE

WARNINGS	:= -Wall

ifneq ($(WERROR),0)
	WARNINGS += -Werror
endif

ifeq ($(DEBUG),1)
	DEFINES += -DCR_DEBUG
	CFLAGS	+= -O0 -ggdb3
else
	CFLAGS	+= -O2
endif

CFLAGS		+= $(WARNINGS) $(DEFINES)

export E Q CC ECHO MAKE CFLAGS LIBS ARCH DEFINES MAKEFLAGS
export SH RM OBJCOPY LDARCH LD CP MKDIR CD LN
export ESED SED CAT

include scripts/Makefile.rules

build := -r -R --no-print-directory -f scripts/Makefile.build makefile=Makefile obj
run := -r -R --no-print-directory

PROGRAM		:= compel

.PHONY: all clean tags docs

cflags-y += -iquote src/include
cflags-y += -iquote src/lib/arch/$(ARCH)/include
export cflags-y

#
# First order targets, usually pregenerated
EARLY-GEN := $(VERSION_HEADER) config

include Makefile.headers
include Makefile.plugins
include Makefile.devel

#
# The library command line tool is using
src/lib/%: $(EARLY-GEN)
	$(Q) $(MAKE) $(build)=src/lib $@
src/lib/built-in.o: $(EARLY-GEN)
	$(Q) $(MAKE) $(build)=src/lib all

#
# Source command line tool
src/%: $(EARLY-GEN)
	$(Q) $(MAKE) $(build)=src $@
src/built-in.o: src $(EARLY-GEN)
	$(Q) $(MAKE) $(build)=src all

$(all-plugins): | plugins-headers-prepare
$(PROGRAM): | src/lib/libcompel.so $(all-plugins)

$(PROGRAM): src/built-in.o
	$(E) "  LINK    " $@
	$(Q) $(CC) $(CFLAGS) $^ $(LIBS) -Lsrc/lib -lcompel -ldl -o $@

all: $(PROGRAM)

docs:
	$(Q) $(MAKE) -s -C Documentation all

tags:
	$(E) "  GEN     " $@
	$(Q) $(RM) tags
	$(Q) $(FIND) -L . -name '*.[hcS]' ! -path './.*' -print | xargs ctags -a

clean: clean-plugins clean-plugins-headers-prepared
	$(Q) $(MAKE) $(build)=src clean
	$(Q) $(MAKE) $(build)=src/lib clean
	$(Q) $(MAKE) -s -C Documentation clean
	$(Q) $(MAKE) -s -f test/Makefile clean
	$(Q) $(RM) $(PROGRAM)
	$(Q) $(RM) $(CONFIG)
	$(Q) $(RM) $(VERSION_HEADER)

#
# Note we install development files into default
# directory here, so clean it up manually then!
test: | $(all-plugins)
test: | install-devel
test: $(PROGRAM)
	$(Q) $(MAKE) -f test/Makefile all

.PHONY: test

.DEFAULT_GOAL := all
