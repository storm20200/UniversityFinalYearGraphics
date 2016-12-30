#pragma once

#if !defined    _RENDERING_OBJECTS_BUFFER_BINDER_
#define         _RENDERING_OBJECTS_BUFFER_BINDER_


// Personal headers.
#include <Rendering/Objects/Buffer.hpp>


/// <summary>
/// A simple RAII utility to bind a given buffer on construction and then unbind it when going out of scope.
/// </summary>
template <GLenum target>
struct BufferBinder final
{
    inline BufferBinder() noexcept = default;
    
    inline BufferBinder (const Buffer& buffer) noexcept
    {
        bind (buffer);
    }

    inline BufferBinder (const GLuint buffer) noexcept
    {
        bind (buffer);
    }

    inline ~BufferBinder()
    {
        unbind();
    }

    inline void bind (const Buffer& buffer) noexcept
    {
        glBindBuffer (target, buffer.getID());
    }

    inline void bind (const GLuint buffer) noexcept
    {
        glBindBuffer (target, buffer);
    }

    inline void unbind() noexcept
    {
        glBindBuffer (target, 0);
    }
};


#endif // _RENDERING_OBJECTS_BUFFER_BINDER_