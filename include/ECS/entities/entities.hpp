#ifndef ENTITIES_INCLUDED
#define ENTITIES_INCLUDED

#include "ECS/ECS.hpp"
#include "ECS/components/components.hpp"
#include "rendering/textures.hpp"
#include "ECS/EntityWorld.hpp"
#include "utils/vectors.hpp"

using ExplosionCreateFunction = Entity(EntityWorld*, Vec2);

using EntityTypeID = Uint32;

namespace EntityIDs {
    enum IDs {
        Grenade,
        Banana
    };
}

namespace Entities {

    #define MARK_START_ENTITY_CREATION(ecs) {ecs->StartDeferringEvents();}
    #define MARK_END_ENTITY_CREATION(ecs) {ecs->StopDeferringEvents();}

    using ExplosionType = EntityT<EC::Position, EC::Explosion>;
    struct Explosion : ExplosionType {
        Explosion(EntityWorld* ecs, Vec2 position, const EC::Explosion& explosionComponent) : ExplosionType(ecs, "explosion") {
            MARK_START_ENTITY_CREATION(ecs);

            Add<EC::Position>(ecs, position);
            Add<EC::Explosion>(ecs, explosionComponent);

            MARK_END_ENTITY_CREATION(ecs);
        }
    };

    using ExplosiveType = EntityT<EC::Position, EC::Size, EC::Explosive, EC::Render, EC::Rotation>;
    struct Explosive : ExplosiveType {
        Explosive(EntityWorld* ecs, Vec2 position, Vec2 size, TextureID texture, const EC::Explosion& explosion, float startRotation = 0.0f)
        : ExplosiveType(ecs, "explosive") {
            MARK_START_ENTITY_CREATION(ecs);

            Add<EC::Position>(ecs, position);
            Add<EC::Size>(ecs, {size.x, size.y});
            Add<EC::Explosive>(ecs, EC::Explosive(explosion));
            Add<EC::Render>(ecs, EC::Render(texture, RenderLayer::Particles));
            Add<EC::Rotation>(ecs, {startRotation});

            MARK_END_ENTITY_CREATION(ecs);
        }
    };

    Explosive Grenade(EntityWorld* ecs, Vec2 position, Vec2 size);
    Explosive Airstrike(EntityWorld* ecs, Vec2 position, Vec2 size, Vec2 target);
    Explosive ThrownGrenade(EntityWorld* ecs, Vec2 position, Vec2 target);

    Entity Chest(EntityWorld* ecs, Vec2 position, int inventorySize, int width, int height);

    Entity Particle(EntityWorld* ecs, Vec2 position, Vec2 size, EC::Render render, EC::Motion motion);

    Entity Inserter(EntityWorld* ecs, Vec2 position, int reach, IVec2 inputTile, IVec2 outputTile);

    Entity Enemy(EntityWorld* ecs, Vec2 position, EntityT<EC::Position> following);

    #define ENTITY_PLAYER_COMPONENTS EC::Health, EC::Nametag,\
        EC::Rotation, EC::Size, EC::Inventory,\
        EC::Render, EC::Position, EC::Immortal
    struct Player : public EntityT<ENTITY_PLAYER_COMPONENTS> {
        Player(EntityWorld* ecs, Vec2 position) : EntityT<ENTITY_PLAYER_COMPONENTS>(ecs, "player") {
            MARK_START_ENTITY_CREATION(ecs);

            Add<EC::Position>(ecs, position);
            Add<EC::Health>(ecs, {1000});
            Add<EC::Rotation>(ecs, {0.0f});
            Add<EC::Size>(ecs, {0.8f, 0.8f});
            Inventory inventory = Inventory(PLAYER_INVENTORY_SIZE);
            Add<EC::Inventory>(ecs, {inventory});
            Add<EC::Render>(ecs, {TextureIDs::Player, RenderLayer::Player});
            Add<EC::Immortal>(ecs, {});

            MARK_END_ENTITY_CREATION(ecs);
        }
    };

    #define ENTITY_ITEMSTACK_COMPONENTS EC::Position, EC::Size, EC::Render,\
        EC::Grabable, EC::ItemStack
    struct ItemStack : EntityT<ENTITY_ITEMSTACK_COMPONENTS> {
        //using EntityT<ENTITY_ITEMSTACK_COMPONENTS>::EntityT;
        ItemStack(EntityWorld* ecs, Vec2 position, ::ItemStack item) : EntityT<ENTITY_ITEMSTACK_COMPONENTS>(ecs, "itemstack") {
            MARK_START_ENTITY_CREATION(ecs);
            Add<EC::Position>(ecs, position);
            Add<EC::Size>(ecs, {0.5f, 0.5f});
            Add<EC::ItemStack>(ecs, ::ItemStack(item));
            Add<EC::Grabable>(ecs, {});
            Add<EC::Render>(ecs, EC::Render(ItemData[item.item].icon, RenderLayer::Items));
            MARK_END_ENTITY_CREATION(ecs);
        }
    };

    #define ENTITY_TRANSPORT_BELT_COMPONENTS EC::Health, EC::Rotation, EC::Rotatable,\
        EC::Render, EC::Position, EC::Size, EC::Transporter
    struct TransportBelt : EntityT<ENTITY_TRANSPORT_BELT_COMPONENTS> {
        TransportBelt(EntityWorld* ecs, Vec2 position) : EntityT<ENTITY_TRANSPORT_BELT_COMPONENTS>(ecs, "transport_belt") {
            MARK_START_ENTITY_CREATION(ecs);
            Add<EC::Health>(ecs, {100.0f});
            Add<EC::Position>(ecs, position);
            Add<EC::Size>(ecs, {1.0f, 1.0f});
            Add<EC::Render>(ecs, EC::Render(TextureIDs::Inserter, RenderLayer::Buildings));
            Add<EC::Rotation>(ecs, {0.0f});
            Add<EC::Rotatable>(ecs, EC::Rotatable(0.0f, 90.0f));
            Add<EC::Transporter>(ecs, {0.15f});
            MARK_END_ENTITY_CREATION(ecs);
        }
    };

    using TreeType = EntityT<
        EC::Health, EC::Growth, EC::Position,
        EC::Render, EC::Size, EC::Nametag,
        EC::Inventory
    >;
    struct Tree : TreeType {
        //using TreeType::TreeType;
        Tree(EntityWorld* ecs, Vec2 position, Vec2 size) : TreeType(ecs, "tree") {
            MARK_START_ENTITY_CREATION(ecs);
            Add(ecs, EC::Health(100.0f));
            Add(ecs, EC::Position(position));
            Add(ecs, EC::Growth(0.0f));
            Add(ecs, EC::Render(TextureIDs::Tree, RenderLayer::Trees));
            Add(ecs, EC::Size(size.x, size.y));
            Add(ecs, EC::Inventory(Inventory(10)));
            MARK_END_ENTITY_CREATION(ecs);
        }
    };
}

#endif