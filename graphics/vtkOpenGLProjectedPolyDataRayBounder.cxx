/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLProjectedPolyDataRayBounder.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Lisa Sobierajski Avila who developed this class.

Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/

#include "vtkOpenGLProjectedPolyDataRayBounder.h"
#include "vtkNew2VolumeRenderer.h"

// Description:
// Construct a new vtkOpenGLProjectedPolyDataRayBounder.  The depth range
// buffer is initially NULL and no display list has been created
vtkOpenGLProjectedPolyDataRayBounder::vtkOpenGLProjectedPolyDataRayBounder()
{
  this->DisplayList         = 0;
  this->DepthRangeBuffer    = NULL;
}

// Description:
// Destruct the vtkOpenGLProjectedPolyDataRayBounder.  Free the 
// DepthRangeBuffer if necessary
vtkOpenGLProjectedPolyDataRayBounder::~vtkOpenGLProjectedPolyDataRayBounder()
{  
  if ( this->DepthRangeBuffer )
    delete this->DepthRangeBuffer;
}

// Description:
// Create a display list from the polygons contained in pdata.
// Lines and vertices are ignored, polys and strips are used.
void vtkOpenGLProjectedPolyDataRayBounder::Build( vtkPolyData *pdata )
{
  vtkCellArray   *polys;
  vtkCellArray   *strips;
  vtkPoints      *points;
  int            *pts, npts;
  int            current_num_vertices = -1;
  int            i;

  polys = pdata->GetPolys();
  points = pdata->GetPoints();
  strips = pdata->GetStrips();

  if ( !glIsList( this->DisplayList ) )
    this->DisplayList = glGenLists( 1 );

  glNewList( this->DisplayList, GL_COMPILE );

  for ( polys->InitTraversal(); polys->GetNextCell( npts, pts ); )
    {
    // If we are doing a different number of vertices, or if this
    // is a polygon, then end what we were doing and begin again
    if ( current_num_vertices != npts || npts > 4 )
      {
      // Unless of course this is our first time through - then we
      // don't want to end
      if ( current_num_vertices != -1 )
	glEnd();

      // How many vertices do we have?
      if ( npts == 3 ) 
	glBegin( GL_TRIANGLES );
      else if ( npts == 4 )
	glBegin( GL_QUADS );
      else
	glBegin( GL_POLYGON );
      }

    // Draw the vertices
    for ( i = 0; i < npts; i++ )
      glVertex3fv( points->GetPoint( pts[i] ) );

    current_num_vertices = npts;
    }
  
  glEnd();

  for ( strips->InitTraversal(); strips->GetNextCell( npts, pts ); )
    {
    glBegin( GL_TRIANGLE_STRIP );

    // Draw the vertices
    for ( i = 0; i < npts; i++ )
      glVertex3fv( points->GetPoint( pts[i] ) );

    glEnd();
    }

  glEndList();
}

