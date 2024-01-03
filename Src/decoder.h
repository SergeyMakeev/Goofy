#pragma once

#include <cstddef>
namespace DecoderBC {
void decodeBlockDXT1(const unsigned char* source, unsigned char* target, size_t targetStide);
void decodeBlockDXT5(const unsigned char* source, unsigned char* target, size_t targetStide);
void decodeBlockETC1(const unsigned char* source, unsigned char* target, size_t targetStide);
void decodeBlockETC2(const unsigned char* source, unsigned char* target, size_t targetStide);
} // namespace DecoderBC
