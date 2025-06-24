#ifndef WORLD_ENTITIES_PROTOTYPES_BASIC_INCLUDED
#define WORLD_ENTITIES_PROTOTYPES_BASIC_INCLUDED

#include "ECS/Prototype.hpp"
#include "world/EntityWorld.hpp"
#include "world/entities/methods.hpp"

namespace World {

namespace Entities {


// struct EntityMaker : Entity {
//     EntityMaker(EntityWorld* ecs, ECS::PrototypeID prototype = PrototypeIDs::Default)
//     : Entity(ecs->New(prototype)) {}

//     template<class C>
//     void Add(EntityWorld* ecs, const C& component) {
//         ecs->Add<C>(*this, component);
//     }
// };

#define START_ENTITY() ecs->startMakingEntity(ID)

//namespace Prototypes {
    struct Player : PrototypeDecl<PrototypeIDs::Player> {

        Player(PrototypeManager& manager) : PrototypeDecl(manager) {
            using namespace EC::Proto;
            setName("player");
            add(EntityRenderLayer{RenderLayers::Player});
            add(Living{});
        }

        static Entity make(EntityWorld* ecs, Vec2 position, ItemManager& inventoryAllocator) {
            auto& e = START_ENTITY();

            e.Add<EC::Position>(position);
            e.Add<EC::Dynamic>(Vec2{-5,-5});
            e.Add<EC::ViewBox>({Vec2(0), Vec2(1.0f)});
            e.Add<EC::CollisionBox>({Vec2(0.05f), Vec2(0.9f)});
            e.Add<EC::Health>({300});
            e.Add<EC::Rotation>({0.0f});
            Inventory inventory = Inventory(&inventoryAllocator, PLAYER_INVENTORY_SIZE);
            e.Add<EC::Inventory>({inventory});
            EC::Render::Texture textures[] = {
                {TextureIDs::Player, RenderLayers::Player},
                {TextureIDs::PlayerShadow, RenderLayers::Shadows, 0.4f}
            };
            e.Add<EC::Render>(EC::Render(textures, 2));
            e.Add<EC::Special>({});
            e.Add<EC::Immortal>({});

            return e.make();
        }
    };

    struct Tree : PrototypeDecl<PrototypeIDs::Tree> {
        Tree(PrototypeManager& manager) : PrototypeDecl(manager) {
            using namespace EC::Proto;
            setName("tree");
            add<Nature>({});
        }

        static EntityMaker& make(EntityBuilder builder, Vec2 position, Vec2 size) {
            auto& e = builder.New(ID);
            e.Add(EC::Health(100.0f));
            e.Add(EC::Growth(0.0f));
            Box texBox = Box{Vec2(-0.1), Vec2(1.2)};
            EC::Render::Texture textures[] = {
                {TextureIDs::TreeBottom, RenderLayers::Tilemap, texBox},
                {TextureIDs::TreeTop, RenderLayers::Trees, texBox, 0.8f}
            };
            e.Add(EC::Render(textures, 2));
            e.Add(EC::Position(position));
            
            e.Add(EC::ViewBox(Box{Vec2(-0.5) * size, Vec2(size)}));
            e.Add(EC::CollisionBox(Box{Vec2(-0.4) * size, Vec2(0.8) * size}));
            return e;
        }
    };

    struct ItemStack : PrototypeDecl<PrototypeIDs::ItemStack> {
        ItemStack(PrototypeManager& manager) : PrototypeDecl(manager) {
            setName("item-stack");
            add(EC::Proto::EntityRenderLayer{RenderLayer::Items});
        }

        static EntityMaker& make(EntityBuilder ecs, Vec2 position, ::ItemStack item, const ItemManager& itemManager) {
            auto& e = ecs.New(ID);
            e.Add<EC::Position>({position});
            e.Add<EC::ViewBox>({Box{Vec2(0), Vec2(1)}});
            e.Add<EC::ItemStack>({item});
            e.Add<EC::Grabbable>({item});
            e.Add<EC::Render>(EC::Render(itemManager.getComponent<ITC::Display>(item.item)->inventoryIcon, RenderLayers::Items));
            return e;
        }
    };

    struct Monster : PrototypeDecl<PrototypeIDs::Monster> {
        Monster(PrototypeManager& manager) : PrototypeDecl(manager) {
            using namespace EC::Proto;
            setName("monster");
            add<Hostile>({});
        }

        static EntityMaker& make(EntityWorld* ecs, Vec2 pos) {
            auto& e = START_ENTITY();
            e.setPos(pos);
            e.Add(EC::Render(TextureIDs::Player, RenderLayers::Player));
            e.Add(EC::CollisionBox(Vec2(0.05f), Vec2(0.9f)));
            e.Add<EC::Rotation>({0.0f});
            e.Add<EC::Health>({300});
            return e;
        }
    };

    struct Spider : PrototypeDecl<PrototypeIDs::Monster> {
        Spider(PrototypeManager& manager) : PrototypeDecl(manager) {
            using namespace EC::Proto;
            setName("spider");
            add<Hostile>({});
        }

        static EntityMaker& make(EntityBuilder& builder, Vec2 pos) {
            auto& e = builder.New(PrototypeIDs::Monster);
            e.setPos(pos);
            e.Add(EC::Render(TextureIDs::Player, RenderLayers::Player));
            e.Add(EC::CollisionBox(Vec2(0.05f), Vec2(0.9f)));
            e.Add<EC::Rotation>({0.0f});
            e.Add<EC::Health>({300});
            return e;
        }
    };

    template<int Tid>
    struct Explosive : PrototypeDecl<Tid> {
        Explosive(PrototypeManager& manager) : PrototypeDecl<Tid>(manager) {

        }

        static EntityMaker& make(EntityBuilder ecs, Vec2 position, Vec2 size, TextureID texture, const EC::Explosion& explosion, float startRotation = 0.0f) {
            auto& e = ecs.New(Tid);

            Box box = {{0.0f,0.0f}, size};
            e.Add<EC::Position>(position);
            e.Add<EC::Dynamic>(position);
            e.Add<EC::ViewBox>({box});
            e.Add<EC::Explosive>(EC::Explosive(explosion));
            e.Add<EC::Render>(EC::Render(texture, RenderLayers::Particles));
            e.Add<EC::Rotation>({startRotation});

            return e;
        }
    };

    struct Grenade : Explosive<PrototypeIDs::Grenade> {
        Grenade(PrototypeManager& manager) : Explosive<PrototypeIDs::Grenade>(manager) {
            setName("grenade");
        }

        static EntityMaker& make(EntityBuilder ecs, Vec2 position) {
            const auto grenadeExplosion = EC::Explosion(4, 25, 1.0f, 60);
            auto& e = Explosive::make(ecs, position, {0.5,0.5}, TextureIDs::Grenade, grenadeExplosion);
            return e;
        }
    };

    const EC::Explosion airstrikeExplosion = EC::Explosion(8, 10000, 1.0f, 30);

    struct Airstrike : Explosive<PrototypeIDs::Airstrike> {
        Airstrike(PrototypeManager& manager) : Explosive<PrototypeIDs::Airstrike>(manager) {
            setName("airstrike");
        }

        static Entity make(EntityWorld* ecs, Vec2 position, Vec2 size, Vec2 target) {
            Entity airstrike = Explosive<PrototypeIDs::Airstrike>::make(ecs, position, size, TextureIDs::Grenade, airstrikeExplosion)();
            Entities::throwEntity(*ecs, airstrike, target, 0.2f);
            return airstrike;
        }
    };
//}



}

}

#undef START_ENTITY

#endif