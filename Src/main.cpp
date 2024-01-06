#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <unordered_map>

#ifndef _WIN32
#define __cdecl
#define __stdcall
#define __fastcall
//and others
#endif

#define GOOFYTC_IMPLEMENTATION
#include "../GoofyTC/goofy_tc.h"

#define STB_DXT_IMPLEMENTATION
#include "../ThirdParty/ryg/stb_dxt.h"

#include "../ThirdParty/rgetc/rg_etc1.h"
#include "../ThirdParty/rgetc/rg_etc1.cpp"

#define RGBCX_IMPLEMENTATION
#include "../ThirdParty/bc7enc/rgbcx.h"

#define ICBC_IMPLEMENTATION
#include "../ThirdParty/icbc/icbc.h"

// reference goofy implementation
#include "goofy_tc_reference.h"

#include "../ThirdParty/lodepng/lodepng.h"
#include "../ThirdParty/lodepng/lodepng.cpp"

#include "decoder.h"

const int kNumberOfIterations = 128;

struct Timer
{
    std::chrono::time_point<std::chrono::high_resolution_clock> startingTime;

    Timer() {}

    void begin() { startingTime = std::chrono::high_resolution_clock::now(); }

    // return number of microseconds
    uint64_t end()
    {
        const auto endingTime = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(endingTime - startingTime).count();
    }
};

// KTX / DDS / TGA support
// ============================================================================================
static const uint8_t kKtxIdentifier[12] = { 0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A };
static const uint32_t kKtxEndianness = 0x04030201;
static const uint32_t kKtxFormatETC1 = 0x8D64; // GL_ETC1_RGB8_OES
static const uint32_t kKtxFormatETC2 = 0x9278; // GL_COMPRESSED_RGBA8_ETC2_EAC
static const uint32_t kKtxFormatDXT1 = 0x83F0; // GL_COMPRESSED_RGB_S3TC_DXT1_EXT 
static const uint32_t kKtxFormatDXT5 = 0x83F3; // GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
static const uint32_t kKtxRGB = 0x1907;        // GL_RGB
static const uint32_t kKtxRGBA = 0x1908;       // GL_RGBA

static const uint32_t kDdsFormatDXT1 = 0x31545844;
static const uint32_t kDdsFormatDXT5 = 0x35545844;

#pragma pack(push)
#pragma pack(1)

//
// https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/#2.7
//
struct KtxHeader
{
    uint8_t  identifier[12];
    uint32_t endianness;
    uint32_t glType;
    uint32_t glTypeSize;
    uint32_t glFormat;
    uint32_t glInternalFormat;
    uint32_t glBaseInternalFormat;
    uint32_t pixelWidth;
    uint32_t pixelHeight;
    uint32_t pixelDepth;
    uint32_t numberOfArrayElements;
    uint32_t numberOfFaces;
    uint32_t numberOfMipmapLevels;
    uint32_t bytesOfKeyValueData;
};

//
// https://docs.microsoft.com/en-us/windows/win32/direct3ddds/dds-header
//
struct DdsPixelFormat
{
    uint32_t size;
    uint32_t flags;
    uint32_t fourCC;
    uint32_t rgbBitCount;
    uint32_t rBitMask;
    uint32_t gBitMask;
    uint32_t bBitMask;
    uint32_t aBitMask;
};

struct DdsHeader
{
    uint32_t magic;
    uint32_t size;
    uint32_t flags;
    uint32_t height;
    uint32_t width;
    uint32_t pitchOrLinearSize;
    uint32_t depth;
    uint32_t mipMapCount;
    uint32_t reserved1[11];
    DdsPixelFormat ddspf;
    uint32_t caps;
    uint32_t caps2;
    uint32_t caps3;
    uint32_t caps4;
    uint32_t reserved2;
};

//
// http://www.paulbourke.net/dataformats/tga/
//
struct TgaHeader
{
    uint8_t idLength;
    uint8_t colourMapType;
    uint8_t imageType;
    uint16_t firstEntry;
    uint16_t numEntries;
    uint8_t bitsPerEntry;
    uint16_t xOrigin;
    uint16_t yOrigin;
    uint16_t width;
    uint16_t height;
    uint8_t bitsPerPixel;
    uint8_t descriptor;
};

#pragma pack(pop)

