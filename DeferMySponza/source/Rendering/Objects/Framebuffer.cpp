#include "Framebuffer.hpp"


// STL headers.
#include <utility>


// Personal headers.
#include <Rendering/Binders/RenderbufferBinder.hpp>


Framebuffer::Framebuffer (Framebuffer&& move) noexcept
{
    *this = std::move (move);
}


Framebuffer& Framebuffer::operator= (Framebuffer&& move) noexcept
{
    if (this != &move)
    {
        // Ensure we don't leak.
        clean();

        m_buffer        = move.m_buffer;
        move.m_buffer   = 0U;
    }

    return *this;
}


bool Framebuffer::initialise() noexcept
{
    // Generate an object.
    auto buffer = GLuint { 0 };
    glGenFramebuffers (1, &buffer);

    // Check the validity before using it.
    if (buffer == 0U)
    {
        return false;
    }

    // Ensure we don't leak.
    clean();
    m_buffer = buffer;

    return true;
}


void Framebuffer::clean() noexcept
{
    if (isInitialised())
    {
        glDeleteFramebuffers (1, &m_buffer);
        m_buffer = 0U;
    }
}


void Framebuffer::attachRenderbuffer (const Renderbuffer& renderbuffer, GLenum attachment) noexcept
{
    // We need to bind the current framebuffer and the given renderbuffer to attach it.
    const auto fbBinder = FramebufferBinder<GL_FRAMEBUFFER> { m_buffer };
    const auto rbBinder = RenderbufferBinder<GL_RENDERBUFFER> { renderbuffer };

    // Add the renderbuffer as an attachment.
    glFramebufferRenderbuffer (GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, renderbuffer.getID());
}


bool Framebuffer::validate() noexcept
{
    // Bind the current framebuffer to check its status.
    const auto binder = FramebufferBinder<GL_FRAMEBUFFER> { m_buffer };

    // Check the status is valid.
    return glCheckFramebufferStatus (GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
}