Compel
======

An utility to execute code in foreign process address space. The
code should be compiled with compel flags and packed, then it can
be executed in other task's context.


Get compel ready
=======

Get the sources and run 

$ make
$ make install-devel

This will prepare the following

1. compel comman line tool,
2. libcompel.so library (put path to it into LD_LIBRARY_PATH variable,
   the compel tool is dynamically linked with it),
3. devel/ directory with headers and libs needed for further work.


Write parasite code
=======

The parasite code executes in an environment w/o glibc, thus you cannot
call the usual stdio/stdlib/etc. functions. Compel provides a set of
plugins for your convenience. Plugins get linked to parasite binary on the
pack stage (see below).

1. std

This plugins gets packed with your binary by default and provides standard
Linux system calls, strings functions and printf-like priting helper.

The std plugin API is in include/compel/plugin-std.h header.

2. fds

This one allows you to send and receive fds in parasite to/from the master
process.

The fds plugin API is in include/compel/plugin-fds.h header.

3. shmem

This one sets up shared memory between parasite and master.

The shmem plugin API is in include/compel/plugin-shmem.h header.

Execution of the parasite code starts with the function

   int main(void *arg_p, unsigned int arg_s);

that should be present in your code. The arg_p and arg_s is the binary
argument that will get delivered to parasite code by complel _start()
call (see below). Sometimes this binary argument can be treated as CLI
arguments argc/argv.


Compile the sources and pack the binary
=======

1. Take a program on C and compile it with compel flags

 $ gcc -c foo.c -o foo.o $(compel cflags) -I<compel-headers>

 To combine the foo.o out of many sources, they should all be
 linked with compel flags as well

 $ ld foo_1.o foo_2.o -o foo.o $(compel ldflags)

The compel-headers is devel/include/ after make install-devel.

2. Pack the binary. Packing would link the object file with
compel plugins (see below)

 $ compel pack foo.o -o foo.compel.o -L<compel-libs> [-l<lib>]

The compel-libs is devel/lib/compel/ after make install-devel.


The foo.compel.o is ready for remote execution (foo.o was not).


Execute the code remotely
=======

Using CLI like this

 $ compel run -f foo.compel.o -p $pid

Or, you can link with libcompel.so and use

 libcompel_exec()
 libcompel_exec_start()/libcompel_exec_end()

calls described in include/compel/compel.h header. The test/
directory contains several examples of how to launch parasites.

The library calls require binary argument that will get copied
into parasite context and passed to it via arg_p/arg_s pair.
When run from CLI the arguments are packed in argc/argv manner.


How to communicate to parasite code
=======
There are several ways for doing this.

1. If you run the parasite binary from CLI, the tail command line
arguments are passed into the parasite main() function.

  $ compel run -f foo.compel.o -p 123 -- arg1 arg2 arg3

In the main() common argc and argv are accessed using the

  argc = std_argc(arg_p);
  argv = std_argv(arg_p, argc);

calls. Then use argc and argv as you would use them in normal C
program run from shell.

2. If you run the parasite using library _start/_end calls, you
can pass file descriptors to parasite using fds plugin or setup
shmem between these two using shmem plugin.

See test/async_fds/ and test/async_shmem/ for code examples.

Warning
=======
The project is in early alfa stage, for testing purposes only!

Credits
=======

 - The Linux kernel code (https://www.kernel.org/)
 - Tejun Heo's ptrace-parasite (https://code.google.com/p/ptrace-parasite/)
 - Pavel Emelyanov
 - Andrey Vagin
 - Cyrill Gorcunov
