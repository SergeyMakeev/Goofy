#include "decoder.h"
#include <stdint.h>
#include <string.h> // memset

// -------------------------------------------------------------------------------------------------------------------
// This code is borrowed from libktx
// (C) Ericsson AB 2013. All Rights Reserved.
// https://github.com/KhronosGroup/KTX-Software/blob/master/lib/etcdec.cxx
// -------------------------------------------------------------------------------------------------------------------

#define SHIFT(size,startpos) ((startpos)-(size)+1)
#define MASK(size, startpos) (((2<<(size-1))-1) << SHIFT(size,startpos))
#define PUTBITS( dest, data, size, startpos) dest = ((dest & ~MASK(size, startpos)) | ((data << SHIFT(size, startpos)) & MASK(size,startpos)))
#define SHIFTHIGH(size, startpos) (((startpos)-32)-(size)+1)
#define MASKHIGH(size, startpos) (((1<<(size))-1) << SHIFTHIGH(size,startpos))
#define PUTBITSHIGH(dest, data, size, startpos) dest = ((dest & ~MASKHIGH(size, startpos)) | ((data << SHIFTHIGH(size, startpos)) & MASKHIGH(size,startpos)))
#define GETBITS(source, size, startpos)  (( (source) >> ((startpos)-(size)+1) ) & ((1<<(size)) -1))
#define GETBITSHIGH(source, size, startpos)  (( (source) >> (((startpos)-32)-(size)+1) ) & ((1<<(size)) -1))
#define CLAMP(ll,x,ul) (((x)<(ll)) ? (ll) : (((x)>(ul)) ? (ul) : (x)))
#define RED_CHANNEL(img,width,x,y,channels)   img[channels*(y*width+x)+0]
#define GREEN_CHANNEL(img,width,x,y,channels) img[channels*(y*width+x)+1]
#define BLUE_CHANNEL(img,width,x,y,channels)  img[channels*(y*width+x)+2]
#define ALPHA_CHANNEL(img,width,x,y,channels)  img[channels*(y*width+x)+3]

static const int alphaTable[16][8] = {
    { -3, -6, -9, -15, 2, 5, 8, 14 },
    { -3, -7, -10, -13, 2, 6, 9, 12 },
    { -2, -5, -8, -13, 1, 4, 7, 12 },
    { -2, -4, -6, -13, 1, 3, 5, 12 },
    { -3, -6, -8, -12, 2, 5, 7, 11 },
    { -3, -7, -9, -11, 2, 6, 8, 10 },
    { -4, -7, -8, -11, 3, 6, 7, 10 },
    { -3, -5, -8, -11, 2, 4, 7, 10 },
    { -2, -6, -8, -10, 1, 5, 7, 9 },
    { -2, -5, -8, -10, 1, 4, 7, 9 },
    { -2, -4, -8, -10, 1, 3, 7, 9 },
    { -2, -5, -7, -10, 1, 4, 6, 9 },
    { -3, -4, -7, -10, 2, 3, 6, 9 },
    { -1, -2, -3, -10, 0, 1, 2, 9 },
    { -4, -6, -8, -9, 3, 5, 7, 8 },
    { -3, -5, -7, -9, 2, 4, 6, 8 },
};

static void unstuff57bits(uint32_t planar_word1, uint32_t planar_word2, uint32_t &planar57_word1, uint32_t &planar57_word2)
{
    // Get bits from twotimer configuration for 57 bits
    // 
    // Go to this bit layout:
    //
    //      63 62 61 60 59 58 57 56 55 54 53 52 51 50 49 48 47 46 45 44 43 42 41 40 39 38 37 36 35 34 33 32 
    //      -----------------------------------------------------------------------------------------------
    //     |R0               |G01G02              |B01B02  ;B03     |RH1           |RH2|GH                 |
    //      -----------------------------------------------------------------------------------------------
    //
    //      31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
    //      -----------------------------------------------------------------------------------------------
    //     |BH               |RV               |GV                  |BV                | not used          |   
    //      -----------------------------------------------------------------------------------------------
    //
    //  From this:
    // 
    //      63 62 61 60 59 58 57 56 55 54 53 52 51 50 49 48 47 46 45 44 43 42 41 40 39 38 37 36 35 34 33 32 
    //      ------------------------------------------------------------------------------------------------
    //     |//|R0               |G01|/|G02              |B01|/ // //|B02  |//|B03     |RH1           |df|RH2|
    //      ------------------------------------------------------------------------------------------------
    //
    //      31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
    //      -----------------------------------------------------------------------------------------------
    //     |GH                  |BH               |RV               |GV                   |BV              |
    //      -----------------------------------------------------------------------------------------------
    //
    //      63 62 61 60 59 58 57 56 55 54 53 52 51 50 49 48 47 46 45 44 43 42 41 40 39 38 37 36 35 34  33  32 
    //      ---------------------------------------------------------------------------------------------------
    //     | base col1    | dcol 2 | base col1    | dcol 2 | base col 1   | dcol 2 | table  | table  |diff|flip|
    //     | R1' (5 bits) | dR2    | G1' (5 bits) | dG2    | B1' (5 bits) | dB2    | cw 1   | cw 2   |bit |bit |
    //      ---------------------------------------------------------------------------------------------------

    uint8_t RO = GETBITSHIGH(planar_word1, 6, 62);
    uint8_t GO1 = GETBITSHIGH(planar_word1, 1, 56);
    uint8_t GO2 = GETBITSHIGH(planar_word1, 6, 54);
    uint8_t BO1 = GETBITSHIGH(planar_word1, 1, 48);
    uint8_t BO2 = GETBITSHIGH(planar_word1, 2, 44);
    uint8_t BO3 = GETBITSHIGH(planar_word1, 3, 41);
    uint8_t RH1 = GETBITSHIGH(planar_word1, 5, 38);
    uint8_t RH2 = GETBITSHIGH(planar_word1, 1, 32);
    uint8_t GH = GETBITS(planar_word2, 7, 31);
    uint8_t BH = GETBITS(planar_word2, 6, 24);
    uint8_t RV = GETBITS(planar_word2, 6, 18);
    uint8_t GV = GETBITS(planar_word2, 7, 12);
    uint8_t BV = GETBITS(planar_word2, 6, 5);

    planar57_word1 = 0; planar57_word2 = 0;
    PUTBITSHIGH(planar57_word1, RO, 6, 63);
    PUTBITSHIGH(planar57_word1, GO1, 1, 57);
    PUTBITSHIGH(planar57_word1, GO2, 6, 56);
    PUTBITSHIGH(planar57_word1, BO1, 1, 50);
    PUTBITSHIGH(planar57_word1, BO2, 2, 49);
    PUTBITSHIGH(planar57_word1, BO3, 3, 47);
    PUTBITSHIGH(planar57_word1, RH1, 5, 44);
    PUTBITSHIGH(planar57_word1, RH2, 1, 39);
    PUTBITSHIGH(planar57_word1, GH, 7, 38);
    PUTBITS(planar57_word2, BH, 6, 31);
    PUTBITS(planar57_word2, RV, 6, 25);
    PUTBITS(planar57_word2, GV, 7, 19);
    PUTBITS(planar57_word2, BV, 6, 12);
}

