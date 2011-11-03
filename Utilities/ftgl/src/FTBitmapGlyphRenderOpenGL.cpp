#include  "FTBitmapGlyph.h"
#include  "FTGLgl.h"
#ifdef FTGL_DEBUG
  #include "mmgr.h"
#endif

#ifndef RenderFunctionName
#define RenderFunctionName RenderOpenGL
#endif

#ifdef FTGL_USE_NAMESPACE
namespace ftgl
{
#endif

void FTBitmapGlyph::RenderFunctionName(const FT_Vector& pen,
                                       const FTGLRenderContext *)
{
  // Move the glyph origin
  glBitmap( 0, 0, 0.0, 0.0, (float)(pen.x + pos.x), (float)(pen.y - pos.y), (const GLubyte *)0 );

  glBitmap( destWidth, destHeight, 0.0f, 0.0, 0.0, 0.0, (const GLubyte *)data);

  // Restore the glyph origin
  glBitmap( 0, 0, 0.0, 0.0, (float)(-pen.x - pos.x), (float)(-pen.y + pos.y), (const GLubyte *)0 );
}

#ifdef FTGL_USE_NAMESPACE
} // namespace ftgl
#endif
