#pragma once

#if !defined    _RENDERING_COMPOSITES_PERSISTANT_MAPPED_BUFFER_
#define         _RENDERING_COMPOSITES_PERSISTANT_MAPPED_BUFFER_

// STL headers.
#include <array>
#include <cassert>


// Personal headers.
#include <Rendering/Objects/Buffer.hpp>


/// <summary>
/// Specifies a range of data that has been modified in a persistently mapped buffer.
/// </summary>
struct ModifiedRange final
{
    GLintptr    offset  { 0 };  //!< How many bytes into the buffer to modified data.
    GLsizeiptr  length  { 0 };  //!< How many bytes have been modified.

    ModifiedRange (const GLintptr offset, const GLsizei length) noexcept : offset (offset), length (length) {}

    ModifiedRange()                                             = default;
    ModifiedRange (ModifiedRange&&)                             = default;
    ModifiedRange (const ModifiedRange&) noexcept               = default;
    ModifiedRange& operator= (const ModifiedRange&) noexcept    = default;
    ModifiedRange& operator= (ModifiedRange&&) noexcept         = default;
    ~ModifiedRange()                                            = default;
};


/// <summary>
/// Manages a buffer that is intiaised with immutable storage and then it is mapped persistently, allowing for data
/// to be written at any time. This is a potentially dangerous object and needs to be handled carefully as to not
/// write to data which is already in use by the GPU.

/// The template paramenter determines how many partitions to split the buffer into. This allows for double and triple
/// buffering on the same buffer.
/// </summary>
template <size_t Partitions>
class PersistentMappedBuffer final
{
    public:

        static_assert (Partitions > 0, "Invalid partition value given to PersistentMappedBuffer. Must be larger than zero.");

        constexpr static auto partitions = Partitions;  //!< How many partitions the buffer is split into.

    public:
        PersistentMappedBuffer() noexcept                                   = default;
        
        PersistentMappedBuffer (PersistentMappedBuffer&& move) noexcept;
        PersistentMappedBuffer& operator= (PersistentMappedBuffer&& move) noexcept;

        PersistentMappedBuffer (const PersistentMappedBuffer&)              = delete;
        PersistentMappedBuffer& operator= (const PersistentMappedBuffer&)   = delete;
        ~PersistentMappedBuffer() { clean(); }


        /// <summary> Check if the buffer has been initialised and is ready to be used. </summary>
        inline bool isInitialised() const noexcept      { return m_buffer.isInitialised(); }

        /// <summary> Retrieves the internally stored buffer object. </summary>
        inline const Buffer& getBuffer() const noexcept { return m_buffer; }
        
        /// <summary> Gets the OpenGL ID of buffer object. </summary>
        inline GLuint getID() const noexcept            { return m_buffer.getID(); }

        /// <summary> Gets the size of the buffer in bytes. </summary>
        inline GLsizeiptr getSize() const noexcept        { return m_size; }
        

        /// <summary> 
        /// Initialise overload where the size is specified instead of the data to fill the buffer with. The contents
        /// of the buffer will be undefined, therefore write access is enforced with this overload.
        /// </summary>
        /// <param name="partition"> How much data to allocate for each partition. </param>
        /// <param name="read"> Will the mapped buffer be used for reading? </param>
        /// <param name="coherent"> Should reading and writing of data be synchronised with the GPU? </param>
        /// <returns> Whether the buffer was successfully created or not. </returns>
        bool initialise (const GLsizeiptr partitionSize, const bool read,const bool coherent) noexcept;

        /// <summary> 
        /// Attempt to construct and map a buffer with the given parameters. Will fail if the given size is not 
        /// divisible by the number of partitions. Successive calls will replace the buffer, invalidating any
        /// previously retrieved pointer. Upon failure the object will not be changed.
        /// </summary>
        /// <param name="data"> The data to fill the buffer with. </param>
        /// <param name="read"> Will the mapped buffer be used for reading? </param>
        /// <param name="write"> Will the mapped buffer be used for writing? </param>
        /// <param name="coherent"> Should reading and writing of data be synchronised with the GPU? </param>
        /// <returns> Whether the buffer was successfully created or not. </returns>
        template <typename T>
        bool initialise (const T& data, const bool read, const bool write, const bool coherent) noexcept;