static void unstuff58bits(uint32_t thumbH_word1, uint32_t thumbH_word2, uint32_t &thumbH58_word1, uint32_t &thumbH58_word2)
{
    // Go to this layout:
    //
    //     |63 62 61 60 59 58|57 56 55 54 53 52 51|50 49|48 47 46 45 44 43 42 41 40 39 38 37 36 35 34 33|32   |
    //     |-------empty-----|part0---------------|part1|part2------------------------------------------|part3|
    //
    //  from this:
    // 
    //      63 62 61 60 59 58 57 56 55 54 53 52 51 50 49 48 47 46 45 44 43 42 41 40 39 38 37 36 35 34 33 32 
    //      --------------------------------------------------------------------------------------------------|
    //     |//|part0               |// // //|part1|//|part2                                          |df|part3|
    //      --------------------------------------------------------------------------------------------------|

    // move parts
    uint32_t part0 = GETBITSHIGH(thumbH_word1, 7, 62);
    uint32_t part1 = GETBITSHIGH(thumbH_word1, 2, 52);
    uint32_t part2 = GETBITSHIGH(thumbH_word1, 16, 49);
    uint32_t part3 = GETBITSHIGH(thumbH_word1, 1, 32);
    thumbH58_word1 = 0;
    PUTBITSHIGH(thumbH58_word1, part0, 7, 57);
    PUTBITSHIGH(thumbH58_word1, part1, 2, 50);
    PUTBITSHIGH(thumbH58_word1, part2, 16, 48);
    PUTBITSHIGH(thumbH58_word1, part3, 1, 32);

    thumbH58_word2 = thumbH_word2;
}

static void unstuff59bits(uint32_t thumbT_word1, uint32_t thumbT_word2, uint32_t &thumbT59_word1, uint32_t &thumbT59_word2)
{
    // Get bits from twotimer configuration 59 bits. 
    // 
    // Go to this bit layout:
    //
    //     |63 62 61 60 59|58 57 56 55|54 53 52 51|50 49 48 47|46 45 44 43|42 41 40 39|38 37 36 35|34 33 32|
    //     |----empty-----|---red 0---|--green 0--|--blue 0---|---red 1---|--green 1--|--blue 1---|--dist--|
    //
    //     |31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00|
    //     |----------------------------------------index bits---------------------------------------------|
    //
    //
    //  From this:
    // 
    //      63 62 61 60 59 58 57 56 55 54 53 52 51 50 49 48 47 46 45 44 43 42 41 40 39 38 37 36 35 34 33 32 
    //      -----------------------------------------------------------------------------------------------
    //     |// // //|R0a  |//|R0b  |G0         |B0         |R1         |G1         |B1          |da  |df|db|
    //      -----------------------------------------------------------------------------------------------
    //
    //     |31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00|
    //     |----------------------------------------index bits---------------------------------------------|
    //
    //      63 62 61 60 59 58 57 56 55 54 53 52 51 50 49 48 47 46 45 44 43 42 41 40 39 38 37 36 35 34 33 32 
    //      -----------------------------------------------------------------------------------------------
    //     | base col1    | dcol 2 | base col1    | dcol 2 | base col 1   | dcol 2 | table  | table  |df|fp|
    //     | R1' (5 bits) | dR2    | G1' (5 bits) | dG2    | B1' (5 bits) | dB2    | cw 1   | cw 2   |bt|bt|
    //      ------------------------------------------------------------------------------------------------

    // Fix middle part
    thumbT59_word1 = thumbT_word1 >> 1;
    // Fix db (lowest bit of d)
    PUTBITSHIGH(thumbT59_word1, thumbT_word1, 1, 32);
    // Fix R0a (top two bits of R0)
    uint8_t R0a = GETBITSHIGH(thumbT_word1, 2, 60);
    PUTBITSHIGH(thumbT59_word1, R0a, 2, 58);

    // Zero top part (not needed)
    PUTBITSHIGH(thumbT59_word1, 0, 5, 63);

    thumbT59_word2 = thumbT_word2;
}

