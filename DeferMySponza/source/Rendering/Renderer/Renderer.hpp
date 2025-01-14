#pragma once

#if !defined    _RENDERING_RENDERER_
#define         _RENDERING_RENDERER_

// STL headers.
#include <utility>


// Engine headers.
#include <glm/fwd.hpp>
#include <scene/scene_fwd.hpp>


// Personal headers.
#include <Rendering/Objects/Buffer.hpp>
#include <Rendering/Objects/Sync.hpp>
#include <Rendering/Objects/Query.hpp>
#include <Rendering/Renderer/Drawing/GeometryBuffer.hpp>
#include <Rendering/Renderer/Drawing/LightBuffer.hpp>
#include <Rendering/Renderer/Drawing/Resolution.hpp>
#include <Rendering/Renderer/Drawing/ShadowMaps.hpp>
#include <Rendering/Renderer/Drawing/SMAA.hpp>
#include <Rendering/Renderer/Geometry/Geometry.hpp>
#include <Rendering/Renderer/Materials/Materials.hpp>
#include <Rendering/Renderer/Programs/Programs.hpp>
#include <Rendering/Renderer/Uniforms/Uniforms.hpp>


/// <summary>
/// An OpenGL 4.5 deferred renderer which maintains geometric, material and uniform data, along with the shaders and
/// programs required to render a given scene.
/// </summary>
class Renderer final
{
    public:

        Renderer() noexcept {}
        Renderer (Renderer&&) noexcept              = default;
        Renderer& operator= (Renderer&&) noexcept   = default;
        ~Renderer()                                 = default;

        Renderer (const Renderer&)                  = delete;
        Renderer& operator= (const Renderer&)       = delete;

        /// <summary> Gets how many times the GPU had to be manually flushed. </summary>
        GLuint getSyncCount() const noexcept                        { return m_syncCount; }

        /// <summary> Gets the total number of rendered frames. </summary>
        GLuint getFrameCount() const noexcept                       { return m_frames; }

        /// <summary> Gets the accumulated time taken to render every frame (ms). </summary>
        float getTotalFrameTime() const noexcept                    { return m_totalTime; }

        /// <summary> Get the minimum amount of time taken to render a frame (ms). </summary>
        float getMinFrameTime() const noexcept                      { return m_minTime; }

        /// <summary> Get the maximum amount of time taken to render a frame (ms). </summary>
        float getMaxFrameTime() const noexcept                      { return m_maxTime; }

        /// <summary> Sets whether the rendering should use multiple threads or not. </summary>
        void setThreadingMode (bool useMultipleThreads) noexcept    { m_multiThreaded = useMultipleThreads; }

        /// <summary> Sets whether deferred or forward rendering should be performed.
        void setRenderingMode (bool useDeferredRendering) noexcept  { m_deferredRender = useDeferredRendering; }

        /// <summary> Sets which reflection models should be used. This will cause a recompile of shaders. </summary>
        void setShadingMode (bool usePhysicallyBasedShading) noexcept;

        /// <summary> Sets the quality setting of the antialiasing to be performed. </summary>
        void setAntiAliasingMode (SMAA::Quality quality) noexcept;

        /// <summary> Resets calculated frame timings to zero. </summary>
        void resetFrameTimings() noexcept;

        /// <summary> Sets the resolution of the off-screen rendering buffers. </summary>
        void setInternalResolution (const glm::ivec2& resolution) noexcept;

        /// <summary> 
        /// Sets the resolution of the display, the internal resolution will be upscaled or downscaled to this value.
        /// </summary>
        void setDisplayResolution (const glm::ivec2& resolution) noexcept;


        /// <summary> 
        /// Attempt to initialise the renderer, building all mesh and material data, preparing the renderer for 
        /// rendering. Successive calls will completely rebuild the entire scene. Upon failure the object will
        /// clean itself to an uninitialised state.
        /// </summary>
        /// <param name="scene"> A context to use for rendering a scene. </param>
        /// <param name="internalRes"> The internal resolution for the framebuffers. </param>
        /// <param name="displayRes"> The resolution of the display to output to. </param>
        /// <returns> Whether initialisation was successful or not. </returns>
        bool initialise (scene::Context* scene, const glm::ivec2& internalRes, const glm::ivec2& displayRes) noexcept;

        /// <summary> Cleans all resources, putting the renderer in a state where it can be safely initialised. </summary>
        void clean() noexcept;


        /// <summary> Causes the renderer to render a frame to the display. </summary>
        void render() noexcept;

    private:

