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

    ComponentArray<GUI::EC::ViewBox> viewbox;
    ComponentArray<const GUI::EC::SizeConstraint> maxSize;

    const Entity* entity;

    void Init(JobData& data) {
        data.getComponentArray(viewbox);
        data.getComponentArray(maxSize);
        entity = data.getEntityArray();
    }

    void Execute(int N) {
        if (viewbox[N].absolute.size.x > maxSize[N].maxSize.x) viewbox[N].absolute.size.x = maxSize[N].maxSize.x;
        if (viewbox[N].absolute.size.y > maxSize[N].maxSize.y) viewbox[N].absolute.size.y = maxSize[N].maxSize.y;   
    }
};

JOB_PARALLEL_FOR(EnforceMinSizeJob) {
    ComponentArray<GUI::EC::ViewBox> viewbox;
    ComponentArray<const GUI::EC::SizeConstraint> minSize;

    void Init(JobData& data) {
        data.getComponentArray(viewbox);
        data.getComponentArray(minSize);
    }

    void Execute(int N) {
        if (viewbox[N].absolute.size.x < minSize[N].minSize.x) viewbox[N].absolute.size.x = minSize[N].minSize.x;
        if (viewbox[N].absolute.size.y < minSize[N].minSize.y) viewbox[N].absolute.size.y = minSize[N].minSize.y;
    }
};


struct SizeConstraintSystem : ISystem {
    static constexpr ComponentGroup<
        ReadWrite<EC::ViewBox>,
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
    MutArrayRef<GuiRenderLevel> levels;

    GetViewLevelsJob(MutArrayRef<GuiRenderLevel> levels) : levels(levels) {}

    void Init(JobData& data) {
        data.getComponentArrays(viewbox);
    }

    void Execute(int N) {
        levels[N] = viewbox[N].level;
    }
};

JOB_PARALLEL_FOR(BoxToQuadJob) {
    ComponentArray<const EC::ViewBox> viewbox;

    MutArrayRef<QuadRenderer::Quad> quads;

    BoxToQuadJob(MutArrayRef<QuadRenderer::Quad> quadsArray) : quads(quadsArray) {
        
    }

    void Init(JobData& data) {
        data.getComponentArray(viewbox);
    }

    void Execute(int N) {
        Box box = viewbox[N].absolute;
        Vec2 min = box.min;
        Vec2 max = box.max();

        quads[N][0].pos = {min.x, min.y, 0};
        quads[N][1].pos = {min.x, max.y, 0};
        quads[N][2].pos = {max.x, max.y, 0};
        quads[N][3].pos = {max.x, min.y, 0};
    }
};

JOB_PARALLEL_FOR(ColorToQuadJob) {
    ComponentArray<const EC::Background> background;

    MutArrayRef<QuadRenderer::Quad> quads;

    ColorToQuadJob(MutArrayRef<QuadRenderer::Quad> quadsArray)
    : quads(quadsArray) {}

    void Init(JobData& data) {
        data.getComponentArray(background);
    }

    void Execute(int N) {
        SDL_Color color = background[N].color;

        for (int i = 0; i < 4; i++) {
            quads[N][i].color = color;
            quads[N][i].texCoord = QuadRenderer::NullCoord;
        }
    }
};

JOB_SINGLE_THREADED(BufferQuadJob) {
    ComponentArray<const EC::ViewBox> viewbox;

    ArrayRef<QuadRenderer::Quad> quads;
    ArrayRef<GuiRenderLevel> levels;

    GuiRenderer* guiRenderer;

    BufferQuadJob(ArrayRef<QuadRenderer::Quad> quads, ArrayRef<GuiRenderLevel> levels, GuiRenderer* guiRenderer)
    : quads(quads), levels(levels), guiRenderer(guiRenderer) {}

    void Init(JobData& data) {
        data.getComponentArray(viewbox);
    }

    void Execute(int N) {
        const QuadRenderer::Quad& quad = quads[N];
        GuiRenderLevel level = levels[N];
        if (level < 0 || level > guiRenderer->levels.size) {
            return;
        }
        guiRenderer->levels[level].quads.push(quad);
    }
};

using BorderQuads = std::array<QuadRenderer::Quad, 4>;