static void decompressColor(int R_B, int G_B, int B_B, uint8_t colors_RGB444[2][3], uint8_t colors[2][3])
{
    // The color should be retrieved as:
    //
    // c = round(255/(r_bits^2-1))*comp_color
    //
    // This is similar to bit replication
    // 
    // Note -- this code only work for bit replication from 4 bits and up --- 3 bits needs
    // two copy operations.

    colors[0][0] = (colors_RGB444[0][0] << (8 - R_B)) | (colors_RGB444[0][0] >> (R_B - (8 - R_B)));
    colors[0][1] = (colors_RGB444[0][1] << (8 - G_B)) | (colors_RGB444[0][1] >> (G_B - (8 - G_B)));
    colors[0][2] = (colors_RGB444[0][2] << (8 - B_B)) | (colors_RGB444[0][2] >> (B_B - (8 - B_B)));
    colors[1][0] = (colors_RGB444[1][0] << (8 - R_B)) | (colors_RGB444[1][0] >> (R_B - (8 - R_B)));
    colors[1][1] = (colors_RGB444[1][1] << (8 - G_B)) | (colors_RGB444[1][1] >> (G_B - (8 - G_B)));
    colors[1][2] = (colors_RGB444[1][2] << (8 - B_B)) | (colors_RGB444[1][2] >> (B_B - (8 - B_B)));
}

static void calculatePaintColors58H(uint8_t d, uint8_t colors[2][3], uint8_t possible_colors[4][3])
{
    //////////////////////////////////////////////
    //
    //		C3      C1		C4----C1---C2
    //		|		|			  |
    //		|		|			  |
    //		|-------|			  |
    //		|		|			  |
    //		|		|			  |
    //		C4      C2			  C3
    //
    //////////////////////////////////////////////

    const uint8_t table58H[8] = { 3,6,11,16,23,32,41,64 };  // 3-bit table for the 58 bit H-mode

                                                            // C4
    possible_colors[3][0] = CLAMP(0, colors[1][0] - table58H[d], 255);
    possible_colors[3][1] = CLAMP(0, colors[1][1] - table58H[d], 255);
    possible_colors[3][2] = CLAMP(0, colors[1][2] - table58H[d], 255);

    // PATTERN_H
    // C1
    possible_colors[0][0] = CLAMP(0, colors[0][0] + table58H[d], 255);
    possible_colors[0][1] = CLAMP(0, colors[0][1] + table58H[d], 255);
    possible_colors[0][2] = CLAMP(0, colors[0][2] + table58H[d], 255);
    // C2
    possible_colors[1][0] = CLAMP(0, colors[0][0] - table58H[d], 255);
    possible_colors[1][1] = CLAMP(0, colors[0][1] - table58H[d], 255);
    possible_colors[1][2] = CLAMP(0, colors[0][2] - table58H[d], 255);
    // C3
    possible_colors[2][0] = CLAMP(0, colors[1][0] + table58H[d], 255);
    possible_colors[2][1] = CLAMP(0, colors[1][1] + table58H[d], 255);
    possible_colors[2][2] = CLAMP(0, colors[1][2] + table58H[d], 255);
}

static void calculatePaintColors59T(uint8_t d, uint8_t colors[2][3], uint8_t possible_colors[4][3])
{
    //////////////////////////////////////////////
    //
    //		C3      C1		C4----C1---C2
    //		|		|			  |
    //		|		|			  |
    //		|-------|			  |
    //		|		|			  |
    //		|		|			  |
    //		C4      C2			  C3
    //
    //////////////////////////////////////////////

    const uint8_t table59T[8] = { 3,6,11,16,23,32,41,64 };  // 3-bit table for the 59 bit T-mode

                                                            // C4
    possible_colors[3][0] = CLAMP(0, colors[1][0] - table59T[d], 255);
    possible_colors[3][1] = CLAMP(0, colors[1][1] - table59T[d], 255);
    possible_colors[3][2] = CLAMP(0, colors[1][2] - table59T[d], 255);

    // PATTERN_T
    // C3
    possible_colors[0][0] = colors[0][0];
    possible_colors[0][1] = colors[0][1];
    possible_colors[0][2] = colors[0][2];
    // C2
    possible_colors[1][0] = CLAMP(0, colors[1][0] + table59T[d], 255);
    possible_colors[1][1] = CLAMP(0, colors[1][1] + table59T[d], 255);
    possible_colors[1][2] = CLAMP(0, colors[1][2] + table59T[d], 255);
    // C1
    possible_colors[2][0] = colors[1][0];
    possible_colors[2][1] = colors[1][1];
    possible_colors[2][2] = colors[1][2];
}

static void decompressBlockPlanar57c(uint32_t compressed57_1, uint32_t compressed57_2, uint8_t *img, int width, int height, int startx, int starty, int channels)
{
    uint8_t colorO[3], colorH[3], colorV[3];

    colorO[0] = GETBITSHIGH(compressed57_1, 6, 63);
    colorO[1] = GETBITSHIGH(compressed57_1, 7, 57);
    colorO[2] = GETBITSHIGH(compressed57_1, 6, 50);
    colorH[0] = GETBITSHIGH(compressed57_1, 6, 44);
    colorH[1] = GETBITSHIGH(compressed57_1, 7, 38);
    colorH[2] = GETBITS(compressed57_2, 6, 31);
    colorV[0] = GETBITS(compressed57_2, 6, 25);
    colorV[1] = GETBITS(compressed57_2, 7, 19);
    colorV[2] = GETBITS(compressed57_2, 6, 12);

    colorO[0] = (colorO[0] << 2) | (colorO[0] >> 4);
    colorO[1] = (colorO[1] << 1) | (colorO[1] >> 6);
    colorO[2] = (colorO[2] << 2) | (colorO[2] >> 4);

    colorH[0] = (colorH[0] << 2) | (colorH[0] >> 4);
    colorH[1] = (colorH[1] << 1) | (colorH[1] >> 6);
    colorH[2] = (colorH[2] << 2) | (colorH[2] >> 4);

    colorV[0] = (colorV[0] << 2) | (colorV[0] >> 4);
    colorV[1] = (colorV[1] << 1) | (colorV[1] >> 6);
    colorV[2] = (colorV[2] << 2) | (colorV[2] >> 4);

    for (int xx = 0; xx < 4; xx++)
    {
        for (int yy = 0; yy < 4; yy++)
        {
            img[channels*width*(starty + yy) + channels*(startx + xx) + 0] = (uint8_t)CLAMP(0, ((xx*(colorH[0] - colorO[0]) + yy*(colorV[0] - colorO[0]) + 4 * colorO[0] + 2) >> 2), 255);
            img[channels*width*(starty + yy) + channels*(startx + xx) + 1] = (uint8_t)CLAMP(0, ((xx*(colorH[1] - colorO[1]) + yy*(colorV[1] - colorO[1]) + 4 * colorO[1] + 2) >> 2), 255);
            img[channels*width*(starty + yy) + channels*(startx + xx) + 2] = (uint8_t)CLAMP(0, ((xx*(colorH[2] - colorO[2]) + yy*(colorV[2] - colorO[2]) + 4 * colorO[2] + 2) >> 2), 255);
        }
    }
}

