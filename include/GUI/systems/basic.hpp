#ifndef GUI_SYSTEMS_BASIC_INCLUDED
#define GUI_SYSTEMS_BASIC_INCLUDED

#include "ECS/system.hpp"
#include "GUI/components.hpp"
#include "rendering/gui.hpp"

namespace GUI {

namespace Systems {

using namespace ECS::System;

using CopyNamesJob = CopyComponentArrayJob<GUI::EC::Name>;

template<typename Job>
IJob* cloneJob(const Job& job) {
    return new Job(job);
}

JOB_PARALLEL_FOR(EnforceMaxSizeJob) {

    ComponentArray<GUI::EC::DisplayBox> displayBox;
    ComponentArray<const GUI::EC::SizeConstraint> maxSize;

    const Entity* entity;

    void Init(JobData& data) {
        data.getComponentArray(displayBox);
        data.getComponentArray(maxSize);
        entity = data.getEntityArray();
    }

    void Execute(int N) {
        if (displayBox[N].box.size.x > maxSize[N].maxSize.x) displayBox[N].box.size.x = maxSize[N].maxSize.x;
        if (displayBox[N].box.size.y > maxSize[N].maxSize.y) displayBox[N].box.size.y = maxSize[N].maxSize.y;   
    }
};

JOB_PARALLEL_FOR(EnforceMinSizeJob) {
    ComponentArray<GUI::EC::DisplayBox> displayBox;
    ComponentArray<const GUI::EC::SizeConstraint> minSize;

    void Init(JobData& data) {
        data.getComponentArray(displayBox);
        data.getComponentArray(minSize);
    }

    void Execute(int N) {
        if (displayBox[N].box.size.x < minSize[N].minSize.x) displayBox[N].box.size.x = minSize[N].minSize.x;
        if (displayBox[N].box.size.y < minSize[N].minSize.y) displayBox[N].box.size.y = minSize[N].minSize.y;
    }
};


struct SizeConstraintSystem : ISystem {
    static constexpr ComponentGroup<
        ReadWrite<EC::DisplayBox>,
        ReadOnly<EC::SizeConstraint>
    > group;

    SizeConstraintSystem(SystemManager& manager) : ISystem(manager) {
        
    }

