/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRayCaster.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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

#include <stdlib.h>
#include <string.h>

#include "vtkRayCaster.h"
#include "vtkRenderWindow.h"
#include "vtkMath.h"
#include "vtkVoxel.h"
#include "vtkTimerLog.h"

static vtkMath math;

void vtkRayCaster::NearestNeighborZoom(float *smallImage, 
				       float *largeImage,
				       int smallDims[2],
				       int largeDims[2] )
{
  int     i, j;
  float   *iptr1, *iptr2;
  float   xscale, yscale;
  int     yoffset, offset;

  iptr2 = largeImage;

  if ( smallDims[0] > largeDims[0] ||
       smallDims[1] > largeDims[1] )
    {
    vtkErrorMacro( << "Invalid dimensions to Nearest Neighbor Zoom:\n" <<
      smallDims[0] << " " << smallDims[1] << " " << 
      largeDims[0] << " " << largeDims[1] );
    return;
    }

  xscale = ((float)(smallDims[0]))/((float)(largeDims[0]));
  yscale = ((float)(smallDims[1]))/((float)(largeDims[1]));

  for ( j = 0; j < largeDims[1]; j++ )
    {
    yoffset = ((int)((float)(j) * yscale)) * smallDims[0] * 4;
    for ( i = 0; i < largeDims[0]; i++ )
      {
      offset = yoffset + 4*((int)((float)(i) * xscale));
      iptr1 = smallImage + offset;
      memcpy( iptr2, iptr1, 4*sizeof(float) );
      iptr2 += 4;
      iptr1 += 4;
      }
    }
}

void vtkRayCaster::BilinearZoom(float *smallImage, 
				float *largeImage,
				int smallDims[2],
				int largeDims[2] )
{
  int     i, j;
  float   *iptr1, *iptr2;
  float   xscale, yscale;
  float   y_position, x_position;
  float   x_bilin_factor, y_bilin_factor;
  float   A_coeff, B_coeff, C_coeff, D_coeff;
  float   A, B, C, D;
  float   *A_ptr, *B_ptr, *C_ptr, *D_ptr;
  float   val;
  int     yoffset, offset;

  iptr2 = largeImage;

  if ( smallDims[0] < 2 || 
       smallDims[1] < 2 ||
       largeDims[0] < 2 ||
       largeDims[1] < 2 ||
       smallDims[0] > largeDims[0] ||
       smallDims[1] > largeDims[1] )
    {
    vtkErrorMacro( << "Invalid dimensions to Bilinear Zoom:\n" <<
      smallDims[0] << " " << smallDims[1] << " " << 
      largeDims[0] << " " << largeDims[1] );
    return;
    }

  xscale = ((float)(smallDims[0]-1))/((float)(largeDims[0]-1));
  yscale = ((float)(smallDims[1]-1))/((float)(largeDims[1]-1));

  for ( j = 0; j < largeDims[1]; j++ )
    {
    y_position = (float)(j) * yscale;
    y_bilin_factor = y_position - ((int)y_position);
    yoffset = ((int)y_position) * smallDims[0] * 4;
    for ( i = 0; i < largeDims[0]; i++ )
      {
      x_position = (float)(i) * xscale;
      x_bilin_factor = x_position - ((int)x_position);
      offset = yoffset + 4*((int)x_position);
      iptr1 = smallImage + offset;

      A_coeff = (1.0 - y_bilin_factor)*(1.0 - x_bilin_factor);
      B_coeff = (1.0 - y_bilin_factor)*(x_bilin_factor);
      C_coeff = (y_bilin_factor)*(1.0 - x_bilin_factor);
      D_coeff = (y_bilin_factor)*(x_bilin_factor);

      A_ptr = (iptr1);
      B_ptr = (iptr1 + 4);
      C_ptr = (iptr1 + 4 * smallDims[0]);
      D_ptr = (iptr1 + 4 * smallDims[0] + 4);

      // Bilinearly interpolate the red channel
      A = *(A_ptr++);
      B = *(B_ptr++);
      C = *(C_ptr++);
      D = *(D_ptr++);

      val = A*A_coeff + B*B_coeff + C*C_coeff + D*D_coeff;
      *(iptr2++) = val;

      // Bilinearly interpolate the green channel
      A = *(A_ptr++);
      B = *(B_ptr++);
      C = *(C_ptr++);
      D = *(D_ptr++);

      val = A*A_coeff + B*B_coeff + C*C_coeff + D*D_coeff;
      *(iptr2++) = val;

      // Bilinearly interpolate the blue channel
      A = *(A_ptr++);
      B = *(B_ptr++);
      C = *(C_ptr++);
      D = *(D_ptr++);

      val = A*A_coeff + B*B_coeff + C*C_coeff + D*D_coeff;
      *(iptr2++) = val;

      // Bilinearly interpolate the alpha channel
      A = *(A_ptr);
      B = *(B_ptr);
      C = *(C_ptr);
      D = *(D_ptr);

      val = A*A_coeff + B*B_coeff + C*C_coeff + D*D_coeff;
      *(iptr2++) = val;
      }
    }
}

