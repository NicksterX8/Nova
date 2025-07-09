#ifndef UTILS_ALLOCATORS_INCLUDED
#define UTILS_ALLOCATORS_INCLUDED

#include "utils/Allocator.hpp"
#include "llvm/SmallVector.h"
#include "My/String.hpp"

template<size_t BlockSize, size_t BlocksPerAlloc, typename Allocator = Mallocator>
class BlockAllocator : private AllocatorHolder<Allocator>, public AllocatorBase<BlockAllocator<BlockSize, BlocksPerAlloc, Allocator>> {
    using Base = AllocatorBase<BlockAllocator<BlockSize, BlocksPerAlloc, Allocator>>;
    using Me = BlockAllocator<BlockSize, BlocksPerAlloc, Allocator>;
    using AllocatorHolderT = AllocatorHolder<Allocator>;
    static_assert(BlockSize > 0 && BlocksPerAlloc > 0, "Block size or blocks per alloc too small!");

    struct Chunk {
        char* ptr;
        size_t blockCount;
    };

    llvm::SmallVector<Chunk, 0> freeChunks; 
    llvm::SmallVector<Chunk, 0> chunks; 
#ifndef NDEBUG
    size_t bytesAllocated = 0;
#endif

    static constexpr size_t BlockAlignment = alignof(std::max_align_t);
public:
    BlockAllocator() = default;

    
    BlockAllocator(Allocator allocator) : AllocatorHolderT(allocator) {}

    BlockAllocator(const Me& other) = delete;

    using AllocatorHolderT::getAllocator;;

    void* allocateMultipleBlocks(size_t neededBlocks) {
        for (int i = (int)freeChunks.size()-1; i >= 0; i--) {
            Chunk& freeChunk = freeChunks[i];
            if (freeChunk.blockCount >= neededBlocks) {
                char* block = freeChunk.ptr;
                if (freeChunk.blockCount == neededBlocks) {
                    // remove chunk if empty
                    // replace used chunk with back most chunk and pop
                    freeChunks[i] = freeChunks.back();
                    freeChunks.pop_back();
                } else {
                    freeChunk.ptr += BlockSize * neededBlocks;
                    freeChunk.blockCount -= neededBlocks;
                }
                return block;
            }
        }
        // no free blocks or none were big enough
        // allocate atleast neededBlocks
        size_t blockCount = std::max(BlocksPerAlloc, neededBlocks);
        Chunk& freeChunk = allocateNewBlocks(blockCount);
        char* block = freeChunk.ptr;
        if (freeChunk.blockCount == neededBlocks) {
            freeChunks.pop_back();
        } else {
            freeChunk.ptr += BlockSize * neededBlocks;
            freeChunk.blockCount -= neededBlocks;
        }
        return block;
    }

    void* allocate(size_t size, size_t alignment) {
        assert(alignment <= BlockAlignment && "Alignment must be compatible with block alignment. kinda due to laziness");
        size_t neededBlocks = (size + BlockSize - 1) / BlockSize; // ceil(size / BlockSize)
        void* ptr;
        if (neededBlocks == 1) {
            if (!freeChunks.empty()) {
                Chunk& freeChunk = freeChunks.back();
                ptr = freeChunk.ptr;
                if (freeChunk.blockCount == 1) {
                    // remove chunk if empty
                    freeChunks.pop_back();
                } else {
                    freeChunk.ptr += BlockSize;
                    freeChunk.blockCount -= 1;
                }
            } else {
                // no free blocks, need to allocate more
                Chunk& freeChunk = allocateNewBlocks(BlocksPerAlloc);
                ptr = freeChunk.ptr;
                if constexpr (BlocksPerAlloc == 1) {
                    // remove chunk if empty
                    freeChunks.pop();
                } else {
                    freeChunk.ptr += BlockSize;
                    freeChunk.blockCount -= 1;
                }
            }
        } else {
            ptr = allocateMultipleBlocks(neededBlocks);
        }
        __asan_unpoison_memory_region(ptr, size);
    #ifndef NDEBUG
        bytesAllocated += size;
    #endif
        return ptr;
    }
private:
    // returns a reference to the new free chunk made
    Chunk& allocateNewBlocks(size_t count) {
        char* memory = (char*)getAllocator().allocate(count * BlockSize, BlockAlignment);
        __asan_poison_memory_region(memory, count * BlockSize);
        Chunk chunk = {
            memory,
            count
        };
        chunks.push_back(chunk);
        freeChunks.push_back(chunk);
        return freeChunks.back();
    }
public:
    using Base::allocate;
    using Base::New;

