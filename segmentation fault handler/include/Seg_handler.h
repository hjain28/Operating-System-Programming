
#ifndef SEG_HANDLER_H_
#define SEG_HANDLER_H_

#include <stddef.h>
#include <unistd.h>

// have to override malloc() and free()
extern "C" {
void* __malloc_impl(size_t size);
#ifdef __linux__
void* malloc(size_t size) throw();
void free(void* ptr) throw();
#elif defined(__APPLE__)
void* __malloc_zone(struct _malloc_zone_t* zone, size_t size);
void __free_zone(struct _malloc_zone_t* zone, void* ptr);
#endif
}

#ifdef __linux__
// Comment this out on systems without quick_exit()
#define QUICK_EXIT
#endif

namespace Debug {


class SEGHandler {
 public:
  typedef ssize_t (*OutputCallback)(const char*, size_t);

  SEGHandler(bool altstack = false);
  ~SEGHandler();

  bool cleanup() const;

  void set_cleanup(bool value);

  bool generate_core_dump() const;


  void set_generate_core_dump(bool value);

#ifdef QUICK_EXIT

  bool quick_exit() const;

  void set_quick_exit(bool value);
#endif

  int frames_count() const;

  void set_frames_count(int value);
  bool cut_common_path_root() const;

  void set_cut_common_path_root(bool value);

  bool cut_relative_paths() const;

  void set_cut_relative_paths(bool value);

  bool append_pid() const;

  void set_append_pid(bool value);

  bool color_output() const;

  void set_color_output(bool value);

  bool thread_safe() const;

  void set_thread_safe(bool value);

  OutputCallback output_callback() const;

  void set_output_callback(OutputCallback value);

 private:
  friend void* ::__malloc_impl(size_t);
#ifdef __linux__
  friend void* ::malloc(size_t) throw();
  friend void ::free(void*) throw();
#elif defined(__APPLE__)
  friend void* ::__malloc_zone(struct _malloc_zone_t*, size_t);
  friend void ::__free_zone(struct _malloc_zone_t*, void*);
#endif
 
  inline static void print(const char* msg, size_t len = 0);

  static const size_t kNeededMemory;

  static void HandleSignal(int sig, void* info, void* secret);
  static void* malloc_;
  static void* free_;
  static bool heap_trap_active_;

  static bool generate_core_dump_;
  static bool cleanup_;
#ifdef QUICK_EXIT
  static bool quick_exit_;
#endif
  static int frames_count_;
  static bool cut_common_path_root_;
  static bool cut_relative_paths_;
  static bool append_pid_;
  static bool color_output_;
  static bool thread_safe_;
  static OutputCallback output_callback_;
  /// @brief The preallocated memory to use in the signal handler.
  static char* memory_;
};

} 
#endif  /* SEG_HANDLER_H_ */
