include scripts/pie-build.mak
cflags-y	+= $(PIE_BUILD_FLAGS)
# For user api provided by plugins
cflags-y	+= -iquote plugins/include