void saveDds(const char* fileName, const unsigned char* data, size_t dataSize, uint32_t width, uint32_t height, uint32_t ddsFourCC)
{
    DdsHeader header;
    memset(&header, 0, sizeof(DdsHeader));
    header.magic = 0x20534444;
    header.size = sizeof(DdsHeader) - sizeof(uint32_t);
    header.flags = 0x000A1007; // CAPS, HEIGHT, WIDTH, PIXELFORMAT, MIPMAPCOUNT, DEPTH
    header.width = width;
    header.height = height;
    header.depth = 1;
    header.pitchOrLinearSize = (uint32_t)dataSize;
    header.mipMapCount = 1;
    header.ddspf.size = sizeof(DdsPixelFormat);
    header.ddspf.flags = 0x4; // FOURCC
    header.ddspf.fourCC = ddsFourCC;

    FILE* file = fopen(fileName, "wb");
    if (file)
    {
        fwrite(&header, 1, sizeof(DdsHeader), file);
        fwrite(data, 1, dataSize, file);
        fclose(file);
    }
    else
    {
        printf("Can't write file '%s'\n", fileName);
    }

}

void saveKtx(const char* fileName, const unsigned char* data, size_t dataSize, uint32_t width, uint32_t height, uint32_t ktxFormat)
{
    KtxHeader header;
    memset(&header, 0, sizeof(KtxHeader));
    memcpy(&header.identifier[0], &kKtxIdentifier[0], 12);
    header.endianness = kKtxEndianness;
    //header.glType = 0;
    header.glTypeSize = 1;
    //header.glFormat = 0;
    header.glInternalFormat = ktxFormat;
    header.glBaseInternalFormat = (ktxFormat == kKtxFormatETC1) ? kKtxRGB : kKtxRGBA;
    header.pixelWidth = width;
    header.pixelHeight = height;
    //header.pixelDepth = 0;
    //header.numberOfArrayElements = 0;
    header.numberOfFaces = 1;
    header.numberOfMipmapLevels = 1;
    //header.bytesOfKeyValueData = 0;

    uint32_t imageSize = (uint32_t)dataSize;

    FILE* file = fopen(fileName, "wb");
    if (file)
    {
        fwrite(&header, 1, sizeof(KtxHeader), file);
        fwrite(&imageSize, 1, sizeof(uint32_t), file);
        fwrite(data, 1, dataSize, file);
        fclose(file);
    }
    else
    {
        printf("Can't write file '%s'\n", fileName);
    }

}

void saveTga(const char* fileName, const unsigned char* data, uint32_t width, uint32_t height)
{
    TgaHeader header;
    memset(&header, 0, sizeof(TgaHeader));
    header.imageType = 2;
    header.width = width;
    header.height = height;
    header.bitsPerPixel = 32; // RGBA
    header.descriptor = 0x20; // set proper image orientation

    // convert RGBA to BGRA
    uint32_t bytesCount = width * height * 4;
    std::vector<uint8_t> bgra(bytesCount);
    for (uint32_t i = 0; i < bytesCount; i+=4)
    {
        bgra[i + 2] = data[i + 0];
        bgra[i + 1] = data[i + 1];
        bgra[i + 0] = data[i + 2];
        bgra[i + 3] = data[i + 3];
    }

    FILE* file = fopen(fileName, "wb");
    if (file)
    {
        fwrite(&header, 1, sizeof(header), file);
        fwrite(&bgra[0], 1, bytesCount, file);
        fclose(file);
    }
    else
    {
        printf("Can't write file '%s'\n", fileName);
    }
}

unsigned char* loadPngAsRgba8(const char* fileName, unsigned int* pWidth, unsigned int* pHeight)
{
    if (!pWidth || !pHeight)
    {
        return nullptr;
    }

    std::vector<unsigned char> image;
    unsigned int width, height;
    unsigned int error = lodepng::decode(image, width, height, fileName);

    if (error)
    {
        printf("Can't open image : %s, Error : %s\n", fileName, lodepng_error_text(error));
        return nullptr;
    }

    if ((width % 16) != 0)
    {
        printf("Incorrect width. Width should be a multiple of 16\n");
        return nullptr;
    }

    if ((height % 4) != 0)
    {
        printf("Incorrect height. Height should be a multiple of 4\n");
        return nullptr;
    }

    size_t sizeInBytes = width * height * 4;
#ifdef _WIN32
    unsigned char* rgbaBuffer = (unsigned char*) _aligned_malloc(sizeInBytes, 64);
#else
    unsigned char* rgbaBuffer = (unsigned char*) aligned_alloc(64, sizeInBytes);
#endif

    int bytesPerPixel = ((int) image.size() / (width * height));

    for (unsigned int y = 0; y < height; y++)
    {
        for (unsigned int x = 0; x < width; x++)
        {
            unsigned char* srcPixel = &image[(y * width + x) * bytesPerPixel];
            unsigned char* dstPixel = &rgbaBuffer[(y * width + x) * 4];
            
            dstPixel[0] = srcPixel[0];
            dstPixel[1] = srcPixel[1];
            dstPixel[2] = srcPixel[2];
            if (bytesPerPixel <= 3)
            {
                dstPixel[3] = 0xFF;
            } else
            {
                //dstPixel[3] = srcPixel[3];
                dstPixel[3] = 0xFF;
            }
        }
    }

    *pWidth = width;
    *pHeight = height;

    return rgbaBuffer;
}

