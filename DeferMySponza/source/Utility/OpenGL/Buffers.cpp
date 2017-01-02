#include "Buffers.hpp"


// Personal headers.
#include <Rendering/Binders/BufferBinder.hpp>


namespace util
{
    bool bindBlockToProgram (const Buffer& buffer, const GLuint program, const GLchar* name, const GLuint bindingPoint,
        const GLintptr offsetIntoBuffer, const GLsizeiptr blockSize) noexcept
    {
        // Automatically bind and unbind the buffer.
        const auto binder = BufferBinder<GL_UNIFORM_BUFFER> (buffer);

        // Determine the block index.
        const auto index = glGetUniformBlockIndex (program, name);
        if (index == GL_INVALID_INDEX) return false;

        // Specify the block binding to be used.
        glUniformBlockBinding (program, index, bindingPoint);

        // Finally inform the GPU which part of the UBO to read data from.
        glBindBufferRange (GL_UNIFORM_BUFFER, bindingPoint, buffer.getID(), offsetIntoBuffer, blockSize);
        return true;
    }
}