#include "rendering/renderers.hpp"

void QuadRenderer::init() {
    auto vertexFormat = GlMakeVertexFormat(0, {
        {2, GL_FLOAT, sizeof(GLfloat)}, // pos
        {4, GL_UNSIGNED_BYTE, sizeof(GLubyte), true /* Normalize */}, // color
        {2, GL_UNSIGNED_SHORT, sizeof(GLushort)} // tex coord
    });
    assert(vertexFormat.totalSize() == sizeof(Vertex));
    static_assert(sizeof(Quad) == 4 * sizeof(Vertex));

    GlBuffer vertexBuffer = {maxQuadsPerBatch * sizeof(Quad), nullptr, GL_STREAM_DRAW};
    GlBuffer indexBuffer = {eboIndexCount * sizeof(VertexIndexType), nullptr, GL_STATIC_DRAW};

    this->model = makeModel(vertexBuffer, indexBuffer, vertexFormat);

    auto* indices = (VertexIndexType*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
    generateQuadVertexIndices(maxQuadsPerBatch, indices);
    glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
}

void QuadRenderer::render(ArrayRef<Quad> quads, Height height) {
    int index = quadBuffer.size();
    quadBuffer.append(quads.begin(), quads.end());
    QuadBatch batch = {
        .bufferIndex = index,
        .quadCount = (int)quads.size(),
        .height = height + heightIncrementer
    };
    heightIncrementer += HeightIncrement;
    batches.push_back(batch);
}

void QuadRenderer::renderVertices(ArrayRef<ColorVertex> vertices, Height height) {
    int numQuads = vertices.size() / 4;
    int numVertices = numQuads * 4;

    Quad* quads = renderManual(numQuads, height);
    
    for (int i = 0; i < numQuads; i++) {
        Quad quad;
        for (int p = 0; p < 4; p++) {
            int vertex = i * 4 + p;
            quad[p].pos = vertices[vertex].position;
            quad[p].color = vertices[vertex].color;
            quad[p].texCoord = NullCoord;
        }
        quads[i] = quad;
    }
}

QuadRenderer::Quad* QuadRenderer::renderManual(int quadCount, Height height) {
    int index = (int)quadBuffer.size();
    batches.push_back(QuadBatch{
        .bufferIndex = index,
        .quadCount = quadCount,
        .height = height + heightIncrementer
    });
    heightIncrementer += HeightIncrement;

    quadBuffer.resize_for_overwrite(index + quadCount);
    return &quadBuffer[index];
}

void QuadRenderer::flush(Shader shader, const glm::mat4& transform, TextureUnit texture) {
    if (batches.empty()) return;

    shader.use();
    shader.setInt("tex", texture);
    shader.setMat4("transform", transform);

    // sort batches by height
    std::sort(batches.begin(), batches.end(), [](const QuadBatch& lhs, const QuadBatch& rhs){
        return lhs.height < rhs.height;
    });
    
    glBindVertexArray(model.vao);
    glBindBuffer(GL_ARRAY_BUFFER, model.vbo);

    Quad* quadsOut = (Quad*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

    int totalQuads = 0;
    int bufferedQuads = 0;
    for (auto& batch : batches) {
        Quad* batchQuads = &quadBuffer[batch.bufferIndex];

        int batchQuadsRendered = 0;
        while (batchQuadsRendered < batch.quadCount) {
            int quadsToRender = std::min<int>(maxQuadsPerBatch - bufferedQuads, batch.quadCount - batchQuadsRendered);
            memcpy(quadsOut + bufferedQuads, batchQuads + batchQuadsRendered, quadsToRender * sizeof(Quad));
            bufferedQuads += quadsToRender;
            if (bufferedQuads == maxQuadsPerBatch) {
                glUnmapBuffer(GL_ARRAY_BUFFER);
                glDrawElements(GL_TRIANGLES, 6 * maxQuadsPerBatch, GL_UNSIGNED_INT, (void*)(0));
                quadsOut = (Quad*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
                totalQuads += maxQuadsPerBatch;
                bufferedQuads = 0;
            }
            batchQuadsRendered += quadsToRender;
        }
    }
    glUnmapBuffer(GL_ARRAY_BUFFER);
    if (bufferedQuads > 0) {
        glDrawElements(GL_TRIANGLES, 6 * bufferedQuads, GL_UNSIGNED_INT, nullptr);
        totalQuads += bufferedQuads;
    }
    assert(totalQuads >= quadBuffer.size() && "Quad buffer has extra quads not in a batch!");
    assert(totalQuads <= quadBuffer.size() && "Batch had quads double rendered!");

    heightIncrementer = 0.0f;

    quadBuffer.clear();
    batches.clear();
}