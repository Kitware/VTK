#include  "FTFace.h"
#include  "FTFont.h"
#include  "FTGlyphContainer.h"
#include  "FTGlyph.h" // for FTBbox
#ifdef FTGL_DEBUG
  #include "mmgr.h"
#endif

#ifdef FTGL_USE_NAMESPACE
namespace ftgl
{
#endif

FTFont::FTFont()
:  numFaces(0),
  glyphList(0),
  numGlyphs(0),
  preCache(true),
  err(0)
{
  pen.x = 0;
  pen.y = 0;
}


FTFont::~FTFont()
{
  Close();
}


bool FTFont::Open( const char* fontname, bool p)
{
  preCache = p;
  
  if( face.Open( fontname))
  {
    FT_Face* ftFace = face.Face();    
    numGlyphs = (*ftFace)->num_glyphs;
    
    return true;
  }
  else
  {
    err = face.Error();
    return false;
  }
}


bool FTFont::Open( const unsigned char *pBufferBytes, size_t bufferSizeInBytes, bool p )
{
  preCache = p;
  
  if( face.Open( pBufferBytes, bufferSizeInBytes ))
  {
    FT_Face* ftFace = face.Face();    
    numGlyphs = (*ftFace)->num_glyphs;
    
    return true;
  }
  else
  {
    err = face.Error();
    return false;
  }
}

bool FTFont::Attach( const char* filename) 
{ 
  return face.Attach( filename); 
} 

void FTFont::Close()
{
  delete glyphList;
}


bool FTFont::FaceSize( const unsigned int size, const unsigned int res )
{
  charSize = face.Size( size, res);

  if( glyphList)
    delete glyphList;
  
  glyphList = new FTGlyphContainer( &face, numGlyphs, preCache);
  
  if( preCache)
    {
    return MakeGlyphList() ? true : false;
    }

  return true;
}


bool FTFont::MakeGlyphList()
{
  if( preCache)
    {
    for( unsigned int c = 0; c < numGlyphs; ++c)
      {
      glyphList->Add( MakeGlyph( c), c);
      }
    }
  else
    {
    for( unsigned int c = 0; c < numGlyphs; ++c)
      {
      glyphList->Add( NULL, c);
      }
    }
  
  return !err; // FIXME what err?
}


bool FTFont::CharMap( FT_Encoding encoding)
{
  err = face.CharMap( encoding);
  return !err;
}


int  FTFont::Ascender() const
{
  return charSize.Ascender();
}


int  FTFont::Descender() const
{
  return charSize.Descender();
}

void FTFont::BBox( const char* string,
                   float& llx, float& lly, float& llz, float& urx, float& ury, float& urz)
{
  const unsigned char* c = (unsigned char*)string;
  llx = lly = llz = urx = ury = urz = 0;
  FTBBox bbox;
 
  while( *c)
  {
    if( !glyphList->Glyph( static_cast<unsigned int>(*c)))
    {
      unsigned int g = face.CharIndex( static_cast<unsigned int>(*c));
      glyphList->Add( MakeGlyph( g), g);
    }
    
    bbox = glyphList->BBox( *c);

    // Lower extent
    lly = lly < bbox.y1 ? lly: bbox.y1;
    // Upper extent
    ury = ury > bbox.y2 ? ury: bbox.y2;
    // Depth
    urz = urz < bbox.z2 ? urz: bbox.z2;

    // Width
    urx += glyphList->Advance( *c, *(c + 1));
    ++c;
  }
  
  //Final adjustments
  llx = glyphList->BBox( *string).x1;
  urx -= glyphList->Advance( *(c - 1), 0);
  urx += bbox.x2;
}

void FTFont::BBox( const wchar_t* string,
                    float& llx, float& lly, float& llz, float& urx, float& ury, float& urz)
{
  const wchar_t* c = string;
  llx = lly = llz = urx = ury = urz = 0;
  FTBBox bbox;
 
  while( *c)
  {
    if( !glyphList->Glyph( static_cast<unsigned int>(*c)))
    {
      unsigned int g = face.CharIndex( static_cast<unsigned int>(*c));
      glyphList->Add( MakeGlyph( g), g);
    }
    
    bbox = glyphList->BBox( *c);
    
    // Lower extent
    lly = lly < bbox.y1 ? lly: bbox.y1;
    // Upper extent
    ury = ury > bbox.y2 ? ury: bbox.y2;
    // Depth
    urz = urz < bbox.z2 ? urz: bbox.z2;

    // Width
    urx += glyphList->Advance( *c, *(c + 1));
    ++c;
  }
  
  //Final adjustments
  llx = glyphList->BBox( *string).x1;
  urx -= glyphList->Advance( *(c - 1), 0);
  urx += bbox.x2;
}


float FTFont::Advance( const wchar_t* string)
{
  const wchar_t* c = string;
  float width = 0;

  while( *c)
  {
    width += doAdvance( *c, *(c + 1));
    ++c;
  }

  return width;
}


float FTFont::Advance( const char* string)
{
  const unsigned char* c = (unsigned char*)string;
  float width = 0;

  while( *c)
  {
    width += doAdvance( *c, *(c + 1));
    ++c;
  }

  return width;
}


float FTFont::doAdvance( const unsigned int chr, const unsigned int nextChr)
{
  if( !glyphList->Glyph( chr))
  {
    unsigned int g = face.CharIndex( chr);
    glyphList->Add( MakeGlyph( g), g);
  }

  return glyphList->Advance( chr, nextChr);
}


void FTFont::render( const char* string, 
                     const FTGLRenderContext *context)
{
  const unsigned char* c = (unsigned char*)string;
  pen.x = 0; pen.y = 0;

  while( *c)
  {
    doRender( *c, *(c + 1), context);
    ++c;
  }
}


void FTFont::render( const wchar_t* string, 
                     const FTGLRenderContext *context)
{
  const wchar_t* c = string;
  pen.x = 0; pen.y = 0;

  while( *c)
  {
    doRender( *c, *(c + 1), context);
    ++c;
  }
}


void FTFont::doRender( const unsigned int chr, 
                       const unsigned int nextChr, 
                       const FTGLRenderContext *context)
{
  if( !glyphList->Glyph( chr))
  {
    unsigned int g = face.CharIndex( chr);
    glyphList->Add( MakeGlyph( g), g);
  }

  FT_Vector kernAdvance = glyphList->render( chr, nextChr, pen, context);
  
  pen.x += kernAdvance.x;
  pen.y += kernAdvance.y;
}

#ifdef FTGL_USE_NAMESPACE
} // namespace ftgl
#endif
