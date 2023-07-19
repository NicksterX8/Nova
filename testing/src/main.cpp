#include "My/HashMap.hpp"
#include "My/BucketArray.hpp"
#include "rendering/TexturePacker.hpp"
#include "Chunks.hpp"
#include <vector>
#include <string>
#include <sstream>
#include <iostream>

#define T_EQ(a, b) assert(a == b)

using namespace std;

int main() {
    /*

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
    printf("Test successful!\n");
    */

    string s = "5.323, 1256734875928374928375987495823475982347958237495872349085723498572039485792348579234875982347598234759823475908234759823475908234750923487565.43283";

    stringstream stream(s);
    //stream.precision(30);

    float x,y;

    stream >> x;
    stream.ignore(1, ',');
    stream >> y;

    cout.precision(5);

    cout << x << ", " << y;
}