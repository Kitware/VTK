#ifndef    __FTGLExtrdFont__
#define    __FTGLExtrdFont__

#include "FTGL.h"

#include  "FTFont.h"

class FTGlyph;


/**
 * FTGLPolygonFont is a specialisation of the FTFont class for handling
 * tesselated Polygon Mesh fonts
 *
 * @see    FTFont
 */
class FTGL_EXPORT FTGLExtrdFont : public FTFont
{
  public:
    /**
     * Default Constructor
     */
    FTGLExtrdFont();
    
    /**
     * Destructor
     */
    ~FTGLExtrdFont();
    
    void Depth( float d) { depth = d;}
    
  private:
    /**
     * Construct a FTPolyGlyph.
     *
     * @param g  The glyph index NOT the char code.
     * @return  An FTPolyGlyph or <code>null</code> on failure.
     */
    virtual FTGlyph* MakeGlyph( unsigned int g);
    
    float depth;
    
};


#endif  //  __FTGLExtrdFont__

