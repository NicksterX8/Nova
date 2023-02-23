#include "My/HashMap.hpp"
#include "My/BucketArray.hpp"
#include "Chunks.hpp"
#include <vector>

#define T_EQ(a, b) assert(a == b)



int main() {
    auto map = My::HashMap<int, int>::Empty();
    for (int i = 0; i < 200; i++) {
        auto key = i;
        auto value = i;
        T_EQ(*map.insert(key, value), value);
    }
    T_EQ(map.size, 200);
    
    ChunkMap chunkmap;
    chunkmap.init();
    auto array = My::BucketArray<Chunk, 16>::Empty();
    for (int i = 0; i < 200; i++) {
        auto key = IVec2{i, i};
        auto value = i;
        auto chunkdata = chunkmap.newChunkAt(key);
        auto chunk = array.reserveBack();
        memset(chunk, CHAR_MAX, sizeof(Chunk));
        chunkdata->chunk = chunk;
        T_EQ(chunkdata->position, key);
        auto real = chunkmap.get(key);
        T_EQ(real, chunkdata);
        T_EQ(real->chunk, chunkdata->chunk);
        T_EQ(memcmp(real->chunk, chunk, sizeof(Chunk)), 0);
    }
    T_EQ(chunkmap.size(), 200);
    T_EQ(array.size(), 200);
}