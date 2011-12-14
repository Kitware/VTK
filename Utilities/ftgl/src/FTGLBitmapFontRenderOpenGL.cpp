#include  "FTGLBitmapFont.h"
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

void FTGLBitmapFont::RenderFunctionName(const char* string,
                                        const FTGLRenderContext *context)
{  
  glPushClientAttrib( GL_CLIENT_PIXEL_STORE_BIT);
  glPushAttrib( GL_ENABLE_BIT);
    
  glPixelStorei( GL_UNPACK_LSB_FIRST, GL_FALSE);
  glPixelStorei( GL_UNPACK_ROW_LENGTH, 0);
  glPixelStorei( GL_UNPACK_ALIGNMENT, 1);

  glDisable( GL_BLEND);

  FTFont::render(string, context);

  glPopAttrib();
  glPopClientAttrib();
}

void FTGLBitmapFont::RenderFunctionName(const wchar_t* string,
                                        const FTGLRenderContext *context)
{  
  glPushClientAttrib( GL_CLIENT_PIXEL_STORE_BIT);
  glPushAttrib( GL_ENABLE_BIT);
    
  glPixelStorei( GL_UNPACK_LSB_FIRST, GL_FALSE);
  glPixelStorei( GL_UNPACK_ROW_LENGTH, 0);
  glPixelStorei( GL_UNPACK_ALIGNMENT, 1);

  glDisable( GL_BLEND);

  FTFont::render(string, context);

  glPopAttrib();
  glPopClientAttrib();
}

#ifdef FTGL_USE_NAMESPACE
} // namespace ftgl
#endif