// Description:
// Draw the display list and create the depth range buffer.
//
// Known problem:
// camera clipping planes (near/far) may clip the projected
// geometry resulting in incorrect results.
float *vtkOpenGLProjectedPolyDataRayBounder::Draw( vtkRenderer *ren, 
				     vtkMatrix4x4 *position_matrix )
{
  GLboolean              lighting_on;
  int                    size[2];
  float                  *near_buffer, *far_buffer;
  vtkTransform           *transform;
  vtkMatrix4x4           *matrix;
  float                  ren_aspect[2], aspect, range[2];
  float                  *ray_ptr;
  float                  *near_ptr, *far_ptr;
  float                  *range_ptr;
  vtkNew2VolumeRenderer  *volren;
  float                  z_numerator, z_denom_mult, z_denom_add;
  float                  zfactor;
  int                    i, j;
  GLint                  current_viewport[4];


  volren = (vtkNew2VolumeRenderer *) ren->GetNewVolumeRenderer();

  // Create some objects that we will need later
  transform       = vtkTransform::New();
  matrix          = vtkMatrix4x4::New();

  // The size of the view rays is the size of the image we are creating
  ((vtkNew2VolumeRenderer *)
   (ren->GetNewVolumeRenderer()))->GetViewRaysSize( size );

  // This should be fixed - I should not be off in someone else's viewport
  // if there are more than one of them...
  glGetIntegerv( GL_VIEWPORT, current_viewport );
  glPushAttrib(GL_VIEWPORT_BIT);
  glViewport( current_viewport[0], current_viewport[1], 
	      (GLsizei) size[0], (GLsizei) size[1] );

  // Create the near buffer storage
  near_buffer = new float[ size[0] * size[1] ];

  // Create the far buffer storage
  far_buffer = new float[ size[0] * size[1] ];

  if ( this->DepthRangeBuffer )
    delete this->DepthRangeBuffer;
  this->DepthRangeBuffer = new float[ size[0] * size[1] * 2 ];

  // Save previous lighting state, and turn lighting off
  glGetBooleanv( GL_LIGHTING, &lighting_on );
  glDisable( GL_LIGHTING );

  // Put the volume's matrix on the stack
  position_matrix->Transpose();
  glPushMatrix();
  glMultMatrixf( (*position_matrix)[0] );

  // Do the far buffer 
  glDepthFunc( GL_GREATER );
  glClearColor( (GLclampf)(0.0), 
		(GLclampf)(0.0), 
		(GLclampf)(0.0), 
		(GLclampf)(0.0) );
  glClearDepth( (GLclampd)(0.0) );
  glClear((GLbitfield)(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

  glCallList( this->DisplayList );

  glReadPixels( 0, 0, size[0], size[1], GL_DEPTH_COMPONENT, GL_FLOAT,
		far_buffer );

  // Do the near buffer
  glDepthFunc( GL_LESS );
  glClearColor( (GLclampf)(0.0), 
		(GLclampf)(0.0), 
		(GLclampf)(0.0), 
		(GLclampf)(0.0) );
  glClearDepth( (GLclampd)(1.0) );
  glClear((GLbitfield)(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

  glCallList( this->DisplayList );

  glReadPixels( (GLint) 0, (GLint) 0, (GLsizei) size[0], (GLsizei) size[1], 
		GL_DEPTH_COMPONENT, GL_FLOAT,
		near_buffer );

  // Clean up
  glPopMatrix();
  glDepthFunc( GL_LEQUAL );
  if ( lighting_on ) glEnable( GL_LIGHTING );

  glPopAttrib();

  near_ptr  = near_buffer;
  far_ptr   = far_buffer;
  range_ptr = this->DepthRangeBuffer;

  if( ren->GetActiveCamera()->GetParallelProjection() )
    {

    // Get the aspect ratio of the renderer
    ren->GetAspect( ren_aspect );
    aspect = ren_aspect[0]/ren_aspect[1];
    
    // Get the clipping range of the active camera
    ren->GetActiveCamera()->GetClippingRange( range );
    
    // Create the perspective matrix for the camera.  This will be used
    // to decode z values, so we will need to inverted
    transform->SetMatrix( ren->GetActiveCamera()->GetPerspectiveTransform(
      aspect, 0, 1 ) );
    transform->Inverse();
    
    // To speed things up, we pull the matrix out of the transform. 
    // This way, we can decode z values faster since we know which elements
    // of the matrix are important, and which are zero.
    transform->GetMatrix( *matrix );
    
    // Just checking that our assumptions are correct.  This code should
    // be removed after the debugging phase is complete
    if( this->Debug )
      {
      if (  matrix->Element[3][0] || matrix->Element[3][1]  ||
	    matrix->Element[3][2] || matrix->Element[2][3]  ||
	    (matrix->Element[3][3] != 1.0) )
        {
        vtkErrorMacro( << "Oh no! They aren't 0 like they're supposed to be!");
        cout << *transform;
        }
      }
    
    // These are the important elements of the matrix.  We will decode
    // z values by : ((zbuffer value)*znum3)
    zfactor = -(matrix->Element[2][2]);
    
    for ( j = 0; j < size[1]; j++ )
      for ( i = 0; i < size[0]; i++ )
	{
	if ( *near_ptr < 1.0 )
	  {
	  *(range_ptr++) = ((*(near_ptr++))*2.0 -1.0) * zfactor;
	  *(range_ptr++) = ((*( far_ptr++))*2.0 -1.0) * zfactor;	  
	  }
	else
	  {
	  *(range_ptr++) = -1.0;
	  *(range_ptr++) = -1.0;
	  near_ptr++;
	  far_ptr++;
	  }
	}
    }
  else
    {
    // Get the aspect ratio of the renderer
    ren->GetAspect( ren_aspect );
    aspect = ren_aspect[0]/ren_aspect[1];
    
    // Get the clipping range of the active camera
    ren->GetActiveCamera()->GetClippingRange( range );
    
    // Create the perspective matrix for the camera.  This will be used
    // to decode z values, so we will need to invert it
    transform->SetMatrix( 
      ren->GetActiveCamera()->GetPerspectiveTransform( aspect, 0, 1 ) );
    transform->Inverse();
    
    // To speed things up, we pull the matrix out of the transform. 
    // This way, we can decode z values faster since we know which elements
    // of the matrix are important, and which are zero.
    transform->GetMatrix( *matrix );
    
    // Just checking that our assumptions are correct.  This code should
    // be removed after the debugging phase is complete
    if( this->Debug )
      {
      if ( matrix->Element[2][0] || matrix->Element[2][1]  ||
	   matrix->Element[3][0] || matrix->Element[3][1]  ||
	   matrix->Element[2][2] )
        vtkErrorMacro( << "Oh no! They aren't 0 like they're supposed to be!");
      }

    // These are the important elements of the matrix.  We will decode
    // z values by taking the znum1 and dividing by the zbuffer z value times
    // zdenom1 plus zdenom2.
    z_numerator    = matrix->Element[2][3];
    z_denom_mult   = matrix->Element[3][2];
    z_denom_add    = matrix->Element[3][3];

    ray_ptr = volren->GetPerspectiveViewRays();
    ray_ptr += 2;

    for ( j = 0; j < size[1]; j++ )
      for ( i = 0; i < size[0]; i++ )
	{
	if ( *near_ptr < 1.0 )
	  {
	  *(range_ptr++) = 
	    (-z_numerator / 
	     ( ((*(near_ptr++))*2.0 -1.0) *
	       z_denom_mult + z_denom_add )) / (-(*ray_ptr));

	  *(range_ptr++) = 
	    (-z_numerator / 
	     ( ((*(far_ptr++))*2.0 -1.0) *
	       z_denom_mult + z_denom_add )) / (-(*ray_ptr));
	  
	  ray_ptr += 3;
	  }
	else
	  {
	  *(range_ptr++) = -1.0;
	  *(range_ptr++) = -1.0;
	  near_ptr++;
	  far_ptr++;
	  ray_ptr += 3;
	  }
	}

    }

  delete near_buffer;
  delete far_buffer;

  // Delete the objects we created
  transform->Delete();
  matrix->Delete();

  return ( this->DepthRangeBuffer );
}

// Description:
// Print the vtkOpenGLProjectedPolyDataRayBounder
void vtkOpenGLProjectedPolyDataRayBounder::PrintSelf(ostream& os, 
						     vtkIndent indent)
{
  vtkProjectedPolyDataRayBounder::PrintSelf(os,indent);
}

