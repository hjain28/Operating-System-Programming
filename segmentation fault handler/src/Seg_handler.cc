

#include "Seg_handler.h"
#include <assert.h>
#include <execinfo.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>
#ifdef __APPLE__
#include <malloc/malloc.h>
#include <sys/mman.h>
#endif

#define INLINE __attribute__((always_inline)) inline

namespace Debug {
namespace Safe {
  INLINE void print(const char *msg, size_t len = 0);
}  // namespace Safe
}  // namespace Debug

extern "C" {

void* __malloc_impl(size_t size) {
    char* malloc_buffer =
        Debug::SEGHandler::memory_ + Debug::SEGHandler::kNeededMemory - 512;
    if (size > 512U) {
      const char* msg = "malloc() replacement function should not return "
          "a memory block larger than 512 bytes\n";
      Debug::SEGHandler::print(msg, strlen(msg) + 1);
      _Exit(EXIT_FAILURE);
    }
    return malloc_buffer;
}

#ifdef __linux__
void* malloc(size_t size) throw() {
  if (!Debug::SEGHandler::heap_trap_active_) {
    if (!Debug::SEGHandler::malloc_) {
      Debug::SEGHandler::malloc_ = dlsym(RTLD_NEXT, "malloc");
    }
    return ((void*(*)(size_t))Debug::SEGHandler::malloc_)(size);
  }
  return __malloc_impl(size);
}

void free(void* ptr) throw() {
  if (!Debug::SEGHandler::heap_trap_active_) {
    if (!Debug::SEGHandler::free_) {
      Debug::SEGHandler::free_ = dlsym(RTLD_NEXT, "free");
    }
    ((void(*)(void*))Debug::SEGHandler::free_)(ptr);
  }
  // no-op
}
#elif defined(__APPLE__)
void* __malloc_zone(struct _malloc_zone_t* zone, size_t size) {
  if (!Debug::SEGHandler::heap_trap_active_) {
     return ((void*(*)(struct _malloc_zone_t*, size_t))
         Debug::SEGHandler::malloc_)(zone, size);
  }
  return __malloc_impl(size);
}

void __free_zone(struct _malloc_zone_t* zone, void *ptr) {
  if (!Debug::SEGHandler::heap_trap_active_) {
    return ((void(*)(struct _malloc_zone_t*, void*))
         Debug::SEGHandler::free_)(zone, ptr);
  }
  // no-op
}
#endif // #ifdef __linux__
}  // extern "C"

#ifdef __APPLE__
static void SetMallocZone(malloc_zone_t* zone, void* malloc, void* free,
                          void** zone_malloc = NULL, void** zone_free = NULL) {
  if (zone_malloc) {
    *zone_malloc = reinterpret_cast<void*>(zone->malloc);
  }
  if (zone_free) {
    *zone_free = reinterpret_cast<void*>(zone->free);
  }
  mprotect(zone, sizeof(*zone), PROT_READ | PROT_WRITE);
  zone->malloc = (void*(*)(struct _malloc_zone_t*, size_t))malloc;
  zone->free = (void(*)(struct _malloc_zone_t*, void*))free;
  mprotect(zone, sizeof(*zone), PROT_READ);
}
#endif

#pragma GCC poison malloc realloc free backtrace_symbols \
  printf fprintf sprintf snprintf scanf sscanf  // NOLINT(runtime/printf)

#define checked(x) do { if ((x) <= 0) _Exit(EXIT_FAILURE); } while (false)

namespace Debug {

/// @brief This namespace contains some basic supplements
/// of the needed libc functions which potentially use heap.
namespace Safe {
  /// @brief Converts an integer to a preallocated string.
  /// @pre base must be less than or equal to 16.
  INLINE char *itoa(int val, char* memory, int base = 10) {
    char* res = memory;
    if (val == 0) {
      res[0] = '0';
      res[1] = '\0';
      return res;
    }
    const int res_max_length = 32;
    int i;
    bool negative = val < 0;
    res[res_max_length - 1] = 0;
    for (i = res_max_length - 2; val != 0 && i != 0; i--, val /= base) {
      res[i] = "0123456789ABCDEF"[val % base];
    }
    if (negative) {
      res[i--] = '-';
    }
    return &res[i + 1];
  }

