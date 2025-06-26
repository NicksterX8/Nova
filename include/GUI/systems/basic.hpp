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

struct EnforceMaxSizeJob : IJobParallelFor {

    ComponentArray<GUI::EC::ViewBox> viewbox;
    ComponentArray<const GUI::EC::SizeConstraint> maxSize;

    const Entity* entity;

    IJob* Clone() const {
        return cloneJob(EnforceMaxSizeJob());
    }

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

struct EnforceMinSizeJob : IJobParallelFor {
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

struct GetViewLevelsJob : IJobParallelFor {
    ComponentArray<const EC::ViewBox> viewbox;

    GuiRenderLevel* levels;

    GetViewLevelsJob(GuiRenderLevel* levels)
    : levels(levels) {}

    void Init(JobData& data) {
        data.getComponentArrays(viewbox);
    }

    void Execute(int N) {
        levels[N] = viewbox[N].level;
    }
};

struct BoxToQuadJob : IJobParallelFor {
    ComponentArray<const EC::ViewBox> viewbox;

    QuadRenderer::Quad* quads;

    BoxToQuadJob(QuadRenderer::Quad* quadsArray) : quads(quadsArray) {
        
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

struct ColorToQuadJob : IJobParallelFor {
    ComponentArray<const EC::Background> background;

    QuadRenderer::Quad* quads;

    ColorToQuadJob(QuadRenderer::Quad* quadsArray)
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

struct BufferQuadJob : IJobSingleThreaded {
    ComponentArray<const EC::ViewBox> viewbox;

    const QuadRenderer::Quad* quads;
    const GuiRenderLevel* levels;

    GuiRenderer* guiRenderer;

    BufferQuadJob(const QuadRenderer::Quad* quads, const GuiRenderLevel* levels, GuiRenderer* guiRenderer)
    : quads(quads), levels(levels), guiRenderer(guiRenderer) {}

    BufferQuadJob* Clone() const {
        return new BufferQuadJob(quads, levels, guiRenderer);
    }

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

    void AfterExecution(int capacity) {
        
    }
};

using BorderQuads = std::array<QuadRenderer::Quad, 4>;

struct BufferBordersJob : IJobSingleThreaded {
    const BorderQuads* quads;
    const GuiRenderLevel* levels;

    GuiRenderer* guiRenderer;

    BufferBordersJob(const BorderQuads* quads, const GuiRenderLevel* levels, GuiRenderer* guiRenderer)
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

struct BorderQuadsJob : IJobParallelFor {
    BorderQuads* quads;
    ComponentArray<const EC::ViewBox> viewbox;
    ComponentArray<const EC::Border> border;

    BorderQuadsJob(BorderQuads* quads)
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


    void ScheduleJobs() {
        auto* levelsBg = makeTempGroupArray<GuiRenderLevel>(backgroundGroup);
        auto* quadsBg = makeTempGroupArray<QuadRenderer::Quad>(backgroundGroup);
        
        auto bufferQuadsJob = 
        Schedule(backgroundGroup, BufferQuadJob(quadsBg, levelsBg, guiRenderer),
            Schedule(backgroundGroup, BoxToQuadJob(quadsBg),
               {Schedule(backgroundGroup, GetViewLevelsJob(levelsBg)),
                Schedule(backgroundGroup, ColorToQuadJob(quadsBg))}
            )
        );

        auto* levelsBd = makeTempGroupArray<GuiRenderLevel>(borderGroup);
        auto* quadsBd = makeTempGroupArray<BorderQuads>(borderGroup);

        auto bufferBordersJob = 
        Schedule(borderGroup, BufferBordersJob(quadsBd, levelsBd, guiRenderer),
            Schedule(borderGroup, BorderQuadsJob(quadsBd)));

        AddDependency(bufferBordersJob, bufferQuadsJob);
    }

    void BeforeExecution() {}
};

struct MakeTextureQuadsJob : IJobParallelFor {
    QuadRenderer::Quad* quads;
    ComponentArray<const EC::SimpleTexture> textures;
    ComponentArray<const EC::ViewBox> viewbox;
    
    const TextureAtlas* textureAtlas;

    MakeTextureQuadsJob(QuadRenderer::Quad* quads, const TextureAtlas* textureAtlas)
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
        GuiRenderLevel* levels = makeTempGroupArray<GuiRenderLevel>(textureGroup);
        QuadRenderer::Quad* quads = makeTempGroupArray<QuadRenderer::Quad>(textureGroup);
        Schedule(textureGroup, BufferQuadJob(quads, levels, guiRenderer), {
            Schedule(textureGroup, GetViewLevelsJob(levels)),
            Schedule(textureGroup, MakeTextureQuadsJob(quads, &guiRenderer->guiAtlas))
        });
    }
};



}

}

#endif