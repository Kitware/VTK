#include  "FTPolyGlyph.h"
#include  "FTVectoriser.h"
#ifdef FTGL_DEBUG
  #include "mmgr.h"
#endif

#ifdef FTGL_USE_NAMESPACE
namespace ftgl
{
#endif

FTPolyGlyph::FTPolyGlyph( FT_Glyph glyph)
:  FTGlyph(),
  vectoriser(0),
  numPoints(0),
  data(0),
  glList(0)
{
  if( ft_glyph_format_outline != glyph->format)
  {
    return;
  }

  vectoriser = new FTVectoriser( glyph);
  
  vectoriser->Process();

  vectoriser->MakeMesh(1.0);
  numPoints = vectoriser->MeshPoints();

  bBox = FTBBox( glyph);
  advance = (float)(glyph->advance.x >> 16);

  if ( numPoints < 3)
  {
    delete vectoriser;
    return;
  }
  
  data = new FTGL_DOUBLE[ numPoints * 3];
  vectoriser->GetMesh( data);
  delete vectoriser;
  
  int d = 0;
  glList = glGenLists(1);
  glNewList( glList, GL_COMPILE);
    int BEPairs = static_cast<int>(data[0]);
    for( int i = 0; i < BEPairs; ++i)
    {
      int polyType = (int)data[d + 1];
      glBegin( polyType);

      int verts = (int)data[d+2];
      d += 3;
      for( int x = 0; x < verts; ++x)
      {
        glVertex3dv( data + d);
        d += 3;
      }
      glEnd();
    }
  glEndList();

  delete [] data; // FIXME
  data = 0;

  // discard glyph image (bitmap or not)
  FT_Done_Glyph( glyph); // Why does this have to be HERE
}


FTPolyGlyph::~FTPolyGlyph()
{
//  if( data)
//    delete [] data; // FIXME
}


float FTPolyGlyph::Render( const FT_Vector& pen,
                           const FTGLRenderContext *context)
{
  if( glList)
  {
    glTranslatef( (float)pen.x, (float)pen.y, 0);
      glCallList( glList);  
    glTranslatef( (float)-pen.x, (float)-pen.y, 0);
  }
  
  return advance;
}

#ifdef FTGL_USE_NAMESPACE
} // namespace ftgl
#endif
