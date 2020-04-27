#include "goofy_tc_reference.h"

#include <assert.h>
#include <string.h> // memcpy
#include <algorithm> // min/max

namespace goofyRef
{

//
// https://docs.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-block-compression#bc1
//
struct Dxt1
{
    // bitmap address if using iN
    // +---+---+---+---+
    // | A | B | C | D |
    // +---+---+---+---+
    // | E | F | G | H |
    // +---+---+---+---+
    // | I | J | K | L |
    // +---+---+---+---+
    // | M | N | O | P |
    // +---+---+---+---+
    union
    {
        struct
        {
            uint16_t c0_max;    // RGB565
            uint16_t c1_min;    // RGB565
            uint32_t indices;   // 4x4 block of 2 bit indices 
        };

        struct
        {
            uint64_t c0_b : 5;
            uint64_t c0_g : 6;
            uint64_t c0_r : 5;

            uint64_t c1_b : 5;
            uint64_t c1_g : 6;
            uint64_t c1_r : 5;

            //    C0(max)     C2        C3      C1(min)  
            //  |    0    |    2    |    3    |    1    |
            //  |   00b   |   10b   |   11b   |   01b   |

            uint64_t iA : 2; // LSB
            uint64_t iB : 2;
            uint64_t iC : 2;
            uint64_t iD : 2;
            uint64_t iE : 2;
            uint64_t iF : 2;
            uint64_t iG : 2;
            uint64_t iH : 2;
            uint64_t iI : 2;
            uint64_t iJ : 2;
            uint64_t iK : 2;
            uint64_t iL : 2;
            uint64_t iM : 2;
            uint64_t iN : 2;
            uint64_t iO : 2;
            uint64_t iP : 2; // MSB
        };
    };
};

//
// https://www.khronos.org/registry/DataFormat/specs/1.3/dataformat.1.3.html#ETC1
// Note: this is Etc1S "simplified" block
struct Etc1S
{
    // 5 bit-color + lowest 3 bits zeroes (color delta)
    // 5 bit-color + lowest 3 bits zeroes (color delta)
    // 5 bit-color + lowest 3 bits zeroes (color delta)
    // table codeword 1 (3-bit), table codeword 2 (3-bit) (both the same), diff (1-bit always on), flip (1-bit doesn't matter)
    // 4x4 block of 2 bit indices (bright intensity modifiers)

    // bitmap address if using iN/nN
    // +---+---+---+---+
    // | A | B | C | D |
    // +---+---+---+---+
    // | E | F | G | H |
    // +---+---+---+---+
    // | I | J | K | L |
    // +---+---+---+---+
    // | M | N | O | P |
    // +---+---+---+---+
    //
    // bitmap address using raw bits
    // addressing is vertical (bottom to top) and even/odd rows swapped due to endianness
    // +---+---+---+---+
    // | C | G | K | O |
    // +---+---+---+---+
    // | D | H | L | P |
    // +---+---+---+---+
    // | A | E | I | M |
    // +---+---+---+---+
    // | B | F | J | N |
    // +---+---+---+---+
    union
    {
        struct
        {
            uint8_t data[8];
        };

        struct
        {
            uint32_t rgbAndControlByte;
            uint32_t indices;
        };

        struct
        {
            uint64_t dr : 3;
            uint64_t r : 5;

            uint64_t dg : 3;
            uint64_t g : 5;

            uint64_t db : 3;
            uint64_t b : 5;

            uint64_t flip : 1;
            uint64_t diff : 1;
            uint64_t codeword2 : 3;
            uint64_t codeword1 : 3;

            // sign bit (1 = negative, 0 = positive)
            uint64_t nC : 1;  // LSB
            uint64_t nG : 1;
            uint64_t nK : 1;
            uint64_t nO : 1;
            uint64_t nD : 1;
            uint64_t nH : 1;
            uint64_t nL : 1;
            uint64_t nP : 1;
            uint64_t nA : 1;
            uint64_t nE : 1;
            uint64_t nI : 1;
            uint64_t nM : 1;
            uint64_t nB : 1;
            uint64_t nF : 1;
            uint64_t nJ : 1;
            uint64_t nN : 1;  // MSB