        constexpr static auto gbufferStartingTextureUnit    = GLuint { 0 };         //!< The starting texture unit for the gbuffer, the gbuffer occupies three units.
        constexpr static auto lbufferStartingTextureUnit    = GLuint { 4 };         //!< The starting texture unit for the lbuffer, the lbuffer occupies a single unit.
        constexpr static auto shadowMapStartingTextureUnit  = GLuint { 5 };         //!< The starting texture unit for the shadow map array.
        constexpr static auto smaaStartingTextureUnit       = GLuint { 6 };         //!< The starting texture unit for the antialiasing textures, occupies three units.
        constexpr static auto materialsStartingTextureUnit  = GLuint { 9 };         //!< The starting texture unit for the material data.
        constexpr static auto defaultAA                     = SMAA::Quality::Ultra; //!< The default value for antialiasing.

        struct MeshInstances final
        {
            using Instances = std::vector<scene::InstanceId>;

            Mesh        mesh        { };    //!< Rendering data for a particular scene mesh.
            Instances   instances   { };    //!< A list of instances requiring the stored mesh.

            MeshInstances (const Mesh& mesh, Instances&& instances) noexcept
                : mesh (std::move (mesh)), instances (std::move (instances)) {}
        };

        struct ModifiedDynamicObjectRanges final
        {
            ModifiedRange drawCommands, transforms, materialIDs;

            ModifiedDynamicObjectRanges() = default;
            ModifiedDynamicObjectRanges (const ModifiedRange& a, const ModifiedRange& b, const ModifiedRange& c)
                : drawCommands (a), transforms (b), materialIDs (c) { }
        };

        struct ModifiedLightVolumeRanges final
        {
            ModifiedRange uniforms, transforms;
            
            ModifiedLightVolumeRanges() = default;
            ModifiedLightVolumeRanges (const ModifiedRange& a, const ModifiedRange& b)
                : uniforms (a), transforms (b) { }
        };

        struct ASyncActions;

        using DrawableObjects   = std::vector<MeshInstances>;
        using DrawCommands      = MultiDrawCommands<types::PMB>;
        using SyncObjects       = std::array<Sync, types::multiBuffering>;
        using QueryObjects      = std::array<Query, types::multiBuffering>;
                
        scene::Context*     m_scene             { };            //!< Used to render the scene from the correct viewpoint.
        Uniforms            m_uniforms          { };            //!< Uniform data which is accessible to any program that requests it.
        Programs            m_programs          { };            //!< Stores the programs used in different rendering passes.

        DrawableObjects     m_dynamics          { };            //!< A collection of dynamic mesh instances that need drawing.
        ShadowMaps          m_shadowMaps        { };            //!< Used to produce shadow maps for spotlights in the scene.
        Materials           m_materials         { };            //!< Contains every material in the scene, used for filling instancing data for dynamic objects.
        
        DrawCommands        m_objectDrawing     { };            //!< Draw commands for dynamic objects.
        types::PMB          m_objectMaterialIDs { };            //!< Material ID instancing data for dynamic objects.
        types::PMB          m_objectTransforms  { };            //!< Model transforms for dynamic objects.

        DrawCommands        m_lightDrawing      { };            //!< Draw commands for light volumes.
        types::PMB          m_lightTransforms   { };            //!< Model transforms for light volumes.

        GeometryBuffer      m_gbuffer           { };            //!< The initial framebuffer where geometry is drawn to.
        LightBuffer         m_lbuffer           { };            //!< A colour buffer where lighting is applied using data stored in the gbuffer.
        
        Geometry            m_geometry          { };            //!< A collection of OpenGL objects which store the scene geometry.
        SMAA                m_smaa              { };            //!< Used to perform antialiasing.

        Resolution          m_resolution        { };            //!< The internal and display resolution of drawing operations.
        
        size_t              m_partition         { 0 };          //!< The buffer partition to use when rendering the current frame.
        SyncObjects         m_syncs             { };            //!< Contains sync objects for each level of buffering, allows us to manually synchronise with the GPU if needed.
        QueryObjects        m_queries           { };            //!< A collection of query objects used to check how long each frame took to complete.
       
        bool                m_deferredRender    { true };       //!< Whether a deferred or forward render should be performed.
        bool                m_multiThreaded     { true };       //!< Whether the renderer should be multi-threaded or not.
        bool                m_pbs               { true };       //!< Whether physically based shaders should be used.
        SMAA::Quality       m_smaaQuality       { defaultAA };  //!< The current quality setting for SMAA.

        GLuint              m_syncCount         { 0 };          //!< How many times we've had to manually synchronise the GPU with the CPU.
        GLuint              m_frames            { 0 };          //!< How many frames have been renderered.
        GLfloat             m_totalTime         { 0 };          //!< The total time elapsed for all frames.
        GLfloat             m_minTime           { 0 };          //!< The minimum amount of time for a frame to render.
        GLfloat             m_maxTime           { 0 };          //!< The maximum amount of time for a frame to render.

