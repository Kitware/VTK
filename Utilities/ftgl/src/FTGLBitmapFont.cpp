#include  "FTGLBitmapFont.h"
#include  "FTBitmapGlyph.h"
#ifdef FTGL_DEBUG
  #include "mmgr.h"
#endif


FTGLBitmapFont::FTGLBitmapFont()
{}


FTGLBitmapFont::~FTGLBitmapFont()
{}


FTGlyph* FTGLBitmapFont::MakeGlyph( unsigned int g)
{
  FT_Glyph* ftGlyph = face.Glyph( g, FT_LOAD_DEFAULT);

  if( ftGlyph)
  {
    FTBitmapGlyph* tempGlyph = new FTBitmapGlyph( *ftGlyph);
//                FT_Done_Glyph( *ftGlyph );

    return tempGlyph;
  }

  err = face.Error();
  return NULL;
}


void FTGLBitmapFont::render(const char* string,
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


void FTGLBitmapFont::render(const wchar_t* string,
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