void destroyPng(unsigned char* rgba8)
{
    if (!rgba8)
    {
        return;
    }
#ifdef _WIN32
    _aligned_free(rgba8);
#else
    free(rgba8);
#endif
}

// ============================================================================================

// PSNR / MSE
//
// Peak signal-to-noise ratio
// https://en.wikipedia.org/wiki/Peak_signal-to-noise_ratio
// https://en.wikipedia.org/wiki/Mean_squared_error
//
struct MsePsnr
{
    double mseR;
    double mseG;
    double mseB;
    double mseA;
    double mseMax;

    double mseRGB;
    double mseY;

    double psnrR;
    double psnrG;
    double psnrB;
    double psnrA;
    double psnrMin;

    double psnrRGB;
    double psnrY;
};

double getPSNR(double mse, const double pixelMaxValue = 255.0)
{
    if (mse < 0.0000001)
    {
        return DBL_MAX;
    }

    double psnr = 10.0 * log10((pixelMaxValue * pixelMaxValue) / mse);
    return psnr;
}

static double getLuminosity(double r, double g, double b)
{
    return 0.21 * r + 0.72 * g + 0.07 * b;
}

static MsePsnr getMsePsnr(const unsigned char* buf1, const unsigned char* buf2, uint32_t width, uint32_t height)
{
    const uint32_t* img1 = (const uint32_t*)buf1;
    const uint32_t* img2 = (const uint32_t*)buf2;

    MsePsnr res;
    res.mseR = 0.0;
    res.mseG = 0.0;
    res.mseB = 0.0;
    res.mseA = 0.0;
    res.mseRGB = 0.0;
    res.mseY = 0.0;
    for (uint32_t y = 0; y < height; y++)
    {
        for (uint32_t x = 0; x < width; x++)
        {
            uint32_t addr = y * width + x;
            const uint32_t& pix1 = img1[addr];
            const uint32_t& pix2 = img2[addr];
            
            double r1 = (double)((pix1 >> 16) & 0xFF);
            double g1 = (double)((pix1 >> 8) & 0xFF);
            double b1 = (double)(pix1 & 0xFF);
            double a1 = (double)((pix1 >> 24) & 0xFF);

            double r2 = (double)((pix2 >> 16) & 0xFF);
            double g2 = (double)((pix2 >> 8) & 0xFF);
            double b2 = (double)(pix2 & 0xFF);
            double a2 = (double)((pix2 >> 24) & 0xFF);

            double y1 = getLuminosity(r1, g1, b1);
            double y2 = getLuminosity(r2, g2, b2);

            double dY = (y2 - y1);
            res.mseY += (dY * dY);

            double dR = r1 - r2;
            double dG = g1 - g2;
            double dB = b1 - b2;
            double dA = a1 - a2;

            res.mseRGB += (dR * dR) + (dG * dG) + (dB * dB);

            res.mseR += (dR * dR);
            res.mseG += (dG * dG);
            res.mseB += (dB * dB);
            res.mseA += (dA * dA);
        }
    }

    double pixelsCount = width * height;
    res.mseR = res.mseR / pixelsCount;
    res.mseG = res.mseG / pixelsCount;
    res.mseB = res.mseB / pixelsCount;
    res.mseA = res.mseA / pixelsCount;
    res.mseRGB = res.mseRGB / pixelsCount;
    res.mseY = res.mseY / pixelsCount;
    res.mseMax = std::max(std::max(res.mseR, res.mseG), std::max(res.mseB, res.mseA));
    res.psnrR = getPSNR(res.mseR);
    res.psnrG = getPSNR(res.mseG);
    res.psnrB = getPSNR(res.mseB);
    res.psnrA = getPSNR(res.mseA);
    res.psnrMin = std::min(std::min(res.psnrR, res.psnrG), std::min(res.psnrB, res.psnrA));
    res.psnrRGB = getPSNR(res.mseRGB, 768.0f);
    res.psnrY = getPSNR(res.mseY);
    return res;
}

static char printBuffer[512];
std::string psnrToString(double psnr)
{
    if (psnr > 999999.0)
    {
        strcpy(printBuffer, "+inf");
        return printBuffer;
    }

    sprintf(printBuffer, "%3.5f", psnr);
    return printBuffer;
}

std::string formatResultsRGB(const MsePsnr& res)
{
    std::string psnrR = psnrToString(res.psnrR);
    std::string psnrG = psnrToString(res.psnrG);
    std::string psnrB = psnrToString(res.psnrB);
    std::string psnrA = psnrToString(res.psnrA);
    std::string psnrMin = psnrToString(res.psnrMin);
    std::string psnrRGB = psnrToString(res.psnrRGB);
    std::string psnrY = psnrToString(res.psnrY);
    sprintf(printBuffer, "%3.5f;%3.5f;%3.5f;%3.5f;%3.5f;%3.5f;%s;%s;%s;%s;%s;%s", 
        res.mseR, res.mseG, res.mseB, res.mseMax, res.mseRGB, res.mseY,
        psnrR.c_str(), psnrG.c_str(), psnrB.c_str(), psnrMin.c_str(), psnrRGB.c_str(), psnrY.c_str());
    return printBuffer;
}