    void ScheduleJobs() {
        Schedule(group, EnforceMaxSizeJob());
    }
};

JOB_PARALLEL_FOR(GetViewLevelsJob) {
    ComponentArray<const EC::ViewBox> viewbox;
    MutArrayRef<GUI::RenderLevel> levels;

    GetViewLevelsJob(MutArrayRef<GUI::RenderLevel> levels) : levels(levels) {}

    void Init(JobData& data) {
        data.getComponentArrays(viewbox);
    }

    void Execute(int N) {
        levels[N] = viewbox[N].level;
    }
};

struct QuadAndHeight {
    QuadRenderer::Quad quad;
    GUI::RenderHeight height;
};

JOB_PARALLEL_FOR(BoxToQuadJob) {
    ComponentArray<const EC::DisplayBox> displayBox;

    MutArrayRef<QuadAndHeight> quads;

    BoxToQuadJob(MutArrayRef<QuadAndHeight> quadsArray) : quads(quadsArray) {
        
    }

    void Init(JobData& data) {
        data.getComponentArray(displayBox);
    }

    void Execute(int N) {
        Box box = displayBox[N].box;
        Vec2 min = box.min;
        Vec2 max = box.max();

        quads[N].quad[0].pos = {min.x, min.y};
        quads[N].quad[1].pos = {min.x, max.y};
        quads[N].quad[2].pos = {max.x, max.y};
        quads[N].quad[3].pos = {max.x, min.y};
        quads[N].height = displayBox[N].height;
    }
};

JOB_PARALLEL_FOR(ColorToQuadJob) {
    ComponentArray<const EC::Background> background;

    MutArrayRef<QuadAndHeight> quads;

    ColorToQuadJob(MutArrayRef<QuadAndHeight> quadsArray)
    : quads(quadsArray) {}

    void Init(JobData& data) {
        data.getComponentArray(background);
    }

    void Execute(int N) {
        SDL_Color color = background[N].color;

        for (int i = 0; i < 4; i++) {
            quads[N].quad[i].color = color;
            quads[N].quad[i].texCoord = QuadRenderer::NullCoord;
        }
    }
};

JOB_SINGLE_THREADED(BufferQuadJob) {
    ComponentArray<const EC::ViewBox> viewbox;

    ArrayRef<QuadAndHeight> quads;

    GuiRenderer* guiRenderer;

    BufferQuadJob(ArrayRef<QuadAndHeight> quads, GuiRenderer* guiRenderer)
    : quads(quads), guiRenderer(guiRenderer) {}

    void Init(JobData& data) {
        data.getComponentArray(viewbox);
    }

    void Execute(int N) {
        const QuadRenderer::Quad& quad = quads[N].quad;
        const GUI::RenderHeight height = quads[N].height;
        guiRenderer->quad->render(quad, height);
        // TODO: optimize
    }
};

using BorderQuads = std::pair<std::array<QuadRenderer::Quad, 4>, GUI::RenderHeight>;

JOB_SINGLE_THREADED(BufferBordersJob) {
    ArrayRef<BorderQuads> quads;

    GuiRenderer* guiRenderer;

    BufferBordersJob(ArrayRef<BorderQuads> quads, GuiRenderer* guiRenderer)
    : quads(quads), guiRenderer(guiRenderer) {}

    void Init(JobData& data) {
        
    }

    void Execute(int N) {
        const auto& borderQuads = quads[N];
        guiRenderer->quad->render(borderQuads.first, borderQuads.second);
    }
};

JOB_PARALLEL_FOR(BorderQuadsJob) {
    MutArrayRef<BorderQuads> quads;
    ComponentArray<const EC::DisplayBox> displayBox;
    ComponentArray<const EC::Border> border;

    BorderQuadsJob(MutArrayRef<BorderQuads> quads)
    : quads(quads) {}

    QuadRenderer::Quad colorRect(glm::vec2 min, glm::vec2 max, SDL_Color color) {
        return QuadRenderer::Quad{{
            {glm::vec2{min.x, min.y}, color, QuadRenderer::NullCoord},
            {glm::vec2{min.x, max.y}, color, QuadRenderer::NullCoord},
            {glm::vec2{max.x, max.y}, color, QuadRenderer::NullCoord},
            {glm::vec2{max.x, min.y}, color, QuadRenderer::NullCoord}
        }};
    }

    void Init(JobData& data) {
        data.getComponentArrays(displayBox, border);
    }

    void Execute(int N) {
        Box box = displayBox[N].box;
        Vec2 min = box.min;
        Vec2 max = box.max();

        SDL_Color color = border[N].color;
        Vec2 strokeIn = border[N].strokeIn;
        Vec2 strokeOut = border[N].strokeOut;
        
        quads[N].first[0] = colorRect({min.x - strokeOut.x, min.y - strokeOut.y}, {max.x -  strokeIn.x, min.y +  strokeIn.y}, color);
        quads[N].first[1] = colorRect({max.x -  strokeIn.x, min.y - strokeOut.y}, {max.x + strokeOut.x, max.y -  strokeIn.y}, color);
        quads[N].first[2] = colorRect({max.x + strokeOut.x, max.y -  strokeIn.y}, {min.x +  strokeIn.x, max.y + strokeOut.y}, color);
        quads[N].first[3] = colorRect({min.x - strokeOut.x, max.y + strokeOut.y}, {min.x +  strokeIn.x, min.y +  strokeIn.y}, color);
        quads[N].second = displayBox[N].height;
    }
};

struct RenderBackgroundSystem : ISystem {
    static constexpr ComponentGroup<
        ReadOnly<EC::ViewBox>,
        ReadOnly<EC::DisplayBox>,
        ReadOnly<EC::Background>,
        Subtract<EC::Hidden>
    > backgroundGroup;

    static constexpr ComponentGroup<
        ReadOnly<EC::ViewBox>,
        ReadOnly<EC::DisplayBox>,
        ReadOnly<EC::Border>,
        Subtract<EC::Hidden>
    > borderGroup;


    GuiRenderer* guiRenderer;

    RenderBackgroundSystem(SystemManager& manager, GuiRenderer* guiRenderer)
    : ISystem(manager), guiRenderer(guiRenderer) {}


    // void ScheduleJobsO() {
    //     auto levelsBg = makeTempGroupArray<GUI::RenderLevel>(backgroundGroup);
    //     Schedule(backgroundGroup, InitializeArrayJob<GUI::RenderLevel>(levelsBg, GUI::RenderLevel::Null));
    //     auto quadsBg = makeTempGroupArray<QuadAndHeight>(backgroundGroup);
        
    //     auto bufferQuadsJob = 
    //     Schedule(backgroundGroup, BufferQuadJob(quadsBg, levelsBg, guiRenderer),
    //         Schedule(backgroundGroup, BoxToQuadJob(quadsBg),
    //            {Schedule(backgroundGroup, GetViewLevelsJob(levelsBg)),
    //             Schedule(backgroundGroup, ColorToQuadJob(quadsBg))}
    //         )
    //     );

    //     auto levelsBd = makeTempGroupArray<GUI::RenderLevel>(borderGroup);
    //     auto quadsBd = makeTempGroupArray<BorderQuads>(borderGroup);

