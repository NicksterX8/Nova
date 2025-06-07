#ifndef GUI_SYSTEMS_BASIC_INCLUDED
#define GUI_SYSTEMS_BASIC_INCLUDED

#include "ECS/generic/system.hpp"
#include "GUI/components.hpp"
#include "rendering/gui.hpp"

namespace GUI {

namespace Systems {

using namespace EcsSystem;
using GECS::ElementCommandBuffer;

struct IJobParallelFor : IJob {
    IJobParallelFor() : IJob(IJob::Parallel) {}
};

struct IJobSingleThreaded : IJob {
    IJobSingleThreaded() : IJob(IJob::MainThread) {}
};

template<class T>
struct CopyGroupArrayJob : IJob {
    GroupArray<T> dst;
    GroupArray<const T> src;

    CopyGroupArrayJob(const GroupArray<T>& dst, const GroupArray<const T>& src) : dst(this), src(this) {
        this->dst = dst;
        this->src = src;
    }

    void Execute(int N) {
        dst[N] = src[N];
    }
};

template<class Component>
struct CopyComponentArrayJob : CopyGroupArrayJob<Component> {
    CopyComponentArrayJob(const PlainArray<Component>& dst)
    : CopyGroupArrayJob<Component>(dst, ComponentArray<const Component>(nullptr)) {}
};

struct CopyElementArrayJob : IJobParallelFor {
    GroupArray<Element> dst;
    ElementArray src;

    CopyElementArrayJob(const GroupArray<Element>& dst) : dst(this, dst), src(this) {
 
    }

    void Execute(int N) {
        dst[N] = src[N];
    }
};

template<class Component>
struct FillComponentsFromArrayJob : IJob {
    PlainArray<Component> src;
    ComponentArray<const Component> dst;

    FillComponentsFromArrayJob(PlainArray<Component>& src) : src(this), dst(this) {
        this->src = src;
    }

    void Execute(int N) {
        dst[N] = src[N];
    }
};

template<typename T>
struct InitializeArrayJob : IJob {
    GroupArray<T> values;

    T value;

    InitializeArrayJob(const GroupArray<T>& values, const T& initializationValue) : values(this), value(initializationValue) {
        this->values = values;
    }

    void Execute(int N) {
        values[N] = value;
    }
};

using CopyNamesJob = CopyComponentArrayJob<GUI::EC::Name>;


struct EnforceMaxSizeJob : IJobParallelFor {

    ComponentArray<GUI::EC::ViewBox> viewbox;
    ComponentArray<const GUI::EC::SizeConstraint> maxSize;

    ElementArray element;

    float dt;

    // instantiate component arrays
    EnforceMaxSizeJob()
    : viewbox(this), maxSize(this), element(this) {
        
    }

    void update(float deltaTime) {
        this->dt = deltaTime;
    }

    void Execute(int N) {
        if (viewbox[N].absolute.size.x > maxSize[N].maxSize.x) {
            viewbox[N].absolute.size.x = maxSize[N].maxSize.x;
            commands.addComponent(element[N], EC::Hidden{});
        }
        if (viewbox[N].absolute.size.y > maxSize[N].maxSize.y) viewbox[N].absolute.size.y = maxSize[N].maxSize.y;
        
    }
};

struct EnforceMinSizeJob : IJobParallelFor {
    ComponentArray<GUI::EC::ViewBox> viewbox;
    ComponentArray<const GUI::EC::SizeConstraint> minSize;

    // instantiate component arrays
    EnforceMinSizeJob()
    : viewbox(this), minSize(this) {
        
    }

    void update() {
        
    }

    void Execute(int N) {
        if (viewbox[N].absolute.size.x < minSize[N].minSize.x) viewbox[N].absolute.size.x = minSize[N].minSize.x;
        if (viewbox[N].absolute.size.y < minSize[N].minSize.y) viewbox[N].absolute.size.y = minSize[N].minSize.y;
    }
};

struct ComponentWithElement {
    Element element;
};

struct SizeConstraintSystem : ISystem {
    ComponentGroup<
        ReadWrite<EC::ViewBox>,
        ReadOnly<EC::SizeConstraint>
    > group;

    EnforceMaxSizeJob maxSizeJob;
    EnforceMinSizeJob minSizeJob;

    SizeConstraintSystem(SystemManager& manager) : ISystem(manager), maxSizeJob() {
        
    }

