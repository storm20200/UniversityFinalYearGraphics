#include "Geometry.hpp"


// STL headers.
#include <algorithm>
#include <iostream>


// Engine headers.
#include <scene/Scene.hpp>
#include <tsl/shapes.hpp>


// Personal headers.
#include <Rendering/Renderer/Geometry/Internals/Vertex.hpp>
#include <Rendering/Renderer/Materials/Materials.hpp>
#include <Rendering/Renderer/Types.hpp>
#include <Utility/Scene.hpp>
#include <Utility/TSL.hpp>


// Namespace inclusions.
using namespace types;


Geometry::Geometry() noexcept
{
    try
    {
        m_internals = std::make_unique<Internals>();
    }

    catch (const std::exception& e)
    {
        std::cerr << "Geometry::Geometry() couldn't allocate memory for internal data: " << e.what() << std::endl;
    }
}


const Mesh& Geometry::operator[] (const scene::MeshId id) const noexcept
{
    return m_internals->sceneMeshes[id];
}


bool Geometry::isInitialised() const noexcept
{
    return m_scene.vao.isInitialised() && m_drawCommands.buffer.isInitialised() && m_triangle.vao.isInitialised() &&
        m_lighting.vao.isInitialised() && m_internals->isInitialised();
}


const std::unordered_map<scene::MeshId, Mesh>& Geometry::getMeshes() const noexcept
{
    return m_internals->sceneMeshes;
}


void Geometry::clean() noexcept
{
     m_scene.vao.clean();
     m_drawCommands.buffer.clean();
     m_triangle.vao.clean();
     m_lighting.vao.clean();
     m_internals->clean();

     m_quad = m_sphere = m_cone = Mesh { };
}


void Geometry::buildMeshData (Internals& internals) const noexcept
{
    // Begin to construct the scene. We take a copy of the meshes data so we can sort it.
    auto meshes = scene::GeometryBuilder().getAllMeshes();

    // Ensure the meshes are sorted in order of their ID.
    std::sort (std::begin (meshes), std::end (meshes), 
        [] (const auto& a, const auto& b) { return a.getId() < b.getId(); });

    // We'll need a temporary vectors to store the vertex and element data. 
    auto vertices       = std::vector<Vertex> { };
    auto elements       = std::vector<Element> { };
    auto vertexCount    = size_t { 0 };
    auto elementCount   = size_t { 0 };

    // Start by allocating enough memory in each container to store the scene.
    util::calculateSceneSize (meshes, vertexCount, elementCount);
    internals.sceneMeshes.reserve (meshes.size());
    vertices.reserve (vertexCount);
    elements.reserve (elementCount);

    // Iterate through each mesh adding the vertices, elements and mapping to their corresponding container.
    auto mesh           = Mesh { };
    auto vertexIndex    = GLuint { 0 };
    auto elementsIndex  = GLuint { 0 };
    
	for (const auto& sceneMesh : meshes)
    {
        // Retrieve the required mesh data.
        const auto meshVertices     = util::assembleVertices (sceneMesh);
        const auto& meshElements    = sceneMesh.getElementArray();
        
        // Set the mesh parameters, the element offset must be a pointer type.
        mesh.verticesIndex  = vertexIndex;
        mesh.elementsIndex  = elementsIndex;
        mesh.elementCount   = static_cast<GLuint> (meshElements.size());

        // Now we can add the mesh to the map and the vertices/elements to the vectors.
        internals.sceneMeshes[sceneMesh.getId()] = mesh;
        vertices.insert (std::end (vertices), std::begin (meshVertices), std::end (meshVertices));
        elements.insert (std::end (elements), std::begin (meshElements), std::end (meshElements));

        // The vertexIndex needs an actual index value whereas elementOffset needs to be in bytes.
        vertexIndex     += static_cast<GLuint> (meshVertices.size());
        elementsIndex   += mesh.elementCount;
    }

    // Now we can fill the mesh and element buffer. We will leave them with no access flags so they can be static.
    internals.buffers[internals.sceneVerticesIndex].immutablyFillWith (vertices);
    internals.buffers[internals.sceneElementsIndex].immutablyFillWith (elements);
}


