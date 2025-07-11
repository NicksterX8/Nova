#pragma once

#include "ADT/SmallVector.hpp"

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

    SmallVector<Chunk, 0> freeChunks; 
    SmallVector<Chunk, 0> chunks; 
#ifndef NDEBUG
    size_t bytesAllocated = 0;
#endif

    static constexpr size_t BlockAlignment = alignof(std::max_align_t);
public:
    BlockAllocator() = default;

    
    BlockAllocator(Allocator allocator) : AllocatorHolderT(allocator) {}

    BlockAllocator(const Me& other) = delete;

    using AllocatorHolderT::getAllocator;

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