#ifndef GAMESAVE_CHUNKS_INCLUDED
#define GAMESAVE_CHUNKS_INCLUDED

#include "Chunk.hpp"
#include "Chunks.hpp"
#include "utils/Log.hpp"
#include <stdio.h>

namespace GameSave {


int writeChunksToFile(const char* filename, const ChunkMap* chunkmap) {
    FILE* file = fopen(filename, "w+");
    if (!file) {
        LogError("Failed to open file for writing chunk data! Filename: %s", filename);
        return -1;
    }

    // write number of chunks
    size_t sizeValue = chunkmap->size();
    fwrite(&sizeValue, sizeof(size_t), 1, file);

    int code = 0;

    chunkmap->iterateChunkdata([&](const ChunkData* chunkdata){
        int nItemsWritten = 0;

        // write chunk position to file
        nItemsWritten += fwrite(&chunkdata->position, sizeof(IVec2), 1, file);

        // write actual chunk to file
        nItemsWritten += fwrite(chunkdata->chunk, sizeof(Chunk), 1, file);

        if (nItemsWritten != 2) {
            LogCritical("Failed to write chunk data to file! Number of items written: %d", nItemsWritten);
            code |= -1;
        }
        return false;
    });

    fclose(file);

    return code;
}

int readChunksFromFile(const char* filename, ChunkMap* chunkmap) {
    int code = 0;
    FILE* file = fopen(filename, "r");
    if (!file) {
        LogCritical("Failed to open file for reading chunk data! Filename: %s", filename);
        return -1;
    }

    char *source = NULL;
    long fileSize = 0;
    /* Go to the end of the file. */
    if (fseek(file, 0L, SEEK_END) == 0) {
        /* Get the size of the file. */
        long bufsize = ftell(file);
        fileSize = bufsize;
        if (bufsize == -1) {
            code |= -1;
            return code;
        }

        /* Allocate our buffer to that size. */
        source = (char*)malloc(sizeof(char) * (bufsize + 1));

        /* Go back to the start of the file. */
        if (fseek(file, 0L, SEEK_SET)) {
            code |= -2;
            return code;
        }

        /* Read the entire file into memory. */
        size_t newLen = fread(source, sizeof(char), bufsize, file);
        if (ferror(file)) {
            fputs("Error reading file", stderr);
            code |= -3;
            return code;
        } else {
            source[newLen++] = '\0'; /* Just to be safe. */
        }
    }

    size_t numChunks = 0;
    memcpy(&numChunks, source, sizeof(size_t));

    size_t sizePerChunk = sizeof(Chunk) + sizeof(IVec2);
    int chunksInFile = (fileSize - sizeof(size_t)) / sizePerChunk;

    const char* filePos = source + sizeof(size_t);
    for (int c = 0; c < chunksInFile; c++) {
        IVec2 position;
        memcpy(&position, filePos, sizeof(IVec2));
        filePos += sizeof(IVec2);

        ChunkData* chunkdata = chunkmap->getOrMakeNew(position);
        if (chunkdata) {
            memcpy(chunkdata->chunk, filePos, sizeof(Chunk));
        }
        filePos += sizeof(Chunk);
    }

    fclose(file);
    return code;
}


}

#endif