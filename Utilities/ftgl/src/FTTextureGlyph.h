#ifndef    __FTTextureGlyph__
#define    __FTTextureGlyph__


#include <vtk_freetype.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include "FTGL.h"
#include "FTGLgl.h"
#include "FTGlyph.h"

#ifdef FTGL_USE_NAMESPACE
namespace ftgl
{
#endif

/**
 * FTTextureGlyph is a specialisation of FTGlyph for creating texture
 * glyphs.
 * 
 * @see FTGlyphContainer
 *
 */
class FTGL_EXPORT FTTextureGlyph : public FTGlyph
{
  public:
    /**
     * Constructor
     *
     * @param glyph    The Freetype glyph to be processed
     * @param id    The id the texture that this glyph will be
     *          drawn in
     * @param xOffset  The x offset into the parent texture to draw
     *          this glyph
     * @param yOffset  The y offset into the parent texture to draw
     *          this glyph
     * @param width    The width of the parent texture
     * @param height  The height (number of rows) of the parent texture
     */
    FTTextureGlyph( FT_Glyph glyph, int id, int xOffset, int yOffset, GLsizei width, GLsizei height);

    /**
     * Destructor
     */
    virtual ~FTTextureGlyph();

    /**
     * Renders this glyph at the current pen position.
     *
     * @param pen  The current pen position.
     * @return    The advance distance for this glyph.
     */
    virtual float Render( const FT_Vector& pen,
                          const FTGLRenderContext *context = 0);
    
  private:
    /**
     * Pointer to the 'image' data
     */
    unsigned char* data;
     
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
     * A structure to hold the uv co-ords.
     */
    struct FTPoint
    {
      float x;
      float y;
    };

    /**
     * The texture co-ords of this glyph within the texture.
     */
    FTPoint uv[2];
    
    /**
     * The texture index that this glyph is contained in.
     */
    int glTextureID;

    /**
     * The texture index of the currently active texture
     *
     * We call glGetIntegerv( GL_TEXTURE_2D_BINDING, activeTextureID);
     * to get the currently active texture to try to reduce the number
     * of texture bind operations
     */
    GLint activeTextureID;
    
};

#ifdef FTGL_USE_NAMESPACE
} // namespace ftgl
#endif

#endif  //  __FTTextureGlyph__
