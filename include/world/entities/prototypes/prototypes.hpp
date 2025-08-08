#ifndef WORLD_ENTITIES_PROTOTYPES_BASIC_INCLUDED
#define WORLD_ENTITIES_PROTOTYPES_BASIC_INCLUDED

#include "ECS/Prototype.hpp"
#include "world/EntityWorld.hpp"
#include "world/entities/methods.hpp"
#include "utils/random_random.hpp"

namespace ECS {
    struct CommandRegistry {
        MutArrayRef<char> buffer;
        Uint16 bufferUsed;

        template<class C>
        void Add(C component) {

        }
    };
}

namespace World {

struct EntityMaker {
    EntityWorld* ecs = nullptr;
    
    Entity entity = NullEntity;
    ECS::Signature components = {0};
    ECS::PrototypeID prototype = -1;

    Uint16 bufUsed = 0;
    MutArrayRef<char> buf;

    static constexpr size_t MaxComponentMemory = std::numeric_limits<Uint16>::max();
    struct ComponentValue {
        ECS::ComponentID id;
        Uint16 valueBufPos;
    };
    SmallVector<ComponentValue> componentValues;

    EntityMaker() {}

    EntityMaker(EntityWorld* ecs);
    
    EntityMaker(const EntityMaker& other) = delete;

    EntityMaker& operator=(const EntityMaker& other);

    // if an entity was in progress or already made, that data will be replaced with a fresh start
    void start(ECS::PrototypeID prototype) {
        clear();
        this->prototype = prototype;
    }

    template<class C>
    void Add(const C& value) {
        components.setComponent<C>();
        Uint16 offset = (Uint16)((uintptr_t)buf.data() % alignof(C));
        if (bufUsed + offset + sizeof(C) > buf.size()) {
            assert(0 && "Ran out of buffer space");
        }
        *((C*)(buf.data() + bufUsed + offset)) = value;
        componentValues.push_back(ComponentValue{C::ID, (Uint16)(bufUsed + offset)});
        bufUsed += sizeof(C) + offset;
    }

    template<class... Cs>
    void Add(const Cs&... components) {
        FOR_EACH_VAR_TYPE(Add<Cs>(components));
    }

    void setPos(Vec2 pos) {
        Add<EC::Position>(pos);
    }

    Entity make();

    void make(Entity* pEntity) {
        *pEntity = make();
    }

    Entity operator()() {
        return make();
    }

    void clear();
};

namespace Entities {


// struct EntityMaker : Entity {
//     EntityMaker(EntityWorld* ecs, ECS::PrototypeID prototype = PrototypeIDs::Default)
//     : Entity(ecs->New(prototype)) {}

//     template<class C>
//     void Add(EntityWorld* ecs, const C& component) {
//         ecs->Add<C>(*this, component);
//     }
// };

#define START_ENTITY() ECS::EntityMaker(ID)

using EntityOut = ECS::EntityCommandOutput;

//namespace Prototypes {
    struct Zombie : PrototypeDecl<PrototypeIDs::Monster> {
        Zombie(PrototypeManager& manager) : PrototypeDecl(manager) {
            using namespace EC::Proto;
            setName("zombie");
            add(EntityRenderLayer{RenderLayers::Player});

            // Maker.setMake([](EntityMaker& m, Vec2 position){
            //     m.Add<EC::Position>(position);
            // });

            // Maker.Add<EC::Health>({100.0f});
            // e.constructor = [](auto output, Entity entity, Vec2 position, Vec2 size){
            //     output.Add(EC::Position{position});
            //     output.Add(EC::ViewBox::BottomLeft(size));
            // }
        }
    };

    struct Player : PrototypeDecl<PrototypeIDs::Player> {

        Player(PrototypeManager& manager) : PrototypeDecl(manager) {
            using namespace EC::Proto;
            setName("player");
            add(EntityRenderLayer{RenderLayers::Player});
            add(Living{});
        }

