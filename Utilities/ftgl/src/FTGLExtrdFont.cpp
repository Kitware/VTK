#include  "FTGLExtrdFont.h"
#include  "FTExtrdGlyph.h"
#ifdef FTGL_DEBUG
  #include "mmgr.h"
#endif


FTGLExtrdFont::FTGLExtrdFont()
:  depth(0)
{}


FTGLExtrdFont::~FTGLExtrdFont()
{}


FTGlyph* FTGLExtrdFont::MakeGlyph( unsigned int g)
{
  FT_Glyph* ftGlyph = face.Glyph( g, FT_LOAD_DEFAULT);

  if( ftGlyph)
  {
    FTExtrdGlyph* tempGlyph = new FTExtrdGlyph( *ftGlyph, depth);
    return tempGlyph;
  }

  err = face.Error();
  return NULL;
}