    void ScheduleJobs() override {
        Schedule(group, maxSizeJob);
        Schedule(group, minSizeJob);
    }
};

struct BoxToQuadJob : IJobParallelFor {
    ComponentArray<const EC::ViewBox> viewbox;

    PlainArray<QuadRenderer::Quad> quads;

    BoxToQuadJob(const PlainArray<QuadRenderer::Quad>& quadsArray)
    : viewbox(this), quads(this, quadsArray) {}

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

    PlainArray<QuadRenderer::Quad> quads;

    ColorToQuadJob(const PlainArray<QuadRenderer::Quad>& quadsArray) : background(this), quads(this, quadsArray) {}

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

    PlainArray<QuadRenderer::Quad> quads;
    PlainArray<const GuiRenderLevel> levels;

    GuiRenderer* guiRenderer;

    BufferQuadJob(const PlainArray<QuadRenderer::Quad>& quadArray, const PlainArray<GuiRenderLevel>& levels, GuiRenderer* guiRenderer)
    : viewbox(this), quads(this, quadArray), levels(this, levels), guiRenderer(guiRenderer) {
    
    }

    void Execute(int N) {
        const QuadRenderer::Quad& quad = quads[N];
        GuiRenderLevel level = viewbox[N].level;
        if (level < 0 || level > guiRenderer->levels.size) {
            return;
        }
        guiRenderer->levels[level].quads.push(quad);
    }

    void AfterExecution(int capacity) {
        
    }
};

namespace SystemOrder {
    enum SystemOrdering {
        BeforeRendering,
        AfterRendering
    };
}

using BorderQuads = std::array<QuadRenderer::Quad, 4>;

struct BufferBordersJob : IJobSingleThreaded {
    PlainArray<const BorderQuads> quads;
    PlainArray<const GuiRenderLevel> levels;

    GuiRenderer* guiRenderer;

    BufferBordersJob(const PlainArray<BorderQuads>& quads, const PlainArray<GuiRenderLevel>& levels, GuiRenderer* guiRenderer)
    : quads(this, quads), levels(this, levels), guiRenderer(guiRenderer) {}

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
    PlainArray<BorderQuads> quads;
    ComponentArray<const EC::ViewBox> viewbox;
    ComponentArray<const EC::Border> border;

    BorderQuadsJob(const PlainArray<BorderQuads>& quads)
    : quads(this, quads), viewbox(this), border(this) {}

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
    ComponentGroup<
        ReadOnly<EC::ViewBox>,
        ReadOnly<EC::Background>
    > backgroundGroup;

    ComponentGroup<
        ReadOnly<EC::Border>,
        ReadOnly<EC::ViewBox>
    > borderGroup;

    PlainArray<QuadRenderer::Quad> backgroundQuads;
    PlainArray<BorderQuads> borderQuads;
    PlainArray<GuiRenderLevel> levels;

    ColorToQuadJob fillBackgroundQuadColors;
    BoxToQuadJob fillBackgroundQuadBoxes;
    BufferQuadJob bufferBackgroundQuads;

    BorderQuadsJob makeBorderQuads;
    BufferBordersJob bufferBorderQuads;
    

    RenderBackgroundSystem(SystemManager& manager, GuiRenderer* guiRenderer)
    : ISystem(manager, SystemOrder::AfterRendering),
    backgroundQuads(nullptr), borderQuads(nullptr), levels(nullptr),
    fillBackgroundQuadColors(backgroundQuads), fillBackgroundQuadBoxes(backgroundQuads),
    bufferBackgroundQuads(backgroundQuads, levels, guiRenderer),
    makeBorderQuads(borderQuads), bufferBorderQuads(borderQuads, levels, guiRenderer) {

    }

    void BeforeExecution() {}

    void ScheduleJobs() {
        Schedule(backgroundGroup, fillBackgroundQuadColors);
        Schedule(backgroundGroup, fillBackgroundQuadBoxes);

        Schedule(borderGroup, bufferBorderQuads)
            .DependentOn(fillBackgroundQuadColors)
            .DependentOn(fillBackgroundQuadBoxes);

        // borders
        Schedule(borderGroup, makeBorderQuads);
        Schedule(backgroundGroup, bufferBackgroundQuads)
            .DependentOn(makeBorderQuads);
    }
};



struct TestSystem : ISystem {
    ComponentGroup<> all;

    
    TestSystem(SystemManager& manager) : ISystem(manager) {

    }
};



}

}

#endif