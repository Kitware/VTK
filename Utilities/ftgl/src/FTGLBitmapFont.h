#ifndef    __FTGLBitmapFont__
#define    __FTGLBitmapFont__

#include "FTGL.h"

#include "FTFont.h"

class FTGlyph;

/**
 * FTGLBitmapFont is a specialisation of the FTFont class for handling
 * Bitmap fonts
 *
 * @see    FTFont
 */
class FTGL_EXPORT FTGLBitmapFont : public FTFont
{
  public:
    /**
     * Constructor
     */
    FTGLBitmapFont();

    /**
     * Destructor
     */
    ~FTGLBitmapFont();
    
    /**
     * Renders a string of characters
     * 
     * @param string  'C' style string to be output.   
     */
    void render( const char* string,
                 const FTGLRenderContext *context = 0);

    /**
     * Renders a string of characters
     * 
     * @param string  'C' style wide string to be output.   
     */
    void render( const wchar_t* string,
                 const FTGLRenderContext *context = 0);

    // attributes
    
  private:
    /**
     * Construct a FTBitmapGlyph.
     *
     * @param g  The glyph index NOT the char code.
     * @return  An FTBitmapGlyph or <code>null</code> on failure.
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

#endif  //  __FTGLBitmapFont__
