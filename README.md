# Goofy - Realtime DXT1/ETC1 encoder

## About

This is a very fast DXT/ETC encoder that I wrote, checking out the following idea. **"What if while we design a block compression algorithm, we put the compression speed before everything else?"**
Of course, our compressed results should be reasonable enough; let's say it should be better than a baseline.
Let's set our baseline to a texture downsampled by the factor of two in RGB565 format which gives us the same memory footprint as DXT1/BC1.

Why would we need a compressor that is very-very fast but cannot compete with well-known codecs in terms of quality?

I think such a compressor might be useful for several reasons:
- To quickly encode an uncompressed texture on the fly. When you need to use uncompressed texture for rendering, it may be a good option to compress it first using Goofy to save some device memory and performance.
- To make a "preview" build for massive projects. Usually, you need to compress thousands of textures before you can play or test the  build. Sometimes you don't care about texture quality that much, and you only want to get you playable build as fast as possible.
- Quick preview for live-sync tools. You can immediately show any texture changes using Goofy and then run a more high-quality but slow encoder in parallel to improve the final look of the texture (progressive live texture sync)

## Design Principles

- Performance over quality.
- One heuristics to rule them all. In favor of speed, I can't afford to check different combinations or explore a solution space deep enough.
- SSE2 friendly. Let's get this SSE2 thing to the extreme! I should be able to run encoder sixteen SIMD lanes wide using SSE2 instruction set.

## Goofy Algorithm

1. Find the principal axis using the diagonal of the bounding box in RGB space.
2. Convert the principal axis to perceptual brightness using the YCoCg color model.
3. Convert all the 16 block pixel from RGB to perceptual brightness
4. Project 16 pixels to the principal axis using brightness values
5. For ETC1 encoder, get a base color as an average color of 16 pixels but adjust the brightness to get into the center of the principal axis.

Of course, the devil is in the detail; there are a lot of small optimizations/tricks on how to make it fast
and parallel for 16 pixels at a time. I recommend you to look at the code for this, I tried to make it as
clear as possible and made a lot of comments to keep the data transformation flow clear.

ETC1 always encoded using ETC1s format.


NOTE: Due to quantization based on perceptual brightness and because of ETC1s format limitation Goofy codec doesn't fit well for Normal Maps.

## Performance and Quality

All the performance timings below gathered for the following CPU: i7-7820HQ, 2.9Ghz single thread.
To compute timings, I ran encoder 128 times and chose the fastest timing from the run to avoid noise from OS.

Encoder | MP/s | RGB-PSNR (db)
--- | --- | ---
Baseline | n/a | 33.39 
Goofy DXT1 | 1429 | 37.02
Goofy ETC1 | 1221 | 36.30

Those numbers looks pretty good. As far as I can tell, this is the fastest CPU compressor available at the moment.
https://github.com/castano/nvidia-texture-tools/wiki/RealTimeDXTCompression

## Examples of Compressed Images

![Kodim17](https://raw.githubusercontent.com/SergeyMakeev/goofy/master/Images/kodim17_sample.png)
![Kodim18](https://raw.githubusercontent.com/SergeyMakeev/goofy/master/Images/kodim18_sample.png)
![Lena](https://raw.githubusercontent.com/SergeyMakeev/goofy/master/Images/lena_sample.png)

## Comparison with other Encoders

Encoder | MP/s | RGB-PSNR (db)
--- | --- | ---
Baseline | n/a | 33.39 
Goofy DXT1 | 1429 | 37.02
icbc DXT1 | 24 | 41.00
rgbcx | 60 | 40.85
ryg | 43 | 40.82
Goofy ETC1 | 1221 | 36.30
basisu ETC1 | n/a | 36.27
rg ETC1 | 3 | 40.87

The following chart shows the RGB-PSNR vs. Performance for every image in the test image set.
![Comparison Chart](https://raw.githubusercontent.com/SergeyMakeev/goofy/master/Images/comparison_chart.png)

## Usage

Goofy is a header-only library and it's very easy to use.

```cpp

// Add a preprocessor definition and include goofy header
#define GOOFYTC_IMPLEMENTATION
#include <goofy_tc.h>

// You are all set
void test(unsigned char* result, const unsigned char* input, unsigned int width, unsigned int height, unsigned int stride)
{
  goofy::compressDXT1(dest, source, width, height, stride);
  goofy::compressETC1(dest, source, width, height, stride);
}

```


