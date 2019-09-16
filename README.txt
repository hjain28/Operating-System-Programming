SEGHandler class installs SEGFAULT, SIGABRT and SIGFPE signal handlers to print
a nice stack trace and (if requested) generate a core dump.
In SEGHandler's constructor, signal handlers
are installed through `sigaction()`. If your program encounters a segmentation
fault, the call stack is unwinded with `backtrace()`, converted into
function names with line numbers via `addr2line` (`fork()` + `execlp()`).
Addresses from shared libraries are also converted thanks to dladdr().
All C++ symbols are demangled. Printed stack trace includes the faulty
thread id obtained with `pthread_self()` and each line contains the process
id to distinguish several stack traces printed by different processes at
the same time.

The code works primarily on Linux and tested on x86_64 and ARM with glibc.
Besides, there is a partial support of MacOSX (addr2line -> atos is not implemented).

Since `backtrace()` uses `malloc()` and the heap my be corrupted by the time it is called,
`malloc()` and `free()` have to be overridden. On Linux, it is done via a simple symbol
overload; on MacOSX, `malloc_default_zone()` approach is taken because overloading does
not work out of the box.

Example
=======

test.cc:
~~~~{.cc}
#include "Seg_handler.h"

int main() {
  Debug::SEGHandler dh;
  int* p = NULL;
  *p = 0;
  return 0;
}
~~~~

~~~~{.sh}
g++ -g Seg_handler.cc test.cc -ldl -o test
./test
~~~~


