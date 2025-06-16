#ifndef ENTITIES_INCLUDED
#define ENTITIES_INCLUDED

#include "ECS/ECS.hpp"
#include "world/components/components.hpp"
#include "rendering/textures.hpp"
#include "world/EntityWorld.hpp"
#include "utils/vectors_and_rects.hpp"
#include "prototypes/prototypes.hpp"

namespace World {

using ExplosionCreateFunction = Entity(EntityWorld*, Vec2);

namespace Entities {

    #define MARK_START_ENTITY_CREATION(ecs) {ecs->StartDeferringEvents();}
    #define MARK_END_ENTITY_CREATION(ecs) {ecs->StopDeferringEvents();}

    struct EntityMaker : Entity {
        EntityMaker(EntityWorld* ecs, ECS::PrototypeID prototype = PrototypeIDs::Default)
        : Entity(ecs->New(prototype)) {}

        template<class C>
        void Add(EntityWorld* ecs, const C& component) {
            ecs->Add<C>(*this, component);
        }
    };

    struct Explosion : EntityMaker {
        Explosion(EntityWorld* ecs, Vec2 position, const EC::Explosion& explosionComponent)
        : EntityMaker(ecs) {
            MARK_START_ENTITY_CREATION(ecs);

            Add<EC::Point>(ecs, position);
            Add<EC::Explosion>(ecs, explosionComponent);

            MARK_END_ENTITY_CREATION(ecs);
        }
    };

    struct Explosive : EntityMaker {
        Explosive(EntityWorld* ecs, Vec2 position, Vec2 size, TextureID texture, const EC::Explosion& explosion, float startRotation = 0.0f)
        : EntityMaker(ecs, PrototypeIDs::Explosive) {
            MARK_START_ENTITY_CREATION(ecs);

            Box box = {{0.0f,0.0f}, size};
            Add<EC::Position>(ecs, position);
            Add<EC::Dynamic>(ecs, position);
            Add<EC::ViewBox>(ecs, {box});
            Add<EC::Explosive>(ecs, EC::Explosive(explosion));
            Add<EC::Render>(ecs, EC::Render(texture, RenderLayers::Particles));
            Add<EC::Rotation>(ecs, {startRotation});

            MARK_END_ENTITY_CREATION(ecs);
        }
    };

    Explosive Grenade(EntityWorld* ecs, Vec2 position);
    Explosive Airstrike(EntityWorld* ecs, Vec2 position, Vec2 size, Vec2 target);
    Explosive ThrownGrenade(EntityWorld* ecs, Vec2 position, Vec2 target);

    struct Player : EntityMaker {
        Player(EntityWorld* ecs, Vec2 position, ItemManager& inventoryAllocator)
        : EntityMaker(ecs) {
            MARK_START_ENTITY_CREATION(ecs);

            Add<EC::Position>(ecs, position);
            Add<EC::Dynamic>(ecs, {position});
            Add<EC::ViewBox>(ecs, {Vec2(0), Vec2(1.0f)});
            Add<EC::CollisionBox>(ecs, EC::CollisionBox(Vec2(0.05f), Vec2(0.9f)));
            Add<EC::Health>(ecs, {300});
            Add<EC::Rotation>(ecs, {0.0f});
            Inventory inventory = Inventory(&inventoryAllocator, PLAYER_INVENTORY_SIZE);
            Add<EC::Inventory>(ecs, {inventory});
            EC::Render::Texture textures[] = {
                {TextureIDs::Player, RenderLayers::Player},
                {TextureIDs::PlayerShadow, RenderLayers::Shadows, 0.4f}
            };
            Add<EC::Render>(ecs, EC::Render(textures, 2));
            Add<EC::Special>(ecs, {});
            Add<EC::Immortal>(ecs, {});

            MARK_END_ENTITY_CREATION(ecs);
        }
    };

    #define ENTITY_ITEMSTACK_COMPONENTS EC::ViewBox, EC::Render,\
        EC::Grabable, EC::ItemStack
    struct ItemStack : EntityMaker {
        //using EntityT<ENTITY_ITEMSTACK_COMPONENTS>::EntityT;
        ItemStack(EntityWorld* ecs, Vec2 position, ::ItemStack item, const ItemManager& itemManager) : EntityMaker(ecs) {
            MARK_START_ENTITY_CREATION(ecs);
            Add<EC::Position>(ecs, {position});
            Add<EC::ViewBox>(ecs, {Box{Vec2(0), Vec2(1)}});
            Add<EC::ItemStack>(ecs, {item});
            Add<EC::Grabbable>(ecs, {item});
            Add<EC::Render>(ecs, EC::Render(itemManager.getComponent<ITC::Display>(item.item)->inventoryIcon, RenderLayers::Items));
            
            MARK_END_ENTITY_CREATION(ecs);
        }
    };

    #define ENTITY_TRANSPORT_BELT_COMPONENTS EC::Health, EC::Rotation, EC::Rotatable,\
        EC::Render, EC::ViewBox, EC::Transporter, EC::Position
    struct TransportBelt : EntityMaker {
        TransportBelt(EntityWorld* ecs, Vec2 position) : EntityMaker(ecs) {
            MARK_START_ENTITY_CREATION(ecs);
            Add<EC::Health>(ecs, {100.0f});
            Add<EC::Position>(ecs, position);
            Add<EC::ViewBox>(ecs, {Box{Vec2(0), Vec2(1)}});
            Add<EC::Render>(ecs, EC::Render(TextureIDs::Buildings::TransportBelt, RenderLayers::Buildings));
            Add<EC::Rotation>(ecs, {0.0f});
            Add<EC::Rotatable>(ecs, EC::Rotatable(0.0f, 90.0f));
            Add<EC::Transporter>(ecs, {0.15f});
            MARK_END_ENTITY_CREATION(ecs);
        }
    };

    

    #define ENTITY_TRANSPORT_LINE_COMPONENTS EC::Position
    struct TransportLine : EntityMaker {
        TransportLine(EntityWorld* ecs, Vec2 position) : EntityMaker(ecs) {
            MARK_START_ENTITY_CREATION(ecs);

            Add<EC::Position>(ecs, position);
            //Add<EC::TransportLineEC(ecs, {});
            
            MARK_END_ENTITY_CREATION(ecs);
        }
    };

    struct Tree : EntityMaker {
        //using TreeType::TreeType;
        Tree(EntityWorld* ecs, Vec2 position, Vec2 size) : EntityMaker(ecs) {
            MARK_START_ENTITY_CREATION(ecs);
            Add(ecs, EC::Health(100.0f));
            Add(ecs, EC::Growth(0.0f));
            Box texBox = Box{Vec2(-0.1), Vec2(1.2)};
            EC::Render::Texture textures[] = {
                {TextureIDs::TreeBottom, RenderLayers::Tilemap, texBox},
                {TextureIDs::TreeTop, RenderLayers::Trees, texBox, 0.8f}
            };
            Add(ecs, EC::Render(textures, 2));
            Add(ecs, EC::Position(position));
            
            Add(ecs, EC::ViewBox(Box{Vec2(-0.5) * size, Vec2(size)}));
            Add(ecs, EC::CollisionBox(Box{Vec2(-0.4) * size, Vec2(0.8) * size}));
            MARK_END_ENTITY_CREATION(ecs);
        }
    };
}

}

#endif