// Description:
// Constructor for vtkRayCaster
vtkRayCaster::vtkRayCaster()
{
  int   i;
  float scale;

  this->zbuffer                      = NULL;
  this->cbuffer                      = NULL;
  this->SelectedImageScaleIndex      = 0;
  this->AutomaticScaleAdjustment     = 1;
  this->AutomaticScaleLowerLimit     = 0.15;
  this->BilinearImageZoom            = 0;
  this->Renderer	             = NULL;
  this->ImageRenderTime[0]           = 0.0;
  this->ImageRenderTime[1]           = 0.0;
  this->StableImageScaleCounter      = 10;
  scale = 1.0;
  for ( i = 0; i < VTK_MAX_VIEW_RAYS_LEVEL; i++ )
    {
    this->ImageScale[i]         = scale;
    this->ViewRaysStepSize[i]   = 1.0;
    scale /= 2.0;
    }

  this->ImageScale[VTK_MAX_VIEW_RAYS_LEVEL] = 0.5;

}

// Description:
// Destructor for vtkRayCaster
vtkRayCaster::~vtkRayCaster()
{
}

// Description:
// Set the scale factor for a given level. This is used during multi-
// resolution interactive rendering
void vtkRayCaster::SetImageScale( int level, float scale )
{
  // Check if the level is out of range
  if ( level >= VTK_MAX_VIEW_RAYS_LEVEL || level < 1 )
    {
    vtkErrorMacro( << "Level: " << level << " is outside range: 1 to " << 
                   VTK_MAX_VIEW_RAYS_LEVEL-1 );
    }
  // Check if the scale is out of range
  else if ( scale < 0.01 || scale > 1.0 )
    {
    vtkErrorMacro( << "Scale: " << scale << " must be between 0.01 and 1.0" );
    }
  // Check if the scale is greater than or equal to the previous level scale
  else if ( scale >= this->ImageScale[level-1] )
    {
    vtkErrorMacro( << "Scale: " << scale << " is >= previous level scale" );
    }
  // Check if the scale is less than or equal to the next level scale
  else if ( level < VTK_MAX_VIEW_RAYS_LEVEL-1 &&
	    scale <= this->ImageScale[level+1] )
    {
    vtkErrorMacro( << "Scale: " << scale << " is <= next level scale" );
    }
  // Everything is ok - actually set it
  else
    this->ImageScale[level] = scale;
}

// Description:
// Get the scale factor for a given level. This is used during multi-
// resolution interactive rendering
float vtkRayCaster::GetImageScale( int level )
{
  // Check for out of range level
  if ( level >= VTK_MAX_VIEW_RAYS_LEVEL || level < 0 )
    {
    vtkErrorMacro( << "Level: " << level << " is outside range: 0 to " << 
                   VTK_MAX_VIEW_RAYS_LEVEL-1 );
    return -1.0;
    }
  // Level is ok - return the ImageScale
  else
    return this->ImageScale[level]; 
}

// Description:
// Turn the automatic scale adjustment on
void vtkRayCaster::AutomaticScaleAdjustmentOn( void )
{
  this->AutomaticScaleAdjustment = 1;
}

// Description:
// Turn the automatic scale adjustment off
void vtkRayCaster::AutomaticScaleAdjustmentOff( void )
{
  // If we turn automatic scale adjustment off, we reset the selected
  // image scale index to 0 since we have been using this for other
  // purposes while automatic scale adjustment was on
  this->AutomaticScaleAdjustment = 0;
  this->SelectedImageScaleIndex  = 0;
}

