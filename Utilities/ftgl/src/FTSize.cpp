#include  "FTSize.h"
#ifdef FTGL_DEBUG
  #include "mmgr.h"
#endif

#ifdef FTGL_USE_NAMESPACE
namespace ftgl
{
#endif

FTSize::FTSize()
:  ftFace(0),
  size(0),
  err(0)
{}


FTSize::~FTSize()
{}


bool FTSize::CharSize( FT_Face* face, unsigned int point_size, unsigned int x_resolution, unsigned int y_resolution )
{
  ftFace = face;
  size = point_size;
  err = FT_Set_Char_Size( *ftFace, 0L, point_size * 64, x_resolution, y_resolution);
  
  ftSize = (*ftFace)->size;
  
  return !err;
}


int FTSize::Ascender() const
{
  return ftSize->metrics.ascender >> 6;
}


int FTSize::Descender() const
{
  return ftSize->metrics.descender >> 6;
}


int FTSize::Height() const
{
  if( FT_IS_SCALABLE((*ftFace)))
  {
    float height;
    if( FT_IS_SFNT((*ftFace))) // Don't think this is correct
    {
      height = (float)((*ftFace)->bbox.yMax - (*ftFace)->bbox.yMin); // bbox.yMax-bbox.yMin
    }
    else
    {
      height = (float)(((*ftFace)->bbox.yMax - (*ftFace)->bbox.yMin) >> 16); // bbox.yMax-bbox.yMin
    }

    height =  height * ( (float)ftSize->metrics.y_ppem / (float)(*ftFace)->units_per_EM);
    return static_cast<int>(height);
  }
  else
  {
    return ftSize->metrics.height >> 6;
  }
}


int FTSize::Width() const
{
  if( FT_IS_SCALABLE((*ftFace)))
  {
    float width;
    if( FT_IS_SFNT((*ftFace))) // Don't think this is correct
    {
      width = (float)((*ftFace)->bbox.xMax - (*ftFace)->bbox.xMin); // bbox.xMax-bbox.xMin
    }
    else
    {
      width = (float)(((*ftFace)->bbox.xMax - (*ftFace)->bbox.xMin) >> 16); // bbox.xMax-bbox.xMin
    }
    
    width = width * ( (float)ftSize->metrics.x_ppem / (float)(*ftFace)->units_per_EM);
    return static_cast<int>(width);
  }
  else
  {
    return ftSize->metrics.max_advance >> 6;
  }
}


int FTSize::Underline() const
{
  return 0;
}

#ifdef FTGL_USE_NAMESPACE
} // namespace ftgl
#endif