        static Entity make(EntityOut output, Vec2 position, ItemManager& inventoryAllocator) {
            auto e = START_ENTITY();

            e.Add<EC::Position>(position);
            e.Add<EC::Dynamic>(Vec2{-5,-5});
            e.Add<EC::ViewBox>({Vec2(0), Vec2(PLAYER_DIAMETER)});
            e.Add<EC::CollisionBox>({Vec2(0.05f), Vec2(PLAYER_DIAMETER - 0.1f)});
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

            return e.output(output);
        }
    };

    struct Tree : PrototypeDecl<PrototypeIDs::Tree> {
        Tree(PrototypeManager& manager) : PrototypeDecl(manager) {
            using namespace EC::Proto;
            setName("tree");
            add<Nature>({});
        }

        static Entity make(EntityOut out, Vec2 position, Vec2 size) {
            auto e = START_ENTITY();
            e.Add(
                EC::Health(100.0f),
                EC::Growth(0.0f));
            Box texBox = Box{Vec2(-0.1), Vec2(1.2)};
            EC::Render::Texture textures[] = {
                {TextureIDs::TreeBottom, RenderLayers::Tilemap, texBox},
                {TextureIDs::TreeTop, RenderLayers::Trees, texBox, 0.8f}
            };
            e.Add(EC::Render(textures, 2));
            e.Add(EC::Position(position));
            
            e.Add(EC::ViewBox(Box{Vec2(-0.5) * size, Vec2(size)}));
            e.Add(EC::CollisionBox(Box{Vec2(-0.4) * size, Vec2(0.8) * size}));
            return e.output(out);
        }
    };

    struct ItemStack : PrototypeDecl<PrototypeIDs::ItemStack> {
        ItemStack(PrototypeManager& manager) : PrototypeDecl(manager) {
            setName("item-stack");
            add(EC::Proto::EntityRenderLayer{RenderLayer::Items});
        }

        static Entity make(EntityOut out, Vec2 position, ::ItemStack item, const ItemManager& itemManager) {
            auto e = START_ENTITY();
            e.Add<EC::Position>({position});
            e.Add<EC::ViewBox>({Box{Vec2(0), Vec2(1)}});
            e.Add<EC::ItemStack>({item});
            e.Add<EC::Grabbable>({item});
            e.Add<EC::Render>(EC::Render(itemManager.getComponent<ITC::Display>(item.item)->inventoryIcon, RenderLayers::Items));
            return e.output(out);
        }
    };

    struct Monster : PrototypeDecl<PrototypeIDs::Monster> {
        Monster(PrototypeManager& manager) : PrototypeDecl(manager) {
            using namespace EC::Proto;
            setName("monster");
            add<Hostile>({});
        }

        static Entity make(EntityOut out, Vec2 pos) {
            auto e = START_ENTITY();
            e.Add(EC::Position(pos));
            e.Add(EC::Render(TextureIDs::Player, RenderLayers::Player));
            e.Add(EC::CollisionBox(Vec2(0.05f), Vec2(0.9f)));
            e.Add<EC::Rotation>({0.0f});
            e.Add<EC::Health>({300});
            return e.output(out);
        }
    };

    struct Spider : PrototypeDecl<PrototypeIDs::Monster> {
        Spider(PrototypeManager& manager) : PrototypeDecl(manager) {
            using namespace EC::Proto;
            setName("spider");
            add<Hostile>({});
        }

        static Entity make(EntityOut out, Vec2 pos) {
            auto e = START_ENTITY();
            e.Add(EC::Position(pos));
            e.Add(EC::Render(TextureIDs::Player, RenderLayers::Player));
            e.Add(EC::CollisionBox(Vec2(0.05f), Vec2(0.9f)));
            e.Add<EC::Rotation>({0.0f});
            e.Add<EC::Health>({300});
            return e.output(out);
        }
    };

    template<int Tid>
    struct Explosive : PrototypeDecl<Tid> {
        Explosive(PrototypeManager& manager) : PrototypeDecl<Tid>(manager) {

        }

