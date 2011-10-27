#include  "FTGlyph.h"
#ifdef FTGL_DEBUG
  #include "mmgr.h"
#endif

#ifdef FTGL_USE_NAMESPACE
namespace ftgl
{
#endif

FTGlyph::FTGlyph()
:  advance(0),
  err(0)  
{
  pos.x = 0;
  pos.y = 0;
  this->glyphHasBeenConverted = 0;
  this->glyph = 0;
}


FTGlyph::~FTGlyph()
{ 
  if(this->glyph)
    {
    FT_Done_Glyph( glyph );
    }
}

#ifdef FTGL_USE_NAMESPACE
} // namespace ftgl
#endif