  /// @brief Converts an unsigned integer to a preallocated string.
  /// @pre base must be less than or equal to 16.
  INLINE char *utoa(uint64_t val, char* memory, int base = 10) {
    char* res = memory;
    if (val == 0) {
      res[0] = '0';
      res[1] = '\0';
      return res;
    }
    const int res_max_length = 32;
    int i;
    res[res_max_length - 1] = 0;
    for (i = res_max_length - 2; val != 0 && i != 0; i--, val /= base) {
      res[i] = "0123456789abcdef"[val % base];
    }
    return &res[i + 1];
  }

  /// @brief Converts a pointer to a preallocated string.
  INLINE char *ptoa(const void *val, char* memory) {
    char* buf = utoa(reinterpret_cast<uint64_t>(val), memory + 32, 16);
    char* result = memory;  // 32
    strcpy(result + 2, buf);  // NOLINT(runtime/printf
    result[0] = '0';
    result[1] = 'x';
    return result;
  }

  ssize_t write2stderr(const char* msg, size_t len) {
    return write(STDERR_FILENO, msg, len);
  }
}  // namespace Safe

const size_t SEGHandler::kNeededMemory = 16384;
bool SEGHandler::generate_core_dump_ = true;
bool SEGHandler::cleanup_ = true;
#ifdef QUICK_EXIT
bool SEGHandler::quick_exit_ = false;
#endif
int SEGHandler::frames_count_ = 16;
bool SEGHandler::cut_common_path_root_ = true;
bool SEGHandler::cut_relative_paths_ = true;
bool SEGHandler::append_pid_ = false;
bool SEGHandler::color_output_ = true;
bool SEGHandler::thread_safe_ = true;
char* SEGHandler::memory_ = NULL;
void* SEGHandler::malloc_ = NULL;
void* SEGHandler::free_ = NULL;
bool SEGHandler::heap_trap_active_ = false;
SEGHandler::OutputCallback SEGHandler::output_callback_ = Safe::write2stderr;

typedef void (*sa_sigaction_handler) (int, siginfo_t *, void *);

SEGHandler::SEGHandler(bool altstack) {
  if (memory_ == NULL) {
    memory_ = new char[kNeededMemory + (altstack? MINSIGSTKSZ : 0)];
  }
  if (altstack) {
    stack_t altstack;
    altstack.ss_sp = memory_ + kNeededMemory;
    altstack.ss_size = MINSIGSTKSZ;
    altstack.ss_flags = 0;
    if (sigaltstack(&altstack, NULL) < 0) {
      perror("SEGHandler - sigaltstack()");
    }
  }
  struct sigaction sa;
  sa.sa_sigaction = (sa_sigaction_handler)HandleSignal;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART | SA_SIGINFO | (altstack? SA_ONSTACK : 0);
  if (sigaction(SIGSEGV, &sa, NULL) < 0) {
    perror("SEGHandler - sigaction(SIGSEGV)");
  }
  if (sigaction(SIGABRT, &sa, NULL) < 0) {
    perror("SEGHandler - sigaction(SIGABBRT)");
  }
  if (sigaction(SIGFPE, &sa, NULL) < 0) {
    perror("SEGHandler - sigaction(SIGFPE)");
  }
  #ifdef __APPLE__
  malloc_zone_t* zone = malloc_default_zone();
  if (!zone) {
    print("Failed to override malloc() and free()");
    return;
  }
  // Override malloc() and free()
  SetMallocZone(zone, reinterpret_cast<void*>(__malloc_zone),
                reinterpret_cast<void*>(__free_zone), &malloc_, &free_);
  #endif
}

SEGHandler::~SEGHandler() {
  // Disable alternative signal handler stack
  stack_t altstack;
  altstack.ss_sp = NULL;
  altstack.ss_size = 0;
  altstack.ss_flags = SS_DISABLE;
  sigaltstack(&altstack, NULL);

  struct sigaction sa;

  sigaction(SIGSEGV, NULL, &sa);
  sa.sa_handler = SIG_DFL;
  sigaction(SIGSEGV, &sa, NULL);

  sigaction(SIGABRT, NULL, &sa);
  sa.sa_handler = SIG_DFL;
  sigaction(SIGABRT, &sa, NULL);

  sigaction(SIGFPE, NULL, &sa);
  sa.sa_handler = SIG_DFL;
  sigaction(SIGFPE, &sa, NULL);
  delete[] memory_;

  #ifdef __APPLE__
  malloc_zone_t* zone = malloc_default_zone();
  SetMallocZone(zone, malloc_, free_);
  #endif
}

void SEGHandler::print(const char* msg, size_t len) {
  if (len > 0) {
    checked(output_callback_(msg, len));
  } else {
    checked(output_callback_(msg, strlen(msg)));
  }
}

bool SEGHandler::generate_core_dump() const {
  return generate_core_dump_;
}

void SEGHandler::set_generate_core_dump(bool value) {
  generate_core_dump_ = value;
}

bool SEGHandler::cleanup() const {
  return cleanup_;
}

void SEGHandler::set_cleanup(bool value) {
  cleanup_ = value;
}

#ifdef QUICK_EXIT
bool SEGHandler::quick_exit() const {
  return quick_exit_;
}

void SEGHandler::set_quick_exit(bool value) {
  quick_exit_ = value;
}
#endif

int SEGHandler::frames_count() const {
  return frames_count_;
}

void SEGHandler::set_frames_count(int value) {
  assert(value > 0 && value <= 100);
  frames_count_ = value;
}

bool SEGHandler::cut_common_path_root() const {
  return cut_common_path_root_;
}

void SEGHandler::set_cut_common_path_root(bool value) {
  cut_common_path_root_ = value;
}

bool SEGHandler::cut_relative_paths() const {
  return cut_relative_paths_;
}

void SEGHandler::set_cut_relative_paths(bool value) {
  cut_relative_paths_ = value;
}

bool SEGHandler::append_pid() const {
  return append_pid_;
}

void SEGHandler::set_append_pid(bool value) {
  append_pid_ = value;
}

bool SEGHandler::color_output() const {
  return color_output_;
}

void SEGHandler::set_color_output(bool value) {
  color_output_ = value;
}

bool SEGHandler::thread_safe() const {
  return thread_safe_;
}

void SEGHandler::set_thread_safe(bool value) {
  thread_safe_ = value;
}

SEGHandler::OutputCallback SEGHandler::output_callback() const {
  return output_callback_;
}

void SEGHandler::set_output_callback(SEGHandler::OutputCallback value) {
  output_callback_ = value;
}

INLINE static void safe_abort() {
  struct sigaction sa;
  sigaction(SIGABRT, NULL, &sa);
  sa.sa_handler = SIG_DFL;
  kill(getppid(), SIGCONT);
  sigaction(SIGABRT, &sa, NULL);
  abort();
}

/// @brief Invokes addr2line utility to determine the function name
/// and the line information from an address in the code segment.
static char *addr2line(const char *image, void *addr, bool color_output,
                       char** memory) {
  int pipefd[2];
  if (pipe(pipefd) != 0) {
    safe_abort();
  }
  pid_t pid = fork();
  if (pid == 0) {
    close(pipefd[0]);
    dup2(pipefd[1], STDOUT_FILENO);
    dup2(pipefd[1], STDERR_FILENO);
    if (execlp("addr2line", "addr2line",
               Safe::ptoa(addr, *memory), "-f", "-C", "-e", image,
               reinterpret_cast<void*>(NULL)) == -1) {
      safe_abort();
    }
  }

  close(pipefd[1]);
  const int line_max_length = 4096;
  char* line = *memory;
  *memory += line_max_length;
  ssize_t len = read(pipefd[0], line, line_max_length);
  close(pipefd[0]);
  if (len == 0) {
    safe_abort();
  }
  line[len] = 0;

  if (waitpid(pid, NULL, 0) != pid) {
    safe_abort();
  }
  if (line[0] == '?') {
    char* straddr = Safe::ptoa(addr, *memory);
    if (color_output) {
      strcpy(line, "\033[32;1m");  // NOLINT(runtime/printf)
    }
    strcat(line, straddr);  // NOLINT(runtime/printf)
    if (color_output) {
      strcat(line, "\033[0m");  // NOLINT(runtime/printf)
    }
    strcat(line, " at ");  // NOLINT(runtime/printf)
    strcat(line, image);  // NOLINT(runtime/printf)
    strcat(line, " ");  // NOLINT(runtime/printf)
  } else {
    if (*(strstr(line, "\n") + 1) == '?') {
      char* straddr = Safe::ptoa(addr, *memory);
      strcpy(strstr(line, "\n") + 1, image);  // NOLINT(runtime/printf)
      strcat(line, ":");  // NOLINT(runtime/printf)
      strcat(line, straddr);  // NOLINT(runtime/printf)
      strcat(line, "\n");  // NOLINT(runtime/printf)
    }
  }
  return line;
}

#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

void SEGHandler::HandleSignal(int sig, void * /* info */, void *secret) {
  // Stop all other running threads by forking
  pid_t forkedPid = fork();
  if (forkedPid != 0) {
    int status;
    if (thread_safe_) {
      // Freeze the original process, until it's child prints the stack trace
      kill(getpid(), SIGSTOP);
      // Wait for the child without blocking and exit as soon as possible,
      // so that no zombies are left.
      waitpid(forkedPid, &status, WNOHANG);
    } else {
      // Wait for the child, blocking only the current thread.
      // All other threads will continue to run, potentially crashing the parent.
      waitpid(forkedPid, &status, 0);
    }
#ifdef QUICK_EXIT
    if (quick_exit_) {
      ::quick_exit(EXIT_FAILURE);
    }
#endif
    if (generate_core_dump_) {
      struct sigaction sa;
      sigaction(SIGABRT, NULL, &sa);
      sa.sa_handler = SIG_DFL;
      sigaction(SIGABRT, &sa, NULL);
      abort();
    } else {
      if (cleanup_) {
        exit(EXIT_FAILURE);
      } else {
        _Exit(EXIT_FAILURE);
      }
    }
  }

  ucontext_t *uc = reinterpret_cast<ucontext_t *>(secret);

  if (dup2(STDERR_FILENO, STDOUT_FILENO) == -1) {  // redirect stdout to stderr
    print("Failed to redirect stdout to stderr\n");
  }
  char* memory = memory_;
  {
    char* msg = memory;
    const int msg_max_length = 128;
    if (color_output_) {
      // \033[31;1mSegmentation fault\033[0m \033[33;1m(%i)\033[0m\n
      strcpy(msg, "\033[31;1m");  // NOLINT(runtime/printf)
    } else {
      msg[0] = '\0';
    }
    switch (sig) {
      case SIGSEGV:
        strcat(msg, "Segmentation fault");  // NOLINT(runtime/printf)
        break;
      case SIGABRT:
        strcat(msg, "Aborted");  // NOLINT(runtime/printf)
        break;
      case SIGFPE:
        strcat(msg, "Floating point exception");  // NOLINT(runtime/printf)
        break;
      default:
        strcat(msg, "Caught signal ");  // NOLINT(runtime/printf)
        strcat(msg, Safe::itoa(sig, msg + msg_max_length));  // NOLINT(*)
        break;
    }
    if (color_output_) {
      strcat(msg, "\033[0m");  // NOLINT(runtime/printf)
    }
    strcat(msg, " (thread ");  // NOLINT(runtime/printf)
    if (color_output_) {
      strcat(msg, "\033[33;1m");  // NOLINT(runtime/printf)
    }
  #ifndef __APPLE__
    strcat(msg, Safe::utoa(pthread_self(), msg + msg_max_length));  // NOLINT(*)
  #else
    strcat(msg, Safe::ptoa(pthread_self(), msg + msg_max_length));  // NOLINT(*)
  #endif
    if (color_output_) {
      strcat(msg, "\033[0m");  // NOLINT(runtime/printf)
    }
    strcat(msg, ", pid ");  // NOLINT(runtime/printf)
    if (color_output_) {
      strcat(msg, "\033[33;1m");  // NOLINT(runtime/printf)
    }
    strcat(msg, Safe::itoa(getppid(), msg + msg_max_length));  // NOLINT(*)
    if (color_output_) {
      strcat(msg, "\033[0m");  // NOLINT(runtime/printf)
    }
    strcat(msg, ")");  // NOLINT(runtime/printf)
    print(msg);
  }

  print("\nStack trace:\n");
  void **trace = reinterpret_cast<void**>(memory);
  memory += (frames_count_ + 2) * sizeof(void*);
  // Workaround malloc() inside backtrace()
  heap_trap_active_ = true;
  int trace_size = backtrace(trace, frames_count_ + 2);
  heap_trap_active_ = false;
  if (trace_size <= 2) {
    safe_abort();
  }

  // Overwrite sigaction with caller's address
#ifdef __linux__
#if defined(__arm__)
  trace[1] = reinterpret_cast<void *>(uc->uc_mcontext.arm_pc);
#else
#if !defined(__i386__) && !defined(__x86_64__)
#error Only ARM, x86 and x86-64 are supported
#endif
#if defined(__x86_64__)
  trace[1] = reinterpret_cast<void *>(uc->uc_mcontext.gregs[REG_RIP]);
#else
  trace[1] = reinterpret_cast<void *>(uc->uc_mcontext.gregs[REG_EIP]);
#endif
#endif

  const int path_max_length = 2048;
  char* name_buf = memory;
  ssize_t name_buf_length = readlink("/proc/self/exe", name_buf,
                                     path_max_length - 1);
  if (name_buf_length < 1) {
    safe_abort();
  }
  name_buf[name_buf_length] = 0;
  memory += name_buf_length + 1;
  char* cwd = memory;
  if (getcwd(cwd, path_max_length) == NULL) {
    safe_abort();
  }
  strcat(cwd, "/");  // NOLINT(runtime/printf)
  memory += strlen(cwd) + 1;
  char* prev_memory = memory;

  int stackOffset = trace[2] == trace[1]? 2 : 1;
  for (int i = stackOffset; i < trace_size; i++) {
    memory = prev_memory;
    char *line;
    Dl_info dlinf;
    if (dladdr(trace[i], &dlinf) == 0 || dlinf.dli_fname[0] != '/' ||
        !strcmp(name_buf, dlinf.dli_fname)) {
      line = addr2line(name_buf, trace[i], color_output_, &memory);
    } else {
      line = addr2line(dlinf.dli_fname, reinterpret_cast<void *>(
          reinterpret_cast<char *>(trace[i]) -
          reinterpret_cast<char *>(dlinf.dli_fbase)),
          color_output_, &memory);
    }

    char *function_name_end = strstr(line, "\n");
    if (function_name_end != NULL) {
      *function_name_end = 0;
      {
        // "\033[34;1m[%s]\033[0m \033[33;1m(%i)\033[0m\n
        char* msg = memory;
        const int msg_max_length = 512;
        if (color_output_) {
          strcpy(msg, "\033[34;1m");  // NOLINT(runtime/printf)
        } else {
          msg[0] = 0;
        }
        strcat(msg, "[");  // NOLINT(runtime/printf)
        strcat(msg, line);  // NOLINT(runtime/printf)
        strcat(msg, "]");  // NOLINT(runtime/printf)
        if (append_pid_) {
          if (color_output_) {
            strcat(msg, "\033[0m\033[33;1m");  // NOLINT(runtime/printf)
          }
          strcat(msg, " (");  // NOLINT(runtime/printf)
          strcat(msg, Safe::itoa(getppid(), msg + msg_max_length));  // NOLINT(*)
          strcat(msg, ")");  // NOLINT(runtime/printf)
          if (color_output_) {
            strcat(msg, "\033[0m");  // NOLINT(runtime/printf)
          }
          strcat(msg, "\n");  // NOLINT(runtime/printf)
        } else {
          if (color_output_) {
            strcat(msg, "\033[0m");  // NOLINT(runtime/printf)
          }
          strcat(msg, "\n");  // NOLINT(runtime/printf)
        }
        print(msg);
      }
      line = function_name_end + 1;

      // Remove the common path root
      if (cut_common_path_root_) {
        int cpi;
        for (cpi = 0; cwd[cpi] == line[cpi]; cpi++) {};
        if (line[cpi - 1] != '/') {
          for (; line[cpi - 1] != '/'; cpi--) {};
        }
        if (cpi > 1) {
          line = line + cpi;
        }
      }

      // Remove relative path root
      if (cut_relative_paths_) {
        char *path_cut_pos = strstr(line, "../");
        if (path_cut_pos != NULL) {
          path_cut_pos += 3;
          while (!strncmp(path_cut_pos, "../", 3)) {
            path_cut_pos += 3;
          }
          line = path_cut_pos;
        }
      }

      // Mark line number
      if (color_output_) {
        char* number_pos = strstr(line, ":");
        if (number_pos != NULL) {
          char* line_number = memory;  // 128
          strcpy(line_number, number_pos);  // NOLINT(runtime/printf)
          // Overwrite the new line char
          line_number[strlen(line_number) - 1] = 0;
          // \033[32;1m%s\033[0m\n
          strcpy(number_pos, "\033[32;1m");  // NOLINT(runtime/printf)
          strcat(line, line_number);  // NOLINT(runtime/printf)
          strcat(line, "\033[0m\n");  // NOLINT(runtime/printf)
        }
      }
    }

    // Overwrite the new line char
    line[strlen(line) - 1] = 0;

    // Append pid
    if (append_pid_) {
      // %s\033[33;1m(%i)\033[0m\n
      strcat(line, " ");  // NOLINT(runtime/printf)
      if (color_output_) {
        strcat(line, "\033[33;1m");  // NOLINT(runtime/printf)
      }
      strcat(line, "(");  // NOLINT(runtime/printf)
      strcat(line, Safe::itoa(getppid(), memory));  // NOLINT(runtime/printf)
      strcat(line, ")");  // NOLINT(runtime/printf)
      if (color_output_) {
        strcat(line, "\033[0m");  // NOLINT(runtime/printf)
      }
    }

    strcat(line, "\n");  // NOLINT(runtime/printf)
    print(line);
  }

  // Write '\0' to indicate the end of the output
  char end = '\0';
  write(STDERR_FILENO, &end, 1);

#elif defined(__APPLE__)
  for (int i = 0; i < trace_size; i++) {
    Safe::ptoa(trace[i], memory);
    strcat(memory, "\n");
    print(memory);
  }
#endif
  if (thread_safe_) {
    // Resume the parent process
    kill(getppid(), SIGCONT);
  }

  // This is called in the child process
  _Exit(EXIT_SUCCESS);
}

#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif
#ifdef __clang__
#pragma clang diagnostic pop
#endif

}  // namespace Debug
