/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLProjectedPolyDataRayBounder.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Lisa Sobierajski Avila who developed this class.

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkOpenGLProjectedPolyDataRayBounder.h"
#include "vtkRenderer.h"
#include "vtkRayCaster.h"
#include "vtkObjectFactory.h"


#ifndef VTK_IMPLEMENT_MESA_CXX
//------------------------------------------------------------------------------
vtkOpenGLProjectedPolyDataRayBounder* vtkOpenGLProjectedPolyDataRayBounder::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkOpenGLProjectedPolyDataRayBounder");
  if(ret)
    {
    return (vtkOpenGLProjectedPolyDataRayBounder*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkOpenGLProjectedPolyDataRayBounder;
}
#endif




// Construct a new vtkOpenGLProjectedPolyDataRayBounder.  The depth range
// buffer is initially NULL and no display list has been created
vtkOpenGLProjectedPolyDataRayBounder::vtkOpenGLProjectedPolyDataRayBounder()
{
  this->DisplayList         = 0;
  this->DepthRangeBuffer    = NULL;
}

// Destruct the vtkOpenGLProjectedPolyDataRayBounder.  Free the 
// DepthRangeBuffer if necessary
vtkOpenGLProjectedPolyDataRayBounder::~vtkOpenGLProjectedPolyDataRayBounder()
{  
  if ( this->DepthRangeBuffer )
    {
    delete [] this->DepthRangeBuffer;
    }
}

// Create a display list from the polygons contained in pdata.
// Lines and vertices are ignored, polys and strips are used.
void vtkOpenGLProjectedPolyDataRayBounder::Build( vtkPolyData *pdata )
{
  vtkCellArray   *polys;
  vtkCellArray   *strips;
  vtkPoints      *points;
  int            npts;
  vtkIdType      *pts;
  int            current_num_vertices = -1;
  int            i;

  polys = pdata->GetPolys();
  points = pdata->GetPoints();
  strips = pdata->GetStrips();

  if ( !glIsList( this->DisplayList ) )
    {
    this->DisplayList = glGenLists( 1 );
    }

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
	{
	glEnd();
	}

      // How many vertices do we have?
      if ( npts == 3 ) 
	{
	glBegin( GL_TRIANGLES );
	}
      else if ( npts == 4 )
	{
	glBegin( GL_QUADS );
	}
      else
	{
	glBegin( GL_POLYGON );
	}
      }

    // Draw the vertices
    for ( i = 0; i < npts; i++ )
      {
      glVertex3fv( points->GetPoint( pts[i] ) );
      }

    current_num_vertices = npts;
    }
  
  glEnd();

  for ( strips->InitTraversal(); strips->GetNextCell( npts, pts ); )
    {
    glBegin( GL_TRIANGLE_STRIP );

    // Draw the vertices
    for ( i = 0; i < npts; i++ )
      {
      glVertex3fv( points->GetPoint( pts[i] ) );
      }

    glEnd();
    }

  glEndList();
}

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
  vtkRayCaster           *ray_caster;
  float                  z_numerator, z_denom_mult, z_denom_add;
  float                  zbias;
  float                  zscale;
  int                    i, j;
  GLint                  current_viewport[4];


  ray_caster = ren->GetRayCaster();

  // Create some objects that we will need later
  transform       = vtkTransform::New();
  matrix          = vtkMatrix4x4::New();

  // The size of the view rays is the size of the image we are creating
  ren->GetRayCaster()->GetViewRaysSize( size );

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
    {
    delete [] this->DepthRangeBuffer;
    }
  this->DepthRangeBuffer = new float[ size[0] * size[1] * 2 ];

  // Save previous lighting state, and turn lighting off
  glGetBooleanv( GL_LIGHTING, &lighting_on );
  glDisable( GL_LIGHTING );

  // Put the volume's matrix on the stack
  position_matrix->Transpose();
  glPushMatrix();
  glMultMatrixd( &(position_matrix->Element[0][0]) );

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
  if ( lighting_on )
    {
    glEnable( GL_LIGHTING );
    }

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
    transform->SetMatrix( ren->GetActiveCamera() \
                          ->GetPerspectiveTransformMatrix( aspect, -1, 1 ) );
    transform->Inverse();
    
    // To speed things up, we pull the matrix out of the transform. 
    // This way, we can decode z values faster since we know which elements
    // of the matrix are important, and which are zero.
    transform->GetMatrix(matrix);
    
    // Just checking that our assumptions are correct.
    if( this->Debug )
      {
      if (  matrix->Element[3][0] || matrix->Element[3][1]  ||
	    matrix->Element[3][2] || (matrix->Element[3][3] != 1.0) )
        {
        vtkErrorMacro( << "Oh no! They aren't 0 like they're supposed to be!");
        cout << *transform;
        }
      }
    
    // These are the important elements of the matrix.  We will decode
    // z values by : (((zbuffer value)*zscale + zbias)
    zscale = (matrix->Element[2][2]);
    zbias  = (matrix->Element[2][3]);
    
    for ( j = 0; j < size[1]; j++ )
      {
      for ( i = 0; i < size[0]; i++ )
	{
	if ( *near_ptr < 1.0 )
	  {
	  *(range_ptr++) = -(((*(near_ptr++))*2.0 -1.0) * zscale + zbias);
	  *(range_ptr++) = -(((*( far_ptr++))*2.0 -1.0) * zscale + zbias);
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
    transform->SetMatrix( ren->GetActiveCamera()\
                   ->GetPerspectiveTransformMatrix( aspect, -1, 1 ) );
    transform->Inverse();
    
    // To speed things up, we pull the matrix out of the transform. 
    // This way, we can decode z values faster since we know which elements
    // of the matrix are important, and which are zero.
    transform->GetMatrix(matrix);
    
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

    ray_ptr = ray_caster->GetPerspectiveViewRays();
    ray_ptr += 2;

    for ( j = 0; j < size[1]; j++ )
      {
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
    }

  delete [] near_buffer;
  delete [] far_buffer;

  // Delete the objects we created
  transform->Delete();
  matrix->Delete();

  return ( this->DepthRangeBuffer );
}


void
vtkOpenGLProjectedPolyDataRayBounder::ReleaseGraphicsResources(vtkWindow *vtkNotUsed(renWin))
{
  if (this->DisplayList)
    {
    glDeleteLists(this->DisplayList,1);
    this->DisplayList = 0;
    }
}


// Print the vtkOpenGLProjectedPolyDataRayBounder
void vtkOpenGLProjectedPolyDataRayBounder::PrintSelf(ostream& os, 
						     vtkIndent indent)
{
  vtkProjectedPolyDataRayBounder::PrintSelf(os,indent);
}