// ============================================================================================

void generateDownsamlpedRgb565Test(const unsigned char* inRgba8, uint32_t width, uint32_t height, unsigned char* outRgba8)
{
    const uint32_t* inputRgba8 = (const uint32_t*)inRgba8;
    uint32_t* outputRgba8 = (uint32_t*)outRgba8;
    for (uint32_t y = 0; y < height; y += 2)
    {
        for (uint32_t x = 0; x < width; x += 2)
        {
            const uint32_t& p0 = inputRgba8[(y + 0) * width + (x + 0)];
            const uint32_t& p1 = inputRgba8[(y + 1) * width + (x + 0)];
            const uint32_t& p2 = inputRgba8[(y + 0) * width + (x + 1)];
            const uint32_t& p3 = inputRgba8[(y + 1) * width + (x + 1)];

            // simple box filter
            uint32_t r = 2;
            uint32_t g = 2;
            uint32_t b = 2;

            r += uint32_t((p0 >> 16) & 0xFF);
            g += uint32_t((p0 >> 8) & 0xFF);
            b += uint32_t(p0 & 0xFF);

            r += uint32_t((p1 >> 16) & 0xFF);
            g += uint32_t((p1 >> 8) & 0xFF);
            b += uint32_t(p1 & 0xFF);

            r += uint32_t((p2 >> 16) & 0xFF);
            g += uint32_t((p2 >> 8) & 0xFF);
            b += uint32_t(p2 & 0xFF);

            r += uint32_t((p3 >> 16) & 0xFF);
            g += uint32_t((p3 >> 8) & 0xFF);
            b += uint32_t(p3 & 0xFF);

            r = r >> 2;
            g = g >> 2;
            b = b >> 2;

            // RGB888 to RGB565
            r = r >> 3;
            g = g >> 2;
            b = b >> 2;

            // RGB565 back to RGB888 (replicate higher bits to lower bits to get better quality)
            r = (r << 3) | ((r >> 2) & 7);
            g = (g << 2) | ((g >> 4) & 3);
            b = (b << 2) | ((b >> 2) & 7);

            uint32_t pix = ((r << 16) | (g << 8) | b | 0xFF000000);

            outputRgba8[(y + 0) * width + (x + 0)] = pix;
            outputRgba8[(y + 1) * width + (x + 0)] = pix;
            outputRgba8[(y + 0) * width + (x + 1)] = pix;
            outputRgba8[(y + 1) * width + (x + 1)] = pix;
        }
    }
}

// ============================================================================================

void decompressDXT1(const unsigned char* data, uint32_t width, uint32_t height, unsigned char* rgba8)
{
    memset(rgba8, 0, width * height * 4);
    uint32_t blockW = width / 4;
    uint32_t blockH = height / 4;
    uint32_t stride = width * 4;
    for (uint32_t by = 0; by < blockH; by++)
    {
        for (uint32_t bx = 0; bx < blockW; bx++)
        {
            uint32_t x = bx * 4;
            uint32_t y = by * 4;
            DecoderBC::decodeBlockDXT1(data, rgba8 + (y * width + x) * 4, stride);
            data+=8;
        }
    }
}

void decompressDXT5(const unsigned char* data, uint32_t width, uint32_t height, unsigned char* rgba8)
{
    memset(rgba8, 0, width * height * 4);
    uint32_t blockW = width / 4;
    uint32_t blockH = height / 4;
    uint32_t stride = width * 4;
    for (uint32_t by = 0; by < blockH; by++)
    {
        for (uint32_t bx = 0; bx < blockW; bx++)
        {
            uint32_t x = bx * 4;
            uint32_t y = by * 4;
            DecoderBC::decodeBlockDXT5(data, rgba8 + (y * width + x) * 4, stride);
            data += 16;
        }
    }
}

void decompressETC1(const unsigned char* data, uint32_t width, uint32_t height, unsigned char* rgba8)
{
    memset(rgba8, 0, width * height * 4);
    uint32_t blockW = width / 4;
    uint32_t blockH = height / 4;
    uint32_t stride = width * 4;
    for (uint32_t by = 0; by < blockH; by++)
    {
        for (uint32_t bx = 0; bx < blockW; bx++)
        {
            uint32_t x = bx * 4;
            uint32_t y = by * 4;
            DecoderBC::decodeBlockETC1(data, rgba8 + (y * width + x) * 4, stride);
            data += 8;
        }
    }
}

