#pragma once

#include "ADT/SmallVector.hpp"
#include "llvm/BitVector.h"

namespace OLD {

template<size_t BlockSize, size_t BlocksPerAlloc, typename Allocator = Mallocator>
class BlockAllocator : public AllocatorBase<BlockAllocator<BlockSize, BlocksPerAlloc, Allocator>> {
    using Base = AllocatorBase<BlockAllocator<BlockSize, BlocksPerAlloc, Allocator>>;
    using Me = BlockAllocator<BlockSize, BlocksPerAlloc, Allocator>;
    static_assert(BlockSize > 0 && BlocksPerAlloc > 0, "Block size or blocks per alloc too small!");

    struct Chunk {
        char* ptr;
        size_t blockCount;
    };

    SmallVector<Chunk, 0> freeChunks; 
    SmallVector<Chunk, 0> chunks; 
    EMPTY_BASE_OPTIMIZE Allocator allocator;
#ifndef NDEBUG
    size_t bytesAllocated = 0;
#endif

    static constexpr size_t BlockAlignment = alignof(std::max_align_t);
public:
    BlockAllocator() = default;

    
    BlockAllocator(Allocator allocator) : allocator(allocator) {}

    BlockAllocator(const Me& other) = delete;

    void* allocateMultipleBlocks(size_t neededBlocks) {
        // for (int i = (int)freeChunks.size()-1; i >= 0; i--) {
        //     Chunk& freeChunk = freeChunks[i];
        //     if (freeChunk.blockCount >= neededBlocks) {
        //         char* block = freeChunk.ptr;
        //         if (freeChunk.blockCount == neededBlocks) {
        //             // remove chunk if empty
        //             // replace used chunk with back most chunk and pop
        //             freeChunks[i] = freeChunks.back();
        //             freeChunks.pop_back();
        //         } else {
        //             freeChunk.ptr += BlockSize * neededBlocks;
        //             freeChunk.blockCount -= neededBlocks;
        //         }
        //         return block;
        //     }
        // }
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
        char* memory = (char*)allocator.allocate(count * BlockSize, BlockAlignment);
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
    // using Base::allocate;
    // using Base::New;

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
            allocator.deallocate(chunk.ptr, chunk.blockCount * BlockSize, BlockAlignment);
        }
        chunks.clear();
        freeChunks.clear();
    #ifndef NDEBUG
        bytesAllocated = 0;
    #endif
    }

    static constexpr size_t goodSize(size_t minSize) {
        return (minSize + BlockSize - 1) / BlockSize * BlockSize;
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

}

namespace NEW {

// Only alloc sizes <= BlockSize, otherwise make a custom alloc from Allocator
// This allocator is NOT thread safe!
template<size_t BlockSize, size_t BlocksPerAlloc, typename Allocator = Mallocator>
class BlockAllocator : public AllocatorBase<BlockAllocator<BlockSize, BlocksPerAlloc, Allocator>> {
    using Base = AllocatorBase<BlockAllocator<BlockSize, BlocksPerAlloc, Allocator>>;
    using Me = BlockAllocator<BlockSize, BlocksPerAlloc, Allocator>;
    static_assert(BlocksPerAlloc > 0, "BlocksPerAlloc must be positive!");
    static_assert(BlockSize > 0, "Block size must be positive!");

    SmallVector<char*, 0> freeBlocks;
    SmallVector<char*, 0> chunks;

    char* stackFreeBlocks[BlocksPerAlloc];
    uint32_t stackFreeBlockCount = 0;
    uint32_t lastCleanupFreeBlockCount = 0;

    EMPTY_BASE_OPTIMIZE Allocator allocator;
#ifndef NDEBUG
    size_t nonBlockBytesAllocated = 0;
    size_t blockBytesAllocated = 0;
#endif

    static constexpr size_t BlockAlignment = alignof(std::max_align_t);
public:
    BlockAllocator() = default;

    BlockAllocator(Allocator allocator) : allocator(allocator) {}

    BlockAllocator(Allocator allocator, size_t initialBlocks) : allocator(allocator) {
        allocateChunks((initialBlocks + BlocksPerAlloc - 1) / BlocksPerAlloc);
    }

    BlockAllocator(const Me& other) = delete;