void vtkRayCaster::SetViewRaysStepSize( int level, float scale )
{
  // Check for out of range level
  if ( level >= VTK_MAX_VIEW_RAYS_LEVEL || level < 0 )
    {
    vtkErrorMacro( << "Level: " << level << " is outside range: 0 to " << 
                   VTK_MAX_VIEW_RAYS_LEVEL-1 );
    }
  else if ( scale < 0.01 || scale > 100.0 )
    {
    vtkErrorMacro( << "Scale: " << scale << 
                   " must be between 0.01 and 100.0" );
    }
  else
    this->ViewRaysStepSize[level] = scale;
}

float vtkRayCaster::GetViewRaysStepSize( int level )
{
  // Check for out of range level
  if ( level >= VTK_MAX_VIEW_RAYS_LEVEL || level < 0 )
    {
    vtkErrorMacro( << "Level: " << level << " is outside range: 0 to " << 
                   VTK_MAX_VIEW_RAYS_LEVEL-1 );
    return -1.0;
    }
  else
    return this->ViewRaysStepSize[level]; 
}

// Description:
// Get the size in pixels of the view rays for the selected scale indexl
void vtkRayCaster::GetViewRaysSize(int size[2])
{
  int    *rwin_size;
  float  *vp_size;

  // get physical window dimensions
  rwin_size = this->Renderer->GetRenderWindow()->GetSize();
  vp_size = this->Renderer->GetViewport();

  size[0] = (int)(rwin_size[0]*(float)(vp_size[2] - vp_size[0]));
  size[1] = (int)(rwin_size[1]*(float)(vp_size[3] - vp_size[1]));

  size[0] = (int)((float)(size[0]) * 
		this->ImageScale[ this->SelectedImageScaleIndex ]);
  size[1] = (int)((float)(size[1]) * 
		this->ImageScale[ this->SelectedImageScaleIndex ]);
}

float *vtkRayCaster::GetPerspectiveViewRays(void)
{
  int    size[2];
  int    *rwin_size;
  float  *vp_size;

  // get physical window dimensions
  rwin_size = this->Renderer->GetRenderWindow()->GetSize();
  vp_size = this->Renderer->GetViewport();

  size[0] = (int)(rwin_size[0]*(float)(vp_size[2] - vp_size[0]));
  size[1] = (int)(rwin_size[1]*(float)(vp_size[3] - vp_size[1]));

  size[0] = (int)((float)(size[0]) * 
		this->ImageScale[ this->SelectedImageScaleIndex ]);
  size[1] = (int)((float)(size[1]) * 
		this->ImageScale[ this->SelectedImageScaleIndex ]);

  this->ViewRays[this->SelectedImageScaleIndex].SetRenderer(this->Renderer);
  this->ViewRays[this->SelectedImageScaleIndex].SetSize( size );

  return 
    this->ViewRays[this->SelectedImageScaleIndex].GetPerspectiveViewRays();
}

float *vtkRayCaster::GetParallelStartPosition(void)
{
  int    size[2];
  int    *rwin_size;
  float  *vp_size; 
 
  // get physical window dimensions
  rwin_size = this->Renderer->GetRenderWindow()->GetSize();
  vp_size = this->Renderer->GetViewport();
 
  size[0] = (int)(rwin_size[0]*(float)(vp_size[2] - vp_size[0]));
  size[1] = (int)(rwin_size[1]*(float)(vp_size[3] - vp_size[1]));
 
  size[0] = (int)((float)(size[0]) * 
		this->ImageScale[ this->SelectedImageScaleIndex ]);
  size[1] = (int)((float)(size[1]) * 
		this->ImageScale[ this->SelectedImageScaleIndex ]);
 
  this->ViewRays[this->SelectedImageScaleIndex].SetRenderer(this->Renderer);
  this->ViewRays[this->SelectedImageScaleIndex].SetSize( size );

  return 
    this->ViewRays[this->SelectedImageScaleIndex].GetParallelStartPosition();
}