void decompressETC2(const unsigned char* data, uint32_t width, uint32_t height, unsigned char* rgba8)
{
    memset(rgba8, 0, width * height * 4);
    uint32_t blockW = width / 4;
    uint32_t blockH = height / 4;
    uint32_t stride = width * 4;
    for (uint32_t by = 0; by < blockH; by++)
    {
        for (uint32_t bx = 0; bx < blockW; bx++)
        {
            uint32_t x = bx * 4;
            uint32_t y = by * 4;
            DecoderBC::decodeBlockETC2(data, rgba8 + (y * width + x) * 4, stride);
            data += 16;
        }
    }
}

// ============================================================================================

struct TestResult
{
    std::string encoderName;
    std::string format;
    double numberOfPixels;
    double timeInMicroSeconds;
    MsePsnr msePsnr;
};

typedef int (__cdecl* CompressFunc_t)(unsigned char *dst, const unsigned char *src, unsigned int width, unsigned int height, unsigned int stride);

TestResult runTestDXT1(const char* encoderName, const char* imageName, CompressFunc_t func, Timer& timer, unsigned int numberOfIterations, unsigned char *dst, size_t dstSize, unsigned char* src, unsigned int w, unsigned int h, unsigned int stride, unsigned char* scratch)
{
    printf("DXT1 Encoder: %s (%s)                        \r", imageName, encoderName);

    memset(dst, 0, dstSize);
  
    double bestTimeUs = DBL_MAX;
    for (unsigned int iter = 0; iter < numberOfIterations; iter++)
    {
        timer.begin();
        func(dst, src, w, h, stride);
        double iterationTimeUs = timer.end();

        if (iterationTimeUs < bestTimeUs)
        {
            bestTimeUs = iterationTimeUs;
            //printf("%s %3.0f us\r", encoderName, bestTimeUs);
        }
    }
    //printf("\n");

    decompressDXT1(dst, w, h, scratch);
    MsePsnr msePsnr = getMsePsnr(src, scratch, w, h);

    char fileName[256];
    fileName[0] = '\0';
    strcpy(fileName, "./test-results/");
    strcat(fileName, imageName);
    strcat(fileName, "_");
    strcat(fileName, encoderName);
    strcat(fileName, "_dxt1_decompressed.tga");
    saveTga(fileName, scratch, w, h);

    fileName[0] = '\0';
    strcpy(fileName, "./test-results/");
    strcat(fileName, imageName);
    strcat(fileName, "_");
    strcat(fileName, encoderName);
    strcat(fileName, "_dxt1.dds");
    saveDds(fileName, dst, dstSize, w, h, kDdsFormatDXT1);

    TestResult res;
    res.encoderName = encoderName;
    res.format = "DXT1";
    res.msePsnr = msePsnr;
    res.numberOfPixels = (w * h);
    res.timeInMicroSeconds = bestTimeUs;
    return res;
}

TestResult runTestETC1(const char* encoderName, const char* imageName, CompressFunc_t func, Timer& timer, unsigned int numberOfIterations, unsigned char *dst, size_t dstSize, unsigned char* src,  unsigned int w, unsigned int h, unsigned int stride, unsigned char* scratch)
{
    printf("ETC1 Encoder: %s (%s)                        \r", imageName, encoderName);

    memset(dst, 0, dstSize);
    double bestTimeUs = DBL_MAX;
    for (unsigned int iter = 0; iter < numberOfIterations; iter++)
    {
        timer.begin();
        func(dst, src, w, h, stride);
        double iterationTimeUs = timer.end();

        if (iterationTimeUs < bestTimeUs)
        {
            bestTimeUs = iterationTimeUs;
            //printf("%s %3.0f us\r", encoderName, bestTimeUs);
        }
    }
    //printf("\n");

    decompressETC1(dst, w, h, scratch);
    MsePsnr msePsnr = getMsePsnr(src, scratch, w, h);

    char fileName[256];
    fileName[0] = '\0';
    strcpy(fileName, "./test-results/");
    strcat(fileName, imageName);
    strcat(fileName, "_");
    strcat(fileName, encoderName);
    strcat(fileName, "_etc1_decompressed.tga");
    saveTga(fileName, scratch, w, h);

    fileName[0] = '\0';
    strcpy(fileName, "./test-results/");
    strcat(fileName, imageName);
    strcat(fileName, "_");
    strcat(fileName, encoderName);
    strcat(fileName, "_etc1.ktx");
    saveKtx(fileName, dst, dstSize, w, h, kKtxFormatETC1);

    TestResult res;
    res.encoderName = encoderName;
    res.format = "ETC1";
    res.msePsnr = msePsnr;
    res.numberOfPixels = (w * h);
    res.timeInMicroSeconds = bestTimeUs;
    return res;
}

// ============================================================================================

