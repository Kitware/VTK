#ifndef    __FTGlyph__
#define    __FTGlyph__

#include <vtk_freetype.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include "FTGL.h"


/**
 * FTBBox
 *
 *
 */
class FTGL_EXPORT FTBBox
{
  public:
    FTBBox()
    :  x1(0),
      y1(0),
      z1(0),
      x2(0),
      y2(0),
      z2(0)
    {}
    
    FTBBox( FT_Glyph glyph)
    {
      FT_BBox bbox;
      FT_Glyph_Get_CBox( glyph, ft_glyph_bbox_subpixels, &bbox );
      
      x1 = (float)(bbox.xMin >> 6);
      y1 = (float)(bbox.yMin >> 6);
      z1 = 0;
      x2 = (float)(bbox.xMax >> 6);
      y2 = (float)(bbox.yMax >> 6);
      z2 = 0;  
    }    
    
    FTBBox( int a, int b, int c, int d, int e, int f)
    :  x1((float)a),
      y1((float)b),
      z1((float)c),
      x2((float)d),
      y2((float)e),
      z2((float)f)
    {}

    ~FTBBox()
    {}
  
    // Make these ftPoints
    float x1, y1, z1, x2, y2, z2;

  protected:
  
  
  private:
};


/**
 * FTGlyph is the base class for FTGL glyphs.
 *
 * It provides the interface between Freetype glyphs and their openGL
 * renderable counterparts. This is an abstract class and derived classes
 * must implement the <code>render</code> function. 
 * 
 * @see FTGlyphContainer
 *
 */
class FTGL_EXPORT FTGlyph
{
  public:
    /**
     * Constructor
     */
    FTGlyph();

    /**
     * Destructor
     */
    virtual ~FTGlyph();

    /**
     * Renders this glyph at the current pen position.
     *
     * @param pen  The current pen position.
     * @return    The advance distance for this glyph.
     */
    virtual float Render( const FT_Vector& pen,
                          const FTGLRenderContext *context = 0) = 0;
    
    /**
     * Return the advance width for this glyph.
     *
     * @return  advance width.
     */
    float Advance() const { return advance;}
    
    /**
     * Return the bounding box for this glyph.
     *
     * @return  bounding box.
     */
    FTBBox BBox() const { return bBox;}
    
    /**
     * Queries for errors.
     *
     * @return  The current error code.
     */
    FT_Error Error() const { return err;}
    
  protected:
    /**
     * The advance distance for this glyph
     */
    float advance;

    /**
     * Vector from the pen position to the topleft corner of the glyph
     */
    FT_Vector pos;

    
    /**
     * A freetype bounding box
     */
    FTBBox bBox;
    
    /**
     * Current error code. Zero means no error.
     */
    FT_Error err;

  int glyphHasBeenConverted;
  FT_Glyph glyph;

  private:

};


#endif  //  __FTGlyph__