static void decompressBlockTHUMB58Hc(uint32_t block_part1, uint32_t block_part2, uint8_t *img, int width, int height, int startx, int starty, int channels)
{
    // First decode left part of block.
    uint8_t colorsRGB444[2][3];
    colorsRGB444[0][0] = GETBITSHIGH(block_part1, 4, 57);
    colorsRGB444[0][1] = GETBITSHIGH(block_part1, 4, 53);
    colorsRGB444[0][2] = GETBITSHIGH(block_part1, 4, 49);

    colorsRGB444[1][0] = GETBITSHIGH(block_part1, 4, 45);
    colorsRGB444[1][1] = GETBITSHIGH(block_part1, 4, 41);
    colorsRGB444[1][2] = GETBITSHIGH(block_part1, 4, 37);

    uint8_t distance = (GETBITSHIGH(block_part1, 2, 33)) << 1;
    uint32_t col0 = GETBITSHIGH(block_part1, 12, 57);
    uint32_t col1 = GETBITSHIGH(block_part1, 12, 45);

    if (col0 >= col1)
        distance |= 1;

    // Extend the two colors to RGB888	
    uint8_t colors[2][3];
    uint8_t paint_colors[4][3];
    decompressColor(4, 4, 4, colorsRGB444, colors);
    calculatePaintColors58H(distance, colors, paint_colors);

    // Choose one of the four paint colors for each texel
    uint8_t block_mask[4][4];
    for (uint8_t x = 0; x < 4; ++x)
    {
        for (uint8_t y = 0; y < 4; ++y)
        {
            block_mask[x][y] = GETBITS(block_part2, 1, (y + x * 4) + 16) << 1;
            block_mask[x][y] |= GETBITS(block_part2, 1, (y + x * 4));
            img[channels*((starty + y)*width + startx + x) + 0] =
                CLAMP(0, paint_colors[block_mask[x][y]][0], 255); // RED
            img[channels*((starty + y)*width + startx + x) + 1] =
                CLAMP(0, paint_colors[block_mask[x][y]][1], 255); // GREEN
            img[channels*((starty + y)*width + startx + x) + 2] =
                CLAMP(0, paint_colors[block_mask[x][y]][2], 255); // BLUE
        }
    }
}

static void decompressBlockTHUMB59Tc(uint32_t block_part1, uint32_t block_part2, uint8_t *img, int width, int height, int startx, int starty, int channels)
{
    // First decode left part of block.
    uint8_t colorsRGB444[2][3];
    colorsRGB444[0][0] = GETBITSHIGH(block_part1, 4, 58);
    colorsRGB444[0][1] = GETBITSHIGH(block_part1, 4, 54);
    colorsRGB444[0][2] = GETBITSHIGH(block_part1, 4, 50);

    colorsRGB444[1][0] = GETBITSHIGH(block_part1, 4, 46);
    colorsRGB444[1][1] = GETBITSHIGH(block_part1, 4, 42);
    colorsRGB444[1][2] = GETBITSHIGH(block_part1, 4, 38);

    uint8_t distance = GETBITSHIGH(block_part1, 3, 34);

    // Extend the two colors to RGB888	
    uint8_t colors[2][3];
    uint8_t paint_colors[4][3];
    decompressColor(4, 4, 4, colorsRGB444, colors);
    calculatePaintColors59T(distance, colors, paint_colors);

    // Choose one of the four paint colors for each texel
    uint8_t block_mask[4][4];
    for (uint8_t x = 0; x < 4; ++x)
    {
        for (uint8_t y = 0; y < 4; ++y)
        {
            block_mask[x][y] = GETBITS(block_part2, 1, (y + x * 4) + 16) << 1;
            block_mask[x][y] |= GETBITS(block_part2, 1, (y + x * 4));
            img[channels*((starty + y)*width + startx + x) + 0] =
                CLAMP(0, paint_colors[block_mask[x][y]][0], 255); // RED
            img[channels*((starty + y)*width + startx + x) + 1] =
                CLAMP(0, paint_colors[block_mask[x][y]][1], 255); // GREEN
            img[channels*((starty + y)*width + startx + x) + 2] =
                CLAMP(0, paint_colors[block_mask[x][y]][2], 255); // BLUE
        }
    }
}

