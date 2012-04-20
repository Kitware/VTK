#include  "FTGLTextureFont.h"
#include  "FTTextureGlyph.h"
#ifdef FTGL_DEBUG
  #include "mmgr.h"
#endif

#ifdef FTGL_USE_NAMESPACE
namespace ftgl
{
#endif

inline GLuint NextPowerOf2( GLuint in)
{
     in -= 1;

     in |= in >> 16;
     in |= in >> 8;
     in |= in >> 4;
     in |= in >> 2;
     in |= in >> 1;

     return in + 1;
}


FTGLTextureFont::FTGLTextureFont()
:  maxTextSize(0),
  textureWidth(0),
  textureHeight(0),
  numTextures(0),
  textMem(0),
  glyphHeight(0),
  glyphWidth(0),
  padding(1),
  remGlyphs(0),
  xOffset(0),
  yOffset(0)
{}


FTGLTextureFont::~FTGLTextureFont()
{
  glDeleteTextures( numTextures, (const GLuint*)glTextureID);
}


FTGlyph* FTGLTextureFont::MakeGlyph( unsigned int g)
{
  FT_Glyph* ftGlyph = face.Glyph( g, FT_LOAD_NO_HINTING);
  
  if( ftGlyph)
  {
    // Estimate the glyph size size - global bbox
    glyphHeight = ( charSize.Height());
    glyphWidth = ( charSize.Width());
    
    // Is there a current texture
    if( numTextures == 0)
    {
      glTextureID[0] = CreateTexture();
      xOffset = yOffset = padding;
      ++numTextures;
    }
    
    // will it fit in the current texture
    if( xOffset > ( textureWidth - glyphWidth))
    {
      xOffset = padding;
      yOffset += glyphHeight;
      
      if( yOffset > ( textureHeight - glyphHeight))
      {
        // no - make a new texture
        glTextureID[numTextures] = CreateTexture();
        yOffset = padding;
        ++numTextures;
      }
    }
    
    // yes - load the glyph
    FTTextureGlyph* tempGlyph = new FTTextureGlyph( *ftGlyph, glTextureID[numTextures - 1],
                              xOffset, yOffset, textureWidth, textureHeight);
    
    // FIXME ceiling
                xOffset += (int)(tempGlyph->BBox().x2 - tempGlyph->BBox().x1 + padding);
    
    --remGlyphs;
    return tempGlyph;
  }
  
  err = face.Error();
  return NULL;

}


bool FTGLTextureFont::MakeGlyphList()
{
  if( !maxTextSize)
    glGetIntegerv( GL_MAX_TEXTURE_SIZE, (GLint*)&maxTextSize);

  remGlyphs = numGlyphs;

  FTFont::MakeGlyphList();
  
  return !err; // FIXME what err?
}


void FTGLTextureFont::GetSize()
{
  //work out the max width. Most likely maxTextSize
  textureWidth = NextPowerOf2( (remGlyphs * glyphWidth) + padding * 2);
  if( textureWidth > maxTextSize)
  {
    textureWidth = maxTextSize;
  }
  
  int h = static_cast<int>( (textureWidth - padding * 2) / glyphWidth);
        
  textureHeight = NextPowerOf2( (( numGlyphs / h) + 1) * glyphHeight);
  textureHeight = textureHeight > maxTextSize ? maxTextSize : textureHeight;
}


int FTGLTextureFont::CreateTexture()
{  
  // calc the size
  GetSize();
  
  // allocate some mem and clear it to black
  int totalMem = textureWidth * textureHeight;
  textMem = new unsigned char[totalMem]; // GL_ALPHA texture;
  memset( textMem, 0, totalMem);

  // Create the blank texture
  int textID;
  glGenTextures( 1, (GLuint*)&textID);

  glPixelStorei( GL_UNPACK_ALIGNMENT, 1); //What does this do exactly?
  glBindTexture( GL_TEXTURE_2D, textID);
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  glTexImage2D( GL_TEXTURE_2D, 0, GL_ALPHA, textureWidth, textureHeight, 0, GL_ALPHA, GL_UNSIGNED_BYTE, textMem);

  delete [] textMem;

  return textID;
}


void FTGLTextureFont::render( const char* string)
{  
  glPushAttrib( GL_ENABLE_BIT | GL_HINT_BIT | GL_LINE_BIT | GL_PIXEL_MODE_BIT);
  
  glEnable(GL_BLEND);
   glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // GL_ONE
   
   FTFont::render( string);

  glPopAttrib();
}


void FTGLTextureFont::render( const wchar_t* string)
{  
  glPushAttrib( GL_ENABLE_BIT | GL_HINT_BIT | GL_LINE_BIT | GL_PIXEL_MODE_BIT);
  
  glEnable(GL_BLEND);
   glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // GL_ONE
   
   FTFont::render( string);
  
  glPopAttrib();
}

#ifdef FTGL_USE_NAMESPACE
} // namespace ftgl
#endif
