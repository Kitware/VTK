#ifndef    __FTBitmapGlyph__
#define    __FTBitmapGlyph__


#include <vtk_freetype.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include "FTGL.h"
#include  "FTGlyph.h"

#ifdef FTGL_USE_NAMESPACE
namespace ftgl
{
#endif

/**
 * FTBitmapGlyph is a specialisation of FTGlyph for creating bitmaps.
 *
 * It provides the interface between Freetype glyphs and their openGL
 * renderable counterparts. This is an abstract class and derived classes
 * must implement the <code>render</code> function. 
 * 
 * @see FTGlyphContainer
 *
 */
class FTGL_EXPORT FTBitmapGlyph : public FTGlyph
{
  public:
    /**
     * Constructor
     *
     * @param glyph  The Freetype glyph to be processed
     */
    FTBitmapGlyph( FT_Glyph glyph);

    /**
     * Destructor
     */
    virtual ~FTBitmapGlyph();

    /**
     * Renders this glyph at the current pen position.
     *
     * @param pen  The current pen position.
     * @return    The advance distance for this glyph.
     */
    virtual float Render( const FT_Vector& pen,
                          const FTGLRenderContext *context = 0);
    
    virtual void ConvertGlyph();

  private:
    /**
     * The width of the glyph 'image'
     */
    int destWidth;

    /**
     * The height of the glyph 'image'
     */
    int destHeight;
    
    /**
     * Pointer to the 'image' data
     */
    unsigned char* data;
    
    void RenderOpenGL(const FT_Vector& pen,
                      const FTGLRenderContext *context = 0);

#ifdef FTGL_SUPPORT_MANGLE_MESA
    void RenderMesa(const FT_Vector& pen,
                    const FTGLRenderContext *context = 0);
#endif

};

#ifdef FTGL_USE_NAMESPACE
} // namespace ftgl
#endif

#endif  //  __FTBitmapGlyph__