        static Entity make(EntityOut out, Vec2 position, Vec2 size, TextureID texture, const EC::Explosion& explosion, float startRotation = 0.0f) {
            auto e = ECS::EntityMaker(Tid);

            Box box = {{0.0f,0.0f}, size};
            e.Add<EC::Position>(position);
            e.Add<EC::Dynamic>(position);
            e.Add<EC::ViewBox>({box});
            e.Add<EC::Explosive>(EC::Explosive(explosion));
            e.Add<EC::Render>(EC::Render(texture, RenderLayers::Particles));
            e.Add<EC::Rotation>({startRotation});

            return e.output(out);
        }
    };

    struct Grenade : Explosive<PrototypeIDs::Grenade> {
        Grenade(PrototypeManager& manager) : Explosive<PrototypeIDs::Grenade>(manager) {
            setName("grenade");
        }

        static Entity make(EntityOut out, Vec2 position) {
            const auto grenadeExplosion = EC::Explosion(4, 25, 1.0f, 60);
            return Explosive::make(out, position, {0.5,0.5}, TextureIDs::Grenade, grenadeExplosion);
        }
    };

    const EC::Explosion airstrikeExplosion = EC::Explosion(8, 10000, 1.0f, 30);

    struct Airstrike : Explosive<PrototypeIDs::Airstrike> {
        Airstrike(PrototypeManager& manager) : Explosive<PrototypeIDs::Airstrike>(manager) {
            setName("airstrike");
        }

        static Entity make(EntityWorld* ecs, Vec2 position, Vec2 size, Vec2 target) {
            Entity airstrike = Explosive<PrototypeIDs::Airstrike>::make(ecs, position, size, TextureIDs::Grenade, airstrikeExplosion);
            Entities::throwEntity(*ecs, airstrike, target, 0.2f);
            return airstrike;
        }
    };

    struct TextBox : PrototypeDecl<PrototypeIDs::TextBox> {
        TextBox(PrototypeManager& manager) : PrototypeDecl(manager) {
            setName("textbox");
        }

        static Entity make(EntityOut out, Vec2 position, const EC::Text& textComponent) {
            auto e = START_ENTITY();
            e.Add(EC::Position(position));
            e.Add(EC::ViewBox::BottomLeft(textComponent.box.size));
            e.Add(EC::Render(TextureIDs::Null, RenderLayers::Text));
            e.Add(textComponent);
            return e.output(out);
        }
    };

    struct Laser : PrototypeDecl<PrototypeIDs::Laser> {
        Laser(PrototypeManager& manager) : PrototypeDecl(manager) {
            setName("laser");


            // constructor.setConstruct([](EntityConstructor<EC::Position>* constructor, EC::Position position) {
            //     constructor->Add<EC::Position>(position);

            // });
        }

        static Entity make(ECS::EntityCommandOutput output, Vec2 position) {
            auto m = START_ENTITY();
            m.Add(EC::Health(100.0f));
            m.Add(EC::Position{position});
            return m.output(output);
        }

        // static void construct(EntityConstructor<EC::Position>* constructor, EC::Position position) {
        //     // constructor->Set<EC::Position>(position);
        // }

        // static EntityConstructor<EC::Position> constructor{ID, construct};
    };

    struct Cannon : PrototypeDecl<PrototypeIDs::Cannon> {
        Cannon(PrototypeManager& manager) : PrototypeDecl(manager) {
            setName("cannon");
            add(EC::Proto::GunProto{
                .cooldown = 60,
                .projectile = PrototypeIDs::Laser,
            });
        }

        static Entity make(EntityOut out, Vec2 position) {
            //auto e = START_ENTITY();
            auto e = ECS::EntityMaker(ID);
            e.Add(EC::Position{position});
            e.Add(EC::ViewBox::BottomLeft({1, 1}));
            e.Add(EC::Render{TextureIDs::Buildings::TransportBelt, RenderLayers::Buildings});
            e.Add(EC::Gun(60, &Laser::make));
            return e.output(out);
        }
    };
//}



}

}

#undef START_ENTITY

#endif