JOB_SINGLE_THREADED(BufferBordersJob) {
    ArrayRef<BorderQuads> quads;
    ArrayRef<GuiRenderLevel> levels;

    GuiRenderer* guiRenderer;

    BufferBordersJob(ArrayRef<BorderQuads> quads, ArrayRef<GuiRenderLevel> levels, GuiRenderer* guiRenderer)
    : quads(quads), levels(levels), guiRenderer(guiRenderer) {}

    void Init(JobData& data) {
        
    }

    void Execute(int N) {
        GuiRenderLevel level = levels[N];
        if (level < 0 || level > guiRenderer->levels.size) {
            return;
        }
        const auto& borderQuads = quads[N];
        guiRenderer->levels[level].quads.push(borderQuads);
    }
};

JOB_PARALLEL_FOR(BorderQuadsJob) {
    MutArrayRef<BorderQuads> quads;
    ComponentArray<const EC::ViewBox> viewbox;
    ComponentArray<const EC::Border> border;

    BorderQuadsJob(MutArrayRef<BorderQuads> quads)
    : quads(quads) {}

    QuadRenderer::Quad colorRect(glm::vec2 min, glm::vec2 max, SDL_Color color) {
        return QuadRenderer::Quad{{
            {glm::vec3{min.x, min.y, 0}, color, QuadRenderer::NullCoord},
            {glm::vec3{min.x, max.y, 0}, color, QuadRenderer::NullCoord},
            {glm::vec3{max.x, max.y, 0}, color, QuadRenderer::NullCoord},
            {glm::vec3{max.x, min.y, 0}, color, QuadRenderer::NullCoord}
        }};
    }

    void Init(JobData& data) {
        data.getComponentArrays(viewbox, border);
    }

    void Execute(int N) {
        Box box = viewbox[N].absolute;
        Vec2 min = box.min;
        Vec2 max = box.max();

        SDL_Color color = border[N].color;
        Vec2 strokeIn = border[N].strokeIn;
        Vec2 strokeOut = border[N].strokeOut;
        
        quads[N][0] = colorRect({min.x - strokeOut.x, min.y - strokeOut.y}, {max.x -  strokeIn.x, min.y +  strokeIn.y}, color);
        quads[N][1] = colorRect({max.x -  strokeIn.x, min.y - strokeOut.y}, {max.x + strokeOut.x, max.y -  strokeIn.y}, color);
        quads[N][2] = colorRect({max.x + strokeOut.x, max.y -  strokeIn.y}, {min.x +  strokeIn.x, max.y + strokeOut.y}, color);
        quads[N][3] = colorRect({min.x - strokeOut.x, max.y + strokeOut.y}, {min.x +  strokeIn.x, min.y +  strokeIn.y}, color);
    }
};

struct RenderBackgroundSystem : ISystem {
    static constexpr ComponentGroup<
        ReadOnly<EC::ViewBox>,
        ReadOnly<EC::Background>,
        Subtract<EC::Hidden>
    > backgroundGroup;

    static constexpr ComponentGroup<
        ReadOnly<EC::Border>,
        ReadOnly<EC::ViewBox>,
        Subtract<EC::Hidden>
    > borderGroup;


    GuiRenderer* guiRenderer;

    RenderBackgroundSystem(SystemManager& manager, GuiRenderer* guiRenderer)
    : ISystem(manager), guiRenderer(guiRenderer) {}


    void ScheduleJobsOld() {
        auto levelsBg = makeTempGroupArray<GuiRenderLevel>(backgroundGroup);
        Schedule(backgroundGroup, InitializeArrayJob<GuiRenderLevel>(levelsBg, GuiNullRenderLevel));
        auto quadsBg = makeTempGroupArray<QuadRenderer::Quad>(backgroundGroup);
        
        auto bufferQuadsJob = 
        Schedule(backgroundGroup, BufferQuadJob(quadsBg, levelsBg, guiRenderer),
            Schedule(backgroundGroup, BoxToQuadJob(quadsBg),
               {Schedule(backgroundGroup, GetViewLevelsJob(levelsBg)),
                Schedule(backgroundGroup, ColorToQuadJob(quadsBg))}
            )
        );

        auto levelsBd = makeTempGroupArray<GuiRenderLevel>(borderGroup);
        auto quadsBd = makeTempGroupArray<BorderQuads>(borderGroup);

        auto bufferBordersJob = 
        Schedule(borderGroup, BufferBordersJob(quadsBd, levelsBd, guiRenderer),
           {Schedule(borderGroup, GetViewLevelsJob(levelsBd)),
            Schedule(borderGroup, BorderQuadsJob(quadsBd))});

        AddDependency(bufferBordersJob, bufferQuadsJob);
    }

