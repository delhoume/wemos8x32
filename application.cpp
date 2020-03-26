 #include "application.h"

 void Application::display() {
  getMatrix().fillRect(0, 0, getMatrix().width(), getMatrix().height(), CRGB::Black);
  drawDefaultIcon();
  if (getDefaultIcon() && (getDefaultIcon()->getWidth() <= 8)) {
    getMatrix().setCursor(10, 7);
    getMatrix().print(getName());
  }
}

// image can be RGB or I
void Application::drawImage(int xpos, int ypos, Image* image) {
  if (!image)
    return;
   for (int y = 0; y < image->getHeight(); ++y) {
    for (int x = 0; x < image->getWidth(); ++x) {
      CRGB color = image->getPixel(x, y);
      getMatrix().drawPixel(xpos + x, ypos + y, color);
    }
   }
}

Image* Application::decodeFile(const char* name) {
  File f = SPIFFS.open(name, "r");
  Image* image = 0;
  if (f) {
     char header[3];
     f.readBytes((char*)header, 3);
     f.seek(0, SeekSet);
     if ((header[0] == 'B') && (header[1] == 'M')) {
        image = decodeBMPFile(f);
     } else if ((header[0] == 'G') && (header[1] == 'I') && (header[2] == 'F')) {
        image = decodeGIFFile(f);
     }
     f.close();
  } 
  return image;
}

static inline int16_t read_int16(const uint8_t *data, unsigned int offset) {
        return (int16_t) (data[offset] | (data[offset + 1] << 8));
}

static inline uint16_t read_uint16(const uint8_t *data, unsigned int offset) {
        return (uint16_t) (data[offset] | (data[offset + 1] << 8));
}

static inline int32_t read_int32(const uint8_t *data, unsigned int offset) {
        return (int32_t) (data[offset] | (data[offset + 1] << 8) | 
                        (data[offset + 2] << 16) | (data[offset + 3] << 24));
}

static inline uint32_t read_uint32(const uint8_t *data, uint32_t offset) {
        return (uint32_t) (data[offset] | (data[offset + 1] << 8) | 
                          (data[offset + 2] << 16) | (data[offset + 3] << 24));
}


#define BMP_FILE_HEADER_SIZE 14

typedef enum {
        BMP_ENCODING_RGB = 0,
        BMP_ENCODING_RLE8 = 1,
        BMP_ENCODING_RLE4 = 2,
        BMP_ENCODING_BITFIELDS = 3
} bmp_encoding;

typedef enum {
  RLE_EOL = 0,
  RLE_EOB = 1,
  RLE_DELTA = 2
} rle8;