    void* allocate(size_t size, size_t alignment) {
        void* ptr;
        if (LIKELY(size < BlockSize && alignment <= BlockAlignment)) {
            ptr = allocateBlock();
        #ifndef NDEBUG
            blockBytesAllocated += size;
        #endif
        } else {
            ptr = allocateNonBlock(size, alignment);
        #ifndef NDEBUG
            nonBlockBytesAllocated += size;
        #endif
        }
        __asan_unpoison_memory_region(ptr, size);
    
        return ptr;
    }

    using Base::allocate;

private:
    char* allocateBlock() {
        if (UNLIKELY(stackFreeBlockCount == 0)) {
            if (freeBlocks.size() <= BlocksPerAlloc) {
                // allocate a chunk to fill stack blocks
                char* chunk = (char*)allocator.allocate(BlocksPerAlloc * BlockSize, BlockAlignment);
                chunks.push_back(chunk);
                for (unsigned i = 0; i < BlocksPerAlloc-1; i++) {
                    stackFreeBlocks[i] = chunk + BlockSize * i;
                }
                stackFreeBlockCount = BlocksPerAlloc-1;
                return chunk + BlockSize * (BlocksPerAlloc-1);
            } else { // free blocks has enough space to fill up all stack free blocks + 1 for the current block
                // use free blocks to fill stack blocks fully for next time, leaving the backmost block for the current block
                memcpy(stackFreeBlocks, &freeBlocks[freeBlocks.size() - BlocksPerAlloc - 1], BlocksPerAlloc * sizeof(char*));
                stackFreeBlockCount = BlocksPerAlloc;
                char* block = freeBlocks.back();
                freeBlocks.resize(freeBlocks.size() - BlocksPerAlloc - 1);
                return block;
            }
        }
        return stackFreeBlocks[--stackFreeBlockCount];
    }

    void* allocateNonBlock(size_t size, size_t alignment) {
        return allocator.allocate(size, alignment);
    }

    // returns a reference to the new free chunk made
    void allocateChunks(size_t count) {
        char* memory = (char*)allocator.allocate(count * BlocksPerAlloc * BlockSize, BlockAlignment);
        for (int c = 0; c < count; c++) {
            char* chunk = memory + c * BlocksPerAlloc * BlockSize;
            chunks.push_back(chunk);
            for (int b = 0; b < BlocksPerAlloc; b++)
                freeBlocks.push_back(chunk + b * BlockSize);
        }
    }

    void deallocateNonBlock(void* ptr, size_t size, size_t alignment) {
        allocator.deallocate(ptr, size, alignment);
    }

    void* reallocateNonBlock(void* ptr, size_t oldSize, size_t newSize, size_t alignment) {
        return allocator.reallocate(ptr, oldSize, newSize, alignment);
    }
public:

    void deallocate(void* ptr, size_t size, size_t alignment) {
        if (size < BlockSize && alignment <= BlockAlignment) {
            deallocateBlock((char*)ptr);
        #ifndef NDEBUG
            blockBytesAllocated -= size;
        #endif
        } else {
            deallocateNonBlock(ptr, size, alignment);
        #ifndef NDEBUG
            nonBlockBytesAllocated -= size;
        #endif
        }

        __asan_poison_memory_region(ptr, size);
    }

    using Base::deallocate;
private:
    void deallocateBlock(char* block) {
        if (stackFreeBlockCount < BlocksPerAlloc) {
            stackFreeBlocks[stackFreeBlockCount++] = block;
            return;
        }
        freeBlocks.push_back(block);
        if (shouldCleanupChunks()) {
            tryCleanupChunks();
        }
    }

    void deallocateChunk(char* chunk) {
        allocator.deallocate(chunk, BlocksPerAlloc * BlockSize, BlockAlignment);
    }
public:
    void deallocateAll() {
        for (char* chunk : chunks) {
            allocator.deallocate(chunk, BlocksPerAlloc * BlockSize, BlockAlignment);
        }
        chunks.clear();
        freeBlocks.clear();
    #ifndef NDEBUG
        blockBytesAllocated = 0;
        nonBlockBytesAllocated = 0;
    #endif
    }