float *vtkRayCaster::GetParallelIncrements(void)
{
  int    size[2];
  int    *rwin_size;
  float  *vp_size; 
 
  // get physical window dimensions
  rwin_size = this->Renderer->GetRenderWindow()->GetSize();
  vp_size = this->Renderer->GetViewport();
 
  size[0] = (int)(rwin_size[0]*(float)(vp_size[2] - vp_size[0]));
  size[1] = (int)(rwin_size[1]*(float)(vp_size[3] - vp_size[1]));
 
  size[0] = (int)((float)(size[0]) * 
		this->ImageScale[ this->SelectedImageScaleIndex ]);
  size[1] = (int)((float)(size[1]) * 
		this->ImageScale[ this->SelectedImageScaleIndex ]);
 
  this->ViewRays[this->SelectedImageScaleIndex].SetRenderer(this->Renderer);
  this->ViewRays[this->SelectedImageScaleIndex].SetSize( size );

  return 
    this->ViewRays[this->SelectedImageScaleIndex].GetParallelIncrements();
}

// Description:
// This method returns the scale that should be applied to the viewport
// for geometric rendering, and for the image in volume rendering. It 
// is either explicitly set (if AutomaticScaleAdjustment is off) or
// is adjusted automatically to get the desired frame rate.
//
// Note: IMPORTANT!!!! This should only be called once per render!!!
//
float vtkRayCaster::GetViewportScaleFactor( vtkRenderer *ren )
{
  float                time_to_render;
  float                estimated_time;
  int                  selected_level;
  int                  visible_volume;
  vtkVolumeCollection  *volumes;
  vtkVolume            *volume;
  float                estimated_scale;
  float                scale_diff;

  // loop through volumes looking for a visible one
  visible_volume = 0;
  volumes = this->Renderer->GetVolumes();

  for (volumes->InitTraversal(); (volume = volumes->GetNextItem()); )
    {
    if (volume->GetVisibility())
      {
      visible_volume = 1;
      break;
      }
    }

  // There's no visible volume so we shouldn't scale the image
  if ( !visible_volume )
    {
    this->SelectedImageScaleIndex = 0;
    return 1.0;
    }

  // If we aren't automatically adjusting, then just use the selected
  // level that was supplied in the SelectedImageScaleIndex variable
  if ( !this->AutomaticScaleAdjustment )
    return this->ImageScale[ this->SelectedImageScaleIndex ];

  // Otherwise, adjust the level to get the desired frame rate
  // First, figure out how must time we have to render ( a time of
  // 0.0 means take as long as you like )
  time_to_render = ren->GetAllocatedRenderTime();
  if ( time_to_render == 0.0 ) 
    time_to_render = 10000.0;

  // First test the full res level - is that ok?
  selected_level = 0;
  estimated_time = this->ImageRenderTime[0];

  if ( estimated_time > time_to_render )
    {
    // Full res would take too long - use the adjustable level that is
    // stored in ImageScale[VTK_MAX_VIEW_RAYS_LEVEL]
    selected_level = VTK_MAX_VIEW_RAYS_LEVEL;
    // Only allow the scale to be adjusted every 3 renders to avoid
    // trashing
    if ( this->StableImageScaleCounter > 3 )
      {
	// If we have no render time, estimate the scale from the full
	// res render time. If there is no full res render time (this
	// should not happen!) then just pick 0.1 as the scale as a
	// first guess since we have nothing to base a guess on
	if ( this->ImageRenderTime[1] == 0.0 )
	  {
	  if ( this->ImageRenderTime[0] != 0.0 )
	    estimated_scale = 
	      sqrt( (double)( time_to_render / this->ImageRenderTime[0] ) );
	  else
	    estimated_scale = 0.1;
	  }
	// There is a time for this scale - figure out how far off we
	// are from hitting our desired time
	else
	  estimated_scale = this->ImageScale[selected_level] *
	    sqrt( (double)( time_to_render / this->ImageRenderTime[1] ) );
	// Put some bounds on the scale
	if ( estimated_scale < this->AutomaticScaleLowerLimit ) 
	  estimated_scale = this->AutomaticScaleLowerLimit;
	if ( estimated_scale > 1.0 ) estimated_scale = 1.0;
	// How different is this from what we previously used?
	scale_diff = estimated_scale - this->ImageScale[selected_level];
	if ( scale_diff < 0 ) scale_diff = -scale_diff;
	// Make sure the difference is significant to avoid trashing
	if ( scale_diff > 0.02 )
	  {
	  this->ImageScale[selected_level] = estimated_scale;
	  // Reset the counter to 0 so that we have to wait 3 frames
	  // before we can adjust this scale again
	  this->StableImageScaleCounter = 0;
	  }
	else 
	  // Increment the counter since we didn't adjust the scale
	  this->StableImageScaleCounter++;
      }
    else 
      // Increment the counter since we didn't adjust the scale
      this->StableImageScaleCounter++;
    }
  else
    // We used the full res image so set the counter to a high number
    // so that next time we use the adjustable scale we can recompute
    // a new scale value immediately instead of having to wait 3 frames
    this->StableImageScaleCounter = 10;

  this->SelectedImageScaleIndex = selected_level;

  return this->ImageScale[ this->SelectedImageScaleIndex ];
}