        /// <summary> Deletes the buffer, freeing memory to the GPU. Also causes pointers to be invalidated. </summary>
        void clean() noexcept;


        /// <summary> Calculates the size of each individual partition in bytes. </summary>
        inline GLsizeiptr partitionSize() const noexcept                    
        { 
            return m_size / Partitions; 
        }

        /// <summary> Calculates the byte offset into the buffer of the given partition. </summary>
        /// <param name="index"> The partition to determine the offset for. </param>
        inline GLintptr partitionOffset (const size_t index) const noexcept 
        { 
            return index * partitionSize(); 
        }

        /// <summary> 
        /// Gets a pointer to the partition at the given index. Extreme care is required when handling the pointer.
        /// If an invalid index is given then the start of the buffer will be returned.
        /// </summary>
        /// <param name="partition"> The partition to return the pointer for. </param>
        inline const GLbyte* pointer (const size_t partition = 0) const noexcept
        {
            auto pointer = m_mapping + partitionOffset (partition);
            assert (pointer);
            return partition < Partitions ? pointer : m_mapping;
        }

        /// <summary> 
        /// Gets a pointer to the partition at the given index. Extreme care is required when handling the pointer.
        /// If an invalid index is given then the start of the buffer will be returned.
        /// </summary>
        /// <param name="partition"> The partition to return the pointer for. </param>
        inline GLbyte* pointer (const size_t partition = 0) noexcept
        {
            auto pointer = m_mapping + partitionOffset (partition);
            assert (pointer);
            return partition < Partitions ? pointer : m_mapping;
        }
        
        /// <summary> 
        /// Notifies OpenGL that it can find modified data at the specified range. Not required for coherent 
        /// buffers that were initialised as coherent data.
        /// </summary>
        /// <param name="partition"> The partition where data has changed. </param>
        /// <param name="range"> The range of data which has been modified. </param>
        void notifyModifiedDataRange (const size_t partition, const ModifiedRange& range) noexcept;

        /// <summary> 
        /// Notifies OpenGL that it can find modified data at the specified range. Not required for coherent 
        /// buffers that were initialised as coherent data.
        /// </summary>
        /// <param name="range"> The range of data which has been modified, includes partition offset. </param>
        void notifyModifiedDataRange (ModifiedRange range) noexcept;

    private:

        using Regions = std::array<void*, Partitions>;

        Buffer      m_buffer    { };            //!< The persistently mapped buffer.
        GLbyte*     m_mapping   { nullptr };    //!< A pointer provided by the GPU where we can write to.
        GLsizeiptr  m_size      { 0 };          //!< How large the buffer is.
        bool        m_flushable { false };      //!< If the PMB is not coherent but is writable we need to support flushing.

    private:
        
        /// <summary> Gets the necessary map buffer access flags for the given access rights. </summary>
        GLenum getAccessFlags (const bool read, const bool write, const bool coherent) const noexcept;
};


template <size_t Partitions>
using SinglePMB = PersistentMappedBuffer<1>;
using DoublePMB = PersistentMappedBuffer<2>;
using TriplePMB = PersistentMappedBuffer<3>;


// STL headers.
#include <utility>


template <size_t Partitions>
PersistentMappedBuffer<Partitions>::PersistentMappedBuffer (PersistentMappedBuffer<Partitions>&& move) noexcept
{
    *this = std::move (move);
}


template <size_t Partitions>
PersistentMappedBuffer<Partitions>& PersistentMappedBuffer<Partitions>::operator= (PersistentMappedBuffer<Partitions>&& move) noexcept
{
    if (this != &move)
    {
        // Ensure we don't leak.
        clean();

        m_buffer    = std::move (move.m_buffer);
        m_mapping   = move.m_mapping;
        m_size      = move.m_size;
        m_flushable = move.m_flushable;

        move.m_mapping      = nullptr;
        move.m_size         = 0U;
        move.m_flushable    = false;
    }

    return *this;
}


