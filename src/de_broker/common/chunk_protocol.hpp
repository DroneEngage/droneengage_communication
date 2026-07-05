#ifndef CHUNK_PROTOCOL_HPP
#define CHUNK_PROTOCOL_HPP

#include <vector>
#include <cstdint>
#include <cstring>

namespace de {
namespace comm {

/**
 * @brief Chunk protocol constants and helper functions
 * Shared between UDP and Unix socket implementations to avoid code duplication
 */
namespace chunk_protocol {

// Chunk protocol constants
constexpr uint16_t CHUNK_END_MARKER = 0xFFFF;
constexpr size_t CHUNK_HEADER_SIZE = 2 * sizeof(uint8_t);

/**
 * @brief Prepare a chunk header
 * @param chunkNumber The chunk number (0, 1, 2, ...)
 * @param isLastChunk Whether this is the last chunk
 * @param header Buffer to write the 2-byte header (must be at least 2 bytes)
 */
inline void prepareChunkHeader(uint16_t chunkNumber, bool isLastChunk, uint8_t* header) {
    if (isLastChunk) {
        header[0] = 0xFF;
        header[1] = 0xFF;
    } else {
        header[0] = static_cast<uint8_t>(chunkNumber & 0xFF);
        header[1] = static_cast<uint8_t>((chunkNumber >> 8) & 0xFF);
    }
}

/**
 * @brief Parse chunk number from received data
 * @param data The received data buffer (must be at least 2 bytes)
 * @return The chunk number (or CHUNK_END_MARKER if last chunk)
 */
inline uint16_t parseChunkNumber(const uint8_t* data) {
    return (data[1] << 8) | data[0];
}

/**
 * @brief Check if chunk is the end marker
 * @param chunkNumber The parsed chunk number
 * @return true if this is the last chunk (0xFFFF)
 */
inline bool isEndChunk(uint16_t chunkNumber) {
    return chunkNumber == CHUNK_END_MARKER;
}

/**
 * @brief Reassemble chunks into a complete message
 * @param chunks Vector of chunk data (without headers)
 * @return Concatenated message data
 */
inline std::vector<uint8_t> reassembleChunks(const std::vector<std::vector<uint8_t>>& chunks) {
    std::vector<uint8_t> concatenatedData;
    for (const auto& chunk : chunks) {
        concatenatedData.insert(concatenatedData.end(), chunk.begin(), chunk.end());
    }
    return concatenatedData;
}

} // namespace chunk_protocol
} // namespace comm
} // namespace de

#endif // CHUNK_PROTOCOL_HPP