    private:

        /// <summary>
        /// Attempts to build the OpenGL programs. 
        /// </summary>
        bool buildPrograms() noexcept;

        /// <summary>
        /// Attempts to load the texture and material data of every object in the scene.
        /// </summary>
        bool buildMaterials() noexcept;

        /// <summary>
        /// Attempts to build the dynamic object command and instancing buffers. This parses the instances container
        /// in the given scene to determine how many dynamic instances exist and allocates enough memory to accomodate
        /// that.
        /// </summary> 
        bool buildDynamicObjectBuffers() noexcept;

        /// <summary>
        /// Attempts to build the light command and transform buffers. This counts the total number of rendering passes
        /// that will occur and allocates enough memory to perform each pass.
        /// </summary> 
        bool buildLightBuffers() noexcept;

        /// <summary>
        /// Attempts to build the geometry data for every mesh in the scene.
        /// </summary> 
        bool buildGeometry() noexcept;

        /// <summary> 
        /// Attempt to build the geometry and light buffers according to the current internal resolution. 
        /// </summary>
        bool buildFramebuffers() noexcept;

        /// <summary>
        /// Attempts to build the uniform data. This will attempt to bind each block to every loaded program, material
        /// texture units will be registered, frambuffer texture units will be registered and light counts will be
        /// specified.
        /// </summary> 
        bool buildUniforms() noexcept;

        /// <summary> 
        /// Prepares the SMAA object for performing antialiasing.
        /// </summary>
        bool buildSMAA() noexcept;

        /// <summary>
        /// Fills the drawable instances container with non-static instances found in the given scene
        /// </summary> 
        void fillDynamicInstances() noexcept;

        /// <summary> Checks the sync object of the current partition and waits if it hasn't already fired. </summary>
        void syncWithGPUIfNecessary() noexcept;

        /// <summary> Performs a forward render of the entire scene. </summary>
        void forwardRender (const MultiDrawCommands<Buffer>& staticObjects, SceneVAO& sceneVAO, ASyncActions& actions) noexcept;

        /// <summary> Performs a deferred render of the entire scene. </summary>
        void deferredRender (const MultiDrawCommands<Buffer>& staticObjects, SceneVAO& sceneVAO, ASyncActions& actions) noexcept;

        /// <summary> Updates the scene uniforms such as the camera position, ambient lighting and matrices. </summary>
        ModifiedRange updateSceneUniforms() noexcept;

        /// <summary> Updates the draw commands, transforms and materail IDs of dynamic objects. </summary>
        ModifiedDynamicObjectRanges updateDynamicObjects() noexcept;

        /// <summary> Adds a draw command for a full-screen quad and every point and spotlight in the scene. </summary>
        ModifiedRange updateLightDrawCommands (const GLuint pointLights, const GLuint spotlights) noexcept;

        /// <summary> Updates the directional light uniform data with the given light data. </summary>
        ModifiedRange updateDirectionalLights (const std::vector<scene::DirectionalLight>& lights) noexcept;

        /// <summary> Updates the transform and uniform data for every given point light. </summary>
        ModifiedLightVolumeRanges updatePointLights (const std::vector<scene::PointLight>& lights) noexcept;

        /// <summary> Updates the transform and uniform data for every given spot light. </summary>
        ModifiedLightVolumeRanges updateSpotlights (const std::vector<scene::SpotLight>& lights, 
            const size_t transformOffset) noexcept;

        /// <summary> 
        /// Calls the given functions for each dynamic mesh. The function should take a size_t, Mesh and 
        /// MeshInstances::Instances parameter. All of which should be constant.
        /// </summary>
        template <typename... Funcs>
        void forEachDynamicMesh (Funcs&&... funcs) const noexcept;

        /// <summary>
        /// Calls the given function for each dynamic instance. The function should take a size_t and 
        /// scene::InstanceId parameter. All of which should be constant if references. Extra functions will be passed
        /// to forEachDynamicMesh to be called on each mesh.
        /// </summary>
        template <typename Func, typename... MeshFuncs>
        void forEachDynamicMeshInstance (const Func& func, MeshFuncs&&... meshFuncs) const noexcept;

        /// <summary> Calls the given function, passing the index, mesh and instances as parameters. </summary>
        template <typename A, typename B, typename Func>
        void processMesh (const A index, const B& meshInstances, const Func& func) const noexcept
        {
            func (index, meshInstances.mesh, meshInstances.instances);
        }
        