                              // index bit (0 = small delta, 1 = huge delta)
            uint64_t iC : 1;  // LSB
            uint64_t iG : 1;
            uint64_t iK : 1;
            uint64_t iO : 1;
            uint64_t iD : 1;
            uint64_t iH : 1;
            uint64_t iL : 1;
            uint64_t iP : 1;
            uint64_t iA : 1;
            uint64_t iE : 1;
            uint64_t iI : 1;
            uint64_t iM : 1;
            uint64_t iB : 1;
            uint64_t iF : 1;
            uint64_t iJ : 1;
            uint64_t iN : 1;  // MSB
        };

    };
};

struct BlockData
{
    // dxt1
    uint8_t min_r;
    uint8_t min_g;
    uint8_t min_b;

    uint8_t max_r;
    uint8_t max_g;
    uint8_t max_b;

    // etc1
    uint8_t base_r;
    uint8_t base_g;
    uint8_t base_b;

    uint8_t avg_r;
    uint8_t avg_g;
    uint8_t avg_b;

    uint8_t brightRange;

    // shared
    int index[16];
};

//
struct FColor3
{
    union
    {
        struct
        {
            float r;
            float g;
            float b;
        };

        struct
        {
            float y;
            float co;
            float cg;
        };
    };

    static FColor3 create(float v)
    {
        FColor3 res;
        res.r = v;
        res.g = v;
        res.b = v;
        return res;
    }

    static FColor3 create(float r_or_y, float g_or_co, float b_or_cg)
    {
        FColor3 res;
        res.r = r_or_y; // y
        res.g = g_or_co; // co
        res.b = b_or_cg; // cg
        return res;
    }

    static FColor3 min(const FColor3& a, const FColor3& b)
    {
        FColor3 res;
        res.r = std::min(a.r, b.r);
        res.g = std::min(a.g, b.g);
        res.b = std::min(a.b, b.b);
        return res;
    }

    static float brightnessYCoCg(const FColor3& a)
    {
        float Y = a.r * 0.25f + a.g * 0.5f + a.b * 0.25f;
        return Y;
    }

    static float brightnessSimple(const FColor3& a)
    {
        float Y = a.r * 0.25f + a.g * 0.25f + a.b * 0.25f;
        return Y;
    }

    static float brightness(const FColor3& a)
    {
        return brightnessYCoCg(a);

        // this give us a slightly better result for DXT1 compressor (+0.2 db)
        // but it's way worse for ETC (-0.58 db)
        //return brightnessSimple(a);  
    }

    static FColor3 avg(const FColor3& a, const FColor3& b)
    {
        FColor3 res;
        res.r = (a.r + b.r) / 2.0f;
        res.g = (a.g + b.g) / 2.0f;
        res.b = (a.b + b.b) / 2.0f;
        return res;
    }

    static FColor3 max(const FColor3& a, const FColor3& b)
    {
        FColor3 res;
        res.r = std::max(a.r, b.r);
        res.g = std::max(a.g, b.g);
        res.b = std::max(a.b, b.b);
        return res;
    }

    static FColor3 add(const FColor3& a, const FColor3& b)
    {
        FColor3 res;
        res.r = a.r + b.r;
        res.g = a.g + b.g;
        res.b = a.b + b.b;
        return res;
    }

    static FColor3 sub(const FColor3& a, const FColor3& b)
    {
        FColor3 res;
        res.r = a.r - b.r;
        res.g = a.g - b.g;
        res.b = a.b - b.b;
        return res;
    }

    static FColor3 clamp(const FColor3& v)
    {
        FColor3 res;
        res.r = std::max(0.0f, std::min(255.0f, v.r));
        res.g = std::max(0.0f, std::min(255.0f, v.g));
        res.b = std::max(0.0f, std::min(255.0f, v.b));
        return res;
    }

    static FColor3 div(const FColor3& a, float v)
    {
        FColor3 res;
        res.r = a.r / v;
        res.g = a.g / v;
        res.b = a.b / v;
        return res;
    }

