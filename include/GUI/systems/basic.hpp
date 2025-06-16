#ifndef GUI_SYSTEMS_BASIC_INCLUDED
#define GUI_SYSTEMS_BASIC_INCLUDED

#include "ECS/system.hpp"
#include "GUI/components.hpp"
#include "rendering/gui.hpp"

namespace GUI {

namespace Systems {

using namespace ECS::System;

using CopyNamesJob = CopyComponentArrayJob<GUI::EC::Name>;


struct EnforceMaxSizeJob : IJobParallelFor {

    ComponentArray<GUI::EC::ViewBox> viewbox;
    ComponentArray<const GUI::EC::SizeConstraint> maxSize;

    EntityArray entity;

    // instantiate component arrays
    EnforceMaxSizeJob(JobGroup group)
    : IJobParallelFor(group), viewbox(this), maxSize(this), entity(this) {
        
    }

    void update() {
        
    }

    void Execute(int N) {
        if (viewbox[N].absolute.size.x > maxSize[N].maxSize.x) viewbox[N].absolute.size.x = maxSize[N].maxSize.x;
        if (viewbox[N].absolute.size.y > maxSize[N].maxSize.y) viewbox[N].absolute.size.y = maxSize[N].maxSize.y;   
    }
};

struct EnforceMinSizeJob : IJobParallelFor {
    ComponentArray<GUI::EC::ViewBox> viewbox;
    ComponentArray<const GUI::EC::SizeConstraint> minSize;

    // instantiate component arrays
    EnforceMinSizeJob(JobGroup group)
    : IJobParallelFor(group), viewbox(this), minSize(this) {
        
    }

    void update() {
        
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
        auto maxSizeJob = new EnforceMaxSizeJob(&group);
        Schedule(maxSizeJob);
    }
};

struct GetViewLevelsJob : IJobParallelFor {
    ComponentArray<const EC::ViewBox> viewbox;

    GuiRenderLevel* levels;

    GetViewLevelsJob(JobGroup group, GuiRenderLevel* levels)
    : IJobParallelFor(group), viewbox(this), levels(levels) {}

    void Execute(int N) {
        levels[N] = viewbox[N].level;
    }
};

struct BoxToQuadJob : IJobParallelFor {
    ComponentArray<const EC::ViewBox> viewbox;

    QuadRenderer::Quad* quads;

    BoxToQuadJob(JobGroup group, QuadRenderer::Quad* quadsArray)
    : IJobParallelFor(group), viewbox(this), quads(quadsArray) {}

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

    ColorToQuadJob(JobGroup group, QuadRenderer::Quad* quadsArray)
    : IJobParallelFor(group), background(this), quads(quadsArray) {}

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

    BufferQuadJob(JobGroup group, const QuadRenderer::Quad* quads, const GuiRenderLevel* levels, GuiRenderer* guiRenderer)
    : IJobSingleThreaded(group), viewbox(this), quads(quads), levels(levels), guiRenderer(guiRenderer) {}

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

    BufferBordersJob(JobGroup group, const BorderQuads* quads, const GuiRenderLevel* levels, GuiRenderer* guiRenderer)
    : IJobSingleThreaded(group), quads(quads), levels(levels), guiRenderer(guiRenderer) {}

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

    BorderQuadsJob(JobGroup group, BorderQuads* quads)
    : IJobParallelFor(group), quads(quads), viewbox(this), border(this) {}

    QuadRenderer::Quad colorRect(glm::vec2 min, glm::vec2 max, SDL_Color color) {
        return QuadRenderer::Quad{{
            {glm::vec3{min.x, min.y, 0}, color, QuadRenderer::NullCoord},
            {glm::vec3{min.x, max.y, 0}, color, QuadRenderer::NullCoord},
            {glm::vec3{max.x, max.y, 0}, color, QuadRenderer::NullCoord},
            {glm::vec3{max.x, min.y, 0}, color, QuadRenderer::NullCoord}
        }};
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
        ReadOnly<EC::ViewBox>
    > borderGroup;


    GuiRenderer* guiRenderer;

    RenderBackgroundSystem(SystemManager& manager, GuiRenderer* guiRenderer)
    : ISystem(manager), guiRenderer(guiRenderer) {}


    void ScheduleJobs() {
        auto* levelsBg = makeTempGroupArray<GuiRenderLevel>(backgroundGroup);
        auto* quadsBg = makeTempGroupArray<QuadRenderer::Quad>(backgroundGroup);
        
        auto bufferQuadsJob = 
        Schedule(new BufferQuadJob(&backgroundGroup, quadsBg, levelsBg, guiRenderer),
            Schedule(new BoxToQuadJob(&backgroundGroup, quadsBg),
               {Schedule(new GetViewLevelsJob(&backgroundGroup, levelsBg)),
                Schedule(new ColorToQuadJob(&backgroundGroup, quadsBg))}
            )
        );

        auto* levelsBd = makeTempGroupArray<GuiRenderLevel>(borderGroup);
        auto* quadsBd = makeTempGroupArray<BorderQuads>(borderGroup);

        auto bufferBordersJob = 
        Schedule(new BufferBordersJob(&borderGroup, quadsBd, levelsBd, guiRenderer),
            Schedule(new BorderQuadsJob(&borderGroup, quadsBd)));

        AddDependency(bufferBordersJob, bufferQuadsJob);
    }

    void BeforeExecution() {}
};

struct MakeTextureQuadsJob : IJobParallelFor {
    QuadRenderer::Quad* quads;
    ComponentArray<const EC::SimpleTexture> textures;
    ComponentArray<const EC::ViewBox> viewbox;
    
    const TextureAtlas* textureAtlas;

    MakeTextureQuadsJob(JobGroup group, QuadRenderer::Quad* quads, const TextureAtlas* textureAtlas)
    : IJobParallelFor(group), quads(quads), textures(this), viewbox(this), textureAtlas(textureAtlas) {}

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
        Schedule(new BufferQuadJob(&textureGroup, quads, levels, guiRenderer), {
            Schedule(new GetViewLevelsJob(&textureGroup, levels)),
            Schedule(new MakeTextureQuadsJob(&textureGroup, quads, &guiRenderer->guiAtlas))
        });
    }
};



}

}

#endif