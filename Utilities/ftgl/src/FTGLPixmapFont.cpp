#include  "FTGLPixmapFont.h"
#include  "FTPixmapGlyph.h"
#ifdef FTGL_DEBUG
  #include "mmgr.h"
#endif


FTGLPixmapFont::FTGLPixmapFont()
{}


FTGLPixmapFont::~FTGLPixmapFont()
{}


FTGlyph* FTGLPixmapFont::MakeGlyph( unsigned int g)
{
  FT_Glyph* ftGlyph = face.Glyph( g, FT_LOAD_DEFAULT);

  if( ftGlyph)
  {
    FTPixmapGlyph* tempGlyph = new FTPixmapGlyph( *ftGlyph);
    return tempGlyph;
  }

  err = face.Error();
  return NULL;
}


void FTGLPixmapFont::render( const char* string)
{  
  glPushClientAttrib( GL_CLIENT_PIXEL_STORE_BIT);
  glPushAttrib( GL_ENABLE_BIT | GL_PIXEL_MODE_BIT);

  glPixelStorei( GL_UNPACK_ROW_LENGTH, 0);

  glEnable(GL_BLEND);
  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable( GL_TEXTURE_2D);

  FTFont::render( string);

  glPopAttrib();
  glPopClientAttrib();
}


void FTGLPixmapFont::render( const wchar_t* string)
{  
  glPushClientAttrib( GL_CLIENT_PIXEL_STORE_BIT);
  glPushAttrib( GL_ENABLE_BIT | GL_PIXEL_MODE_BIT);

  glPixelStorei( GL_UNPACK_ROW_LENGTH, 0);

  glEnable(GL_BLEND);
  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable( GL_TEXTURE_2D);

  FTFont::render( string);

  glPopAttrib();
  glPopClientAttrib();
}


