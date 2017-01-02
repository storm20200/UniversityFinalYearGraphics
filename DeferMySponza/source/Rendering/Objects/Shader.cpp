#include "Shader.hpp"


// STL headers.
#include <iostream>
#include <utility>


// Engine headers.
#include <tgl/tgl.h>
#include <tygra/FileHelper.hpp>


Shader::Shader (Shader&& move) noexcept
{
    *this = std::move (move);
}


Shader& Shader::operator= (Shader&& move) noexcept
{
    if (this != &move)
    {
        // Ensure we don't leak a shader.
        clean();

        m_shader    = move.m_shader;
        m_type      = move.m_type;

        move.m_shader   = 0U;
        move.m_type     = 0U;
    }

    return *this;
}


bool Shader::initialise (const std::string& file, const GLenum type) noexcept
{
    // Ensure we separate the .c_str() call as daisy chaining the function causes garbage data.
    const auto  string  = tygra::createStringFromFile (file);
    auto        code    = string.c_str();

    // Attempt to compile the shader.
    auto shaderID = glCreateShader (type);

    glShaderSource (shaderID, 1, &code, NULL);
    glCompileShader (shaderID);

    // Check whether compilation was successful.
    auto compileStatus = GLint { 0 };
    glGetShaderiv (shaderID, GL_COMPILE_STATUS, &compileStatus);
    
    if (compileStatus != GL_TRUE)
    {
        // Output error information.
        const auto stringLength = 1024U;
        GLchar log[stringLength] = "";

        glGetShaderInfoLog (shaderID, stringLength, NULL, log);
        std::cerr << log << std::endl;
        return false;
    }

    // Delete any previously compiled shader.
    clean();

    // Success!
    m_shader    = shaderID;
    m_type      = type;
    
    return true;
}


void Shader::clean() noexcept
{
    if (isInitialised())
    {   
        glDeleteShader (m_shader);
        m_shader    = 0U;
        m_type      = 0U;
    }
}