    //     auto bufferBordersJob = 
    //     Schedule(borderGroup, BufferBordersJob(quadsBd, levelsBd, guiRenderer),
    //        {Schedule(borderGroup, GetViewLevelsJob(levelsBd)),
    //         Schedule(borderGroup, BorderQuadsJob(quadsBd))});

    //     AddDependency(bufferBordersJob, bufferQuadsJob);
    // }

    void ScheduleJobs() {
    auto quadsBg = makeTempGroupArray<QuadAndHeight>(backgroundGroup);
        auto bufferQuadsJob = Do({
            Schedule(backgroundGroup, ColorToQuadJob(quadsBg))
        }).Then(
            Schedule(backgroundGroup, BoxToQuadJob(quadsBg))
        ).Then(
            Schedule(backgroundGroup, BufferQuadJob(quadsBg, guiRenderer))
        ).handle();

        auto levelsBd = makeTempGroupArray<GUI::RenderLevel>(borderGroup);
        auto quadsBd = makeTempGroupArray<BorderQuads>(borderGroup);

        auto bufferBordersJob = Do(
            Schedule(borderGroup, InitializeArrayJob(levelsBd, GUI::RenderLevel::Null)) // i think totally unnecessary
        ).Then({
            Schedule(borderGroup, GetViewLevelsJob(levelsBd)),
            Schedule(borderGroup, BorderQuadsJob(quadsBd))
        })
        .Then(
            Schedule(borderGroup, BufferBordersJob(quadsBd, guiRenderer)))
        .handle();

        Conflict(bufferBordersJob, bufferQuadsJob);
    }
};

JOB_PARALLEL_FOR(MakeTextureQuadsJob) {
    MutArrayRef<QuadAndHeight> quads;
    ComponentArray<const EC::SimpleTexture> textures;
    ComponentArray<const EC::DisplayBox> displaybox;
    
    const TextureAtlas* textureAtlas;

    MakeTextureQuadsJob(MutArrayRef<QuadAndHeight> quads, const TextureAtlas* textureAtlas)
    : quads(quads), textureAtlas(textureAtlas) {}

    MakeTextureQuadsJob* Clone() const {
        return new MakeTextureQuadsJob(quads, textureAtlas);
    }

    void Init(JobData& data) {
        data.getComponentArray(textures);
        data.getComponentArray(displaybox);
    }

    void Execute(int N) {
        TextureID texture = textures[N].texture;
        TextureAtlas::Space space = getTextureAtlasSpace(textureAtlas, texture);
        Vec2 texMin = space.min;
        Vec2 texMax = space.max;

        Vec2 min = displaybox[N].box.min + textures[N].texBox.min;
        Vec2 max = displaybox[N].box.min + textures[N].texBox.max();

        SDL_Color color = {0,0,0,0};

        quads[N].quad[0] = {glm::vec2{min.x, min.y}, color, {texMin.x, texMin.y}};
        quads[N].quad[1] = {glm::vec2{min.x, max.y}, color, {texMin.x, texMax.y}};
        quads[N].quad[2] = {glm::vec2{max.x, max.y}, color, {texMax.x, texMax.y}};
        quads[N].quad[3] = {glm::vec2{max.x, min.y}, color, {texMax.x, texMin.y}};
        quads[N].height = displaybox[N].height;
    }
};

struct RenderTexturesSystem : ISystem {
    ComponentGroup<
        ReadOnly<EC::SimpleTexture>,
        ReadOnly<EC::DisplayBox>,
        Subtract<EC::Hidden>
    > textureGroup;

    GuiRenderer* guiRenderer;

    RenderTexturesSystem(SystemManager& manager, GuiRenderer* guiRenderer)
    : ISystem(manager), guiRenderer(guiRenderer) {}

    void ScheduleJobs() {
        auto quads = makeTempGroupArray<QuadAndHeight>(textureGroup);
        Schedule(textureGroup, BufferQuadJob(quads, guiRenderer),
            Schedule(textureGroup, MakeTextureQuadsJob(quads, &guiRenderer->guiAtlas))
        );
    }
};

struct DoElementUpdatesJob : IJobMainThread<DoElementUpdatesJob> {
    Game* game;

    DoElementUpdatesJob(Game* game)
    : IJobMainThread<DoElementUpdatesJob>(true), game(game) {}

    ComponentArray<const EC::Update> updates;
    EntityArray elements;
    
    void Init(JobData& data) {
        data.getComponentArray(updates);
        data.getEntityArray(elements);
    }

    void Execute(int N);
};

struct DoElementUpdatesSystem : ISystem {
    ComponentGroup<
        ReadOnly<EC::Update>
    > group;

private:
    Game* game;
public:

    DoElementUpdatesSystem(SystemManager& manager, Game* game)
    : ISystem(manager), game(game) {}

    void ScheduleJobs() {
        Schedule(group, DoElementUpdatesJob(game));
    }
};



}

}

#endif