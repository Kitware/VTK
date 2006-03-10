#ifndef    __FTPolyGlyph__
#define    __FTPolyGlyph__


#include <vtk_freetype.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include "FTGL.h"
#include "FTGLgl.h"
#include "FTGlyph.h"

class FTVectoriser;

/**
 * FTPolyGlyph is a specialisation of FTGlyph for creating tessellated
 * polygon glyphs.
 * 
 * @see FTGlyphContainer
 * @see FTVectoriser
 *
 */
class FTGL_EXPORT FTPolyGlyph : public FTGlyph
{
  public:
    /**
     * Constructor
     *
     * @param glyph  The Freetype glyph to be processed
     */
    FTPolyGlyph( FT_Glyph glyph);

    /**
     * Destructor
     */
    virtual ~FTPolyGlyph();

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
     * Convert the point data into a mesh.
     *
     * Uses GLUtesselator to create a mesh
     */
    void Tesselate();
    
    /**
     * An object that helps convert freetype outlines into point
     * data
     */
    FTVectoriser* vectoriser;

    /**
     * The total number of points in the Freetype outline
     */
    int numPoints;

    /**
     * Pointer to the point data
     */
    FTGL_DOUBLE* data;
    
    /**
     * OpenGL display list
     */
    GLuint glList;
  
};


#endif  //  __FTPolyGlyph__