static void decompressBlockDiffFlipC(uint32_t block_part1, uint32_t block_part2, uint8_t *img, int width, int height, int startx, int starty, int channels)
{
    static const int unscramble[4] = { 2, 3, 1, 0 };
    static const int compressParams[16][4] = {
        { -8, -2,  2, 8 },
        { -8, -2,  2, 8 },
        { -17, -5, 5, 17 },
        { -17, -5, 5, 17 },
        { -29, -9, 9, 29 },
        { -29, -9, 9, 29 },
        { -42, -13, 13, 42 },
        { -42, -13, 13, 42 },
        { -60, -18, 18, 60 },
        { -60, -18, 18, 60 },
        { -80, -24, 24, 80 },
        { -80, -24, 24, 80 },
        { -106, -33, 33, 106 },
        { -106, -33, 33, 106 },
        { -183, -47, 47, 183 },
        { -183, -47, 47, 183 }
    };

    int diffbit = (GETBITSHIGH(block_part1, 1, 33));
    int flipbit = (GETBITSHIGH(block_part1, 1, 32));

    if (!diffbit)
    {
        // First decode left part of block.
        uint8_t avg_color[3];
        avg_color[0] = GETBITSHIGH(block_part1, 4, 63);
        avg_color[1] = GETBITSHIGH(block_part1, 4, 55);
        avg_color[2] = GETBITSHIGH(block_part1, 4, 47);

        // Here, we should really multiply by 17 instead of 16. This can
        // be done by just copying the four lower bits to the upper ones
        // while keeping the lower bits.
        avg_color[0] |= (avg_color[0] << 4);
        avg_color[1] |= (avg_color[1] << 4);
        avg_color[2] |= (avg_color[2] << 4);

        // TODO: uint32_t ?
        int table = GETBITSHIGH(block_part1, 3, 39) << 1;

        uint32_t pixel_indices_MSB, pixel_indices_LSB;
        pixel_indices_MSB = GETBITS(block_part2, 16, 31);
        pixel_indices_LSB = GETBITS(block_part2, 16, 15);

        if ((flipbit) == 0)
        {
            // We should not flip
            int shift = 0;
            for (int x = startx; x < startx + 2; x++)
            {
                for (int y = starty; y < starty + 4; y++)
                {
                    int index = ((pixel_indices_MSB >> shift) & 1) << 1;
                    index |= ((pixel_indices_LSB >> shift) & 1);
                    shift++;
                    index = unscramble[index];

                    RED_CHANNEL(img, width, x, y, channels) = (uint8_t)CLAMP(0, avg_color[0] + compressParams[table][index], 255);
                    GREEN_CHANNEL(img, width, x, y, channels) = (uint8_t)CLAMP(0, avg_color[1] + compressParams[table][index], 255);
                    BLUE_CHANNEL(img, width, x, y, channels) = (uint8_t)CLAMP(0, avg_color[2] + compressParams[table][index], 255);
                }
            }
        }
        else
        {
            // We should flip
            int shift = 0;
            for (int x = startx; x < startx + 4; x++)
            {
                for (int y = starty; y < starty + 2; y++)
                {
                    int index = ((pixel_indices_MSB >> shift) & 1) << 1;
                    index |= ((pixel_indices_LSB >> shift) & 1);
                    shift++;
                    index = unscramble[index];

                    RED_CHANNEL(img, width, x, y, channels) = (uint8_t)CLAMP(0, avg_color[0] + compressParams[table][index], 255);
                    GREEN_CHANNEL(img, width, x, y, channels) = (uint8_t)CLAMP(0, avg_color[1] + compressParams[table][index], 255);
                    BLUE_CHANNEL(img, width, x, y, channels) = (uint8_t)CLAMP(0, avg_color[2] + compressParams[table][index], 255);
                }
                shift += 2;
            }
        }

        // Now decode other part of block. 
        avg_color[0] = GETBITSHIGH(block_part1, 4, 59);
        avg_color[1] = GETBITSHIGH(block_part1, 4, 51);
        avg_color[2] = GETBITSHIGH(block_part1, 4, 43);

        // Here, we should really multiply by 17 instead of 16. This can
        // be done by just copying the four lower bits to the upper ones
        // while keeping the lower bits.
        avg_color[0] |= (avg_color[0] << 4);
        avg_color[1] |= (avg_color[1] << 4);
        avg_color[2] |= (avg_color[2] << 4);

        table = GETBITSHIGH(block_part1, 3, 36) << 1;
        pixel_indices_MSB = GETBITS(block_part2, 16, 31);
        pixel_indices_LSB = GETBITS(block_part2, 16, 15);

        if ((flipbit) == 0)
        {
            // We should not flip
            int shift = 8;
            for (int x = startx + 2; x < startx + 4; x++)
            {
                for (int y = starty; y < starty + 4; y++)
                {
                    int index = ((pixel_indices_MSB >> shift) & 1) << 1;
                    index |= ((pixel_indices_LSB >> shift) & 1);
                    shift++;
                    index = unscramble[index];

                    RED_CHANNEL(img, width, x, y, channels) = (uint8_t)CLAMP(0, avg_color[0] + compressParams[table][index], 255);
                    GREEN_CHANNEL(img, width, x, y, channels) = (uint8_t)CLAMP(0, avg_color[1] + compressParams[table][index], 255);
                    BLUE_CHANNEL(img, width, x, y, channels) = (uint8_t)CLAMP(0, avg_color[2] + compressParams[table][index], 255);
                }
            }
        }
        else
        {
            // We should flip
            int shift = 2;
            for (int x = startx; x < startx + 4; x++)
            {
                for (int y = starty + 2; y < starty + 4; y++)
                {
                    int index = ((pixel_indices_MSB >> shift) & 1) << 1;
                    index |= ((pixel_indices_LSB >> shift) & 1);
                    shift++;
                    index = unscramble[index];

                    RED_CHANNEL(img, width, x, y, channels) = (uint8_t)CLAMP(0, avg_color[0] + compressParams[table][index], 255);
                    GREEN_CHANNEL(img, width, x, y, channels) = (uint8_t)CLAMP(0, avg_color[1] + compressParams[table][index], 255);
                    BLUE_CHANNEL(img, width, x, y, channels) = (uint8_t)CLAMP(0, avg_color[2] + compressParams[table][index], 255);
                }
                shift += 2;
            }
        }
    }
    else
    {
        // We have diffbit = 1. 

        //      63 62 61 60 59 58 57 56 55 54 53 52 51 50 49 48 47 46 45 44 43 42 41 40 39 38 37 36 35 34  33  32 
        //      ---------------------------------------------------------------------------------------------------
        //     | base col1    | dcol 2 | base col1    | dcol 2 | base col 1   | dcol 2 | table  | table  |diff|flip|
        //     | R1' (5 bits) | dR2    | G1' (5 bits) | dG2    | B1' (5 bits) | dB2    | cw 1   | cw 2   |bit |bit |
        //      ---------------------------------------------------------------------------------------------------
        // 
        // 
        //     c) bit layout in bits 31 through 0 (in both cases)
        // 
        //      31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3   2   1  0
        //      --------------------------------------------------------------------------------------------------
        //     |       most significant pixel index bits       |         least significant pixel index bits       |  
        //     | p| o| n| m| l| k| j| i| h| g| f| e| d| c| b| a| p| o| n| m| l| k| j| i| h| g| f| e| d| c | b | a |
        //      --------------------------------------------------------------------------------------------------      

        // First decode left part of block.
        uint8_t enc_color1[3];
        enc_color1[0] = GETBITSHIGH(block_part1, 5, 63);
        enc_color1[1] = GETBITSHIGH(block_part1, 5, 55);
        enc_color1[2] = GETBITSHIGH(block_part1, 5, 47);

        // Expand from 5 to 8 bits
        uint8_t avg_color[3];
        avg_color[0] = (enc_color1[0] << 3) | (enc_color1[0] >> 2);
        avg_color[1] = (enc_color1[1] << 3) | (enc_color1[1] >> 2);
        avg_color[2] = (enc_color1[2] << 3) | (enc_color1[2] >> 2);

        int table = GETBITSHIGH(block_part1, 3, 39) << 1;

        uint32_t pixel_indices_MSB, pixel_indices_LSB;

        pixel_indices_MSB = GETBITS(block_part2, 16, 31);
        pixel_indices_LSB = GETBITS(block_part2, 16, 15);

        if ((flipbit) == 0)
        {
            // We should not flip
            int shift = 0;
            for (int x = startx; x < startx + 2; x++)
            {
                for (int y = starty; y < starty + 4; y++)
                {
                    int index = ((pixel_indices_MSB >> shift) & 1) << 1;
                    index |= ((pixel_indices_LSB >> shift) & 1);
                    shift++;
                    index = unscramble[index];

                    RED_CHANNEL(img, width, x, y, channels) = (uint8_t)CLAMP(0, avg_color[0] + compressParams[table][index], 255);
                    GREEN_CHANNEL(img, width, x, y, channels) = (uint8_t)CLAMP(0, avg_color[1] + compressParams[table][index], 255);
                    BLUE_CHANNEL(img, width, x, y, channels) = (uint8_t)CLAMP(0, avg_color[2] + compressParams[table][index], 255);
                }
            }
        }
        else
        {
            // We should flip
            int shift = 0;
            for (int x = startx; x < startx + 4; x++)
            {
                for (int y = starty; y < starty + 2; y++)
                {
                    int index = ((pixel_indices_MSB >> shift) & 1) << 1;
                    index |= ((pixel_indices_LSB >> shift) & 1);
                    shift++;
                    index = unscramble[index];

                    RED_CHANNEL(img, width, x, y, channels) = (uint8_t)CLAMP(0, avg_color[0] + compressParams[table][index], 255);
                    GREEN_CHANNEL(img, width, x, y, channels) = (uint8_t)CLAMP(0, avg_color[1] + compressParams[table][index], 255);
                    BLUE_CHANNEL(img, width, x, y, channels) = (uint8_t)CLAMP(0, avg_color[2] + compressParams[table][index], 255);
                }
                shift += 2;
            }
        }

        // Now decode right part of block. 
        int8_t diff[3];
        diff[0] = GETBITSHIGH(block_part1, 3, 58);
        diff[1] = GETBITSHIGH(block_part1, 3, 50);
        diff[2] = GETBITSHIGH(block_part1, 3, 42);

        // Extend sign bit to entire byte. 
        diff[0] = (diff[0] << 5);
        diff[1] = (diff[1] << 5);
        diff[2] = (diff[2] << 5);
        diff[0] = diff[0] >> 5;
        diff[1] = diff[1] >> 5;
        diff[2] = diff[2] >> 5;

        //  Calculate second color
        uint8_t enc_color2[3];
        enc_color2[0] = enc_color1[0] + diff[0];
        enc_color2[1] = enc_color1[1] + diff[1];
        enc_color2[2] = enc_color1[2] + diff[2];

        // Expand from 5 to 8 bits
        avg_color[0] = (enc_color2[0] << 3) | (enc_color2[0] >> 2);
        avg_color[1] = (enc_color2[1] << 3) | (enc_color2[1] >> 2);
        avg_color[2] = (enc_color2[2] << 3) | (enc_color2[2] >> 2);

        table = GETBITSHIGH(block_part1, 3, 36) << 1;
        pixel_indices_MSB = GETBITS(block_part2, 16, 31);
        pixel_indices_LSB = GETBITS(block_part2, 16, 15);

        if ((flipbit) == 0)
        {
            // We should not flip
            int shift = 8;
            for (int x = startx + 2; x < startx + 4; x++)
            {
                for (int y = starty; y < starty + 4; y++)
                {
                    int index = ((pixel_indices_MSB >> shift) & 1) << 1;
                    index |= ((pixel_indices_LSB >> shift) & 1);
                    shift++;
                    index = unscramble[index];

                    RED_CHANNEL(img, width, x, y, channels) = (uint8_t)CLAMP(0, avg_color[0] + compressParams[table][index], 255);
                    GREEN_CHANNEL(img, width, x, y, channels) = (uint8_t)CLAMP(0, avg_color[1] + compressParams[table][index], 255);
                    BLUE_CHANNEL(img, width, x, y, channels) = (uint8_t)CLAMP(0, avg_color[2] + compressParams[table][index], 255);
                }
            }
        }
        else
        {
            // We should flip
            int shift = 2;
            for (int x = startx; x < startx + 4; x++)
            {
                for (int y = starty + 2; y < starty + 4; y++)
                {
                    int index = ((pixel_indices_MSB >> shift) & 1) << 1;
                    index |= ((pixel_indices_LSB >> shift) & 1);
                    shift++;
                    index = unscramble[index];

                    RED_CHANNEL(img, width, x, y, channels) = (uint8_t)CLAMP(0, avg_color[0] + compressParams[table][index], 255);
                    GREEN_CHANNEL(img, width, x, y, channels) = (uint8_t)CLAMP(0, avg_color[1] + compressParams[table][index], 255);
                    BLUE_CHANNEL(img, width, x, y, channels) = (uint8_t)CLAMP(0, avg_color[2] + compressParams[table][index], 255);
                }
                shift += 2;
            }
        }
    }
}