int rygCompressDXT1(unsigned char *dst, const unsigned char *src, unsigned int w, unsigned int h, unsigned int stride)
{
    unsigned char block[64];
    for (unsigned int y = 0; y < h; y += 4)
    {
        for (unsigned int x = 0; x < w; x += 4)
        {
            const unsigned char * p = src + ((y * w + x) * 4);
            memcpy(&block[0], p, 16);
            memcpy(&block[16], p + stride, 16);
            memcpy(&block[32], p + stride * 2, 16);
            memcpy(&block[48], p + stride * 3, 16);
            stb_compress_dxt_block(dst, block, 0, STB_DXT_NORMAL);
            dst += 8;
        }
    }
    return 0;
}

int icbcCompressDXT1(unsigned char *dst, const unsigned char *src, unsigned int w, unsigned int h, unsigned int stride)
{
    icbc::init_dxt1();
    unsigned char block[64];
    for (unsigned int y = 0; y < h; y += 4)
    {
        for (unsigned int x = 0; x < w; x += 4)
        {
            const unsigned char * p = src + ((y * w + x) * 4);
            memcpy(&block[0], p, 16);
            memcpy(&block[16], p + stride, 16);
            memcpy(&block[32], p + stride * 2, 16);
            memcpy(&block[48], p + stride * 3, 16);
            icbc::compress_dxt1_fast(block, dst);
            dst += 8;
        }
    }
    return 0;
}

int rgbcxCompressDXT1(unsigned char *dst, const unsigned char *src, unsigned int w, unsigned int h, unsigned int stride)
{
    rgbcx::encode_bc1_init(false);
    unsigned char block[64];
    for (unsigned int y = 0; y < h; y += 4)
    {
        for (unsigned int x = 0; x < w; x += 4)
        {
            const unsigned char * p = src + ((y * w + x) * 4);
            memcpy(&block[0], p, 16);
            memcpy(&block[16], p + stride, 16);
            memcpy(&block[32], p + stride * 2, 16);
            memcpy(&block[48], p + stride * 3, 16);
            rgbcx::encode_bc1(rgbcx::LEVEL0_OPTIONS, dst, block, false, false);
            dst += 8;
        }
    }

    return 0;
}

int rgCompressETC1(unsigned char *dst, const unsigned char *src, unsigned int w, unsigned int h, unsigned int stride)
{
    rg_etc1::etc1_pack_params params;
    params.clear();
    rg_etc1::pack_etc1_block_init();

    // this is the fastest way to encode
    params.m_quality = rg_etc1::cLowQuality;
    params.m_dithering = false;

    unsigned char block[64];
    for (unsigned int y = 0; y < h; y += 4)
    {
        for (unsigned int x = 0; x < w; x += 4)
        {
            const unsigned char* p = src + ((y * w + x) * 4);
            memcpy(&block[0], p, 16);
            memcpy(&block[16], p + stride, 16);
            memcpy(&block[32], p + stride * 2, 16);
            memcpy(&block[48], p + stride * 3, 16);
            rg_etc1::pack_etc1_block(dst, (unsigned int*)block, params);
            dst += 8;
        }
    }
    return 0;
}

// ============================================================================================

