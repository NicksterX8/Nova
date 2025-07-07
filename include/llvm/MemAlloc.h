#include <stddef.h>
#include <new>

namespace llvm {

inline void* allocate_buffer(size_t Size, size_t Alignment) {
  void *Result = ::operator new(Size,
#ifdef __cpp_aligned_new
                                std::align_val_t(Alignment),
#endif
                                std::nothrow);
  return Result;
}

inline void deallocate_buffer(void *Ptr, size_t Size, size_t Alignment) {
  ::operator delete(Ptr
#ifdef __cpp_sized_deallocation
                    ,
                    Size
#endif
#ifdef __cpp_aligned_new
                    ,
                    std::align_val_t(Alignment)
#endif
  );
}

}