Image* Application::decodeBMPFile(File file) {
    uint8_t header[BMP_FILE_HEADER_SIZE];
    uint8_t fourbytes[4];
      uint8_t r, g, b;
  if (file.size() <= BMP_FILE_HEADER_SIZE)
    return NULL;
 //   Serial.println(); 
  file.readBytes((char*)header, BMP_FILE_HEADER_SIZE);
  if (header[0] != 'B' || header[1] != 'M') {
    Serial.println("not a BMP Image");
    return NULL;
  }
  uint32_t offset = read_uint32(header, 10);
//  Serial.print("offset "); Serial.println(offset);
  // now we are at BITMAPINFOHEADER POS
  // read size from file
  file.readBytes((char*)fourbytes, 4);
  // rear from memory
  uint32_t biSize = read_uint32(fourbytes, 0);
//  Serial.print("biSize "); Serial.println(biSize);
  // allocate data for BITMAPINFOHEADER and read
  uint8_t* infoheader = (uint8_t*)malloc(biSize - 4);
  file.readBytes((char*)infoheader, biSize - 4);

  // read normally from memory, starting from width
   int32_t biWidth = read_int32(infoheader, 0);
//    Serial.print("biWidth "); Serial.println(biWidth);
     // negative means top down instead of bottom up
    int32_t biHeight = read_int32(infoheader, 4);
//    Serial.print("biHeight "); Serial.println(biHeight);
    boolean reversed = false;
    if (biHeight < 0) {
      reversed = true;
      biHeight = -biHeight;
    }
    uint16_t biBitCount = read_uint16(infoheader, 10);
//    Serial.print("biBitCount "); Serial.println(biBitCount);
   int32_t biCompression = read_int32(infoheader, 12);
 //   Serial.print("biCompression "); Serial.println(biCompression);
    uint32_t bytesPerRow = ((biWidth * biBitCount + 31) / 32) * 4;
   //Serial.print("bytesPerRow "); Serial.println(bytesPerRow);
    // no need for BIH
    free(infoheader);
    uint8_t* palette = 0;
    uint16_t paletteSize = 0;
    if ((biBitCount == 8) || (biBitCount == 4)) { 
      paletteSize = (uint16_t)(1 << biBitCount); // 8 supported
//      Serial.print("Palette size "); Serial.println(paletteSize);
      // read palette from file
      palette = (uint8_t*)malloc(paletteSize * 4);
      // read normally from memory
      file.readBytes((char*)palette, paletteSize * 4);
      // display palette RGBQUAD
      // char buffer[64];     
      //for (uint16_t idx = 0; idx < paletteSize; ++idx) {
      //  sprintf(buffer, "entry %d : %d %d %d", idx, palette[4 * idx + 2], palette[4 * idx + 1], palette[4 * idx + 0]);
       // Serial.println(buffer);
      //}
    } else if (biBitCount == 24) {
      // no palette
//      Serial.println("No palette");
    } else {
//      Serial.println("Not supported");
      Serial.println(biBitCount);
      return 0;
    }
    Image* image = new Image(biWidth, biHeight, paletteSize);
    // copy palette to image
    if (palette != 0) {
      for (int c = 0; c < paletteSize; ++c) {
        uint8_t* paletteEntry = palette + 4 * c;                
        image->setCmap(c, CRGB(paletteEntry[2], paletteEntry[1], paletteEntry[0]));
      }
    }
    // pixels follow at offset
    file.seek(offset, SeekSet);
     switch (biCompression) {
      case BMP_ENCODING_RGB: { // supported case 24 and 8 bits/pixel
        // read data
 //       Serial.println("Reading pixel data");
            // allocating one line
        uint8_t* startRow = (uint8_t*)malloc(bytesPerRow);
        for (int32_t y = 0; y < biHeight; ++y) {
          int32_t yy = reversed ? y : (biHeight - y - 1);
          file.readBytes((char*)startRow, bytesPerRow);
         // then parse data in memory
         if (biBitCount == 24) { 
            for (int32_t x = 0; x < biWidth; ++x) {            
                r = startRow[x * 3 + 2]; g = startRow[x * 3 + 1]; b = startRow[x * 3 + 0];
               image->setPixel(x, yy, CRGB(r, g, b));
            }
          } else if (biBitCount == 8) {  // direct index to palette 
            for (int32_t x = 0; x < biWidth; ++x) { 
                  uint16_t index = startRow[x];
#if 0
                  uint8_t* paletteEntry = palette + 4 * index; 
                   r = paletteEntry[2]; g = paletteEntry[1]; b = paletteEntry[0];
                  image->setPixel(x, yy, CRGB(r, g, b)); 
 #else
                  image->setPixel(x, yy, index);    
 #endif
            }
          } else if (biBitCount == 4) {
            int32_t cpixel = 0;
            for (int32_t x = 0; x < biWidth; ++x) {
                uint8_t index1 = startRow[cpixel] >> 4;
                uint8_t index2 = startRow[cpixel] & 0xf;
#if 0
                uint8_t* paletteEntry1 = palette + 4 * index1;
                r = paletteEntry1[2]; g = paletteEntry1[1]; b = paletteEntry1[0];
                // emit first pixel
                image->setPixel(x, yy, CRGB(r, g, b));
#else
                image->setPixel(x, yy, index1);
 #endif
                ++x;
                if (x < biWidth) {
                  // emit the second pixel
#if 0                  
                  uint8_t* paletteEntry2 = palette + 4 * index2;
                  r = paletteEntry2[2]; g = paletteEntry2[1]; b = paletteEntry2[0];
                  image->setPixel(x, yy, CRGB(r, g, b));
#else
                  image->setPixel(x, yy, index2);
#endif                  
                }
                cpixel++;
             }
           }
        }
       free(startRow);
         break;
      }
      case BMP_ENCODING_RLE8: {
 //          Serial.println("Reading RLE pixel data");
           int32_t x = 0; int32_t y = 0;
           boolean decode = true;
           while (decode == true) {
                uint8_t length;
                file.readBytes((char*)&length, 1);
                if (length == 0) {
                  uint8_t code;
                  file.readBytes((char*)&code, 1);
                  switch (code) {
                    case RLE_EOL: { // end of line
                      x = 0; y++;
                      break;
                    }
                    case RLE_EOB: { // end of file
                      decode = false;
                      break;
                    }
                    case RLE_DELTA: { // offset mode
                      uint8_t delta[2];
                      file.readBytes((char*)delta, 2);
                      x += delta[0]; y += delta[1];
                      break;
                    } 
                    default: { // literal mode
                      file.readBytes((char*)&length, 1);
                      for (uint8_t idx = 0; idx < length; ++idx) {
                        if (x >= biWidth) {
                          x = 0; y++;
                        }
                        // use literal value
                        uint8_t index;
                        file.readBytes((char*)&index, 1);
                        int32_t yy = reversed ? y : (biHeight - y - 1);
#if 0                        
                        const uint8_t* paletteEntry = palette + 4 * index; 
                        r = paletteEntry[2]; g = paletteEntry[1]; b = paletteEntry[0];
                        image->setPixel(x, yy, CRGB(r, g, b));
#else
                        image->setPixel(x, yy, index);
#endif                                                
                        x++;
                      }
                      if (length & 1) // fill byte
                        file.readBytes((char*)index, 1); // not used
                    }
                  }
              } else { // normal run
                  uint8_t index;
                  file.readBytes((char*)&index, 1);
                  for (uint8_t idx = 0; idx < length; ++idx) {
                      if (x >= biWidth) {
                        x = 0; y++;
                      }
                      x++;
                      int32_t yy = reversed ? y : (biHeight - y - 1);
#if 0                      
                      const uint8_t* paletteEntry = palette + 4 * index; 
                      r = paletteEntry[2]; g = paletteEntry[1]; b = paletteEntry[0];
                      image->setPixel(x, yy, CRGB(r, g, b));
#else
                      image->setPixel(x, yy, index);
#endif
                  } 
                  x++;
                  if (y >= biHeight) {
                      decode = false;         
                  }
              } 
           }      
        break;
      }
    } 
    if (palette)
      free(palette);
     return image;
 }

 const char* extensionName(uint8_t type) {
  switch (type) {
    case 0x1: return "Plain type";
    case 0xf9: return "Graphic control";
    case 0xfe: return "Comment";
    case 0xff: return "Application";
    default: return "Unknown";
  }
 }

 Image* Application::decodeGIFFile(File file) {
  Serial.println("Decoding GIF");
  for (uint8 idx = 0; idx < 6; ++idx) { // skip signature and version
    file.read();
  }
  // logical descriptor
  uint8_t logicalDescriptor[7];
  file.readBytes((char*)logicalDescriptor, 7);
  uint16_t width = read_int16(logicalDescriptor, 0);
  uint16_t height = read_int16(logicalDescriptor, 2);
  Serial.println(String(width) + " x " + height);
  uint8_t info = logicalDescriptor[4];
  uint8_t nColors = 1 << ((info & 7) + 1);
  boolean globalColorMap = info & 0x80;
  // read global colormap
  if (globalColorMap) {
   Serial.println(String("Global colormap ") + nColors);
   for (uint8_t c = 0; c < nColors; ++c) {
      uint8_t r, g, b;
      r = file.read(); g = file.read(); b = file.read();
      Serial.println(String(c) + ": " + r + ", " + g + ", " + b);
    }
  }
 uint16_t nImage = 0;
  while (1) { // read all blocks
    uint8_t blockType = file.read();
    if (blockType == 0x21) { // extension
      uint8_t extensionType = file.read();
      uint8_t extensionSize = file.read();
      Serial.println(String("Extension ") + extensionName(extensionType) + " - " + extensionSize);
      for (uint8_t s = 0; s < extensionSize; ++s) { // skip (could use delayTime from Graphic Control)
        uint8_t b = file.read();
      }
     } else if (blockType == 0x2c) { // read an image block
      Serial.println(String("Image ") + nImage++);
      uint8_t imageHeader[9];
      file.readBytes((char*)imageHeader, 9);
      uint16_t left = read_int16(imageHeader, 0);
      uint16_t right = read_int16(imageHeader, 2);
      uint16_t width = read_int16(imageHeader, 4);
      uint16_t height = read_int16(imageHeader, 6);
      Serial.println(String(left) + ", " + right + " - " + width + " x " + height);
      uint8_t info = imageHeader[8];
      uint8_t nColors = 1 << ((info & 7) + 1);
      boolean localColorMap = info & 0x80;
      if (localColorMap) { // skip
          Serial.println(String("Local colormap ") + nColors);
          for (uint8_t c = 0; c < nColors; ++c) {
            uint8_t r, g, b;
            r = file.read(); g = file.read(); b = file.read();
            Serial.println(String(c) + ": " + r + ", " + g + ", " + b);
           }
      }
      boolean interlaced = info & 0x40;
      if (interlaced) {
        Serial.println("Interlaced");
      }
       uint8_t initialCodeSize = file.read();
       Serial.println(String("Initial code size ") + initialCodeSize);
      // now skip all blocks
      while (1) {
        uint8_t dataBlockSize = file.read();
        if (dataBlockSize != 0) { // else end of blocks and end of image
          Serial.print("b");
          for (uint8_t d = 0; d < dataBlockSize; ++d) { // skip (TODO: decompress blocks)
            file.read();
          }
        } else {
          Serial.println();
          break;
        }
      }
    } else if (blockType == 0x3B) { // terminator
      Serial.println("Terminator");
      break;
    }
  }
  return 0;
 }