    void deallocate(void* ptr, size_t size, size_t /* alignment */) {
        size_t blocks = (size + BlockSize - 1) / BlockSize;
        Chunk chunk = {
            (char*)ptr,
            blocks
        };
        freeChunks.push_back(chunk);

        __asan_poison_memory_region(ptr, blocks * BlockSize);

    #ifndef NDEBUG
        bytesAllocated -= size;
    #endif
    }

    void* reallocate(void* ptr, size_t oldSize, size_t newSize, size_t alignment) {
    #ifndef NDEBUG
        bytesAllocated += newSize - oldSize;
    #endif

        assert(alignment <= BlockAlignment && "Non standard alignments not allowed");
        size_t oldBlocks = (oldSize + BlockSize - 1) / BlockSize;
        size_t newBlocks = (oldSize + BlockSize - 1) / BlockSize;
        if (oldBlocks == newBlocks) {
            // already has the amount of blocks requested
            if (newSize > oldSize) {
                __asan_unpoison_memory_region((char*)ptr + oldSize, newSize - oldSize);
            } else {
                __asan_poison_memory_region((char*)ptr + newSize, oldSize - newSize);
            }
            return ptr;
        } else if (oldBlocks < newBlocks) {
            // need to make a new allocation :(
            void* newPtr = allocateMultipleBlocks(newBlocks);
            memcpy(newPtr, ptr, oldSize); // oldSize is greater than new size
            deallocate(ptr, oldSize, BlockAlignment);
            __asan_poison_memory_region(ptr, oldSize);
            __asan_unpoison_memory_region(newPtr, newSize);
            return newPtr;
        } else {
            // in the rare case that someone reallocated to a smaller size, 
            // give up the now unused blocks
            size_t unusedBlocks = oldBlocks - newBlocks;
            Chunk chunk = {
                (char*)ptr + unusedBlocks * BlockSize,
                unusedBlocks
            };
            freeChunks.push_back(chunk);
            return ptr;
        }
    }

    void deallocateAll() {
        for (Chunk& chunk : chunks) {
            getAllocator().deallocate(chunk.ptr, chunk.blockCount * BlockSize, BlockAlignment);
        }
        chunks.clear();
        freeChunks.clear();
    #ifndef NDEBUG
        bytesAllocated = 0;
    #endif
    }

    int countBlocks() const {
        int blocks = 0;
        for (const Chunk& chunk : chunks) {
            blocks += chunk.blockCount;
        }
        return blocks;
    }

    int countFreeBlocks() const {
        int blocks = 0;
        for (const Chunk& chunk : freeChunks) {
            blocks += chunk.blockCount;
        }
        return blocks;
    }

    int countAllocatedBlocks() const {
        return countBlocks() - countFreeBlocks();
    }

    AllocatorStats getAllocatorStats() const {
        int blocks = countBlocks();
        int allocatedBlocks = countAllocatedBlocks();
        std::string allocated;
    #ifndef NDEBUG
        allocated = string_format("%zu bytes allocated. Wasted bytes in blocks: %zu",
            bytesAllocated, BlockSize * allocatedBlocks - bytesAllocated);
    #else
        allocated = string_format("%d allocated blocks of size %zu = %zu bytes",
                allocatedBlocks, BlockSize, allocatedBlocks * BlockSize);
    #endif
        return {
            .name = "Block Allocator",
            .used = string_format("%d used blocks of size %zu = %zu bytes",
                blocks, BlockSize, blocks * BlockSize),
            .allocated = allocated
        };
    }