    static FColor3 toYCoCg(const FColor3& v)
    {
        float co = v.r - v.b;
        float tmp = v.b + co / 2.0f;
        float cg = v.g - tmp;
        float y = tmp + cg / 2.0f;

        FColor3 res;
        res.co = co;
        res.cg = cg;
        res.y = y;
        return res;
    }

    static FColor3 toRgb(const FColor3& v)
    {
        float tmp = v.y - v.cg / 2;
        float g = v.cg + tmp;
        float b = tmp - v.co / 2;
        float r = b + v.co;

        FColor3 res;
        res.r = r;
        res.g = g;
        res.b = b;
        return res;
    }
};

static uint8_t floatToByte(float v)
{
    v = (v + 0.5f);

    if (v < 0.0f)
        v = 0.0f;

    if (v > 255.0f)
        v = 255.0f;

    return (uint8_t)v;
}


// generate Maya MEL script to visualize block pixels and bounds
static void goofyDebugDumpBlock(const uint8_t(&blockRgba)[64], const FColor3& minColor, const FColor3& maxColor, const FColor3& avgColor, const FColor3& baseColor)
{
    for (int y = 0; y < 4; y++)
    {
        for (int x = 0; x < 4; x++)
        {
            FColor3 pixel;
            pixel.r = (float)blockRgba[(y * 4 + x) * 4 + 0];
            pixel.g = (float)blockRgba[(y * 4 + x) * 4 + 1];
            pixel.b = (float)blockRgba[(y * 4 + x) * 4 + 2];

            printf("%d_%d %3.2f, %3.2f, %3.2f (%3.6f)\n", x, y, pixel.r, pixel.g, pixel.b, FColor3::brightness(pixel));
        }
    }

    printf("MIN: %3.2f, %3.2f, %3.2f (%3.6f)\n", minColor.r, minColor.g, minColor.b, FColor3::brightness(minColor));
    printf("MAX: %3.2f, %3.2f, %3.2f (%3.6f)\n", maxColor.r, maxColor.g, maxColor.b, FColor3::brightness(maxColor));

    printf("// ------------------------ cut here ------------------------------------\n");
    printf("curve -d 1 -p %3.4f %3.4f %3.4f -p %3.4f %3.4f %3.4f;\n", minColor.r, minColor.g, minColor.b, maxColor.r, maxColor.g, maxColor.b);

    FColor3 etcOffset = FColor3::create(200, 200, 200);
    FColor3 etcMin = FColor3::sub(baseColor, etcOffset);
    FColor3 etcMax = FColor3::add(baseColor, etcOffset);
    printf("curve -d 1 -p %3.4f %3.4f %3.4f -p %3.4f %3.4f %3.4f;\n", etcMin.r, etcMin.g, etcMin.b, etcMax.r, etcMax.g, etcMax.b);

    printf("string $cube[];\n");
    printf("$cube = `polyCube -w 2.5 -h 2.5 -d 2.5 -sx 1 -sy 1 -sz 1`;\n");
    printf("rename ($cube[0], \"min_c\");\n");
    printf("setAttr \"min_c.translateX\" %3.4f;\n", minColor.r);
    printf("setAttr \"min_c.translateY\" %3.4f;\n", minColor.g);
    printf("setAttr \"min_c.translateZ\" %3.4f;\n", minColor.b);
    printf("polyColorPerVertex -r %3.5f -g %3.5f -b %3.5f -a 1 -cdo;\n", (minColor.r / 255.0f), (minColor.g / 255.0f), (minColor.b / 255.0f));

    printf("$cube = `polyCube -w 2.5 -h 2.5 -d 2.5 -sx 1 -sy 1 -sz 1`;\n");
    printf("rename ($cube[0], \"max_c\");\n");
    printf("setAttr \"max_c.translateX\" %3.4f;\n", maxColor.r);
    printf("setAttr \"max_c.translateY\" %3.4f;\n", maxColor.g);
    printf("setAttr \"max_c.translateZ\" %3.4f;\n", maxColor.b);
    printf("polyColorPerVertex -r %3.5f -g %3.5f -b %3.5f -a 1 -cdo;\n", (maxColor.r / 255.0f), (maxColor.g / 255.0f), (maxColor.b / 255.0f));

    printf("$cube = `polyCube -w 2.5 -h 2.5 -d 2.5 -sx 1 -sy 1 -sz 1`;\n");
    printf("rename ($cube[0], \"avg_c\");\n");
    printf("setAttr \"avg_c.translateX\" %3.4f;\n", avgColor.r);
    printf("setAttr \"avg_c.translateY\" %3.4f;\n", avgColor.g);
    printf("setAttr \"avg_c.translateZ\" %3.4f;\n", avgColor.b);
    printf("polyColorPerVertex -r %3.5f -g %3.5f -b %3.5f -a 1 -cdo;\n", (avgColor.r / 255.0f), (avgColor.g / 255.0f), (avgColor.b / 255.0f));

    printf("$cube = `polyCube -w 2.5 -h 2.5 -d 2.5 -sx 1 -sy 1 -sz 1`;\n");
    printf("rename ($cube[0], \"base_c\");\n");
    printf("setAttr \"base_c.translateX\" %3.4f;\n", baseColor.r);
    printf("setAttr \"base_c.translateY\" %3.4f;\n", baseColor.g);
    printf("setAttr \"base_c.translateZ\" %3.4f;\n", baseColor.b);
    printf("polyColorPerVertex -r %3.5f -g %3.5f -b %3.5f -a 1 -cdo;\n", (baseColor.r / 255.0f), (baseColor.g / 255.0f), (baseColor.b / 255.0f));

    {
        printf("$cube = `polyCube -w 1.5 -h 1.5 -d 1.5 -sx 1 -sy 1 -sz 1`;\n");
        printf("rename ($cube[0], \"bb0_c\");\n");
        printf("setAttr \"bb0_c.translateX\" %3.4f;\n", minColor.r);
        printf("setAttr \"bb0_c.translateY\" %3.4f;\n", maxColor.g);
        printf("setAttr \"bb0_c.translateZ\" %3.4f;\n", maxColor.b);
        printf("polyColorPerVertex -r %3.5f -g %3.5f -b %3.5f -a 1 -cdo;\n", (minColor.r / 255.0f), (maxColor.g / 255.0f), (maxColor.b / 255.0f));

        printf("$cube = `polyCube -w 1.5 -h 1.5 -d 1.5 -sx 1 -sy 1 -sz 1`;\n");
        printf("rename ($cube[0], \"bb1_c\");\n");
        printf("setAttr \"bb1_c.translateX\" %3.4f;\n", maxColor.r);
        printf("setAttr \"bb1_c.translateY\" %3.4f;\n", maxColor.g);
        printf("setAttr \"bb1_c.translateZ\" %3.4f;\n", minColor.b);
        printf("polyColorPerVertex -r %3.5f -g %3.5f -b %3.5f -a 1 -cdo;\n", (maxColor.r / 255.0f), (maxColor.g / 255.0f), (minColor.b / 255.0f));

        printf("$cube = `polyCube -w 1.5 -h 1.5 -d 1.5 -sx 1 -sy 1 -sz 1`;\n");
        printf("rename ($cube[0], \"bb2_c\");\n");
        printf("setAttr \"bb2_c.translateX\" %3.4f;\n", minColor.r);
        printf("setAttr \"bb2_c.translateY\" %3.4f;\n", maxColor.g);
        printf("setAttr \"bb2_c.translateZ\" %3.4f;\n", minColor.b);
        printf("polyColorPerVertex -r %3.5f -g %3.5f -b %3.5f -a 1 -cdo;\n", (minColor.r / 255.0f), (maxColor.g / 255.0f), (minColor.b / 255.0f));

        printf("$cube = `polyCube -w 1.5 -h 1.5 -d 1.5 -sx 1 -sy 1 -sz 1`;\n");
        printf("rename ($cube[0], \"bb3_c\");\n");
        printf("setAttr \"bb3_c.translateX\" %3.4f;\n", minColor.r);
        printf("setAttr \"bb3_c.translateY\" %3.4f;\n", minColor.g);
        printf("setAttr \"bb3_c.translateZ\" %3.4f;\n", maxColor.b);
        printf("polyColorPerVertex -r %3.5f -g %3.5f -b %3.5f -a 1 -cdo;\n", (minColor.r / 255.0f), (minColor.g / 255.0f), (maxColor.b / 255.0f));

        printf("$cube = `polyCube -w 1.5 -h 1.5 -d 1.5 -sx 1 -sy 1 -sz 1`;\n");
        printf("rename ($cube[0], \"bb4_c\");\n");
        printf("setAttr \"bb4_c.translateX\" %3.4f;\n", maxColor.r);
        printf("setAttr \"bb4_c.translateY\" %3.4f;\n", minColor.g);
        printf("setAttr \"bb4_c.translateZ\" %3.4f;\n", minColor.b);
        printf("polyColorPerVertex -r %3.5f -g %3.5f -b %3.5f -a 1 -cdo;\n", (maxColor.r / 255.0f), (minColor.g / 255.0f), (minColor.b / 255.0f));

        printf("$cube = `polyCube -w 1.5 -h 1.5 -d 1.5 -sx 1 -sy 1 -sz 1`;\n");
        printf("rename ($cube[0], \"bb5_c\");\n");
        printf("setAttr \"bb5_c.translateX\" %3.4f;\n", maxColor.r);
        printf("setAttr \"bb5_c.translateY\" %3.4f;\n", minColor.g);
        printf("setAttr \"bb5_c.translateZ\" %3.4f;\n", maxColor.b);
        printf("polyColorPerVertex -r %3.5f -g %3.5f -b %3.5f -a 1 -cdo;\n", (maxColor.r / 255.0f), (minColor.g / 255.0f), (maxColor.b / 255.0f));
    }

    printf("$cube = `polyCube -w 255 -h 255 -d 255 -sx 1 -sy 1 -sz 1`;\n");
    printf("rename ($cube[0], \"rgb_bbox\");\n");

    printf("setAttr \"rgb_bbox.translateX\" %3.4f;\n", 127.5);
    printf("setAttr \"rgb_bbox.translateY\" %3.4f;\n", 127.5);
    printf("setAttr \"rgb_bbox.translateZ\" %3.4f;\n", 127.5);

    printf("setAttr \"rgb_bbox.overrideEnabled\" 1;\n");
    printf("setAttr \"rgb_bbox.overrideShading\" 0;\n");

    float sX = fabs(maxColor.r - minColor.r);
    float sY = fabs(maxColor.g - minColor.g);
    float sZ = fabs(maxColor.b - minColor.b);

    float cX = (maxColor.r + minColor.r) * 0.5f;
    float cY = (maxColor.g + minColor.g) * 0.5f;
    float cZ = (maxColor.b + minColor.b) * 0.5f;

    printf("$cube = `polyCube -w %3.5f -h %3.5f -d %3.5f -sx 1 -sy 1 -sz 1`;\n", sX, sY, sZ);
    printf("rename ($cube[0], \"bbox\");\n");
    printf("setAttr \"bbox.translateX\" %3.4f;\n", cX);
    printf("setAttr \"bbox.translateY\" %3.4f;\n", cY);
    printf("setAttr \"bbox.translateZ\" %3.4f;\n", cZ);

    printf("setAttr \"bbox.overrideEnabled\" 1;\n");
    printf("setAttr \"bbox.overrideShading\" 0;\n");

    for (int y = 0; y < 4; y++)
    {
        for (int x = 0; x < 4; x++)
        {
            FColor3 pixel;
            pixel.r = (float)blockRgba[(y * 4 + x) * 4 + 0];
            pixel.g = (float)blockRgba[(y * 4 + x) * 4 + 1];
            pixel.b = (float)blockRgba[(y * 4 + x) * 4 + 2];

            float br = FColor3::brightness(pixel);
            int brightness = int(br * 100.0f);

            printf("$cube = `polyCube -w 1 -h 1 -d 1 -sx 1 -sy 1 -sz 1`;\n");
            printf("rename ($cube[0], \"p%d_%d_%d\");\n", x, y, brightness);
            printf("setAttr \"p%d_%d_%d.translateX\" %3.4f;\n", x, y, brightness, pixel.r);
            printf("setAttr \"p%d_%d_%d.translateY\" %3.4f;\n", x, y, brightness, pixel.g);
            printf("setAttr \"p%d_%d_%d.translateZ\" %3.4f;\n", x, y, brightness, pixel.b);
            printf("polyColorPerVertex -r %3.5f -g %3.5f -b %3.5f -a 1 -cdo;\n", (pixel.r / 255.0f), (pixel.g / 255.0f), (pixel.b / 255.0f));
        }
    }
    printf("// ------------------------ cut here ------------------------------------\n");
}

static BlockData goofyCompressBlock(const uint8_t(&blockRgba)[64], const float minBrightnessRange)
{
    FColor3 minColor = FColor3::create(99999.0f);
    FColor3 maxColor = FColor3::create(-99999.0f);
    FColor3 avgColor = FColor3::create(0.0f);

    // Find max/min color
    for (int y = 0; y < 4; y++)
    {
        for (int x = 0; x < 4; x++)
        {
            FColor3 pixel;
            pixel.r = (float)blockRgba[(y * 4 + x) * 4 + 0];
            pixel.g = (float)blockRgba[(y * 4 + x) * 4 + 1];
            pixel.b = (float)blockRgba[(y * 4 + x) * 4 + 2];

            minColor = FColor3::min(minColor, pixel);
            maxColor = FColor3::max(maxColor, pixel);
            avgColor = FColor3::add(avgColor, pixel);
        }
    }

    avgColor = FColor3::div(avgColor, 16.0f);

    // Convert to min/max brightness
    float maxY = FColor3::brightness(maxColor);
    float minY = FColor3::brightness(minColor);

    // Get brightness range
    float brightnessRange = maxY - minY;

    // Clamp brighness range to some value
    //     too small brightness difference cannot be encoded anyway
    //
    // minBrightnessRange default value is 8 for DXT and 16 for ETC
    brightnessRange = std::max(brightnessRange, minBrightnessRange);

    float midPointY = (maxY + minY) * 0.5f;
    
    // This works beter than 0.25 for both dxt1 and etc1
    float quantizationThreshold = brightnessRange * 0.375f; 

    // Quantization (generate indices)
    BlockData res;
    for (int y = 0; y < 4; y++)
    {
        for (int x = 0; x < 4; x++)
        {
            FColor3 pixel;
            pixel.r = (float)blockRgba[(y * 4 + x) * 4 + 0];
            pixel.g = (float)blockRgba[(y * 4 + x) * 4 + 1];
            pixel.b = (float)blockRgba[(y * 4 + x) * 4 + 2];

            float pixelY = FColor3::brightness(pixel);
            float diff = pixelY - midPointY;

            float q = quantizationThreshold;

/*          // noise makes the codec quality worse
            float rand01 = ((float)rand() / RAND_MAX);
            float noise = (rand01 * 2.0f - 0.5f) * 0.5f;
            q += noise;
            q = std::max(0.0f, std::min(255.0f, q));
*/

            int idx = -1;
            if (diff > 0.0f)
            {
                idx = (diff < q) ? 2 : 0;
            }
            else
            {
                idx = (fabsf(diff) < q) ? 3 : 1;
            }
            assert(idx >= 0 && idx <= 3);
            res.index[y * 4 + x] = idx;
        }
    }

    // DXT
    res.max_r = floatToByte(maxColor.r);
    res.max_g = floatToByte(maxColor.g);
    res.max_b = floatToByte(maxColor.b);

    res.min_r = floatToByte(minColor.r);
    res.min_g = floatToByte(minColor.g);
    res.min_b = floatToByte(minColor.b);

    // ETC
    res.brightRange = floatToByte(brightnessRange * 0.5f);

    // Keep chromatic component from the average color, but override brightness
    float avgY = FColor3::brightness(avgColor);
    float diffY = midPointY - avgY;
    FColor3 baseColor = FColor3::add(avgColor, FColor3::create(diffY));

    //goofyDebugDumpBlock(blockRgba, minColor, maxColor, avgColor, baseColor);

    // Old-mode (worse quality, but faster)
    //FColor3 mid = FColor3::avg(maxColor, minColor);
    res.base_r = floatToByte(baseColor.r);
    res.base_g = floatToByte(baseColor.g);
    res.base_b = floatToByte(baseColor.b);

    res.avg_r = floatToByte(avgColor.r);
    res.avg_g = floatToByte(avgColor.g);
    res.avg_b = floatToByte(avgColor.b);

    return res;
}

static uint32_t packRgb888ToEtc555(uint8_t __r, uint8_t __g, uint8_t __b)
{
    uint32_t _r = __r;
    uint32_t _g = __g;
    uint32_t _b = __b;
    uint32_t clr = (_r & 0xf8) | ((_g & 0xf8) << 8) | ((_b & 0xf8) << 16);
    return clr;
}

static uint16_t packRgb888ToDxt565(uint8_t __r, uint8_t __g, uint8_t __b)
{
    // in fact convert RGB888 to RGB555 (instead of RGB565) where low G bit is always 0
    uint32_t _r = __r;
    uint32_t _g = __g;
    uint32_t _b = __b;

    uint32_t r = (_r >> 3) << 11;
    uint32_t g = (_g >> 3) << 6;
    uint32_t b = (_b >> 3);
    uint16_t clr = r | g | b;
    return clr;
}

static uint32_t getDxtIndices(const BlockData& d)
{
    uint32_t v = 0;
    for (int n = 0; n < 16; n++)
    {
        uint32_t ii = (d.index[n] & 3);
        v |= (ii << (n*2));
    }
    return v;
}

static void goofyPackBlockDXT1(const unsigned char* inputRGBA, size_t stride, unsigned char* pResult)
{
    unsigned char block[64];
    memcpy(&block[0],  inputRGBA, 16);
    memcpy(&block[16], inputRGBA + stride, 16);
    memcpy(&block[32], inputRGBA + stride * 2, 16);
    memcpy(&block[48], inputRGBA + stride * 3, 16);

    BlockData bl = goofyCompressBlock(block, 8.0f);

    Dxt1* pBlock = (Dxt1*)pResult;
    pBlock->c0_max = packRgb888ToDxt565(bl.max_r, bl.max_g, bl.max_b) | 0x20;
    pBlock->c1_min = packRgb888ToDxt565(bl.min_r, bl.min_g, bl.min_b);
    pBlock->indices = getDxtIndices(bl);
}

//              [0]    [1]    [2]     [3]
// 0x03 [0]     -8     -2      2       8
// 0x27 [1]    -17     -5      5      17
// 0x4B [2]    -29     -9      9      29
// 0x6F [3]    -42    -13     13      42
// 0x93 [4]    -60    -18     18      60
// 0xB7 [5]    -80    -24     24      80
// 0xDB [6]   -106    -33     33     106
// 0xFF [7]   -183    -47     47     183
static uint32_t getEtc1SBlockControlByte(uint8_t brightRange)
{
    // this is works better than standard table 
    //int brightTable[8] = { 8, 17, 29, 42, 60, 80, 106 };
    int brightTable[8] = { 10, 21, 36, 52, 75, 90, 126 };

    if (brightRange <= brightTable[0])
    {
        return 0x03000000;
    }
    else if (brightRange <= brightTable[1])
    {
        return 0x27000000;
    }
    else if (brightRange <= brightTable[2])
    {
        return 0x4B000000;
    }
    else if (brightRange <= brightTable[3])
    {
        return 0x6F000000;
    }
    else if (brightRange <= brightTable[4])
    {
        return 0x93000000;
    }
    else if (brightRange <= brightTable[5])
    {
        return 0xB7000000;
    }
    else if (brightRange <= brightTable[6])
    {
        return 0xDB000000;
    }

    return 0xFF000000;
}

static uint32_t getEtcIndices(const BlockData& d)
{
    // DXT                                ETC (MSB/LSB)
    //  0 = brightest DXT color             01 +b (large positive value)
    //  1 = darkest DXT color               11 -b (large negative value)
    //  2 = 2/3 bright + 1/3 dark           00 +a (small positibe value)
    //  3 = 1/3 bright + 2/3 dark           10 -a (small negative value)

    uint32_t lsbRemapTable[4] = { 1, 1, 0, 0 };
    uint32_t msbRemapTable[4] = { 0, 1, 0, 1 };

    // ETC indices addressing is different from DXT
    uint32_t remapIndex[16] = {
        0x8, 0xC, 0x0, 0x4,
        0x9, 0xD, 0x1, 0x5,
        0xA, 0xE, 0x2, 0x6,
        0xB, 0xF, 0x3, 0x7
    };

    uint32_t v = 0;
    for (int n = 0; n < 16; n++)
    {
        // remap index from DXT left-to-right, to ETC top-to-bottom
        uint32_t bitNumber = remapIndex[n];

        // remap DXT index to ETC index
        uint32_t lsb = (lsbRemapTable[d.index[n]]) << bitNumber;
        uint32_t msb = (msbRemapTable[d.index[n]]) << bitNumber;

        v |= (lsb << 16);
        v |= msb;
    }

    return v;
}

static void goofyPackBlockETC1S(const unsigned char* inputRGBA, size_t stride, unsigned char* pResult)
{
    unsigned char block[64];
    memcpy(&block[0], inputRGBA, 16);
    memcpy(&block[16], inputRGBA + stride, 16);
    memcpy(&block[32], inputRGBA + stride * 2, 16);
    memcpy(&block[48], inputRGBA + stride * 3, 16);

    BlockData bl = goofyCompressBlock(block, 16.0f);

    Etc1S* pBlock = (Etc1S*)pResult;


/*
    // constant color block optimization (worse)
    if ((bl.max_r - bl.min_r) < 4 && (bl.max_g - bl.min_g) < 4 && (bl.max_b - bl.min_b) < 4)
    {
        uint32_t ctrlByte = 0x03000000;
        pBlock->rgbAndControlByte = packRgb888ToEtc555(bl.avg_r, bl.avg_g, bl.avg_b) | ctrlByte;
        // since extend_5to8bits() always replicate 3 higher bits into lower bits it introduces additional quantization error +0..+7
        // adding -2 RGB per-pixel is an attempt to compensate for this error for a constant color block

        // smallest possible negative (-2)
        pBlock->indices = 0x0000FFFF; 
    }
    else
*/
    {
        uint32_t ctrlByte = getEtc1SBlockControlByte(bl.brightRange);
        pBlock->rgbAndControlByte = packRgb888ToEtc555(bl.base_r, bl.base_g, bl.base_b) | ctrlByte;
        pBlock->indices = getEtcIndices(bl);
    }

}


int compressDXT1(unsigned char* result, const unsigned char* input, unsigned int width, unsigned int height, unsigned int stride)
{
    if (width % 4 != 0)
    {
        return -1;
    }

    if (height % 4 != 0)
    {
        return -2;
    }

    uint32_t blockW = width >> 2;
    uint32_t blockH = height >> 2;

    for (uint32_t y = 0; y < blockH; y++)
    {
        const unsigned char* input_line = input;
        for (uint32_t x = 0; x < blockW; x++)
        {
            goofyPackBlockDXT1(input_line, stride, result);
            input_line += 16;
            result += sizeof(Dxt1);
        }
        input += width * 4 * 4; // width * rgba(4) * 4 lines
    }
    return 0;
}

int compressETC1(unsigned char* result, const unsigned char* input, unsigned int width, unsigned int height, unsigned int stride)
{
    if (width % 4 != 0)
    {
        return -1;
    }

    if (height % 4 != 0)
    {
        return -2;
    }

    uint32_t blockW = width >> 2;
    uint32_t blockH = height >> 2;

    for (uint32_t y = 0; y < blockH; y++)
    {
        const unsigned char* input_line = input;
        for (uint32_t x = 0; x < blockW; x++)
        {
            goofyPackBlockETC1S(input_line, stride, result);
            input_line += 16;
            result += sizeof(Etc1S);
        }
        input += width * 4 * 4; // width * rgba(4) * 4 lines
    }
    return 0;
}

} // namespace goofyRef
