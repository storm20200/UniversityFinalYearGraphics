#include "ShadowMaps.hpp"


// Engine headers.
#include <glm/gtc/matrix_transform.hpp>
#include <scene/scene.hpp>


// Personal headers.
#include <Utility/Scene.hpp>


ShadowMaps::ShadowMaps (ShadowMaps&& move) noexcept
{
    *this = std::move (move);
}


GLint ShadowMaps::operator[] (const scene::LightId lightID) const noexcept
{
    const auto iterator = m_ids.find (lightID);
    return iterator != std::cend (m_ids) ? iterator->second : -1;
}


bool ShadowMaps::isInitialised() const noexcept
{
    return m_fbo.isInitialised() && m_maps.isInitialised();
}


bool ShadowMaps::initialise (const std::vector<scene::SpotLight>& spotlights, const GLuint textureUnit) noexcept
{
    // Create temporary objects.
    auto fbo    = decltype (m_fbo) { };
    auto maps   = decltype (m_maps) { };
    auto lights = decltype (m_lights) { };
    auto ids    = decltype (m_ids) { };

    if (!(fbo.initialise() && maps.initialise (textureUnit)))
    {
        return false;
    }

    // Add each light marked as casting shadows.
    lights.reserve (spotlights.size());
    for (const auto& light : spotlights)
    {
        if (light.getCastShadow())
        {
            ids[light.getId()] = static_cast<GLint> (lights.size());
            lights.push_back (light.getId());
        }
    }
    lights.shrink_to_fit();

    // Now set the resolution of the shadow maps to either the hard-coded value or the maximum size for the GPU.
    GLint maxTextureResolution;
    glGetIntegerv (GL_MAX_TEXTURE_SIZE, &maxTextureResolution);

    const auto resolution   = static_cast<GLsizei> (std::min (maxTextureResolution, maxResolution));
    const auto depth        = static_cast<GLsizei> (lights.size());

    // Finally allocate memory in the sample before we use the temporary data.
    maps.allocateImmutableStorage (GL_DEPTH_COMPONENT, resolution, resolution, depth);

    m_fbo       = std::move (fbo);
    m_maps      = std::move (maps);
    m_lights    = std::move (lights);
    m_ids       = std::move (ids);
    return true;
}


void ShadowMaps::clean() noexcept
{
    m_fbo.clean();
    m_maps.clean();
    m_lights.clear();
    m_ids.clear();
}


ModifiedRange ShadowMaps::setUniforms (const scene::Context* scene, glm::mat4* pointer, 
    GLsizeiptr start) const noexcept
{
    // Ensure we have a scene and that there are any spotlights to set uniforms for.
    assert (scene);
    if (m_lights.empty())
    {
        return { 0, 0 };
    }

    // We need to go through each light in the scene and create a view transform for them.
    const auto& spotlights      = scene->getAllSpotLights();
    const auto  shadowCasters   = m_lights.size();
    const auto  upDirection     = util::toGLM (scene->getUpDirection());
    
    // Assume the order hasn't changed since we retrieved light IDs.
    auto currentIndex   = size_t { 0 };
    auto currentID      = m_lights[currentIndex];
    for (const auto& spotlight : spotlights)
    {
        if (spotlight.getId() == currentID)
        {
            // Create a view transform from the perspective of the light.
            const auto position     = util::toGLM (spotlight.getPosition());
            const auto direction    = util::toGLM (spotlight.getDirection());
            pointer[currentIndex++] = glm::lookAt (position, position + direction, upDirection);

            // Increment the ID we're finding.
            if (currentIndex < shadowCasters)
            {
                currentID = m_lights[currentIndex];
            }

            else
            {
                break;
            }
        }
    }

    return { start, static_cast<GLsizei> (sizeof (*pointer) * currentIndex) };
}