#include  <math.h>

#include  "FTExtrdGlyph.h"
#include  "FTVectoriser.h"
#ifdef FTGL_DEBUG
  #include "mmgr.h"
#endif


FTExtrdGlyph::FTExtrdGlyph( FT_Glyph glyph, float d)
:  FTGlyph(),
  vectoriser(0),
  glList(0),
  depth(d)
{
  if( ft_glyph_format_outline != glyph->format)
  {
    return;
  }

  vectoriser = new FTVectoriser( glyph);
  
  vectoriser->Process();

  // Make the front polygons
  vectoriser->MakeMesh( 1.0);
  
  bBox = FTBBox( glyph);
  bBox.z2 = -depth;
  advance = (float)(glyph->advance.x >> 16);
  
  int numPoints = vectoriser->MeshPoints();
  if ( numPoints < 3)
  {
    delete vectoriser;
    return;
  }
  
  FTGL_DOUBLE* frontMesh = new FTGL_DOUBLE[ numPoints * 3];
  vectoriser->GetMesh( frontMesh);
  
  // Make the back polygons
  vectoriser->MakeMesh( -1.0);
  
  numPoints = vectoriser->MeshPoints();
  if ( numPoints < 3)
  {
    delete vectoriser;
    delete [] frontMesh;
    return;
  }
  
  FTGL_DOUBLE* backMesh =  new FTGL_DOUBLE[ numPoints * 3];
  vectoriser->GetMesh( backMesh);
  
  numPoints = vectoriser->points();
  int numContours = vectoriser->contours(); // FIXME
  
  if ( ( numContours < 1) || ( numPoints < 3))
  {
    delete vectoriser;
    delete [] frontMesh;
    delete [] backMesh;
    return;
  }
  
  // Build the edge polygons
  int* contourLength = new int[ numContours];
  for( int cn = 0; cn < numContours; ++cn)
  {
    contourLength[cn] = vectoriser->contourSize( cn);
  }
  
  FTGL_DOUBLE* sidemesh = new FTGL_DOUBLE[ numPoints * 3];
  vectoriser->GetOutline( sidemesh);
  
  delete vectoriser;
  
  // Draw the glyph
  int offset = 0;
  glList = glGenLists(1);
  glNewList( glList, GL_COMPILE);
  // Render Front Mesh
      int i;
    int BEPairs = static_cast<int>(frontMesh[0]);
    for( i = 0; i < BEPairs; ++i)
    {
      int polyType = (int)frontMesh[offset + 1];
      glBegin( polyType);
        glNormal3d(0.0, 0.0, 1.0);
    
        int verts = (int)frontMesh[offset+2];
        offset += 3;
        for( int x = 0; x < verts; ++x)
        {
          glVertex3dv( frontMesh + offset);
          offset += 3;
        }
      glEnd();
    }
    
  // Render Back Mesh
    offset = 0;
    BEPairs = static_cast<int>(backMesh[0]);
    for( i = 0; i < BEPairs; ++i)
    {
      int polyType = (int)backMesh[offset + 1];
      glBegin( polyType);

        glNormal3d(0.0, 0.0, -1.0);
        int verts = (int)backMesh[offset+2];
        offset += 3;
        for( int x = 0; x < verts; ++x)
        {
          glVertex3d( backMesh[offset], backMesh[offset + 1], -depth); // FIXME
          offset += 3;
        }
      glEnd();
    }
    
    FT_OutlineGlyph outline = (FT_OutlineGlyph)glyph;
    FT_Outline ftOutline = outline->outline;
    int contourFlag = ftOutline.flags; // this is broken for winding direction in freetype...      
                // BUT THIS DOESN'T WORK EITHER!!!!!
//                bool winding = Winding( contourLength[0], sidemesh);
                        
  // Join them together.
    // Extrude each contour to make the sides.
    FTGL_DOUBLE* contour = sidemesh;
    for (int c=0; c<numContours; ++c)
    {
      // Make a quad strip using each successive
      // pair of points in this contour.
      numPoints = contourLength[c];
      
      glBegin( GL_QUAD_STRIP);

        for( int j= 0; j <= numPoints; ++j)
        {
          int j1 = (j < numPoints) ? j : 0;
          int j0 = (j1 == 0) ? (numPoints-1) : (j1-1);

          FTGL_DOUBLE* p0 = contour + j0*3;
          FTGL_DOUBLE* p1 = contour + j1*3;

          // Compute normal for this quad.
          FTGL_DOUBLE vx = p1[0] - p0[0];
          FTGL_DOUBLE vy = p1[1] - p0[1];
          // Normalise
          FTGL_DOUBLE length = sqrt( ( ( vx * vx) + ( vy * vy)));
          vx /= length; vy /= length;
          glNormal3d(-vy, vx, 0.0);
          
          // Add vertices to the quad strip.
          // Winding order
          if( contourFlag & ft_outline_reverse_fill)
//          if( winding)
          {
            glVertex3d(p0[0], p0[1], 0);
            glVertex3d(p0[0], p0[1], -depth);
          }
          else
          {
            glVertex3d(p0[0], p0[1], -depth);
            glVertex3d(p0[0], p0[1], 0);
          }
        } // for
      glEnd();
      contour += numPoints*3;
    } // for 
    
    
  glEndList();

  delete [] sidemesh; // FIXME
  delete [] frontMesh;
  delete [] backMesh;
  delete [] contourLength;

  // discard glyph image (bitmap or not)
  FT_Done_Glyph( glyph); // Why does this have to be HERE
}


FTExtrdGlyph::~FTExtrdGlyph()
{
//  if( data)
//    delete [] data; // FIXME
}


bool FTExtrdGlyph::Winding( int numPoints, FTGL_DOUBLE *points)
{
  // Calculate the winding direction. use formula from redbook.
  FTGL_DOUBLE area = 0;
  
  for( int count = 0; count <= numPoints; ++count)
  {
#if 0
  int j1 = (count < numPoints) ? count : 0;
#else
  int j1;
  if (count < numPoints)
    {
    j1 = count;
    } 
  else
    {
    j1 = 0;
    }
#endif
  
  int j0 = (j1 == 0) ? ( numPoints - 1) : ( j1 - 1);
  
  FTGL_DOUBLE* p0 = points + j0 * 3;
    FTGL_DOUBLE* p1 = points + j1 * 3;

    area += ( p0[0] * p1[1]) - ( p1[0] * p0[1]);  
  }
  
  return( area >= 0 );
}


float FTExtrdGlyph::Render( const FT_Vector& pen,
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
