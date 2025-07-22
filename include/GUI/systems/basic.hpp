#ifndef GUI_SYSTEMS_BASIC_INCLUDED
#define GUI_SYSTEMS_BASIC_INCLUDED

#include "ECS/System.hpp"
#include "GUI/components.hpp"
#include "rendering/gui.hpp"

namespace GUI {

namespace Systems {

using namespace ECS::Systems;

using CopyNamesJob = CopyComponentArrayJob<GUI::EC::Name>;

struct EnforceMinMaxSizeJob : JobParallelFor<EnforceMinMaxSizeJob, EC::DisplayBox, const EC::SizeConstraint> {
    void Execute(int N) {
        auto displayBox = Get<EC::DisplayBox>(N);
        auto sizeConstraint = Get<EC::SizeConstraint>(N);
        if (displayBox.box.size.x > sizeConstraint.maxSize.x) displayBox.box.size.x = sizeConstraint.maxSize.x;
        if (displayBox.box.size.y > sizeConstraint.maxSize.y) displayBox.box.size.y = sizeConstraint.maxSize.y;   

        if (displayBox.box.size.x < sizeConstraint.minSize.x) displayBox.box.size.x = sizeConstraint.minSize.x;
        if (displayBox.box.size.y < sizeConstraint.minSize.y) displayBox.box.size.y = sizeConstraint.minSize.y;
    }
};


struct SizeConstraintSystem : System {
    GroupID group = MakeGroup(ComponentGroup<
        ReadWrite<EC::DisplayBox>,
        ReadOnly<EC::SizeConstraint>
    >());

    EnforceMinMaxSizeJob minMaxSizeJob;

    SizeConstraintSystem(SystemManager& manager) : System(manager) {
        Schedule(group, minMaxSizeJob);
    }
};

struct GetViewLevelsJob : JobParallelFor<GetViewLevelsJob, const EC::ViewBox> {
    void Execute(int N, GroupArray<GUI::RenderLevel> levels) {
        levels[N] = Get<EC::ViewBox>(N).level;
    }
};

struct QuadAndHeight {
    QuadRenderer::Quad quad;
    GUI::RenderHeight height;
};

struct BoxToQuadJob : JobParallelFor<BoxToQuadJob, const EC::DisplayBox> {
    void Execute(int N, GroupArray<QuadAndHeight> quads) {
        auto displaybox = Get<EC::DisplayBox>(N);
        Box box = displaybox.box;
        Vec2 min = box.min;
        Vec2 max = box.max();

        quads[N].quad[0].pos = {min.x, min.y};
        quads[N].quad[1].pos = {min.x, max.y};
        quads[N].quad[2].pos = {max.x, max.y};
        quads[N].quad[3].pos = {max.x, min.y};
        quads[N].height = displaybox.height;
    }
};

struct ColorToQuadJob : JobParallelFor<ColorToQuadJob, const EC::Background> {
    void Execute(int N, GroupArray<QuadAndHeight> quads) {
        SDL_Color color = Get<EC::Background>(N).color;

        for (int i = 0; i < 4; i++) {
            quads[N].quad[i].color = color;
            quads[N].quad[i].texCoord = QuadRenderer::NullCoord;
        }
    }
};

struct BufferQuadJob : JobSingleThreaded<BufferQuadJob> {
    GuiRenderer* guiRenderer;
    BufferQuadJob(GuiRenderer* guiRenderer) : guiRenderer(guiRenderer) {}

    void Execute(int N, GroupArray<const QuadAndHeight> quads) {
        const QuadRenderer::Quad& quad = quads[N].quad;
        const GUI::RenderHeight height = quads[N].height;
        guiRenderer->quad->render(quad, height);
        // TODO: optimize
    }
};

using BorderQuads = std::pair<std::array<QuadRenderer::Quad, 4>, GUI::RenderHeight>;

struct BufferBordersJob : JobSingleThreaded<BufferBordersJob> {
    GuiRenderer* guiRenderer;

    BufferBordersJob(GuiRenderer* guiRenderer)
    : guiRenderer(guiRenderer) {}

    void Execute(int N, GroupArray<const BorderQuads> quads) {
        const auto& borderQuads = quads[N];
        guiRenderer->quad->render(borderQuads.first, borderQuads.second);
    }
};

struct BorderQuadsJob : JobParallelFor<BorderQuadsJob, const EC::DisplayBox, const EC::Border> {
    QuadRenderer::Quad colorRect(glm::vec2 min, glm::vec2 max, SDL_Color color) {
        return QuadRenderer::Quad{{
            {glm::vec2{min.x, min.y}, color, QuadRenderer::NullCoord},
            {glm::vec2{min.x, max.y}, color, QuadRenderer::NullCoord},
            {glm::vec2{max.x, max.y}, color, QuadRenderer::NullCoord},
            {glm::vec2{max.x, min.y}, color, QuadRenderer::NullCoord}
        }};
    }

