#include  "FTGLBitmapFont.h"
#include  "FTBitmapGlyph.h"
#ifdef FTGL_DEBUG
  #include "mmgr.h"
#endif


FTGLBitmapFont::FTGLBitmapFont()
{}


FTGLBitmapFont::~FTGLBitmapFont()
{}


FTGlyph* FTGLBitmapFont::MakeGlyph( unsigned int g)
{
  FT_Glyph* ftGlyph = face.Glyph( g, FT_LOAD_DEFAULT);

  if( ftGlyph)
  {
    FTBitmapGlyph* tempGlyph = new FTBitmapGlyph( *ftGlyph);
//                FT_Done_Glyph( *ftGlyph );

    return tempGlyph;
  }

  err = face.Error();
  return NULL;
}


void FTGLBitmapFont::render( const char* string)
{  
  glPushClientAttrib( GL_CLIENT_PIXEL_STORE_BIT);
  glPushAttrib( GL_ENABLE_BIT);
    
  glPixelStorei( GL_UNPACK_LSB_FIRST, GL_FALSE);
  glPixelStorei( GL_UNPACK_ROW_LENGTH, 0);
  glPixelStorei( GL_UNPACK_ALIGNMENT, 1);

  glDisable( GL_BLEND);

  FTFont::render( string);

  glPopAttrib();
  glPopClientAttrib();
}


void FTGLBitmapFont::render( const wchar_t* string)
{  
  glPushClientAttrib( GL_CLIENT_PIXEL_STORE_BIT);
  glPushAttrib( GL_ENABLE_BIT);
    
  glPixelStorei( GL_UNPACK_LSB_FIRST, GL_FALSE);
  glPixelStorei( GL_UNPACK_ROW_LENGTH, 0);
  glPixelStorei( GL_UNPACK_ALIGNMENT, 1);
  
  glDisable( GL_BLEND);
    
  FTFont::render( string);

  glPopAttrib();
  glPopClientAttrib();
}

