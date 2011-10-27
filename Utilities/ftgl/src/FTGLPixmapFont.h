#ifndef    __FTGLPixmapFont__
#define    __FTGLPixmapFont__


#include "FTGL.h"

#include "FTFont.h"

#ifdef FTGL_USE_NAMESPACE
namespace ftgl
{
#endif

class FTGlyph;


/**
 * FTGLPixmapFont is a specialisation of the FTFont class for handling
 * Pixmap (Grey Scale) fonts
 *
 * @see    FTFont
 */
class FTGL_EXPORT FTGLPixmapFont : public FTFont
{
  public:
    /**
     * Default Constructor
     */
    FTGLPixmapFont();
    
    /**
     * Destructor
     */
    ~FTGLPixmapFont();
    
    /**
     * Renders a string of characters
     * 
     * @param string  'C' style string to be output.   
     */
    void render(const char* string, 
                const FTGLRenderContext *context = 0);
    
    /**
     * Renders a string of characters
     * 
     * @param string  wchar_t string to be output.   
     */
    void render(const wchar_t* string, 
                const FTGLRenderContext *context = 0);


  private:
    /**
     * Construct a FTPixmapGlyph.
     *
     * @param g  The glyph index NOT the char code.
     * @return  An FTPixmapGlyph or <code>null</code> on failure.
     */
    virtual FTGlyph* MakeGlyph( unsigned int g);

    void RenderOpenGL(const char* string,
                      const FTGLRenderContext *context = 0);
    void RenderOpenGL(const wchar_t* string, 
                      const FTGLRenderContext *context = 0);

#ifdef FTGL_SUPPORT_MANGLE_MESA
    void RenderMesa(const char* string,
                    const FTGLRenderContext *context = 0);
    void RenderMesa(const wchar_t* string, 
                    const FTGLRenderContext *context = 0);
#endif
    
};

#ifdef FTGL_USE_NAMESPACE
} // namespace ftgl
#endif

#endif  //  __FTGLPixmapFont__

