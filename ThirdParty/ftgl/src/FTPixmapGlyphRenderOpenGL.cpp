#include  "FTPixmapGlyph.h"
#include  "FTGLgl.h"
#ifdef FTGL_DEBUG
  #include "mmgr.h"
#endif

#ifndef GetCurrentColorFunctionName
#define GetCurrentColorFunctionName GetCurrentColorOpenGL
#endif

#ifdef FTGL_USE_NAMESPACE
namespace ftgl
{
#endif

void FTPixmapGlyph::GetCurrentColorFunctionName(float colour[4],
                                                const FTGLRenderContext *)
{
  glGetFloatv( GL_CURRENT_COLOR, colour);
}

#ifndef RenderFunctionName
#define RenderFunctionName RenderOpenGL
#endif

void FTPixmapGlyph::RenderFunctionName(const FT_Vector& pen,
                                       const FTGLRenderContext *)
{
  // Move the glyph origin
  glBitmap( 0, 0, 0.0, 0.0, (float)(pen.x + pos.x), (float)(pen.y - pos.y), (const GLubyte *)0);
  
  glDrawPixels( destWidth, destHeight, GL_RGBA, GL_UNSIGNED_BYTE, (const GLvoid*)data);

  // Restore the glyph origin
  glBitmap( 0, 0, 0.0, 0.0, (float)(-pen.x - pos.x), (float)(-pen.y + pos.y), (const GLubyte *)0);
}

#ifdef FTGL_USE_NAMESPACE
} // namespace ftgl
#endif
