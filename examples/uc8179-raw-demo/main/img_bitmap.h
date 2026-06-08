#include <stdint.h>
#include "sdkconfig.h"

/*
  prawny-sketch-648x480.h (gray).
  convert prawny-sketch-5146141_1280.png -resize 648x480^ 
    -gravity Center -extent 648x480 \
    -monochrome -negate \
    -define h:format=gray -depth 1 img.h

  https://pixabay.com/illustrations/sketch-vintage-drawing-black-ink-5146141/
  Sketch, Vintage, Drawing royalty-free stock illustration.
  Free for use & download
*/

#define SOURCE_WIDTH 648
#define SOURCE_HEIGHT 480

void make_bitmap(size_t w, size_t h, uint8_t *buffer);