    void Execute(int N, GroupArray<BorderQuads> quads) {
        auto displayBox = Get<EC::DisplayBox>(N);
        auto border = Get<EC::Border>(N);
        Box box = displayBox.box;
        Vec2 min = box.min;
        Vec2 max = box.max();

        SDL_Color color = border.color;
        Vec2 strokeIn = border.strokeIn;
        Vec2 strokeOut = border.strokeOut;
        
        quads[N].first[0] = colorRect({min.x - strokeOut.x, min.y - strokeOut.y}, {max.x -  strokeIn.x, min.y +  strokeIn.y}, color);
        quads[N].first[1] = colorRect({max.x -  strokeIn.x, min.y - strokeOut.y}, {max.x + strokeOut.x, max.y -  strokeIn.y}, color);
        quads[N].first[2] = colorRect({max.x + strokeOut.x, max.y -  strokeIn.y}, {min.x +  strokeIn.x, max.y + strokeOut.y}, color);
        quads[N].first[3] = colorRect({min.x - strokeOut.x, max.y + strokeOut.y}, {min.x +  strokeIn.x, min.y +  strokeIn.y}, color);
        quads[N].second = displayBox.height;
    }
};

struct RenderBackgroundSystem : System {
    // borders
    GroupID borderGroup = MakeGroup(ComponentGroup<
        ReadOnly<EC::ViewBox>,
        ReadOnly<EC::DisplayBox>,
        ReadOnly<EC::Border>,
        Subtract<EC::Hidden>
    >());

    GetViewLevelsJob bdViewLevels;
    BorderQuadsJob bdQuadsJob;
    BufferBordersJob bdBuffer;

    GroupArray<GUI::RenderLevel> bdLevels{this, borderGroup};
    GroupArray<BorderQuads> bdQuads{this, borderGroup};

    // backgrounds
    GroupID backgroundGroup = MakeGroup(ComponentGroup<
        ReadOnly<EC::ViewBox>,
        ReadOnly<EC::DisplayBox>,
        ReadOnly<EC::Background>,
        Subtract<EC::Hidden>
    >());

    ColorToQuadJob bgColorQuadJob;
    BoxToQuadJob bgBoxQuadJob;
    BufferQuadJob bgBufferQuads;

    GroupArray<QuadAndHeight> bgQuads{this, backgroundGroup};

    GuiRenderer* guiRenderer;

    RenderBackgroundSystem(SystemManager& manager, GuiRenderer* guiRenderer)
    : System(manager), bdBuffer(guiRenderer), bgBufferQuads(guiRenderer), guiRenderer(guiRenderer) {
        auto bufferQuadsJob = Do({
            Schedule(backgroundGroup, bgColorQuadJob, bgQuads.refMut())
        }).Then(
            Schedule(backgroundGroup, bgBoxQuadJob, bgQuads.refMut())
        ).Then(
            Schedule(backgroundGroup, bgBufferQuads, bgQuads.refConst())
        ).handle();

        auto bufferBordersJob = Do({
            Schedule(borderGroup, bdViewLevels, bdLevels.refMut()),
            Schedule(borderGroup, bdQuadsJob, bdQuads.refMut())
        })
        .Then(
            Schedule(borderGroup, bdBuffer, bdQuads.refConst())
        ).handle();

        Conflict(bufferBordersJob, bufferQuadsJob);
    }
};

struct MakeTextureQuadsJob : JobParallelFor<MakeTextureQuadsJob,
            const EC::SimpleTexture, const EC::DisplayBox> {
    const TextureAtlas* textureAtlas;

    MakeTextureQuadsJob(const TextureAtlas* textureAtlas)
    : textureAtlas(textureAtlas) {}

    void Execute(int N, GroupArray<QuadAndHeight> quads) {
        auto textureEc = Get<EC::SimpleTexture>(N);
        auto displaybox = Get<EC::DisplayBox>(N);
        TextureID texture = textureEc.texture;
        TextureAtlas::Space space = getTextureAtlasSpace(textureAtlas, texture);
        Vec2 texMin = space.min;
        Vec2 texMax = space.max;

        Vec2 min = displaybox.box.min + textureEc.texBox.min;
        Vec2 max = displaybox.box.min + textureEc.texBox.max();

        SDL_Color color = {0,0,0,0};

        quads[N].quad[0] = {glm::vec2{min.x, min.y}, color, {texMin.x, texMin.y}};
        quads[N].quad[1] = {glm::vec2{min.x, max.y}, color, {texMin.x, texMax.y}};
        quads[N].quad[2] = {glm::vec2{max.x, max.y}, color, {texMax.x, texMax.y}};
        quads[N].quad[3] = {glm::vec2{max.x, min.y}, color, {texMax.x, texMin.y}};
        quads[N].height = displaybox.height;
    }
};

struct RenderTexturesSystem : System {
    GroupID textureGroup = MakeGroup(ComponentGroup<
        ReadOnly<EC::SimpleTexture>,
        ReadOnly<EC::DisplayBox>,
        Subtract<EC::Hidden>
    >());

    GuiRenderer* guiRenderer;

    GroupArray<QuadAndHeight> quads{this, textureGroup};

    BufferQuadJob bufferQuads{guiRenderer};
    MakeTextureQuadsJob makeTextureQuads{&guiRenderer->guiAtlas};

    RenderTexturesSystem(SystemManager& manager, GuiRenderer* guiRenderer)
    : System(manager), guiRenderer(guiRenderer) {
        Schedule(textureGroup, bufferQuads, quads.refConst(),
            Schedule(textureGroup, makeTextureQuads, quads.refMut())
        );
    }
};

struct DoElementUpdatesJob : JobBlocking<DoElementUpdatesJob, const EC::Update> {
    Game* game;

    DoElementUpdatesJob(Game* game) : game(game) {}

    void Execute(int N);
};

struct DoElementUpdatesSystem : System {
    GroupID group = MakeGroup(ComponentGroup<
        ReadOnly<EC::Update>
    >());

private:
    Game* game;
public:

    DoElementUpdatesJob updatesJob{game};

    DoElementUpdatesSystem(SystemManager& manager, Game* game)
    : System(manager), game(game) {
        Schedule(group, updatesJob);
    }
};



}

}

#endif