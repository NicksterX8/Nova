#include "main.hpp"
#include <string.h>
#include "GameSave.hpp"
#include "Chunks.hpp"
#include "../GameState.hpp"
#include "../Log.hpp"
#include <map>

namespace GameSave {

struct ChunkPositionPair {
    IVec2 position;
    Chunk chunk;
};

const size_t PropertyNameSize = 32;

struct FileProperty {
    char name[32];
    size_t size;
    const void* data = NULL;
    void* allocatedData = NULL;

    FileProperty(const char* propertyName, const void* data, size_t size) {
        setName(propertyName);
        this->size = size;
        if (!data) {
            Log.Error("data passed to file property is NULL!");
        }
        this->data = data;
    }

    static FileProperty String(const char* propertyName, const char* string) {
        return FileProperty(propertyName, string, strlen(string) + 1);
    }

    template<class T>
    FileProperty(const char* propertyName, T value) {
        setName(propertyName);
        this->size = sizeof(T);
        T* dataPtr = (T*)malloc(sizeof(T));
        *dataPtr = value;
        allocatedData = (void*)dataPtr;
        data = allocatedData;
    }

    ~FileProperty() {
        if (allocatedData) {
            free(allocatedData);
            allocatedData = NULL;
        }
    }

    // return 0 on success, 1 on failure
    int write(FILE* file) {
        return !fwrite(data, size, 1, file);
    }

    void setName(const char* string) {
        size_t nameLength = strlen(name) + 1;
        if (nameLength > PropertyNameSize) {
            Log.Error("Name for file property is too long! Length: %lu", nameLength);
        }
        strlcpy(name, string, PropertyNameSize);
    }
};

struct FilePropertyHeader {
    char name[32] = {'\0'};
    size_t location = 0;
};

struct FileWriter {
    std::vector<FilePropertyHeader> headers;
    std::vector<char> body;
public:

    void set(const char* name, const void* data, size_t size) {
        FilePropertyHeader propHeader;

        size_t nameLen = strlen(name) + 1;
        if (nameLen > 32) {nameLen = 32;}
        memcpy(&propHeader.name, name, nameLen);
        
        propHeader.location = body.size();
        headers.push_back(propHeader);

        // const size_t* sizePtr = (const size_t*)data;

        const char* charPtr = (const char*)data;
        const char* end = &charPtr[size];

        body.insert(body.end(), charPtr, end);
    }

    template<class T>
    void set(const char* name, T value) {
        set(name, &value, sizeof(T));
    }

    void write(FileWriter& writer, const char* name) {
        FilePropertyHeader header;
        size_t nameSize = strlen(name) + 1;
        if (nameSize > PropertyNameSize) {
            nameSize = 32;
        }
        memcpy(&header.name, name, nameSize);

        header.location = body.size();
        headers.push_back(header);
        size_t numHeaders = writer.headers.size();
        size_t headerSize = sizeof(FilePropertyHeader) * writer.headers.size() + sizeof(size_t);
        body.insert(body.end(), (const char*)&numHeaders, ((const char*)&numHeaders) + sizeof(size_t));
        for (auto& writerHeader : writer.headers) {
            writerHeader.location += headerSize;
        }
        body.insert(body.end(), (const char*)&writer.headers[0], ((const char*)&writer.headers[0]) + writer.headers.size() * sizeof(FilePropertyHeader));
        body.insert(body.end(), (const char*)&writer.body[0], ((const char*)&writer.body[0]) + writer.body.size());
    }