    ~BlockAllocator() {
        deallocateAll();
    }
};

struct ScratchMemory : public AllocatorBase<ScratchMemory> {
private:
    using Base = AllocatorBase<ScratchMemory>;
protected:
    void* start;
    void* current;
    void* end;
public:

    ScratchMemory() : start(nullptr), current(nullptr), end(nullptr) {}

    ScratchMemory(void* start, void* end) : start(start), current(start), end(end) {}

    ScratchMemory(void* start, size_t size) : start(start), current(start), end((char*)start + size) {}

    template<typename Allocator>
    static ScratchMemory allocate(Allocator& allocator, size_t size) {
        return ScratchMemory(allocator.allocate(size, 1), size);
    }

    template<typename Allocator>
    static void deallocate(Allocator& allocator, const ScratchMemory& memory) {
        allocator.deallocate(memory.start, memory.allocationSize(), 1);
    }

    // allocate scratch memory with malloc of a given size. must call ScratchMemory::free when finished
    static ScratchMemory malloc(size_t size) {
        return ScratchMemory(::malloc(size), size);
    }

    // free memory created with ScratchMemory::malloc
    static void free(ScratchMemory& memory) {
        ::free(memory.start);
    }

    using Base::allocate;

    void* allocate(size_t size, size_t alignment) {
        void* ptr = alignPtr(current, alignment);
        current = (char*)ptr + size;
        __asan_unpoison_memory_region(ptr, size);
        assert(current <= end && "Out of space!");
        return ptr;
    }

    void deallocate(void* ptr, size_t size, size_t alignment) {
        // can not deallocate
        // act like we did though for sanitizers
        __asan_poison_memory_region(ptr, size);
    }

    void reset() {
        // poision everything
        __asan_poison_memory_region(start, (uintptr_t)current - (uintptr_t)start);

        current = start;
    }

    size_t getAllocatedMemory() const {
        return (uintptr_t)current - (uintptr_t)start;
    }

    size_t spaceRemaining() const {
        return (uintptr_t)end - (uintptr_t)current;
    }

    size_t allocationSize() const {
        return (uintptr_t)end - (uintptr_t)start;
    }
};

template<typename Allocator = Mallocator>
class ScratchAllocator : public ScratchMemory, private AllocatorHolder<Allocator> {
    using Base = ScratchMemory;
    using Me = ScratchAllocator<Allocator>;
public:
    using AllocatorHolder<Allocator>::getAllocator;

    ScratchAllocator() = default;

    ScratchAllocator(size_t size) {
        init(size);
    }

    ScratchAllocator(size_t size, Allocator allocator) : AllocatorHolder<Allocator>(allocator) {
        init(size);
    }

    ScratchAllocator(const Me& other) = delete;

    ScratchAllocator(Me&& moved) {
        start = moved.start;
        current = moved.current;
        end = moved.end;

        moved.start = nullptr;
        moved.current = nullptr;
        moved.end = nullptr;
    }

    void init(size_t size) {
        assert(start == nullptr && "Scratch allocator already initialized!");
        start = getAllocator().allocate(size, 1);
        assert(start);
        __asan_poison_memory_region(start, size);
        current = start;
        end = (char*)start + size;
    }

    using Base::New;
    using Base::reallocate;
    using Base::allocate;
    using Base::deallocate;

    void deallocateAll() {
        __asan_poison_memory_region(start, (uintptr_t)current - (uintptr_t)start);
        getAllocator().deallocate(start, allocationSize(), 1);
        start = nullptr;
        current = nullptr;
        end = nullptr;
    }

    ~ScratchAllocator() {
        deallocateAll();
    }

    AllocatorStats getAllocatorStats() const {
        return {
            .name = "ScratchAllocator",
            .allocated = string_format("Allocated bytes: %zu", getAllocatedMemory()),
            .used = string_format("Used bytes: %zu", (uintptr_t)end - (uintptr_t)start)
        };
    }
};

struct FreelistAllocator {
    struct Node {
        Node* next;
        size_t size;
    };