static void decompressBlockAlphaC(const uint8_t* data, uint8_t* img, int width, int height, int ix, int iy, int channels)
{
    int alpha = data[0];
    int multiplier = (data[1] & 0xF0) >> 4;
    int table = (data[1] & 0x0F);

    int bit = 0;
    int byte = 2;

    // extract an alpha value for each pixel
    for (int x = 0; x < 4; x++)
    {
        for (int y = 0; y < 4; y++)
        {
            // extract table index
            int index = 0;
            for (int bitpos = 0; bitpos < 3; bitpos++)
            {
                int frompos = 7 - bit;
                int topos = 2 - bitpos;
                index |= (frompos > topos) ? ((1 << frompos) & data[byte]) >> (frompos - topos) :
                    ((1 << frompos) & data[byte]) << (topos - frompos);
                bit++;
                if (bit > 7)
                {
                    bit = 0;
                    byte++;
                }
            }

            int val = alpha + alphaTable[table][index] * multiplier;
            img[(ix + x + (iy + y)*width)*channels] = (uint8_t)CLAMP(0, val, 255);
        }
    }
}

static void decompressBlockETC2c(uint32_t block_part1, uint32_t block_part2, uint8_t* img, int width, int height, int startx, int starty, int channels)
{
    int diffbit = (GETBITSHIGH(block_part1, 1, 33));

    if (diffbit)
    {
        // Base color
        int8_t color1[3];
        color1[0] = GETBITSHIGH(block_part1, 5, 63);
        color1[1] = GETBITSHIGH(block_part1, 5, 55);
        color1[2] = GETBITSHIGH(block_part1, 5, 47);

        // Diff color
        int8_t diff[3];
        diff[0] = GETBITSHIGH(block_part1, 3, 58);
        diff[1] = GETBITSHIGH(block_part1, 3, 50);
        diff[2] = GETBITSHIGH(block_part1, 3, 42);

        // Extend sign bit to entire byte. 
        diff[0] = (diff[0] << 5);
        diff[1] = (diff[1] << 5);
        diff[2] = (diff[2] << 5);
        diff[0] = diff[0] >> 5;
        diff[1] = diff[1] >> 5;
        diff[2] = diff[2] >> 5;

        int8_t red = color1[0] + diff[0];
        int8_t green = color1[1] + diff[1];
        int8_t blue = color1[2] + diff[2];

        if (red < 0 || red > 31)
        {
            uint32_t block59_part1, block59_part2;
            unstuff59bits(block_part1, block_part2, block59_part1, block59_part2);
            decompressBlockTHUMB59Tc(block59_part1, block59_part2, img, width, height, startx, starty, channels);
        }
        else if (green < 0 || green > 31)
        {
            uint32_t block58_part1, block58_part2;
            unstuff58bits(block_part1, block_part2, block58_part1, block58_part2);
            decompressBlockTHUMB58Hc(block58_part1, block58_part2, img, width, height, startx, starty, channels);
        }
        else if (blue < 0 || blue > 31)
        {
            uint32_t block57_part1, block57_part2;
            unstuff57bits(block_part1, block_part2, block57_part1, block57_part2);
            decompressBlockPlanar57c(block57_part1, block57_part2, img, width, height, startx, starty, channels);
        }
        else
        {
            decompressBlockDiffFlipC(block_part1, block_part2, img, width, height, startx, starty, channels);
        }
    }
    else
    {
        decompressBlockDiffFlipC(block_part1, block_part2, img, width, height, startx, starty, channels);
    }
}

