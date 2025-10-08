#ifndef __FONTS_H__
#define __FONTS_H__


/// Font data stored PER GLYPH
typedef struct {
  uint16_t bitmapOffset; ///< Pointer into GFXfont->bitmap
  uint8_t width;         ///< Bitmap dimensions in pixels
  uint8_t height;        ///< Bitmap dimensions in pixels
  uint8_t xAdvance;      ///< Distance to advance cursor (x axis)
  int8_t xOffset;        ///< X dist from cursor pos to UL corner
  int8_t yOffset;        ///< Y dist from cursor pos to UL corner
} GFXglyph;

/// Data stored for FONT AS A WHOLE
typedef struct {
  uint8_t *bitmap;  ///< Glyph bitmaps, concatenated
  GFXglyph *glyph;  ///< Glyph array
  uint16_t first;   ///< ASCII extents (first char)
  uint16_t last;    ///< ASCII extents (last char)
  uint8_t yAdvance; ///< Newline distance (y axis)
} GFXfont;

// Free Mono 9pt 7b 
extern const GFXfont FreeMono9pt7b; // Approx. 1516 bytes

// Free Mono 24pt 7b
// extern const GFXfont FreeMono24pt7b; // Approx. 6330 bytes

// Free Mono Bold 18pt 7b
extern const GFXfont FreeMonoBold18pt7b; // Approx. 4485 bytes

// Active font pointer
extern const GFXfont *FONT;

#endif // __FONTS_H__