bool runTest(FILE* resultsFile, const char* imageName, std::vector<TestResult>& results)
{
    results.clear();
    printf("Image: %s                        \r", imageName);

    std::string imageFilename = "./test-data/"; 
    imageFilename += imageName;
    imageFilename += ".png";

    std::string basisFilename = "./test-data/basis/etc1/";
    basisFilename += imageName;
    basisFilename += "_unpacked_rgb_ETC1_RGB_0000.png";

    std::string referenceFilename = "./test-results/"; 
    referenceFilename += imageName;
    referenceFilename += "_original.tga";

    std::string baselineFilename = "./test-results/"; 
    baselineFilename += imageName;
    baselineFilename += "_baseline.tga";

    std::string basisOutFilename = "./test-results/";
    basisOutFilename += imageName;
    basisOutFilename += "_basis.tga";


    unsigned int width = 0;
    unsigned int height = 0;
    unsigned char* testImage = loadPngAsRgba8(imageFilename.c_str(), &width, &height);
    if (testImage == nullptr)
    {
        return false;
    }

    unsigned int basisWidth = 0;
    unsigned int basisHeight = 0;
    unsigned char* basisImage = loadPngAsRgba8(basisFilename.c_str(), &basisWidth, &basisHeight);
    if (basisWidth != width || basisHeight != height)
    {
        destroyPng(basisImage);
        basisImage = nullptr;
    }


    //printf("%d x %d\n", width, height);

    unsigned int stride = width * 4;

    size_t sizeInBytes = width * height * 4;
    unsigned char* scratchBuffer = (unsigned char*)malloc(sizeInBytes);
    if (scratchBuffer == nullptr)
    {
        printf("Can't allocate memory for scratch buffer\n");
        return false;
    }

    size_t compressedBufferSizeInBytes = sizeInBytes / 8;
    unsigned char* compressedBuffer = (unsigned char*)malloc(compressedBufferSizeInBytes);
    if (compressedBuffer == nullptr)
    {
        printf("Can't allocate memory for compressed buffer\n");
        return false;
    }

    saveTga(referenceFilename.c_str(), testImage, width, height);


    generateDownsamlpedRgb565Test(testImage, width, height, scratchBuffer);

    TestResult baselineRes;
    baselineRes.encoderName = "Baseline (Downsample x2)";
    baselineRes.format = "RGB565";
    baselineRes.numberOfPixels = width * height;
    baselineRes.timeInMicroSeconds = 0;
    baselineRes.msePsnr = getMsePsnr(testImage, scratchBuffer, width, height);
    results.emplace_back(baselineRes);
    saveTga(baselineFilename.c_str(), scratchBuffer, width, height);


    TestResult res;
    if (basisImage)
    {
        res.encoderName = "Basis";
        res.format = "ETC1";
        res.numberOfPixels = width * height;
        res.timeInMicroSeconds = 0;
        res.msePsnr = getMsePsnr(testImage, basisImage, width, height);
        results.emplace_back(res);
        saveTga(basisOutFilename.c_str(), scratchBuffer, width, height);
    }

    Timer timer;

    res = runTestETC1("simd_goofy", imageName, goofy::compressETC1, timer, kNumberOfIterations, compressedBuffer, compressedBufferSizeInBytes, testImage, width, height, stride, scratchBuffer);
    results.emplace_back(res);

    res = runTestDXT1("simd_goofy", imageName, goofy::compressDXT1, timer, kNumberOfIterations, compressedBuffer, compressedBufferSizeInBytes, testImage, width, height, stride, scratchBuffer);
    results.emplace_back(res);

    //res = runTestDXT1("ref_goofy", imageName, goofyRef::compressDXT1, timer, kNumberOfIterations, compressedBuffer, compressedBufferSizeInBytes, testImage, width, height, stride, scratchBuffer);
    //results.emplace_back(res);

    //res = runTestETC1("ref_goofy", imageName, goofyRef::compressETC1, timer, kNumberOfIterations, compressedBuffer, compressedBufferSizeInBytes, testImage, width, height, stride, scratchBuffer);
    //results.emplace_back(res);

    res = runTestDXT1("ryg", imageName, rygCompressDXT1, timer, kNumberOfIterations, compressedBuffer, compressedBufferSizeInBytes, testImage, width, height, stride, scratchBuffer);
    results.emplace_back(res);

    res = runTestDXT1("rgbcx", imageName, rgbcxCompressDXT1, timer, kNumberOfIterations, compressedBuffer, compressedBufferSizeInBytes, testImage, width, height, stride, scratchBuffer);
    results.emplace_back(res);

    res = runTestDXT1("icbc", imageName, icbcCompressDXT1, timer, kNumberOfIterations, compressedBuffer, compressedBufferSizeInBytes, testImage, width, height, stride, scratchBuffer);
    results.emplace_back(res);

    res = runTestETC1("rg", imageName, rgCompressETC1, timer, kNumberOfIterations, compressedBuffer, compressedBufferSizeInBytes, testImage, width, height, stride, scratchBuffer);
    results.emplace_back(res);

    // print results
    for (const TestResult& r : results)
    {
        double mps = 0;
        if (r.timeInMicroSeconds > 0)
        {
            double sec = r.timeInMicroSeconds / 1000000.0;
            mps = (r.numberOfPixels / sec) / 1000000.0;
        }
        double deltaPsnrMin = r.msePsnr.psnrMin - baselineRes.msePsnr.psnrMin;
        double deltaPsnrRgb = r.msePsnr.psnrRGB - baselineRes.msePsnr.psnrRGB;
        double deltaPsnrY = r.msePsnr.psnrY - baselineRes.msePsnr.psnrY;
        printf("%s;%s;%s;%3.0f;%3.0f;%3.5f;%s;%3.5f;%3.5f;%3.5f\n", imageName, r.encoderName.c_str(), r.format.c_str(), r.numberOfPixels, r.timeInMicroSeconds, mps, formatResultsRGB(r.msePsnr).c_str(), deltaPsnrMin, deltaPsnrRgb, deltaPsnrY);
        fprintf(resultsFile, "%s;%s;%s;%3.0f;%3.0f;%3.5f;%s;%3.5f;%3.5f;%3.5f\n", imageName, r.encoderName.c_str(), r.format.c_str(), r.numberOfPixels, r.timeInMicroSeconds, mps, formatResultsRGB(r.msePsnr).c_str(), deltaPsnrMin, deltaPsnrRgb, deltaPsnrY);
    }

    free(compressedBuffer);
    free(scratchBuffer);
    destroyPng(testImage);
    destroyPng(basisImage);
    return true;
}


