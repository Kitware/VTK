#ifndef    __FTGLPolygonFont__
#define    __FTGLPolygonFont__

#include "FTGL.h"

#include  "FTFont.h"

#ifdef FTGL_USE_NAMESPACE
namespace ftgl
{
#endif

class FTGlyph;


/**
 * FTGLPolygonFont is a specialisation of the FTFont class for handling
 * tesselated Polygon Mesh fonts
 *
 * @see    FTFont
 */
class FTGL_EXPORT FTGLPolygonFont : public FTFont
{
  public:
    /**
     * Default Constructor
     */
    FTGLPolygonFont();
    
    /**
     * Destructor
     */
    ~FTGLPolygonFont();
    
  private:
    /**
     * Construct a FTPolyGlyph.
     *
     * @param g  The glyph index NOT the char code.
     * @return  An FTPolyGlyph or <code>null</code> on failure.
     */
    virtual FTGlyph* MakeGlyph( unsigned int g);
    
};

#ifdef FTGL_USE_NAMESPACE
} // namespace ftgl
#endif

#endif  //  __FTGLPolygonFont__

