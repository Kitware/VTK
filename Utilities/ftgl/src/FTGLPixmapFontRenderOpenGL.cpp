#include  "FTGLPixmapFont.h"
#include  "FTPixmapGlyph.h"
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

void FTGLPixmapFont::RenderFunctionName(const char* string,
                                        const FTGLRenderContext *context)
{  
  glPushClientAttrib( GL_CLIENT_PIXEL_STORE_BIT);
  glPushAttrib( GL_ENABLE_BIT | GL_PIXEL_MODE_BIT);

  glPixelStorei( GL_UNPACK_ROW_LENGTH, 0);

  glEnable(GL_BLEND);
  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable( GL_TEXTURE_2D);

  FTFont::render(string, context);

  glPopAttrib();
  glPopClientAttrib();
}

void FTGLPixmapFont::RenderFunctionName(const wchar_t* string,
                                        const FTGLRenderContext *context)
{  
  glPushClientAttrib( GL_CLIENT_PIXEL_STORE_BIT);
  glPushAttrib( GL_ENABLE_BIT | GL_PIXEL_MODE_BIT);

  glPixelStorei( GL_UNPACK_ROW_LENGTH, 0);

  glEnable(GL_BLEND);
  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable( GL_TEXTURE_2D);

  FTFont::render(string, context);

  glPopAttrib();
  glPopClientAttrib();
}

#ifdef FTGL_USE_NAMESPACE
} // namespace ftgl
#endif
