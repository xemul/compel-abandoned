#
# These flags should be used to build the PIE blob
# They are used for plugins build and are printed by
# the 'compel cfglags' command, which should be used
# when building the obj file to be 'pack'-ed
#
PIE_BUILD_FLAGS = -fpie -Wstrict-prototypes -Wa,--noexecstack -fno-jump-tables -nostdlib -fomit-frame-pointer
