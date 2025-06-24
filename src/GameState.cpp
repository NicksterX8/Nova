#include "GameState.hpp"

#include <functional>
#include <array>
#include <glm/geometric.hpp>

#include <SDL3/SDL.h>
#include "constants.hpp"
#include "rendering/textures.hpp"
#include "utils/vectors_and_rects.hpp"
#include "Tiles.hpp"
#include "Player.hpp"
#include "world/entities/entities.hpp"
#include "items/manager.hpp"
#include "items/prototypes/prototypes.hpp"

void makeItemPrototypes(ItemManager& im) {
    using namespace items;
    auto& pm = im.prototypes;

    auto tile = new Prototypes::Tile(pm);
    auto grenade = new Prototypes::Grenade(pm);
    auto sandGun = new Prototypes::SandGun(pm);

    ItemPrototype* prototypes[] = {
        tile, grenade, sandGun
    };

    for (int i = 0; i < sizeof(prototypes) / sizeof(prototypes[0]); i++) {
        pm.add(prototypes[i]);
    }
}

void makeEntityPrototypes(EntityWorld& ecs) {
    auto& pm = ecs.em.prototypes;
    using namespace World::Entities;
    auto player = new World::Entities::Player(pm);
    auto itemStack = new World::Entities::ItemStack(pm);
    auto monster = new World::Entities::Monster(pm);
    auto spider = new Spider(pm);
    auto grenade = new World::Entities::Grenade(pm);

    ItemPrototype* prototypes[] = {
        player, itemStack, monster, spider, grenade
    };

    for (int i = 0; i < sizeof(prototypes) / sizeof(prototypes[0]); i++) {
        pm.add(prototypes[i]);
    }
}

void GameState::init(const TextureManager* textureManager) {
    /* Init Chunkmap */
    chunkmap.init();
    int chunkRadius = 4;
    for (int chunkX = -chunkRadius; chunkX < chunkRadius; chunkX++) {
        for (int chunkY = -chunkRadius; chunkY < chunkRadius; chunkY++) {
            ChunkData* chunkdata = chunkmap.newChunkAt({chunkX, chunkY});
            if (chunkdata) {
                generateChunk(chunkdata);
            } else {
                LogError("Failed to create chunk at tile (%d,%d) for initialization", chunkX * CHUNKSIZE, chunkY * CHUNKSIZE);
                continue;
            }
        }
    }

    /* Init ECS */
    ecs = EntityWorld::init(&chunkmap);
    makeEntityPrototypes(*ecs);
    World::setEventCallbacks(*ecs, chunkmap);

    /* Init Items */
    {
        using namespace items::ITC;
        static constexpr auto itemComponentInfo = ECS::getComponentInfoList<ITEM_COMPONENTS_LIST>();
        itemManager = ItemManager(ArrayRef(itemComponentInfo), ItemTypes::Count);
    }

    makeItemPrototypes(itemManager);
    loadTileData(itemManager, textureManager);

    /* Init Player */
    player = Player(ecs, Vec2(0, 0), itemManager);

    ItemStack startInventory[] = {
        ItemStack(items::Prototypes::SandGun::make(itemManager)),
        ItemStack(items::Prototypes::Grenade::make(itemManager), 67),
        ItemStack(items::Prototypes::Tile::make(itemManager, TileTypes::Grass), 64),
        ItemStack(items::Prototypes::Tile::make(itemManager, TileTypes::Sand), 24),
        ItemStack(items::Prototypes::Tile::make(itemManager, TileTypes::TransportBelt), 200),
    };

    Inventory* playerInventory = player.inventory();
    for (int i = 0; i < (int)(sizeof(startInventory) / sizeof(ItemStack)); i++) {
        playerInventory->addItemStack(startInventory[i]);
    }
}

void GameState::destroy() {
    chunkmap.destroy();
    ecs->destroy();
    delete ecs;
}

