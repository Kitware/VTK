#include  "FTGLPixmapFont.h"
#include  "FTPixmapGlyph.h"
#ifdef FTGL_DEBUG
  #include "mmgr.h"
#endif

#ifdef FTGL_USE_NAMESPACE
namespace ftgl
{
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


void FTGLPixmapFont::render(const char* string,
                            const FTGLRenderContext *context)

{  
#ifdef FTGL_SUPPORT_MANGLE_MESA
  if (context && context->UseMangleMesa)
    {
    this->RenderMesa(string, context);
    }
  else
#endif
    {
    this->RenderOpenGL(string, context);
    }
}


void FTGLPixmapFont::render(const wchar_t* string,
                            const FTGLRenderContext *context)

{  
#ifdef FTGL_SUPPORT_MANGLE_MESA
  if (context && context->UseMangleMesa)
    {
    this->RenderMesa(string, context);
    }
  else
#endif
    {
    this->RenderOpenGL(string, context);
    }
}

#ifdef FTGL_USE_NAMESPACE
} // namespace ftgl
#endif