    void ScheduleJobs() {
        auto levelsBg = makeTempGroupArray<GuiRenderLevel>(backgroundGroup);
        auto quadsBg = makeTempGroupArray<QuadRenderer::Quad>(backgroundGroup);
        auto bufferQuadsJob = Do({
            Schedule(backgroundGroup, GetViewLevelsJob(levelsBg)),
            Schedule(backgroundGroup, ColorToQuadJob(quadsBg))
        }).Then(
            Schedule(backgroundGroup, BoxToQuadJob(quadsBg))
        ).Then(
            Schedule(backgroundGroup, BufferQuadJob(quadsBg, levelsBg, guiRenderer))
        ).handle();

        auto levelsBd = makeTempGroupArray<GuiRenderLevel>(borderGroup);
        auto quadsBd = makeTempGroupArray<BorderQuads>(borderGroup);

        auto bufferBordersJob = Do(
            Schedule(borderGroup, InitializeArrayJob(levelsBd, -3)))
        .Then({
            Schedule(borderGroup, GetViewLevelsJob(levelsBd)),
            Schedule(borderGroup, BorderQuadsJob(quadsBd))
        })
        .Then(
            Schedule(borderGroup, BufferBordersJob(quadsBd, levelsBd, guiRenderer)))
        .handle();

        Conflict(bufferBordersJob, bufferQuadsJob);
    }
};

JOB_PARALLEL_FOR(MakeTextureQuadsJob) {
    MutArrayRef<QuadRenderer::Quad> quads;
    ComponentArray<const EC::SimpleTexture> textures;
    ComponentArray<const EC::ViewBox> viewbox;
    
    const TextureAtlas* textureAtlas;

    MakeTextureQuadsJob(MutArrayRef<QuadRenderer::Quad> quads, const TextureAtlas* textureAtlas)
    : quads(quads), textureAtlas(textureAtlas) {}

    MakeTextureQuadsJob* Clone() const {
        return new MakeTextureQuadsJob(quads, textureAtlas);
    }

    void Init(JobData& data) {
        data.getComponentArray(textures);
        data.getComponentArray(viewbox);
    }

    void Execute(int N) {
        TextureID texture = textures[N].texture;
        TextureAtlas::Space space = getTextureAtlasSpace(textureAtlas, texture);
        Vec2 texMin = space.min;
        Vec2 texMax = space.max;

        Vec2 min = viewbox[N].absolute.min + textures[N].texBox.min;
        Vec2 max = viewbox[N].absolute.min + textures[N].texBox.max();

        SDL_Color color = {0,0,0,0};

        quads[N][0] = {glm::vec3{min.x, min.y, 0}, color, {texMin.x, texMin.y}};
        quads[N][1] = {glm::vec3{min.x, max.y, 0}, color, {texMin.x, texMax.y}};
        quads[N][2] = {glm::vec3{max.x, max.y, 0}, color, {texMax.x, texMax.y}};
        quads[N][3] = {glm::vec3{max.x, min.y, 0}, color, {texMax.x, texMin.y}};
    }
};

struct RenderTexturesSystem : ISystem {
    ComponentGroup<
        ReadOnly<EC::SimpleTexture>,
        ReadOnly<EC::ViewBox>,
        Subtract<EC::Hidden>
    > textureGroup;

    GuiRenderer* guiRenderer;

    RenderTexturesSystem(SystemManager& manager, GuiRenderer* guiRenderer)
    : ISystem(manager), guiRenderer(guiRenderer) {}

    void ScheduleJobs() {
        auto levels = makeTempGroupArray<GuiRenderLevel>(textureGroup);
        auto quads = makeTempGroupArray<QuadRenderer::Quad>(textureGroup);
        Schedule(textureGroup, BufferQuadJob(quads, levels, guiRenderer), {
            Schedule(textureGroup, GetViewLevelsJob(levels)),
            Schedule(textureGroup, MakeTextureQuadsJob(quads, &guiRenderer->guiAtlas))
        });
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

    void Execute(int N) {
        updates[N].update(game, elements[N]);
    }
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