    Node* head;

    void allocateNode(size_t size) {
        void* memory = malloc(size + sizeof(Node));
        Node* node = (Node*)memory;
        char* ptr = (char*)memory + sizeof(Node);

        *node = {
            .next = head,
            .size = size
        };

        head = node;
    }

    // return one of the nodes if it has the given size, if no nodes have that size, return null
    void* tryAllocate(size_t size) {
        Node* node = head;
        Node* last = head;
        while (node) {
            if (node->size == size) {
                char* ptr = (char*)node;
                // remove node from linked list
                last->next = node->next; // move last ptr along one
                if (node == head) {
                    head = node->next;
                }
                return ptr;
            }
            last = node;
            node = node->next;
        }

        return nullptr;
    }

    void deallocate(void* ptr, size_t size) {
        // insert node at head of linked list
        Node* node = (Node*)((char*)ptr - sizeof(Node));
        node->next = head;
        node->size = size;
        head = node;
    }
};


// Allocates memory on the stack, while keeping the heap as a backup if the size is larger than a certain size
// StackElements: The number of elements to allocate on the stack
template<typename T, size_t StackElements>
struct StackAllocate {
    constexpr static size_t NumStackElements = StackElements;
    static_assert(NumStackElements > 0, "Max struct size too small to hold any elements!");
private:
    std::array<T, NumStackElements> stackElements;
    T* _data;

    using Me = StackAllocate<T, StackElements>;
public:
    StackAllocate(size_t size) {
        if (size <= NumStackElements) {
            _data = stackElements.data();
        } else {
            _data = new T[size];
        }
    }

    StackAllocate(size_t size, const T& initValue) {
        if (size <= NumStackElements) {
            _data = stackElements.data();
        } else {
            _data = new T[size];
        }

        for (int i = 0; i < size; i++) {
            _data[i] = initValue;
        }
    }

    StackAllocate(const Me& other) = delete;

    T* data() {
        return _data;
    }

    T& operator[](int index) {
        return _data[index];
    }

    ~StackAllocate() {
        if (_data != stackElements.data()) {
            delete[] _data;
        }
    }
};

// Allocates memory on the stack, while keeping the heap as a backup if the size is larger than a certain size
// MaxStructSize: Maximum number of bytes the struct will take up
template<typename T, size_t StackElements, typename Allocator>
struct StackAllocateWithAllocator {
    constexpr static size_t NumStackElements = StackElements;
    static_assert(NumStackElements > 0, "Max struct size too small to hold any elements!");
private:
    std::array<char, std::max(sizeof(T*), NumStackElements)> stackStorage;
    size_t size;
    Allocator* _allocator;

    void setHeapData(T* data) {
        T** stackStoragePtr = (T**)stackStorage.data();
        *stackStoragePtr = data;
    }

    T* getHeapData() {
        return *(T**)stackStorage.data();
    }
public:
    StackAllocateWithAllocator(size_t size, Allocator* allocator) : _allocator(allocator) {
        setData(_allocator->template allocate<T>(size));
    }

    StackAllocateWithAllocator(size_t size, const T& initValue, Allocator* allocator) : _allocator(allocator) {
        T* data;
        if (size <= NumStackElements) {
            data = stackStorage.data();
        } else {
            data = _allocator->template allocate<T>(size);
            setHeapData(data);
        }

        for (int i = 0; i < size; i++) {
            data[i] = initValue;
        }
    }

    T& operator[](int index) {
        return data()[index];
    }

    T* data() {
        if (size <= NumStackElements) {
            return stackStorage.data();
        }
        return getHeapData();
    }

    ~StackAllocateWithAllocator() {
        if (size > StackElements) {
            _allocator->deallocate(getHeapData(), size);
        }
    }
};

template<typename T, typename Allocator>
T* New(const T& value, Allocator* allocator) {
    return allocator->template New<T>(value);
}

template<typename T, typename Allocator>
T* New(Allocator* allocator) {
    return allocator->template New<T>();
}

#endif