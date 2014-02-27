cflags-y	+= -fpie -Wstrict-prototypes -Wa,--noexecstack
cflags-y	+= -fno-jump-tables -nostdlib -fomit-frame-pointer

#
# For user api provided by plugins
cflags-y	+= -iquote plugins/include
