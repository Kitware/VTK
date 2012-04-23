#ifndef    __FTGLTextureFont__
#define    __FTGLTextureFont__

#include "FTGL.h"
#include "FTGLgl.h"
#include "FTFont.h"

#ifdef FTGL_USE_NAMESPACE
namespace ftgl
{
#endif

class FTTextureGlyph;


/**
 * FTGLTextureFont is a specialisation of the FTFont class for handling
 * Texture mapped fonts
 *
 * @see    FTFont
 */
class  FTGL_EXPORT FTGLTextureFont : public FTFont
{
  public:
    /**
     * Default Constructor
     */
    FTGLTextureFont();
    
    /**
     * Destructor
     */
    virtual ~FTGLTextureFont();
    
    /**
     * Get the total width of the texture that holds this font
     */
    GLsizei TextureWidth() const { return textureWidth;}
    
    /**
     * Get the total height of the texture that holds this font
     */
    GLsizei TextureHeight() const { return textureHeight;}
    
    /**
     * Renders a string of characters
     * 
     * @param string  'C' style string to be output.   
     */
    virtual void render( const char* string);
    
    /**
     * Renders a string of characters
     * 
     * @param string  wchar_t string to be output.   
     */
    virtual void render( const wchar_t* string);

    
  private:
    virtual FTGlyph* MakeGlyph( unsigned int g);
        
    /**
     * Constructs the internal glyph cache.
     *
     * This a list of glyphs processed for openGL rendering NOT
     * freetype glyphs
     */
    virtual bool MakeGlyphList();

    /**
     * Get the size of a block of memory required to layout the glyphs
     *
     * Calculates a width and height based on the glyph sizes and the
     * number of glyphs. It over estimates.
     */
    void GetSize();

    /**
     * Creates a 'blank' OpenGL texture object.
     *
     * The format is GL_ALPHA and the params are
     * GL_TEXTURE_WRAP_S = GL_CLAMP
     * GL_TEXTURE_WRAP_T = GL_CLAMP
     * GL_TEXTURE_MAG_FILTER = GL_LINEAR
     * GL_TEXTURE_MIN_FILTER = GL_LINEAR
     * Note that mipmapping is NOT used
     * @param id    The index into an array of glTextureIDs.
     * @param width    The width of the texture in bytes
     * @param height  The number of rows of bytes.
     * @param data    A pointer to the texture data
     */
    int CreateTexture();
    
    /**
     * The maximum texture dimension on this OpenGL implemetation
     */
    GLsizei maxTextSize;
    
    /**
     * The minimum texture width required to hold the glyphs
     */
    GLsizei textureWidth;
    
    /**
     * The minimum texture height required to hold the glyphs
     */
    GLsizei textureHeight;
    
    /**
     *An array of texture ids
     */
    unsigned long glTextureID[1024];
    
    /**
     * The number of textures required to hold the glyphs
     */
    int numTextures;

    /**
     * The memeory where the textures are built before beiing transferred 
     * to OpenGL
     */
    unsigned char* textMem;
    
    /**
     * The max height for glyphs in the current font
     */
    int glyphHeight;

    /**
     * The max width for glyphs in the current font
     */
    int glyphWidth;

    /**
     * A value to be added to the height and width to ensure that
     * glyphs don't overlap in the texture
     */
    int padding;
    
    int remGlyphs;
    int xOffset;
    int yOffset;


};

#ifdef FTGL_USE_NAMESPACE
} // namespace ftgl
#endif

#endif // __FTGLTextureFont__