#undef SHIFT
#undef MASK
#undef PUTBITS
#undef SHIFTHIGH
#undef MASKHIGH
#undef PUTBITSHIGH
#undef GETBITS
#undef GETBITSHIGH
#undef CLAMP
#undef RED_CHANNEL
#undef GREEN_CHANNEL
#undef BLUE_CHANNEL
#undef ALPHA_CHANNEL


// -------------------------------------------------------------------------------------------------------------------
// This code is borrowed from libsquish
// Copyright (c) 2006 Simon Brown. All Rights Reserved.
// https://github.com/Cavewhere/squish/blob/master/colourblock.cpp
// https://github.com/Cavewhere/squish/blob/master/alpha.cpp
// -------------------------------------------------------------------------------------------------------------------

typedef unsigned char u8;

static int Unpack565(u8 const* packed, u8* colour)
{
    // build the packed value
    int value = (int)packed[0] | ((int)packed[1] << 8);

    // get the components in the stored range
    u8 red = (u8)((value >> 11) & 0x1f);
    u8 green = (u8)((value >> 5) & 0x3f);
    u8 blue = (u8)(value & 0x1f);

    // scale up to 8 bits
    colour[0] = (red << 3) | (red >> 2);
    colour[1] = (green << 2) | (green >> 4);
    colour[2] = (blue << 3) | (blue >> 2);
    colour[3] = 255;

    // return the value
    return value;
}