        /// <summary> Calls the given functions, passing the index, mesh and instances as parameters. </summary>
        template <typename A, typename B, typename Func, typename... Funcs>
        void processMesh (const A index, const B& meshInstances, const Func& func, Funcs&&... funcs) const noexcept
        {
            processMesh (index, meshInstances, func);
            processMesh (index, meshInstances, std::forward<Funcs> (funcs)...);
        }

        /// <summary> 
        /// Processes each of the given lights under the assumption that uniform data is being modified. The given
        /// function will be called on each object and it should take a scene light and a uniform light as parameters.
        /// <summary>
        template <typename Lights, typename UniformBlock, typename Func>
        ModifiedRange processLightUniforms (UniformBlock& uniforms, const Lights& lights, const Func& func) const noexcept;

        /// <summary>
        /// Processes each of the given lights under the assumption that uniform data AND transform data is being
        /// modified. Both functions should take a scene light and uniform light of the given light type as parameters.
        /// </summary>
        template <typename Lights, typename UniformBlock, typename FuncA, typename FuncB>
        ModifiedLightVolumeRanges processLightVolumes (UniformBlock& uniforms, const Lights& lights, 
            const size_t transformOffset, const FuncA& uniFunc, const FuncB& transFunc) const noexcept;
};


template <typename... Funcs>
void Renderer::forEachDynamicMesh (Funcs&&... funcs) const noexcept
{
    auto index = size_t { 0 };
    for (const auto& meshInstances : m_dynamics)
    {
        processMesh (index, meshInstances, std::forward<Funcs> (funcs)...);
        ++index;
    }
}


template <typename Func, typename... MeshFuncs>
void Renderer::forEachDynamicMeshInstance (const Func& func, MeshFuncs&&... meshFuncs) const noexcept
{
    // Maintain a count whilst looping through every instance.
    auto index = size_t { 0 };
    forEachDynamicMesh ([&] (const auto meshIndex, const auto& mesh, const auto& instances)
    {
        for (const auto instanceID : instances)
        {
            func (index++, m_scene->getInstanceById (instanceID));
        }
    }, std::forward<MeshFuncs> (meshFuncs)...);
}


template <typename Lights, typename UniformBlock, typename Func>
ModifiedRange Renderer::processLightUniforms (UniformBlock& uniforms, const Lights& lights, const Func& func) const noexcept
{
    // Set the count and iterate through each light.
    const auto count = static_cast<GLuint> (lights.size());
    uniforms.data->count = count;

    // Fudge the brightness because the lights aren't really designed for PBS.
    const auto intensityScale = m_pbs ? 1.35f : 1.f;
    for (GLuint i { 0 }; i < count; ++i)
    {
        uniforms.data->objects[i] = func (lights[i], intensityScale);
    }

    // We need to know the size of the data we've written to.
    constexpr auto countSize = sizeof (uniforms.data->count);
    constexpr auto lightSize = sizeof (uniforms.data->objects[0]);

    return { uniforms.offset, static_cast<GLsizeiptr> (countSize + lightSize * count) };
}


template <typename Lights, typename UniformBlock, typename FuncA, typename FuncB>
Renderer::ModifiedLightVolumeRanges Renderer::processLightVolumes (UniformBlock& uniforms, const Lights& lights, 
    const size_t transformOffset, const FuncA& uniFunc, const FuncB& transFunc) const noexcept
{
    // We need the transform buffer pointer to write to.
    auto transforms = (ModelTransform*) m_lightTransforms.pointer (m_partition);

    // Set the count and iterate through each light.
    const auto count = static_cast<GLuint> (lights.size());
    uniforms.data->count = count;
    
    // Fudge the brightness because the lights aren't really designed for PBS.
    const auto intensityScale = m_pbs ? 1.35f : 1.f;
    for (GLuint i { 0 }; i < count; ++i)
    {
        const auto& sceneLight          = lights[i];
        uniforms.data->objects[i]       = uniFunc (sceneLight, intensityScale);
        transforms[transformOffset + i] = transFunc (sceneLight);
    }

    // We need to know the size of the data we've written to.
    constexpr auto countSize    = sizeof (uniforms.data->count);
    constexpr auto lightSize    = sizeof (uniforms.data->objects[0]);
    constexpr auto matrixSize   = sizeof (ModelTransform);

    // We need to take into account the partition offset and transform offset of the transform buffer.
    const auto partitionOffset  = m_lightTransforms.partitionOffset (m_partition);
    const auto matrixOffset     = static_cast<GLintptr> (partitionOffset + matrixSize * transformOffset);

    return 
    { 
        { uniforms.offset,  static_cast<GLsizeiptr> (countSize + lightSize * count) },
        { matrixOffset,     static_cast<GLsizeiptr> (matrixSize * count) }
    };
}

#endif // _RENDERING_RENDERER_