void Geometry::buildFullScreenTriangle (Internals& internals) const noexcept
{
    // The proportions are intentionally oversized so that the triangle takes up the entire screen.
    const auto positions = std::vector<glm::vec2> 
    { 
        glm::vec2 { -1.0f, -1.0f }, 
        glm::vec2 {  3.0f, -1.0f }, 
        glm::vec2 { -1.0f,  3.0f }
    };

    internals.buffers[internals.triangleVerticesIndex].immutablyFillWith (positions);
}


void Geometry::buildLighting (Internals& internals, Mesh& quad, Mesh& sphere, Mesh& cone) const noexcept
{
    // Light volumes only contain a position but all shapes will be stored in the same buffer like scene meshes.
    auto vertices = std::vector<VertexPosition> { };
    auto elements = std::vector<Element> { };

    // Reserve 512KiB for shape vertices and elements, this will likely be more than enough.
    constexpr auto reservation = 256'000;
    vertices.reserve (reservation / sizeof (VertexPosition));
    elements.reserve (reservation / sizeof (Element));

    // Quads are very simple shapes.
    constexpr auto quadVertices = std::array<VertexPosition, 4> 
    { 
        VertexPosition { -1, -1, 0 }, VertexPosition { 1, -1, 0 },
        VertexPosition { -1,  1, 0 }, VertexPosition { 1,  1, 0 }
    };

    constexpr auto quadElements = std::array<Element, 6> 
    {
        Element { 0 }, Element { 1 }, Element { 2 },
        Element { 1 }, Element { 3 }, Element { 2 }
    };

    // Add the quad to the vectors.
    vertices.insert (std::end (vertices), std::begin (quadVertices), std::end (quadVertices));
    elements.insert (std::end (elements), std::begin (quadElements), std::end (quadElements));
    quad.elementCount = static_cast<GLuint> (elements.size());

    // Now add the sphere.
    util::addTSLMeshData (sphere, vertices, elements, tsl::createSpherePtr (1.f, 12));

    // The cone mesh has the centre at the base, instead of the tip. We need the centre to be the tip so that they can
    // represent spotlights. This will require an offset.
    const auto offset = VertexPosition { 0.f, 0.f, -1.f };
    util::addTSLMeshData (cone, vertices, elements, tsl::createConePtr (1.f, 1.f, 12), offset);

    // Finally fill the GPU buffers.
    internals.buffers[internals.lightVerticesIndex].immutablyFillWith (vertices);
    internals.buffers[internals.lightElementsIndex].immutablyFillWith (elements);
}


void Geometry::fillStaticBuffers (Internals& internals, DrawCommands& drawCommands, const Materials& materials,
            const std::map<scene::MeshId, std::vector<scene::Instance>>& staticInstances) const noexcept
{
    // We'll need vectors to store each piece of data that needs buffering.
    auto commands       = std::vector<MultiDrawElementsIndirectCommand> { };
    auto materialIDs    = std::vector<MaterialID> { };
    auto transforms     = std::vector<ModelTransform> { };

    // We can immediately reserve enough memory for the draw commands.
    commands.reserve (staticInstances.size());

    // Now we can interate through each mesh collecting instancing data.
    for (const auto& meshInstancePair : staticInstances)
    {
        // Cache each component
        const auto meshID       = meshInstancePair.first;
        const auto& instances   = meshInstancePair.second;

        // Speed things up by reserving enough space.
        const auto capacity = materialIDs.size() + instances.size();
        materialIDs.reserve (capacity);
        transforms.reserve (capacity);

        // Add the draw command.
        const auto mesh = internals.sceneMeshes[meshID];
        commands.emplace_back (
            mesh.elementCount,
            static_cast<GLuint> (instances.size()),
            mesh.elementsIndex,
            mesh.verticesIndex,
            static_cast<GLuint> (materialIDs.size())
        );

        // Now collect the instancing data.
        for (const auto& instance : meshInstancePair.second)
        {
            materialIDs.push_back (materials[instance.getMaterialId()]);
            transforms.push_back (util::toGLM (instance.getTransformationMatrix()));
        }
    }

    // Prepare the draw commands objects.
    drawCommands.count      = static_cast<GLsizei> (commands.size());
    drawCommands.capacity   = drawCommands.count;

    // Finally fill the buffers.
    drawCommands.buffer.immutablyFillWith (commands);
    internals.buffers[internals.materialIDsIndex].immutablyFillWith (materialIDs);
    internals.buffers[internals.transformsIndex].immutablyFillWith (transforms);
}