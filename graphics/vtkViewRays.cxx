/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkViewRays.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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

#include "vtkRenderer.h"
#include "vtkViewRays.h"
#include <math.h>

// Description:
// Constructor for vtkViewRays. Default everything to NULL or 0
vtkViewRays::vtkViewRays(void)
{
  this->Renderer         	= NULL;
  this->Size[0]          	= 0;
  this->Size[1]          	= 0;

  this->ViewRaysCamMtime	= 0;
  this->ViewRaysMTime    	= 0;

  this->StartPosition[0]	= 0.0;
  this->StartPosition[1]	= 0.0;
  this->StartPosition[2]	= 0.0;

  this->Increments[0]		= 0.0;
  this->Increments[1]		= 0.0;

  this->ViewRays        	= NULL;
}

// Description:
// Destructor for vtkViewRays. Free up the memory used by the view rays
vtkViewRays::~vtkViewRays(void)
  {
  if (ViewRays)
    {
    delete[] this->ViewRays;
    }
  }

// Description:
// Return the view rays. This is an array of this->Size[0] by
// this->Size[1] by 3 floats per element. The elements are ray directions
// in camera space given the current camera instance variables.
float *vtkViewRays::GetPerspectiveViewRays(void)
{
  unsigned long  cam_mtime;
  int            update_info = 0;

  // Is there a renderer associated with this object?
  if (this->Renderer == 0)
    {
    vtkErrorMacro(<< "A Renderer has not been set in vtkViewRays\n");
    return NULL;
    }

  // Do we have a valid size?
  if (this->Size[0] == 0 || this->Size[1] == 0)
    {
    vtkErrorMacro(<< "View Rays has a 0 area, must SetSize() to area > 0\n");
    return NULL;
    }
 
  // Does the camera model use a perspective projection?
  if( this->Renderer->GetActiveCamera()->GetParallelProjection() )
    {
    vtkErrorMacro(<< "Request for perspective view rays when the camera is parallel\n");
    return NULL;
    }

  // We need to update the rays if this object has been modified more
  // recently than the last time the view rays were calculated
  update_info = (this->GetMTime() > ViewRaysMTime);
 
  // Check to see if camera mtime has changed
  cam_mtime = Renderer->GetActiveCamera()->GetViewingRaysMTime();
 
  // We also need to update if the view rays mtime in the camera is
  // not the same as our copy of it. This means that some ivar in the
  // camera that would affect the view rays has changed
  if( cam_mtime != this->ViewRaysCamMtime )
    {
    this->ViewRaysCamMtime = cam_mtime;
    update_info = 1;
    }
 
  // If we need to update the rays we will delete any old view rays
  // we have and allocate new space for them. We will calculate the
  // view rays and reset our ViewRaysMTime
  if(update_info)
    {
    if ( this->ViewRays )
      delete[] this->ViewRays;

    this->ViewRays = new float[this->Size[0]*this->Size[1]*3];
    this->ComputePerspectiveInfo(this->ViewRays,this->Size);

    this->ViewRaysMTime = this->GetMTime();
    }

  return( this->ViewRays );
}

// Description:
// Private method to create the view rays into vr_ptr or a given size
void vtkViewRays::ComputePerspectiveInfo(float *vr_ptr,int size[2])
{
  float         xpos, ypos, zpos;
  float         xinc, yinc;
  float         mag;
  float         nx, ny, nz;
  float         *aspect;
  int           x, y;
  vtkMatrix4x4  mat;
  float         result[4];


  // Is there a renderer associated with this object?
  if (this->Renderer == 0)
    {
    vtkErrorMacro(<< "A Renderer is not associated with this ViewRays object");
    return;
    }
  
  // Did we pass in a valid pointer?
  if( !vr_ptr)
    {
    vtkErrorMacro(<< "No memory allocated to build perspective viewing rays.");
    return;
    }  

  // Get the aspect ratio of the render area from the renderer
  aspect = Renderer->GetAspect();
  
  // get the perspective transformation from the active camera
  // given the aspect ratio
  mat = Renderer->GetActiveCamera()->GetPerspectiveTransform(
    aspect[0]/aspect[1],-1,1);

  // Invert this matrix because we want to go from screen space to
  // camera space
  mat.Invert();

  // Transpose it for easier use
  mat.Transpose();
  
  // This is the increment between pixel locations in screen space
  xinc = 2.0/(float)(size[0]);
  yinc = 2.0/(float)(size[1]);
  
  // This is the initial y and z positions in screen space
  ypos = -1.0 + yinc/2.0;
  zpos =  1.0;
  
  // Loop through each pixel and compute viewing ray
  for( y=0; y<size[1]; y++ )
    {
    // Compute the initial x position for this row
    xpos = -1.0 + xinc/2.0;
    for( x=0; x<size[0]; x++ )
      {
      result[0] = xpos;
      result[1] = ypos;
      result[2] = zpos;
      result[3] = 1.0;
      
      // Convert this location into camera space - this becomes our
      // view ray direction because we start the ray at (0,0,0) in
      // camera space and go in the direction of this result
      mat.PointMultiply(result,result);
      
      // Normalize view ray
      mag = sqrt( (double)(result[0]*result[0] +
                           result[1]*result[1] + result[2]*result[2]) );
      
      if( mag != 0.0 )
	{
        nx = result[0]/mag;
        ny = result[1]/mag;
        nz = result[2]/mag;
	}
      else
        nx = ny = nz = 0.0;
      
      // Set the value of this view ray
      *(vr_ptr++) = nx;
      *(vr_ptr++) = ny;
      *(vr_ptr++) = nz;
      
      // Increment the x position in screen space
      xpos += xinc;
      }
    // Increment the y position in screen space
    ypos += yinc;
    }
}

