#ifndef    __FTPixmapGlyph__
#define    __FTPixmapGlyph__


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
 * FTPixmapGlyph is a specialisation of FTGlyph for creating pixmaps.
 * 
 * @see FTGlyphContainer
 *
 */
class  FTGL_EXPORT FTPixmapGlyph : public FTGlyph
{
  public:
    /**
     * Constructor
     *
     * @param glyph  The Freetype glyph to be processed
     */
    FTPixmapGlyph( FT_Glyph glyph);

    /**
     * Destructor
     */
    virtual ~FTPixmapGlyph();

    /**
     * Renders this glyph at the current pen position.
     *
     * @param pen  The current pen position.
     * @return    The advance distance for this glyph.
     */
    virtual float Render( const FT_Vector& pen,
                          const FTGLRenderContext *context = 0);
    
    virtual void ConvertGlyph(const FTGLRenderContext *context = 0);

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
     * The number of greys or bit depth of the image
     */
    int numGreys;

    /**
     * Pointer to the 'image' data
     */
    unsigned char* data;
    
    void RenderOpenGL(const FT_Vector& pen,
                      const FTGLRenderContext *context = 0);
    void GetCurrentColorOpenGL(float colour[4],
                               const FTGLRenderContext *context = 0);

#ifdef FTGL_SUPPORT_MANGLE_MESA
    void RenderMesa(const FT_Vector& pen,
                    const FTGLRenderContext *context = 0);
    void GetCurrentColorMesa(float colour[4],
                             const FTGLRenderContext *context = 0);
#endif

};

#ifdef FTGL_USE_NAMESPACE
} // namespace ftgl
#endif

#endif  //  __FTPixmapGlyph__
