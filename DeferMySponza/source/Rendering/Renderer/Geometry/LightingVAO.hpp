#pragma once

#if !defined    _RENDERING_RENDERER_GEOMETRY_LIGHTING_VAO_
#define         _RENDERING_RENDERER_GEOMETRY_LIGHTING_VAO_

// Personal headers.
#include <Rendering/Objects/VertexArray.hpp>
#include <Rendering/Renderer/Types.hpp>


/// <summary> 
/// A VAO used for storing lighting volumes such as a quad, a sphere and a cone for global, point and spotlighting 
/// respectively. It is assumed that all lights will be rendered using instancing without static object optimisations.
/// </summary>
struct LightingVAO final
{
    VertexArray vao { }; //!< A VAO containing all renderable meshes in the scene.

    constexpr static auto meshesBufferIndex             = GLuint { 0 }; //!< The binding index where the mesh buffer for all objects will be bound.
    constexpr static auto modelTransformsBufferIndex    = GLuint { 1 }; //!< The binding index where the transform buffer for all objects will be bound.
    
    constexpr static auto positionAttributeIndex        = GLuint { 0 }; //!< The attribute index for vertex position.
    constexpr static auto modelTransformAttributeIndex  = GLuint { 1 }; //!< The attribute index for instanced model transforms.

    constexpr static auto modelTransformAttributeCount  = GLuint { sizeof (types::ModelTransform) / sizeof (glm::vec3) }; //!< Model transforms require multiple attributes.
    

    LightingVAO() noexcept                                  = default;
    LightingVAO (LightingVAO&&) noexcept                    = default;
    LightingVAO (const LightingVAO&) noexcept               = default;
    LightingVAO& operator= (const LightingVAO&) noexcept    = default;
    LightingVAO& operator= (LightingVAO&&) noexcept         = default;
    ~LightingVAO()                                          = default;


    /// <summary> Attachs the given buffers to the VAO based on the compile-time indices in the class. </summary>
    template <size_t Partitions>
    void attachVertexBuffers (const Buffer& meshes, const Buffer& elements, 
        const PersistentMappedBuffer<Partitions>& modelTransforms) noexcept;

    /// <summary> Sets the binding points and formatting of attributes in the VAO. </summary>
    void configureAttributes() noexcept;

    /// <summary> Configures the instanced attributes to retrieve data from the given partition. </summary>
    /// <param name="materialIDIndex"> The partition of the transform buffer to use. </param>
    void useTransformPartition (const size_t partition) noexcept;
};


// Engine headers.
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>


template <size_t Partitions>
void LightingVAO::attachVertexBuffers (const Buffer& meshes, const Buffer& elements, 
        const PersistentMappedBuffer<Partitions>& modelTransforms) noexcept
{
    // Calculate our strides.
    constexpr auto meshesStride = GLuint { sizeof (types::VertexPosition) };
    constexpr auto modelStride  = GLuint { sizeof (ModelTransform) };

    // Instancing data contains one item per instance.
    constexpr auto divisor = GLuint { 1 };

    // Attach buffers. Vertex attributes are interleaved.
    vao.attachVertexBuffer (meshes, meshesBufferIndex, 0, meshesStride);
    vao.attachPersistentMappedBuffer (modelTransforms, modelTransformsBufferIndex, modelStride, divisor);
    vao.setElementBuffer (elements);
}

#endif // _RENDERING_RENDERER_GEOMETRY_LIGHTING_VAO_