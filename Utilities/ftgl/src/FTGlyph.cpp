#include  "FTGlyph.h"
#ifdef FTGL_DEBUG
  #include "mmgr.h"
#endif


FTGlyph::FTGlyph()
:  advance(0),
  err(0)  
{
  pos.x = 0;
  pos.y = 0;
}


FTGlyph::~FTGlyph()
{}