// Description:
// Return the distance to move to the next ray starting point along the 
// X and Y direction.
float *vtkViewRays::GetParallelIncrements( void )
{
  unsigned long  cam_mtime;
  int update_info = 0;

  // Is there a renderer associated with this object?
  if (this->Renderer == 0)
    {
    vtkErrorMacro(<< "A Renderer has not been set in vtkViewRays\n");
    return NULL;
    }

  // Do we have a valid size?
  if (this->Size[0] == 0 || this->Size[1] == 0)
    {
    vtkErrorMacro(<< "View Rays has a 0 area, must SetSize() to area > 0\n");
    return NULL;
    }
 
  // Does the camera model use a parallel projection?
  if( !(this->Renderer->GetActiveCamera()->GetParallelProjection()) )
    {
    vtkErrorMacro(<< "Request for parallel view rays when the camera is perspective\n");
    return NULL;
    }

  // We need to update the vectors if this object has been modified more
  // recently than the last time the vectors were calculated
  update_info = (this->GetMTime() > ViewRaysMTime);
 
  // Check to see if camera mtime has changed
  cam_mtime = Renderer->GetActiveCamera()->GetViewingRaysMTime();
 
  // We also need to update if the view rays mtime in the camera is
  // not the same as our copy of it. This means that some ivar in the
  // camera that would affect the view rays has changed
  if( cam_mtime != this->ViewRaysCamMtime )
    {
    this->ViewRaysCamMtime = cam_mtime;
    update_info = 1;
    }
 
  if(update_info)
    {
    this->ComputeParallelInfo( this->Size );
    this->ViewRaysMTime = this->GetMTime();
    }

  return( this->Increments );
}

// Description:
// Return the Starting position of the bottom left most ray.
float *vtkViewRays::GetParallelStartPosition( void )
{
  unsigned long  cam_mtime;
  int update_info = 0;

  // Is there a renderer associated with this object?
  if (this->Renderer == 0)
    {
    vtkErrorMacro(<< "A Renderer has not been set in vtkViewRays\n");
    return NULL;
    }

  // Do we have a valid size?
  if (this->Size[0] == 0 || this->Size[1] == 0)
    {
    vtkErrorMacro(<< "View Rays has a 0 area, must SetSize() to area > 0\n");
    return NULL;
    }
 
  // Does the camera model use a parallel projection?
  if( !(this->Renderer->GetActiveCamera()->GetParallelProjection()) )
    {
    vtkErrorMacro(<< "Request for parallel start position when the camera is perspective\n");
    return NULL;
    }

  // We need to update the start position if this object has been modified more
  // recently than the last time the start position was calculated
  update_info = (this->GetMTime() > ViewRaysMTime);
 
  // Check to see if camera mtime has changed
  cam_mtime = Renderer->GetActiveCamera()->GetViewingRaysMTime();
 
  // We also need to update if the view rays mtime in the camera is
  // not the same as our copy of it. This means that some ivar in the
  // camera that would affect the view rays has changed
  if( cam_mtime != this->ViewRaysCamMtime )
    {
    this->ViewRaysCamMtime = cam_mtime;
    update_info = 1;
    }
 
  if(update_info)
    {
    this->ComputeParallelInfo( this->Size );
    this->ViewRaysMTime = this->GetMTime();
    }

  return( this->StartPosition );
}

// Description:
// Calculate the information for stepping from ray to ray when using a parallel
// projection.
void vtkViewRays::ComputeParallelInfo( int size[2] )
{
  float         x_half_inc, y_half_inc;
  float		p_scale;
  float         ren_aspect[2], aspect;

  // Get the aspect ratio of the renderer
  this->Renderer->GetAspect( ren_aspect );
  aspect = ren_aspect[0]/ren_aspect[1];

  // Get the parallel scale of the camera
  p_scale = this->Renderer->GetActiveCamera()->GetParallelScale();

  // This is the increment between pixel locations in screen space
  this->Increments[0] = 2.0/(float)(size[0]) * p_scale * aspect;
  this->Increments[1] = 2.0/(float)(size[1]) * p_scale;

  x_half_inc = this->Increments[0]/2.0;
  y_half_inc = this->Increments[1]/2.0;

  // Calculate the start position at the bottom left corner of the view plane
  this->StartPosition[0] = (-1.0 * p_scale * aspect + x_half_inc);
  this->StartPosition[1] = (-1.0 * p_scale + y_half_inc);
  this->StartPosition[2] = 0.0;

}

// Description:
// Print the class
void vtkViewRays::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os,indent);
  os << "Renderer: " << this->Renderer << "\n";
  os << "Size: " << this->Size[0] << ", " << this->Size[1] << "\n";
  os << "CamMtime: " << this->ViewRaysCamMtime << "\n";
  os << "ViewRaysMTime: " << this->ViewRaysMTime << "\n";
  os << "Parallel Start Position: " << this->StartPosition[0] << ", " 
     << this->StartPosition[1] << ", " << this->StartPosition[2] << "\n";
  os << "Parallel X Increment: " << this->Increments[0] << "\n";
  os << "Parallel Y Increment: " << this->Increments[1] << "\n";
}