void DecompressColour(u8* rgba, void const* block, bool isDxt1)
{
    // get the block bytes
    u8 const* bytes = reinterpret_cast< u8 const* >(block);

    // unpack the endpoints
    u8 codes[16];
    int a = Unpack565(bytes, codes);
    int b = Unpack565(bytes + 2, codes + 4);

    // generate the midpoints
    for (int i = 0; i < 3; ++i)
    {
        int c = codes[i];
        int d = codes[4 + i];

        if (isDxt1 && a <= b)
        {
            codes[8 + i] = (u8)((c + d) / 2);
            codes[12 + i] = 0;
        }
        else
        {
            codes[8 + i] = (u8)((2 * c + d) / 3);
            codes[12 + i] = (u8)((c + 2 * d) / 3);
        }
    }

    // fill in alpha for the intermediate values
    codes[8 + 3] = 255;
    codes[12 + 3] = (isDxt1 && a <= b) ? 0 : 255;

    // unpack the indices
    u8 indices[16];
    for (int i = 0; i < 4; ++i)
    {
        u8* ind = indices + 4 * i;
        u8 packed = bytes[4 + i];

        ind[0] = packed & 0x3;
        ind[1] = (packed >> 2) & 0x3;
        ind[2] = (packed >> 4) & 0x3;
        ind[3] = (packed >> 6) & 0x3;
    }

    // store out the colours
    for (int i = 0; i < 16; ++i)
    {
        u8 offset = 4 * indices[i];
        for (int j = 0; j < 4; ++j)
            rgba[4 * i + j] = codes[offset + j];
    }
}


void DecompressAlphaDxt5(u8* rgba, void const* block)
{
    // get the two alpha values
    u8 const* bytes = reinterpret_cast< u8 const* >(block);
    int alpha0 = bytes[0];
    int alpha1 = bytes[1];

    // compare the values to build the codebook
    u8 codes[8];
    codes[0] = (u8)alpha0;
    codes[1] = (u8)alpha1;
    if (alpha0 <= alpha1)
    {
        // use 5-alpha codebook
        for (int i = 1; i < 5; ++i)
            codes[1 + i] = (u8)(((5 - i)*alpha0 + i*alpha1) / 5);
        codes[6] = 0;
        codes[7] = 255;
    }
    else
    {
        // use 7-alpha codebook
        for (int i = 1; i < 7; ++i)
            codes[1 + i] = (u8)(((7 - i)*alpha0 + i*alpha1) / 7);
    }

    // decode the indices
    u8 indices[16];
    u8 const* src = bytes + 2;
    u8* dest = indices;
    for (int i = 0; i < 2; ++i)
    {
        // grab 3 bytes
        int value = 0;
        for (int j = 0; j < 3; ++j)
        {
            int byte = *src++;
            value |= (byte << 8 * j);
        }

        // unpack 8 3-bit values from it
        for (int j = 0; j < 8; ++j)
        {
            int index = (value >> 3 * j) & 0x7;
            *dest++ = (u8)index;
        }
    }

    // write out the indexed codebook values
    for (int i = 0; i < 16; ++i)
        rgba[4 * i + 3] = codes[indices[i]];
}



namespace DecoderBC
{


void decodeBlockDXT1(const unsigned char* source, unsigned char* target, size_t targetStide)
{
    unsigned char rgba8[64];
    memset(&rgba8[0], 0xFF, 64);
    DecompressColour(&rgba8[0], source, true);

    memcpy(target, &rgba8[0], 16);
    memcpy(target + targetStide, &rgba8[16], 16);
    memcpy(target + targetStide * 2, &rgba8[32], 16);
    memcpy(target + targetStide * 3, &rgba8[48], 16);
}

void decodeBlockDXT5(const unsigned char* source, unsigned char* target, size_t targetStide)
{
    unsigned char rgba8[64];
    memset(&rgba8[0], 0xFF, 64);
    DecompressColour(&rgba8[0], source + 8, false);
    DecompressAlphaDxt5(&rgba8[0], source);

    memcpy(target, &rgba8[0], 16);
    memcpy(target + targetStide, &rgba8[16], 16);
    memcpy(target + targetStide * 2, &rgba8[32], 16);
    memcpy(target + targetStide * 3, &rgba8[48], 16);
}

void decodeBlockETC1(const unsigned char* source, unsigned char* target, size_t targetStide)
{
    unsigned char rgba8[64];
    memset(&rgba8[0], 0xFF, 64);

    uint32_t block1 = (source[0] << 24) | (source[1] << 16) | (source[2] << 8) | source[3];
    uint32_t block2 = (source[4] << 24) | (source[5] << 16) | (source[6] << 8) | source[7];
    decompressBlockETC2c(block1, block2, &rgba8[0], 4, 4, 0, 0, 4);

    memcpy(target, &rgba8[0], 16);
    memcpy(target + targetStide, &rgba8[16], 16);
    memcpy(target + targetStide * 2, &rgba8[32], 16);
    memcpy(target + targetStide * 3, &rgba8[48], 16);
}

void decodeBlockETC2(const unsigned char* source, unsigned char* target, size_t targetStide)
{
    unsigned char rgba8[64];
    memset(&rgba8[0], 0xFF, 64);

    decompressBlockAlphaC(source, &rgba8[3], 4, 4, 0, 0, 4);

    uint32_t block1 = (source[8] << 24) | (source[9] << 16) | (source[10] << 8) | source[11];
    uint32_t block2 = (source[12] << 24) | (source[13] << 16) | (source[14] << 8) | source[15];
    decompressBlockETC2c(block1, block2, &rgba8[0], 4, 4, 0, 0, 4);

    memcpy(target, &rgba8[0], 16);
    memcpy(target + targetStide, &rgba8[16], 16);
    memcpy(target + targetStide * 2, &rgba8[32], 16);
    memcpy(target + targetStide * 3, &rgba8[48], 16);
}

}