llvm::SmallVector<IVec2> raytraceDDA(const Vec2 start, const Vec2 end) {
    const IVec2 start_i = vecFloori(start);
    const IVec2 end_i = vecFloori(end);
    const int lineLength = (abs(end_i.x - start_i.x) + abs(end_i.y - start_i.y)) + 1;
    llvm::SmallVector<IVec2> line(lineLength, {}); // make small vector of size lineLength left uninitialized
    const Vec2 delta = end - start;
    const Vec2 dir = glm::normalize(delta);
    const Vec2 unitStep = {sqrt(1 + (dir.y / dir.x) * (dir.y / dir.x)), sqrt(1 + (dir.x / dir.y) * (dir.x / dir.y))};

    IVec2 mapCheck = start_i;
    Vec2 rayLength;
    IVec2 step;
    if (dir.x < 0) {
        step.x = -1;
        rayLength.x = (start.x - mapCheck.x) * unitStep.x;
    } else {
        step.x = 1;
        rayLength.x = ((mapCheck.x + 1) - start.x) * unitStep.x;
    }
    if (dir.y < 0) {
        step.y = -1;
        rayLength.y = (start.y - mapCheck.y) * unitStep.y;
    } else {
        step.y = 1;
        rayLength.y = ((mapCheck.y + 1) - start.y) * unitStep.y;
    }

    // have to check for horizontal and vertical lines because of divison by zero things

    // horizontal line
    if (start.y == end.y) {
        for (int i = 0; i < lineLength; i++) {
            line[i] = mapCheck;
            mapCheck.x += step.x;
        }
    }
    // vertical line
    else if (start_i.x == end_i.x) {
        for (int i = 0; i < lineLength; i++) {
            line[i] = mapCheck;
            mapCheck.y += step.y;
        }
    }
    // any other line
    else {
        for (int i = 0; i < lineLength; i++) {
            line[i] = mapCheck;
            if (rayLength.x < rayLength.y) {
                mapCheck.x += step.x;
                rayLength.x += unitStep.x;
            } else {
                mapCheck.y += step.y;
                rayLength.y += unitStep.y;
            }
        }
    }
    
    return line;
}

void worldLineAlgorithm(Vec2 start, Vec2 end, const std::function<int(IVec2)>& callback) {
    Vec2 delta = end - start;
    Vec2 dir = glm::normalize(delta);

    IVec2 mapCheck = {(int)floor(start.x), (int)floor(start.y)};
    Vec2 unitStep = {sqrt(1 + (dir.y / dir.x) * (dir.y / dir.x)), sqrt(1 + (dir.x / dir.y) * (dir.x / dir.y))};
    Vec2 rayLength;
    IVec2 step;
    if (dir.x < 0) {
        step.x = -1;
        rayLength.x = (start.x - mapCheck.x) * unitStep.x;
    } else {
        step.x = 1;
        rayLength.x = ((mapCheck.x + 1) - start.x) * unitStep.x;
    }
    if (dir.y < 0) {
        step.y = -1;
        rayLength.y = (start.y - mapCheck.y) * unitStep.y;
    } else {
        step.y = 1;
        rayLength.y = ((mapCheck.y + 1) - start.y) * unitStep.y;
    }

    // horizontal line
    if (floor(start.y) == floor(end.y)) {
        int iterations = abs(floor(end.x) - mapCheck.x) + 1;
        for (int i = 0; i < iterations; i++) {
            if (callback(mapCheck)) {
                return;
            }
            mapCheck.x += step.x;
        }
        return;
    }
    // vertical line
    if (floor(start.x) == floor(end.x)) {
        int iterations = abs(floor(end.y) - mapCheck.y) + 1;
        for (int i = 0; i < iterations; i++) {
            if (callback(mapCheck)) {
                return;
            }
            mapCheck.y += step.y;
        }
        return;
    }

    int iterations = 0; // for making sure it doesn't go infinitely
    while (iterations < 300) {
        if (rayLength.x < rayLength.y) {
            mapCheck.x += step.x;
            rayLength.x += unitStep.x;
        } else {
            mapCheck.y += step.y;
            rayLength.y += unitStep.y;
        }

        if (callback(mapCheck)) {
            return;
        }

        if (mapCheck.x == (int)floor(end.x) && mapCheck.y == (int)floor(end.y)) {
            break;
        }

        iterations++;
    }
}

void forEachChunkContainingBounds(const ChunkMap* chunkmap, Boxf bounds, const std::function<void(ChunkData*)>& callback) {
    IVec2 minChunkPosition = toChunkPosition(bounds[0]);
    IVec2 maxChunkPosition = toChunkPosition(bounds[1]);
    for (int col = minChunkPosition.x; col <= maxChunkPosition.x; col++) {
        for (int row = minChunkPosition.y; row <= maxChunkPosition.y; row++) {
            IVec2 chunkPosition = {col, row};
            ChunkData* chunkdata = chunkmap->get(chunkPosition);
            if (chunkdata) {
                callback(chunkdata);
            }
        }
    }
}