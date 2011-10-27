#ifndef    __FTGLOutlineFont__
#define    __FTGLOutlineFont__

#include "FTGL.h"

#include "FTFont.h"

#ifdef FTGL_USE_NAMESPACE
namespace ftgl
{
#endif

class FTGlyph;


/**
 * FTGLOutlineFont is a specialisation of the FTFont class for handling
 * Vector Outline fonts
 *
 * @see    FTFont
 */
class FTGL_EXPORT FTGLOutlineFont : public FTFont
{
  public:
    /**
     * Default Constructor
     */
    FTGLOutlineFont();
    
    /**
     * Destructor
     */
    ~FTGLOutlineFont();
    
    /**
     * Renders a string of characters
     * 
     * @param string  'C' style string to be output.   
     */
    void render( const char* string);
    
    /**
     * Renders a string of characters
     * 
     * @param string  wchar_t string to be output.   
     */
    void render( const wchar_t* string);

    // attributes
    
  private:
    /**
     * Construct a FTOutlineGlyph.
     *
     * @param g  The glyph index NOT the char code.
     * @return  An FTOutlineGlyph or <code>null</code> on failure.
     */
    virtual FTGlyph* MakeGlyph( unsigned int g);
    
};

#ifdef FTGL_USE_NAMESPACE
} // namespace ftgl
#endif

#endif // __FTGLOutlineFont__
