#ifndef    __FTGlyphContainer__
#define    __FTGlyphContainer__

#include <vtk_freetype.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include "FTGL.h"
#include "FTGlyph.h" // for FTBBox

#include "FTFace.h"

#ifdef FTGL_DO_NOT_USE_STL
#include <NoSTL/FTGlyphVector.h>
#else
#include <vector>
#ifdef USE_STD_NAMESPACE
using namespace std;
#endif
#endif

#ifdef FTGL_USE_NAMESPACE
namespace ftgl
{
#endif

/**
 * FTGlyphContainer holds the post processed FTGlyph objects.
 *
 * @see FTGlyph
 */
class FTGL_EXPORT FTGlyphContainer
{
  public:
    /**
     * Constructor
     *
     * @param face    The Freetype face
     * @param numGlyphs  the number of glyphs in this face
     * @param p      A flag to indicate preprocessing of glyphs.
     *          Not used.
     */
    FTGlyphContainer( FTFace* face, unsigned int numGlyphs, bool p = false);

    /**
     * Destructor
     */
    virtual ~FTGlyphContainer();

    /**
     * Adds a glyph to this glyph list.
     *
     * @param glyph    
     * @return      <code>true</code>
     */
    bool Add( FTGlyph* tempGlyph, unsigned int g)
    {
      glyphs[g] = tempGlyph;
      return true;
    }

    /**
     * Get a glyph from the glyph list
     *
     * @param c  The char code of the glyph NOT the glyph index    
     * @return  An FTGlyph or <code>null</code> is it hasn't been
     * loaded.
     */
    FTGlyph* Glyph( const unsigned int c) const
    {
      return glyphs[face->CharIndex( c)];
    }
    
    FTBBox BBox( const unsigned int index) const
    {
      return glyphs[face->CharIndex( index)]->BBox();
    }
    
    /**
    * Returns the kerned advance width for a glyph.
    *
    * @param index  glyph index of the character
    * @param next  the next glyph in a string
    * @return     advance width
    */
    float Advance( unsigned int index, unsigned int next);
    
    /**
     * renders a character
     * @param index  the glyph to be rendered
     * @param next  the next glyph in the string. Used for kerning.
     * @param pen  the position to render the glyph
     * @return     The distance to advance the pen position after rendering
     */
    FT_Vector& render( unsigned int index, 
                       unsigned int next, 
                       FT_Vector pen, 
                       const FTGLRenderContext *context = 0);
    
    /**
     * Queries the Font for errors.
     *
     * @return  The current error code.
     */
    virtual FT_Error Error() const { return err;}

  private:
    /**
     * A flag to indicate preprocessing of glyphs. Not used.
     */
    bool preCache;

    /**
     * How meny glyphs are stored in this container
     */
    int numGlyphs;

    /**
     * The current Freetype face
     */
    FTFace* face;

    /**
     * The kerning vector for the current pair of glyphs
     */
    FT_Vector kernAdvance;

    /**
     * The advance for the glyph being rendered
     */
    float advance;

    /**
     * A structure to hold the glyphs
     */
#ifdef FTGL_DO_NOT_USE_STL
    typedef FTGlyphVector GlyphVector;
#else
    typedef vector<FTGlyph*> GlyphVector;
#endif
    GlyphVector glyphs;

    /**
     * Current error code. Zero means no error.
     */
    FT_Error err;
    
};

#ifdef FTGL_USE_NAMESPACE
} // namespace ftgl
#endif

#endif  //  __FTGlyphContainer__