template <size_t Partitions>
bool PersistentMappedBuffer<Partitions>::initialise (const GLintptr size, const bool read, const bool coherent) noexcept
{
    // Read can't be enabled without write permissions because the buffer contents will be undefined. Also ensure the
    // size is valid.
    if (size == 0)
    {
        return false;
    }

    // Initialise a new buffer.
    auto buffer = Buffer { };
    if (!buffer.initialise())
    {
        return false;
    }

    // We need to allocate immutable storage to persistently map the buffer so we need to determine applicable flags.
    const auto access = getAccessFlags (read, true, coherent);

    // Buffer storage flags don't support GL_MAP_FLUSH_EXPLICIT_BIT so ensure we don't use that.
    const auto storageFlags = (access & (~GL_MAP_FLUSH_EXPLICIT_BIT));

    // Next we can allocate the storage with the correct bits.
    const auto totalSize = size * Partitions;
    buffer.allocateImmutableStorage (totalSize, storageFlags);

    // Ensure we can map the buffer.
    auto pointer = buffer.mapRange (0, totalSize, access);

    if (!pointer)
    {
        return false;
    }

    // Finally clean up after ourselves and utilise the new data!
    if (m_mapping)
    {
        m_buffer.unmap();
    }

    m_buffer    = std::move (buffer);
    m_mapping   = (GLbyte*) pointer;
    m_size      = totalSize;
    m_flushable = (access & GL_MAP_FLUSH_EXPLICIT_BIT) > 0;

    return true;
}


template <size_t Partitions>
template <typename T>
bool PersistentMappedBuffer<Partitions>::initialise (const T& data, const bool read, const bool write, const bool coherent) noexcept
{
    // First of all, ensure at least read or write is enabled.
    if (!read && !write)
    {
        return false;
    }

    // Initialise a new buffer.
    auto buffer = Buffer { };
    if (!buffer.initialise())
    {
        return false;
    }

    // We need to allocate immutable storage to persistently map the buffer so we need to determine applicable flags.
    const auto access = getAccessFlags (read, write, coherent);

    // The storage flags don't support GL_MAP_FLUSH_EXPLICIT_BIT so ensure we don't use that.
    auto storageFlags = (access & (~GL_MAP_FLUSH_EXPLICIT_BIT));

    // Next we can fill the buffer with data.
    const auto size = buffer.immutablyFillWith (data, storageFlags);

    // Check the size is valid.
    if (size % Partitions != 0 || size == 0)
    {
        return false;
    }

    // Ensure we can map the buffer.
    auto pointer = buffer.mapRange (0, size, access);

    if (!pointer)
    {
        return false;
    }

    // Finally clean up after ourselves and utilise the new data!
    if (m_mapping)
    {
        m_buffer.unmap();
    }

    m_buffer    = std::move (buffer);
    m_mapping   = (GLbyte*) pointer;
    m_size      = size;
    m_flushable = (access & GL_MAP_FLUSH_EXPLICIT_BIT) > 0;

    return true;
}


template <size_t Partitions>
void PersistentMappedBuffer<Partitions>::clean() noexcept
{
    if (isInitialised())
    {
        // Ensure we unmap the buffer first!
        m_buffer.unmap();
        m_buffer.clean();
        m_mapping   = nullptr;
        m_size      = 0U;
        m_flushable = false;
    }
}


template <size_t Partitions>
void PersistentMappedBuffer<Partitions>::notifyModifiedDataRange (const size_t partition, const ModifiedRange& range) noexcept
{
    if (m_flushable)
    {
        glFlushMappedNamedBufferRange (getID(), partitionOffset (partition) + range.offset, range.length);
    }
}


template <size_t Partitions>
void PersistentMappedBuffer<Partitions>::notifyModifiedDataRange (ModifiedRange range) noexcept
{
    if (m_flushable)
    {
        glFlushMappedNamedBufferRange (getID(), range.offset, range.length);
    }
}


template <size_t Partitions>
GLenum PersistentMappedBuffer<Partitions>::getAccessFlags (const bool read, const bool write, const bool coherent) const noexcept
{
    auto access = GL_MAP_PERSISTENT_BIT;
    
    if (read)
    {
        access |= GL_MAP_READ_BIT;
    }

    if (write)
    {
        access |= GL_MAP_WRITE_BIT;
    }

    // Coherency means we don't need to manually flush.
    if (coherent)
    {
        access |= GL_MAP_COHERENT_BIT;
    }

    // The explicity flush bit is only valid on writes.
    else if (write)
    {
        access |= GL_MAP_FLUSH_EXPLICIT_BIT;
    }

    return access;
}

#endif // _RENDERING_COMPOSITES_PERSISTANT_MAPPED_BUFFER_