#define ARRAY_SIZE(v) (sizeof(v) / sizeof(v[0]))

const char* testImages[] = {
#ifdef ENABLE_LONG_TEST_RUN
    "patterns",
    "pbr_bricks_albedo",
    "pbr_ground_albedo",
    "pbr_stones_albedo",
    "pbr_stones_normal",
    "pbr_head_albedo",
#endif
    "parrot_red",
#ifdef ENABLE_LONG_TEST_RUN
    "baboon",
    "lena",
    "monarch",
    "peppers",
    "sail",
    "tulips",
    "kodim01",
    "kodim02",
    "kodim03",
    "kodim04",
    "kodim05",
    "kodim06",
    "kodim07",
    "kodim08",
    "kodim09",
    "kodim10",
    "kodim11",
    "kodim12",
    "kodim13",
    "kodim14",
    "kodim15",
    "kodim16",
    "kodim17",
    "kodim18",
    "kodim19",
    "kodim20",
    "kodim21",
    "kodim22",
    "kodim23",
    "kodim24",
    "roblox01",
    "roblox02",
    "roblox03",
    "roblox04",
    "roblox05",
    "roblox06",
#endif
};

int main()
{
    FILE* resultsFile = fopen("./test-results/results.txt", "w");
    if (!resultsFile)
    {
        printf("Can't create file './test-results/results.txt'\n");
        printf("Run GoofyTC in the project dir?\n");
        return -2;
    }

    FILE* summaryFile = fopen("./test-results/sumary.txt", "w");
    if (!summaryFile)
    {
        printf("Can't create file './test-results/sumary.txt'\n");
        return -3;
    }


    std::vector<TestResult> results;
    results.reserve(512);

    struct CodecAvgPsnr
    {
        double psnrMinSum = 0.0;
        double psnrRGBSum = 0.0;
        double psnrYSum = 0.0;
        double numberOfImages = 0.0;
    };
    std::string tmp;
    std::unordered_map<std::string, CodecAvgPsnr> codecsAvg;

    printf("Image;Encoder;Format;NumberOfPixels;time (us);MP/s;mseR;mseG;mseB;mseMax;mseRGB;mseY;psnrR (db);psnrG (db);psnrB (db);psnrMin (db);psnrRGB (db);psnrY (db);deltaMin (db);deltaRGB (db);deltaY (db)\n");
    fprintf(resultsFile, "Image;Encoder;Format;NumberOfPixels;time (us);MP/s;mseR;mseG;mseB;mseMax;mseRGB;mseY;psnrR (db);psnrG (db);psnrB (db);psnrMin (db);psnrRGB (db);psnrY (db);deltaMin (db);deltaRGB (db);deltaY (db)\n");
    for(unsigned int i = 0; i < ARRAY_SIZE(testImages); i++)
    {
        bool res = runTest(resultsFile, testImages[i], results);
        if (!res)
        {
            printf("Tetst failed\n");
            return -1;
        }

        for (const TestResult& r : results)
        {
            tmp = r.encoderName + ";" + r.format;
            CodecAvgPsnr& codecAvg = codecsAvg[tmp];
            codecAvg.psnrMinSum += r.msePsnr.psnrMin;
            codecAvg.psnrRGBSum += r.msePsnr.psnrRGB;
            codecAvg.psnrYSum += r.msePsnr.psnrY;
            codecAvg.numberOfImages += 1.0;
        }
    }

    printf("---[ Summary ]-------------\n");
    printf("Codec;Format;Avg psnrMin (db);Avg psnrRGB (db); Avg psnrY (db);Number of tests\n");
    fprintf(summaryFile, "Codec;Format;Avg psnrMin (db);Avg psnrRGB (db); Avg psnrY (db);Number of tests\n");
    for (const auto& avg : codecsAvg)
    {
        double psnrMinAvg = avg.second.psnrMinSum / avg.second.numberOfImages;
        tmp = psnrToString(psnrMinAvg) + ";";
        double psnrRGBAvg = avg.second.psnrRGBSum / avg.second.numberOfImages;
        tmp += psnrToString(psnrRGBAvg) + ";";
        double psnrYAvg = avg.second.psnrYSum / avg.second.numberOfImages;
        tmp += psnrToString(psnrYAvg);
        printf("%s;%s;%.0f\n", avg.first.c_str(), tmp.c_str(), avg.second.numberOfImages);
        fprintf(summaryFile, "%s;%s;%.0f\n", avg.first.c_str(), tmp.c_str(), avg.second.numberOfImages);
    }
    printf("---------------------------\n");

    fclose(summaryFile);
    fclose(resultsFile);
    printf("Finished. Check './test-results/results.txt' for details\n");
    return 0;
}