float vtkRayCaster::GetViewportStepSize(vtkRenderer *ren)
  {
    if ( this->SelectedImageScaleIndex >= 0 &&
	 this->SelectedImageScaleIndex < VTK_MAX_VIEW_RAYS_LEVEL )
      return this->ViewRaysStepSize[this->SelectedImageScaleIndex];
    else
      return 1.0;
  }


#ifndef TRUE
#define TRUE        1
#define FALSE       0
#endif

#define VR_NONE         0
#define VR_HARDWARE     1
#define VR_SOFTWARE     2

// Description:
// Main routine to do the volume rendering.
int vtkRayCaster::Render(vtkRenderer *ren)
{
  vtkVolume           *aVolume;
  vtkVolumeCollection *volumes;

  int volume_count = 0;    // Number of visible volumes
  int actor_count = 0;     // Number of visible actors

  int destroy_hw_buffer;   // Specifies if rendering will destroy FB contents
  int image_location;      // Specifies where rendering will place image
  int prev_image_location; // Specifies where previous rendering placed image

  int   *rw_size = NULL;      // Size of render window
  float *vp_size = NULL;      // Size of viewport
  int   img_size[2];          // Size of rendering in pixels
  int   full_img_size[2];

  float *curr_zdata = NULL;   // Current Z Image
  float *curr_cdata = NULL;   // Current RGBA Image

  float *prev_zdata = NULL;   // Previous Z Image
  float *prev_cdata = NULL;   // Previous RGBA Image

  float *full_cdata = NULL;

  float *ccd_ptr;	// Extra Pointers
  float *czd_ptr;

  float *pcd_ptr;
  float *pzd_ptr;

  float alpha, one_m_alpha;

  float *background;

  int	free_prev_data;
  int	free_curr_data;

  int	 i;
  int    vrsize[2];

  vtkTimerLog  *timer;

  timer = vtkTimerLog::New();

  timer->StartTimer();

  actor_count = ren->VisibleActorCount();

  image_location = VR_NONE;

  if ( actor_count )
    prev_image_location = VR_HARDWARE;
  else
    prev_image_location = VR_NONE;

  // Get the background color
  background = ren->GetBackground();

  // Get the physical window dimensions
  rw_size = ren->GetRenderWindow()->GetSize();
  vp_size = ren->GetViewport();

  // Determine the full size of the image - this is the size in pixels
  // of the viewport
  full_img_size[0] = (int)(rw_size[0]*(float)(vp_size[2] - vp_size[0]));
  full_img_size[1] = (int)(rw_size[1]*(float)(vp_size[3] - vp_size[1]));

  // Determine the size of the image that we are going to generate
  // This is also the size of the image that the renderer has rendered
  // for geometric data.  This image will then be rescaled to be the
  // full image size before writing it to the window
  this->GetViewRaysSize( vrsize );
  img_size[0] = vrsize[0];
  img_size[1] = vrsize[1];

  free_prev_data = FALSE;
  free_curr_data = FALSE;

  // Render the volumes
  volumes = this->Renderer->GetVolumes();

  for( volumes->InitTraversal(); (aVolume = volumes->GetNextItem()); )
    {

    // Check visibility of volume and that no other volume has been rendered
    if( aVolume->GetVisibility() && (volume_count == 0) )
      {
      destroy_hw_buffer = 
	(aVolume->GetVolumeMapper())->DestroyHardwareBuffer();
  
      if ( (aVolume->GetVolumeMapper())->ImageLocatedInHardware() )
        image_location = VR_HARDWARE;
      else
        image_location = VR_SOFTWARE;

      // Save color and z images from hardware if necessary
      switch( prev_image_location )
        {
        case VR_NONE:

          prev_cdata = NULL;
          prev_zdata = NULL;

          free_prev_data = FALSE;
          break;

        case VR_HARDWARE:

          if( destroy_hw_buffer )
            {
            // Store the color and zbuffer data
            prev_cdata = ren->GetRenderWindow()->GetRGBAPixelData( 0, 0,
                                          img_size[0]-1, img_size[1]-1, 0 );
            prev_zdata = ren->GetRenderWindow()->GetZbufferData( 0, 0,
                                          img_size[0]-1, img_size[1]-1 );
  
            free_prev_data = TRUE;
            }
          else
            {
            prev_cdata = NULL;
            prev_zdata = NULL;

            free_prev_data = FALSE;
            }
          break;
  
        case VR_SOFTWARE:
  
          prev_cdata = curr_cdata;
          prev_zdata = curr_zdata;
  
          free_prev_data = free_curr_data;
          break;
        }

      if( prev_zdata )
        this->zbuffer = prev_zdata;
      else
        this->zbuffer = NULL;

      if( prev_cdata )
        this->cbuffer = prev_cdata;
      else
        this->cbuffer = NULL;

      //
      // Render the volume
      //
      aVolume->Render( ren );
      volume_count++;

      // If software rendering, get the current image
      if ( image_location == VR_SOFTWARE )
        {
        curr_zdata = aVolume->GetVolumeMapper()->GetZbufferData();
        curr_cdata = aVolume->GetVolumeMapper()->GetRGBAPixelData();
        free_curr_data = FALSE;
        }

      // Merge the rendered images if necessary
      switch ( prev_image_location )
        {
        case VR_NONE:

	  if( background[0] || background[1] || background[2] )
	    {
            // Merge background color into image
            ccd_ptr = curr_cdata;

            for( i=0; i<img_size[0]*img_size[1]; i++ )
              {
              alpha = *(ccd_ptr + 3);
              if( alpha > 0.0 )
                {
                one_m_alpha = 1.0 - alpha;
  	        *(ccd_ptr  ) += background[0]*one_m_alpha;
  	        *(ccd_ptr+1) += background[1]*one_m_alpha;
  	        *(ccd_ptr+2) += background[2]*one_m_alpha;
                *(ccd_ptr+3) = 1.0;
                }
              else
                {
                *(ccd_ptr  ) = background[0];
                *(ccd_ptr+1) = background[1];
                *(ccd_ptr+2) = background[2];
                *(ccd_ptr+3) = 1.0;
                }
              ccd_ptr += 4;
              }
	    }

          break;

        case VR_HARDWARE:
          if( image_location == VR_SOFTWARE )
            {
            // Merge Hardware & Software -> Software

            ccd_ptr = curr_cdata;
            czd_ptr = curr_zdata;

            pcd_ptr = prev_cdata;
            pzd_ptr = prev_zdata;
  
            if( ccd_ptr && pcd_ptr && czd_ptr && pzd_ptr )
              {
              if( 0 && czd_ptr )
                {
                // Perform Software Zbuffering
                for( i=0; i<img_size[0]*img_size[1]; i++ )
                  {
                  if( *czd_ptr < *pzd_ptr )
	            {
		    *(ccd_ptr) = *(pcd_ptr++); ccd_ptr++; 	// R
		    *(ccd_ptr) = *(pcd_ptr++); ccd_ptr++; 	// G
		    *(ccd_ptr) = *(pcd_ptr++); ccd_ptr++; 	// B
                    *(ccd_ptr) = 1.0; ccd_ptr++; pcd_ptr++;	// A
		    }
                  }
                }
              else
               {
                for( i=0; i<img_size[0]*img_size[1]; i++ )
                  {
                  alpha = *(ccd_ptr + 3);
                  if( alpha > 0.0 )
                    {
                    one_m_alpha = 1.0 - alpha;
  	            *(ccd_ptr  ) += *(pcd_ptr  )*one_m_alpha;
  	            *(ccd_ptr+1) += *(pcd_ptr+1)*one_m_alpha;
  	            *(ccd_ptr+2) += *(pcd_ptr+2)*one_m_alpha;
                    *(ccd_ptr+3) = 1.0;
                    }
                  else
                    {
                    *(ccd_ptr  ) = *(pcd_ptr  );
                    *(ccd_ptr+1) = *(pcd_ptr+1);
                    *(ccd_ptr+2) = *(pcd_ptr+2);
                    *(ccd_ptr+3) = 1.0;
                    }
                  ccd_ptr += 4;
                  pcd_ptr += 4;
                  }
                }
              }
  
            }
          break;

        case VR_SOFTWARE:
          if ( image_location == VR_SOFTWARE )
            {
            // Merge Software & Software -> Software
            }
          else if ( image_location == VR_HARDWARE )
            {
            // Merge Software & Hardware -> Software
            }
          break;
        }

      // Clean up from previous image
      if( free_prev_data )
        {
        if( prev_zdata )
          {
          delete( prev_zdata );
          prev_zdata = NULL;
          }
        if( prev_cdata )
          {
          delete( prev_cdata );
          prev_cdata = NULL;
          }
        free_prev_data = FALSE;
        }

      prev_image_location = image_location;

      }
    }

  if ( image_location == VR_SOFTWARE )
    {

    // If the full image size and the volume rendered image size are not 
    // the same, then we are going to need to rescale the image before
    // writing it into the render window
    if ( img_size[0] != full_img_size[0] || img_size[1] != full_img_size[1] )
      {
      // Create the image to write it in to

      // Rescale it.  This also writes it to the render window's 
      // output.
      this->RescaleImage( curr_cdata, img_size);
      }
    else
      {
      // Place final image into frame buffer if necessary - it is the
      // full resolution size so it doesn't need to be rescaled
      ren->GetRenderWindow()->SetRGBAPixelData( 
	0, 0, img_size[0]-1, img_size[1]-1, curr_cdata, 0 );
      }
    }

  // Final clean up
  // If we created a full size image, free the memory
  if ( full_cdata ) delete full_cdata;

  if( free_prev_data )
    {
    if ( prev_zdata ) delete prev_zdata;
    if ( prev_cdata ) delete prev_cdata;
    }

  if( free_curr_data )
    {
    if ( curr_zdata ) delete curr_zdata;
    if ( curr_cdata ) delete curr_cdata;
    }

  timer->StopTimer();
  this->TotalRenderTime = timer->GetElapsedTime();

  if ( this->AutomaticScaleAdjustment )
    {
    if ( this->SelectedImageScaleIndex == 0 )
      this->ImageRenderTime[0] = this->TotalRenderTime;
    else
      this->ImageRenderTime[1] = this->TotalRenderTime;
    }

  delete timer;

  return volume_count;
}

void vtkRayCaster::RescaleImage(float *RGBAImage, int smallSize[2])
{

  int *rw_size;
  float *vp_size;
  int window_size[2];
  float *outputFloat;
  vtkRenderer *ren;

  
  // Is the rendering at full resolution?  If so,
  // Then just copy to the output window

  ren = this->Renderer;

  rw_size = ren->GetRenderWindow()->GetSize();
  vp_size = ren->GetViewport();
  window_size[0] =  (int)(rw_size[0]*(float)(vp_size[2] - vp_size[0]));
  window_size[1] =  (int)(rw_size[1]*(float)(vp_size[3] - vp_size[1]));

  outputFloat = new float[window_size[0]*window_size[1]*4];

  if( this->BilinearImageZoom )
    BilinearZoom( RGBAImage, outputFloat, smallSize, window_size );
  else
    NearestNeighborZoom( RGBAImage, outputFloat, smallSize, window_size );

  ren->GetRenderWindow()->SetRGBAPixelData(0,0,
    window_size[0]-1,window_size[1]-1,outputFloat,0);
  delete[] outputFloat;
}

void vtkRayCaster::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os,indent);

  os << indent << "Bilinear Image Zoom: " << this->BilinearImageZoom << "\n";
}

