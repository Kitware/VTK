#ifndef    __FTExtrdGlyph__
#define    __FTExtrdGlyph__

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include "FTGL.h"
#include "FTGLgl.h"
#include "FTGlyph.h"

class FTVectoriser;

/**
 * FTExtrdGlyph is a specialisation of FTGlyph for creating tessellated
 * extruded polygon glyphs.
 * 
 * @see FTGlyphContainer
 * @see FTVectoriser
 *
 */
class FTGL_EXPORT FTExtrdGlyph : public FTGlyph
{
  public:
    /**
     * Constructor
     *
     * @param glyph  The Freetype glyph to be processed
     */
    FTExtrdGlyph( FT_Glyph glyph, float depth);

    /**
     * Destructor
     */
    virtual ~FTExtrdGlyph();

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
     * Calculate the winding direction of a contour.
     *
     * This uses the signed area of the contour. It is required because
     * freetype doesn't do this despite the docs saying it does:(
     */
    bool Winding( int numPoints, FTGL_DOUBLE *points);
    
    /**
     * An object that helps convert freetype outlines into point
     * data
     */
    FTVectoriser* vectoriser;
    
    /**
     * OpenGL display list
     */
    GLuint glList;
    
    /**
     * Distance to extrude the glyph
     */
    float depth;
  
};


#endif  //  __FTExtrdGlyph__

