#pragma once

// Reference Goofy Compressor implementation (slow)
// only useful to quickly iterate over the different ideas/heuristics
namespace goofyRef
{
    int compressDXT1(unsigned char* result, const unsigned char* input, unsigned int width, unsigned int height, unsigned int stride);
    int compressETC1(unsigned char* result, const unsigned char* input, unsigned int width, unsigned int height, unsigned int stride);
}