    void write(FILE* file) {
        size_t numProperties = headers.size();
        fwrite(&numProperties, sizeof(size_t), 1, file);
        size_t headerSize = sizeof(FilePropertyHeader) * headers.size() + sizeof(size_t);
        for (auto& header : headers) {
            header.location += headerSize;
            size_t x = header.location;
        }
        fwrite(&headers[0], sizeof(FilePropertyHeader), headers.size(), file);
        fwrite(&body[0], body.size(), 1, file);
    }
};

const char* const entitiesFilepath = "entities";

const char* findProperty(const char* property, const char* fileContents) {
    const char* file = fileContents;
    size_t numProperties;
    memcpy(&numProperties, file, sizeof(size_t));
    file += sizeof(size_t);
    for (size_t i = 0; i < numProperties; i++) {
        if (strncmp(file, property, PropertyNameSize) == 0) {
            size_t filePos;
            memcpy(&filePos, file+PropertyNameSize, sizeof(size_t));
            return fileContents+filePos;
        }
        file += sizeof(FilePropertyHeader);
    }
    return NULL;
}

const void* readProp(const char* propName, const char* contents) {
    const char* location = findProperty(propName, contents);
    if (!location) {
        Log.Error("Failed to read prop %s!", propName);
        return NULL;
    }
    const char* propContents = location;
    //size_t size;
    //memcpy(&size, location, sizeof(size_t));
    //if (outPropSize)
    //    *outPropSize = size;
    return propContents;
}

template<class T>
const T* readProp(const char* propName, const char* src) {
    return static_cast<const T*>(readProp(propName, src));
}

template<class C>
void writeComponentPool(FileWriter& ef, const ComponentPool* pool) {

    if (!C::Serializable) {
        Log("non serial component passed!");
    }

    /*
    component name size: 4
    component name: component name size (unknown)
    component size: 4
    number of components: 4
    components: component size * number of components
    component owners: 4 * number of components
    */

    int code = 0;

    FileWriter cf;

    cf.set("componentSize", pool->componentSize);
    cf.set("numComponents", (size_t)pool->size());
    cf.set("components", pool->components, pool->componentSize * pool->size());
    cf.set("componentOwners", pool->componentOwners, sizeof(EntityID) * pool->size());

    ef.write(cf, componentNames[pool->id]);
}

int readComponentPool(const char* source, ComponentPool* pool) {
    int code = 0;

    //ComponentPool* pool = em->pools[getID<C>()];
    pool->_size = 0;

    const char* contents = static_cast<const char*>(
        readProp(componentNames[pool->id], source));

    size_t componentSize = *static_cast<const size_t*>(
        readProp("componentSize", contents));

    size_t numComponents = *static_cast<const size_t*>(
        readProp("numComponents", contents));

    const void* components = static_cast<const void*>(
        readProp("components", contents));

    const EntityID* componentOwners = static_cast<const EntityID*>(
        readProp("componentOwners", contents));
    
    assert(pool->size() == 0 && "pool written to must not have pre-existing components");
    pool->resize(numComponents + 10);
    memcpy(pool->components, components, numComponents * componentSize);
    memcpy(pool->componentOwners, componentOwners, numComponents * sizeof(EntityID));

    for (Uint32 i = 0; i < MAX_ENTITIES; i++) {
        pool->entityComponentSet[i] = NULL_COMPONENTPOOL_INDEX;
    }

    for (size_t i = 0; i < numComponents; i++) {
        EntityID owner = pool->componentOwners[i];
        pool->entityComponentSet[owner] = i;
    }

    pool->_size = (Uint32)numComponents;

    for (Uint32 i = 0; i < (Uint32)numComponents; i++) {
        pool->entityComponentSet[pool->componentOwners[i]] = i;
    }

    return code;
}



template<class... Components>
int writeComponentPools(FileWriter& ef, const EntityManager* em) {
    int dummy[] = {0, ((void)writeComponentPool<Components>(ef, em->getPool<Components>()), 0) ...};
    (void)dummy;
    return 0;
}

template<class... Components>
int readComponentPools(const char* source, EntityManager* em) {
    int codes[] = {0, ( (void)0, readComponentPool(source, em->getPool<Components>()) ) ...};
    int code = codes[0];
    for (size_t i = 1; i < sizeof(codes) / sizeof(int); i++) {
        code |= codes[i];
    }  
    return code;
}

const char* const EntitiesFilename = "entities";
const char* const PlayerFilename = "player";

int writeEverythingToFiles(const char* outputSaveFolderPath, const GameState* state) {
    /*
    number of live entities: 4
    size of entities array: 
    */

    int code = 0;

    saveFolderPath = outputSaveFolderPath;

    char chunksFilepath[256];
    strcpy(chunksFilepath, saveFolderPath);
    strcat(chunksFilepath, ChunksFilename);
    code |= writeChunksToFile(chunksFilepath, &state->chunkmap);

    char entitiesFilepath[256];
    strcpy(entitiesFilepath, saveFolderPath);
    strcat(entitiesFilepath, EntitiesFilename);
    FILE* entityFile = fopen(entitiesFilepath, "w");
    if (!entityFile) {
        code |= -1;
        Log.Critical("Failed to open entity file for writing! File path: %s", entitiesFilepath);
        return code;
    }

    FileWriter ef;

    size_t numLiveEntities = state->ecs.numLiveEntities();
    ef.set("numLiveEntities", &numLiveEntities, sizeof(size_t));
    ef.set("maxEntities", (size_t)MAX_ENTITIES);
    ef.set("entities", state->ecs.manager.entities, sizeof(Entity) * MAX_ENTITIES);
    ef.set("entityComponents", state->ecs.manager.entityComponentFlags, sizeof(ComponentFlags) * MAX_ENTITIES);
    ef.set("numFreeEntities", state->ecs.manager.freeEntities.size());
    ef.set("freeEntities", &state->ecs.manager.freeEntities[0],
        sizeof(Entity) * state->ecs.manager.freeEntities.size());
    ef.set("indices", state->ecs.manager.indices, sizeof(Uint32) * MAX_ENTITIES);

    writeComponentPools<COMPONENTS>(ef, state->ecs.em());

    ef.write(entityFile);
    fclose(entityFile);

    char playerFilepath[256];
    strcpy(playerFilepath, saveFolderPath);
    strcat(playerFilepath, PlayerFilename);
    FILE* playerFile = fopen(playerFilepath, "w");
    if (!playerFile) {
        code |= -1;
        Log.Critical("Failed to open player file for writing! File path: %s", playerFilepath);
        return code;
    }

    FileWriter pf;
    pf.set("entity", state->player.entity);
    pf.set("grenadeThrowCooldown", state->player.grenadeThrowCooldown);
    pf.write(playerFile);

    fclose(playerFile);

    if (code) {
        Log.Critical("Error occurred during writing to save!");
    }
    return code;
}

const char* readSource(void* dst, const char* src, size_t size) {
    memcpy(dst, src, size);
    return src + size;
}

const char* readFileContents(FILE* file) {
    if (!file) {
        Log.Critical("NULL file passed to readFileContents!");
        return NULL;
    }
    
    char *source = NULL;
    long fileSize = 0;
    /* Go to the end of the file. */
    if (fseek(file, 0L, SEEK_END) == 0) {
        /* Get the size of the file. */
        long bufsize = ftell(file);
        fileSize = bufsize;
        if (bufsize == -1) {
            Log.Critical("Couldn't get size of file!");
            fclose(file);
            return NULL;
        }

        /* Allocate our buffer to that size. */
        source = (char*)malloc(sizeof(char) * (bufsize + 1));

        /* Go back to the start of the file. */
        if (fseek(file, 0L, SEEK_SET)) {
            return NULL;
        }

        /* Read the entire file into memory. */
        size_t newLen = fread(source, sizeof(char), bufsize, file);
        if (ferror(file)) {
            fputs("Error reading file", stderr);
            free(source);
            fclose(file);
            return NULL;
        } else {
            source[newLen++] = '\0'; /* Just to be safe. */
        }
    }
    return source;
}

int readEntityDataFromFile(const char* filepath, ECS* ecs) {
    int code = 0;
    FILE* file = fopen(filepath, "r");
    const char* src = readFileContents(file);
    fclose(file);
    if (!src) {
        Log.Error("Failed to read file contents for entity data. Path: %s", filepath);
        return -1;
    }

    // get data from source
    size_t numLiveEntities = *readProp<size_t>("numLiveEntities", src);
    size_t maxEntities = *readProp<size_t>("maxEntities", src);
    const Entity *entities = readProp<Entity>("entities", src);
    const ComponentFlags *entityComponents = readProp<ComponentFlags>("entityComponents", src);
    size_t numFreeEntities = *readProp<size_t>("numFreeEntities", src);
    const Entity *freeEntities = readProp<Entity>("freeEntities", src);
    const Uint32 *indices = readProp<Uint32>("indices", src);

    // validate data

    if (maxEntities > MAX_ENTITIES) {
        Log.Critical("Max entity count while reading entity save file is too large!");
        maxEntities = MAX_ENTITIES;
        code |= -212;
        return code;
    }
    if (maxEntities < MAX_ENTITIES) {
        Log.Critical("Max entity count while reading entity save file is too small!");
        code |= -563;
        return code;
    }
    
    // update data

    ecs->manager.liveEntities = (Uint32)numLiveEntities;
    ecs->manager.freeEntities.clear();
    ecs->manager.freeEntities.insert(ecs->manager.freeEntities.end(), freeEntities, freeEntities + numFreeEntities);
    memcpy(ecs->manager.entities, entities, maxEntities * sizeof(Entity));
    memcpy(ecs->manager.entityComponentFlags, entityComponents, maxEntities * sizeof(ComponentFlags));
    memcpy(ecs->manager.indices, indices, sizeof(Uint32) * maxEntities);

    readComponentPools<COMPONENTS>(src, ecs->em());

    auto inventoryECPool = ecs->manager.getPool<EC::Inventory>();
    for (Uint32 i = 0; i < inventoryECPool->size(); i++) {
        // inventoryECPool->components[i].inventory 
    }

    free((void*)src);
 
    return code;
}

int readPlayerDataFromFile(const char* filepath, Player* player) {
    FILE* file = fopen(filepath, "r");
    if (!file) {
        Log.Critical("Failed to open player file for reading! File path: %s", filepath);
        return -1;
    }
    const char* src = readFileContents(file);
    if (!src) {
        return -1;
    }

    player->entity = *readProp<OptionalEntityT<Entities::Player>>("entity", src);
    player->grenadeThrowCooldown = *readProp<int>("grenadeThrowCooldown", src);

    fclose(file);
    return 0;
}

int readEverythingFromFiles(const char* inputSaveFolderPath, GameState* state) {
    int code = 0;

    saveFolderPath = inputSaveFolderPath;

    char chunksFilepath[256];
    strcpy(chunksFilepath, saveFolderPath);
    strcat(chunksFilepath, ChunksFilename);
    code |= readChunksFromFile(chunksFilepath, &state->chunkmap);

    char entityFilepath[256];
    strcpy(entityFilepath, saveFolderPath);
    strcat(entityFilepath, "entities");
    readEntityDataFromFile(entityFilepath, &state->ecs);

    char playerFilepath[256];
    strcpy(playerFilepath, saveFolderPath);
    strcat(playerFilepath, PlayerFilename);
    readPlayerDataFromFile(playerFilepath, &state->player);


    if (code) {
        Log.Critical("Error occurred during reading of save!");
    }
    return code;
}

int save(const GameState* state) {
    int code = writeEverythingToFiles("save/", state);
    Log("Saved!");
    return code;
}

int load(GameState* state) {
    int code = readEverythingFromFiles("save/", state);
    state->player.releaseHeldItem();
    Log("Loaded!");
    return code;
}

}