    void* reallocate(void* ptr, size_t oldSize, size_t newSize, size_t alignment) {
        assert(alignment <= BlockAlignment && "Non standard alignments not allowed");

        __asan_poison_memory_region(ptr, oldSize);

        void* newPtr;
        if (oldSize <= BlockSize) { 
            // old ptr was block
            if (newSize <= BlockSize) {
                // new ptr is block
                // already have enough space
                newPtr = ptr;
            #ifndef NDEBUG
                blockBytesAllocated += newSize - oldSize;
            #endif
            } else {
                // moving to size too big to fit in block
                deallocateBlock((char*)ptr);
                newPtr = allocateNonBlock(newSize, alignment);

            #ifndef NDEBUG
                nonBlockBytesAllocated += newSize;
                blockBytesAllocated -= oldSize;
            #endif
            }
        } else {
            // old ptr was nonblock
            if (newSize <= BlockSize) {
                // new ptr is block
                deallocateNonBlock(ptr, oldSize, alignment);
                newPtr = allocateBlock();
            #ifndef NDEBUG
                nonBlockBytesAllocated -= oldSize;
                blockBytesAllocated += newSize;
            #endif
            } else {
                // both non blocks
                if (newSize == oldSize) {
                    newPtr = ptr;
                #ifndef NDEBUG
                    nonBlockBytesAllocated += newSize - oldSize;
                #endif
                } else {
                    newPtr = reallocateNonBlock(ptr, oldSize, newSize, alignment);
                #ifndef NDEBUG
                    nonBlockBytesAllocated += newSize - oldSize;
                #endif
                }
            }
        }

        __asan_unpoison_memory_region(newPtr, newSize);
        return newPtr;
    }

    static constexpr size_t goodSize(size_t minSize) {
        if (minSize < BlockSize) return BlockSize;
        else return minSize;
    }

    void reserve(size_t blocks) {
        if (blocks > freeBlocks.size()) {
            allocateChunks((blocks + BlocksPerAlloc - 1) / BlocksPerAlloc);
        }
    }

    bool shouldCleanupChunks() const {
        // number of free blocks is more than half of estimated blocks allocated
        return freeBlocks.size() > MAX(3 * BlocksPerAlloc, 2 * lastCleanupFreeBlockCount);
    }

    void tryCleanupChunks() {
        for (auto block : freeBlocks) {
            // for each chunk, check if all 
            for (int i = (int)chunks.size() - 1; i >= 0; i--) {
                // check if block is within chunk bounds. dont want <= because chunk is [start, end)
                char* chunk = chunks[i];
                int freeBlockCount = 0;
                if (block >= chunk && block < chunk + BlocksPerAlloc * BlockSize) {
                    if (++freeBlockCount == BlocksPerAlloc) {
                        // chunk has no live allocations remaining
                        deallocateChunk(chunk);
                    }
                    break;
                }
            }
        }

        lastCleanupFreeBlockCount = freeBlocks.size();
    }
private:

    int numUsedBlocks() const {
        return chunks.size() * BlocksPerAlloc;
    }

    int numAllocatedBlocks() const {
        return numUsedBlocks() - freeBlocks.size();
    }
public:

    AllocatorStats getAllocatorStats() const {
        int usedBlocks = numUsedBlocks();
        int allocatedBlocks = numAllocatedBlocks();
        std::string allocated;
    #ifndef NDEBUG
        allocated = string_format("%zu block bytes allocated. %zu non block bytes allocated. Wasted bytes in blocks: %zu",
            blockBytesAllocated, nonBlockBytesAllocated, (ssize_t)BlockSize * usedBlocks - (ssize_t)blockBytesAllocated);
    #else
        allocated = string_format("%d allocated blocks of size %zu = %zu bytes",
                allocatedBlocks, BlockSize, allocatedBlocks * BlockSize);
    #endif
        return {
            .name = "Block Allocator",
            .used = string_format("%d used blocks of size %zu = %zu bytes",
                usedBlocks, BlockSize, usedBlocks * BlockSize),
            .estimatedBytesUsed = usedBlocks * BlockSize,
            .allocated = allocated
        };
    }

    ~BlockAllocator() {
        deallocateAll();
    }
};

}

using NEW::BlockAllocator;