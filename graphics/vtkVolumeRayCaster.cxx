/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeRayCaster.cxx
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
#include <math.h>

#include "vtkVolumeRayCaster.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharScalars.h"
#include "vtkUnsignedShortScalars.h"
#include "vtkShortScalars.h"
#include "vtkIntScalars.h"
#include "vtkFloatScalars.h"
#include "vtkRenderer.h"
#include "vtkMath.h"
#include "vtkRenderWindow.h"
#include "vtkRayCaster.h"

// Description:
// Construct a new vtkVolumeRayCaster with default values
vtkVolumeRayCaster::vtkVolumeRayCaster()
{
  this->ViewRays                = NULL;
  this->ViewRaysSize[0]         = 0;
  this->ViewRaysSize[1]         = 0;
  this->RGBAImage               = NULL;
  this->ZImage                  = NULL;
  this->SampleDistance          = 1.0;
  this->InterpolationType       = 0;
  this->ThreadCount             = this->Threader.GetThreadCount();
  this->RayBounder              = NULL;
}

// Description:
// Destruct a vtkVolumeRayCaster - clean up any memory used
vtkVolumeRayCaster::~vtkVolumeRayCaster()
{
  if ( this->RGBAImage )
    delete this->RGBAImage;

  if ( this->ZImage )
    delete this->ZImage;
}

struct vtkVolumeRayCasterInfo
{
  vtkVolumeRayCaster *Caster;
  vtkRenderWindow    *RenderWindow;
};


void vtkVolumeRayCaster::Render( vtkRenderer *ren, vtkVolume *vol )
{
  vtkTimerLog        *timer;
  int                thread_loop;
  vtkVolumeRayCasterInfo info;
  
  // make sure that we have scalar input and update the scalar input
  if ( this->ScalarInput == NULL ) 
    {
    vtkErrorMacro(<< "No ScalarInput!");
    return;
    }
  else
    {
    this->ScalarInput->Update();
    } 

  // Create the time object
  timer = vtkTimerLog::New();
  
  // set up the info object for the thread
  info.Caster = this;
  info.RenderWindow = ren->GetRenderWindow();
  
  // Give the concrete depth parc volume mapper a chance to do any
  // per-render stuff that it may need to do (such as update normals,
  // set up transfer function arrays, etc)
  this->CasterUpdate( ren, vol );

  // Do the Parc rendering.  The near and far buffers are captured here.
  // Also, keep track of how long this takes
  timer->StartTimer();

  if ( this->RayBounder )
    this->DepthRangeBufferPointer = this->RayBounder->GetRayBounds( ren );
  else
    this->DepthRangeBufferPointer = NULL;
  timer->StopTimer();

  this->DrawTime = timer->GetElapsedTime();

  // Render the whole image.  Keep track of how long it takes.
  timer->StartTimer();
  if( ren->GetActiveCamera()->GetParallelProjection() )
    {
    // Do the initialization for this image
    // This is all the stuff that happens before the rays
    // are cast
    this->GeneralImageInitialization( ren, vol );
    this->InitializeParallelImage( ren, vol );

    this->Threader.SetThreadCount( this->ThreadCount );
    this->Threader.SetSingleMethod( RenderParallelImage, (void *)&info);
    this->Threader.SingleMethodExecute();
    }
  else
    {
    // Do the initialization for this image
    // This is all the stuff that happens before the rays
    // are cast
    this->GeneralImageInitialization( ren, vol );
    this->InitializePerspectiveImage( ren, vol );

    this->Threader.SetThreadCount( this->ThreadCount );
    this->Threader.SetSingleMethod( RenderPerspectiveImage, (void *)&info);
    this->Threader.SingleMethodExecute();
    }

  this->TotalRaysCast   = 0;
  this->TotalStepsTaken = 0;
  for ( thread_loop = 0; thread_loop < this->ThreadCount; thread_loop++ )
    {
    this->TotalRaysCast   += this->TotalRaysCastPerId[thread_loop];
    this->TotalStepsTaken += this->TotalStepsTakenPerId[thread_loop];
    }

  timer->StopTimer();

  this->CastTime = timer->GetElapsedTime();

  delete timer;
}

int vtkVolumeRayCaster::ClipRayAgainstVolume( float ray_info[12], 
					      float bound_info[12] )
{
  int    loop;
  float  diff;
  float  t;
  float  *ray_start, *ray_end;
  float  *ray_direction, *unit_ray_direction;
  float  *bounds, *inner_bounds;

  ray_start          = ray_info;
  ray_end            = ray_info + 3;
  ray_direction      = ray_info + 6;
  unit_ray_direction = ray_info + 9;

  bounds       = bound_info;
  inner_bounds = bound_info + 6;

  if ( ray_start[0] >= bounds[3] ||
       ray_start[1] >= bounds[4] ||
       ray_start[2] >= bounds[5] ||
       ray_start[0] < bounds[0] || 
       ray_start[1] < bounds[1] || 
       ray_start[2] < bounds[2] )
    {
    for ( loop = 0; loop < 3; loop++ )
      {
      diff = 0;

      if ( ray_start[loop] < inner_bounds[loop] )
	diff = inner_bounds[loop] - ray_start[loop];
      else if ( ray_start[loop] > inner_bounds[loop+3] )
	diff = inner_bounds[loop+3] - ray_start[loop];
      
      if ( diff )
	{
	if ( unit_ray_direction[loop] != 0.0 ) 
	  t = diff / unit_ray_direction[loop];
	else t = -1.0;
	
	if ( t > 0.0 )
	  {
	  ray_start[0] += unit_ray_direction[0] * t;
	  ray_start[1] += unit_ray_direction[1] * t;
	  ray_start[2] += unit_ray_direction[2] * t;
	  	  
	  }
	}
      }
    }

  // If the voxel still isn't inside the volume, then this ray
  // doesn't really intersect the volume
	  
  if ( ray_start[0] >= bounds[3] ||
       ray_start[1] >= bounds[4] ||
       ray_start[2] >= bounds[5] ||
       ray_start[0] < bounds[0] || 
       ray_start[1] < bounds[1] || 
       ray_start[2] < bounds[2] )
    {
    return 0;
    }

  // The ray does intersect the volume, and we have a starting
  // position that is inside the volume
  if ( ray_end[0] >= bounds[3] ||
       ray_end[1] >= bounds[4] ||
       ray_end[2] >= bounds[5] ||
       ray_end[0] < bounds[0] || 
       ray_end[1] < bounds[1] || 
       ray_end[2] < bounds[2] )
    {
    for ( loop = 0; loop < 3; loop++ )
      {
      diff = 0;
      
      if ( ray_end[loop] < inner_bounds[loop] )
	diff = inner_bounds[loop] - ray_end[loop];
      else if ( ray_end[loop] > inner_bounds[loop+3] )
	diff = inner_bounds[loop+3] - ray_end[loop];
      
      if ( diff )
	{
	if ( unit_ray_direction[loop] != 0.0 ) 
	  t = diff / unit_ray_direction[loop];
	else t = 1.0;
	
	if ( t < 0.0 )
	  {
	  ray_end[0] += unit_ray_direction[0] * t;
	  ray_end[1] += unit_ray_direction[1] * t;
	  ray_end[2] += unit_ray_direction[2] * t;
	  }
	}
      }
    }
  
  if ( ray_end[0] >= bounds[3] ||
       ray_end[1] >= bounds[4] ||
       ray_end[2] >= bounds[5] ||
       ray_end[0] < bounds[0] || 
       ray_end[1] < bounds[1] || 
       ray_end[2] < bounds[2] )
    {
      return 0;
    }
    
  return 1;
}

void vtkVolumeRayCaster::GeneralImageInitialization( vtkRenderer *ren, 
						     vtkVolume *vol )
{
  vtkTransform           *transform;
  vtkTransform           *scalar_transform;
  vtkRayCaster           *ray_caster;
  float                  spacing[3], data_origin[3];
  char                   *data_type;
  unsigned char          *uc_data_ptr;
  unsigned short         *us_data_ptr;
  short                  *s_data_ptr;
  int                    *i_data_ptr;
  float                  *f_data_ptr;

  // Create some objects that we will need later
  transform        = vtkTransform::New();
  scalar_transform = vtkTransform::New();

  // Get a pointer to the volume renderer from the renderer
  ray_caster = ren->GetRayCaster();


  // Get the view rays and the dimensions of the view rays
  ray_caster->GetViewRaysSize( this->ViewRaysSize );

  // Compute the transformation that will map the view rays (currently
  // in camera coordinates) into volume coordinates.
  // First, get the active camera transformation matrix
  this->ViewRaysTransform.SetMatrix(
    ren->GetActiveCamera()->GetViewTransform() );

  // Now invert it so that we go from camera to world instead of
  // world to camera coordinates
  this->ViewRaysTransform.Inverse();

  // Store the matrix of the volume in a temporary transformation matrix
  transform->SetMatrix( vol->vtkProp::GetMatrix() );

  // Get the origin of the data.  This translation is not accounted for in
  // the volume's matrix, so we must add it in.
  this->ScalarInput->GetOrigin( data_origin );

  // Get the data spacing.  This scaling is not accounted for in
  // the volume's matrix, so we must add it in.
  this->ScalarInput->GetSpacing( spacing );

  // Create a transform that will account for the scaling and translation of
  // the scalar data
  scalar_transform->Identity();
  scalar_transform->Translate(data_origin[0], data_origin[1], data_origin[2]);
  scalar_transform->Scale( spacing[0], spacing[1], spacing[2] );

  // Now concatenate the volume's matrix with this scalar data matrix
  transform->PostMultiply();
  transform->Concatenate( scalar_transform->GetMatrix() );

  // Invert this matrix so that we have world to volume instead of
  // volume to world coordinates
  transform->Inverse();

  // Now concatenate the camera to world matrix with the world to volume
  // matrix to get the camera to volume matrix
  this->ViewRaysTransform.PostMultiply();
  this->ViewRaysTransform.Concatenate( transform->GetMatrix() );

  // Get the clipping range of the active camera. This will be used
  // for clipping the rays
  ren->GetActiveCamera()->GetClippingRange( this->CameraClippingRange );

  // Get the size of the data for limit checks and compute increments
  this->ScalarInput->GetDimensions( this->ScalarDataSize );
  this->DataIncrement[2] = this->ScalarDataSize[0] * this->ScalarDataSize[1];
  this->DataIncrement[1] = this->ScalarDataSize[0];
  this->DataIncrement[0] = 1;

  // In order to keep the value computed along a ray independent of the
  // scale of the volume, we multiply the sampling distance by the scale
  // of the volume.  This means that this->SampleDistance is in unscaled
  // world coordinates
  this->WorldSampleDistance = 
    this->SampleDistance * ray_caster->GetViewportStepSize(ren) * vol->GetScale();

  // Delete the objects we created
  transform->Delete();
  scalar_transform->Delete();

  // If there'a a color image already allocated, delete it.  Create a new
  // image. This image is RGBA in floats.
  if ( this->RGBAImage )
    delete this->RGBAImage;
  this->RGBAImage = new float[ this->ViewRaysSize[0] * 
			       this->ViewRaysSize[1] * 4 ];

  if ( this->ZImage )
    delete this->ZImage;
  
  // If there is a depth image already allocated, delete it. Create a new
  // depth image.  The depth image is z values in perspective coordinates
  // as floats.
  this->ZImage = new float[ this->ViewRaysSize[0] * this->ViewRaysSize[1] ];

  // Get the previous zbuffer data
  this->RenderZData = ren->GetRayCaster()->GetCurrentZBuffer();

  // Get the data type and set the data_switch_value accordingly.  This way,
  // we only need to switch on an int instead of doing a string comparison
  // during the image computation.  Also, set the correct data pointer.
  data_type = this->ScalarInput->GetPointData()->GetScalars()->GetDataType();
  if ( strcmp( data_type, "unsigned char" ) == 0 )
    {
    uc_data_ptr = ((vtkUnsignedCharScalars *)
      (this->ScalarInput->GetPointData()->GetScalars()))->GetPointer(0);
    this->ScalarDataPointer = (void *)uc_data_ptr;
    this->ScalarDataType = 0;
    }
  else if ( strcmp( data_type, "unsigned short" ) == 0 )
    {
    us_data_ptr = ((vtkUnsignedShortScalars *)
      (this->ScalarInput->GetPointData()->GetScalars()))->GetPointer(0);
    this->ScalarDataPointer = (void *)us_data_ptr;
    this->ScalarDataType = 1;
    }
  else if ( strcmp( data_type, "short" ) == 0 )
    {
    s_data_ptr = ((vtkShortScalars *)
      (this->ScalarInput->GetPointData()->GetScalars()))->GetPointer(0);
    this->ScalarDataPointer = (void *)s_data_ptr;
    this->ScalarDataType = 2;
    }
  else if ( strcmp( data_type, "int" ) == 0 )
    {
    i_data_ptr = ((vtkIntScalars *)
      (this->ScalarInput->GetPointData()->GetScalars()))->GetPointer(0);
    this->ScalarDataPointer = (void *)i_data_ptr;
    this->ScalarDataType = 3;
    }
  else if ( strcmp( data_type, "float" ) == 0 )
    {
    f_data_ptr = ((vtkFloatScalars *)
      (this->ScalarInput->GetPointData()->GetScalars()))->GetPointer(0);
    this->ScalarDataPointer = (void *)f_data_ptr;
    this->ScalarDataType = 4;
    }
  else
    {
    vtkErrorMacro( << "I don't know what type of data this is: " << 
                   data_type );
    }

}

// Description:
// Initialize casting of parallel rays. This includes setting up the
// matrix for transforming rays from camera space to volume space. It
// also includes setting up all additional information (such as near and
// far z buffer pointers, the renderer's z buffer pointer, some data
// sizes, etc) so that the RenderParallelImage and CastARay calls are
// reentrant (in fact, they do not rely on any information except that
// contained in private instance variables of this depth parc mapper)
void vtkVolumeRayCaster::InitializeParallelImage( vtkRenderer *ren, 
						  vtkVolume *vol )
{
  float                  in[4];
  float                  ray_direction[4], ray_origin[4];
  float                  start_ray[4];
  float                  origin_inc_x[4], origin_inc_y[4];
  vtkTransform           *transform;
  vtkMatrix4x4           *matrix;
  float                  ren_aspect[2], aspect;
  float                  norm;
  float                  *start_pos_ptr;
  float                  *pos_inc_ptr;
  float                  p_scale;
  vtkRayCaster           *ray_caster;


  // Create some objects that we will need later
  transform        = vtkTransform::New();
  matrix           = vtkMatrix4x4::New();

  // Get a pointer to the volume renderer from the renderer
  ray_caster = ren->GetRayCaster();

  ren->GetAspect( ren_aspect );
  aspect = ren_aspect[0]/ren_aspect[1];

  // Create the perspective matrix for the camera.  This will be used
  // to decode z values, so we will need to invert it
  transform->SetMatrix( ren->GetActiveCamera()->GetPerspectiveTransform(
    aspect, -1, 1 ) );
  transform->Inverse();

  // To speed things up, we pull the matrix out of the transform. 
  // This way, we can decode z values faster since we know which elements
  // of the matrix are important, and which are zero.
  transform->GetMatrix( *matrix );

  // Just checking that our assumptions are correct.  This code should
  // be removed after the debugging phase is complete
  if (  matrix->Element[3][0] || matrix->Element[3][1]  ||
        matrix->Element[3][2] || (matrix->Element[3][3] != 1.0) )
    {
    vtkErrorMacro( << "Oh no! They aren't 0 like they are supposed to be!");
    cout << *transform;
    }

  // This is the important element of the matrix.  We will decode
  // z values by : ((zbuffer value)*ParallelZFactor)
  this->ParallelZScale = matrix->Element[2][2];
  this->ParallelZBias  = matrix->Element[2][3];

  p_scale = ren->GetActiveCamera()->GetParallelScale();

  // Pass the ray direction through the ViewRaysTransform to convert it
  // from camera to volume coordinates
  in[0] = in[1] = in[2] = 0.0;
  in[3] = 1.0;
  this->ViewRaysTransform.MultiplyPoint( in, ray_origin );

  in[0] = in[1] = 0.0;
  in[2] = -1.0;
  in[3] = 1.0;
  this->ViewRaysTransform.MultiplyPoint( in, ray_direction );
  ray_direction[0] -= ray_origin[0];
  ray_direction[1] -= ray_origin[1];
  ray_direction[2] -= ray_origin[2];

  memcpy( this->LocalRayDirection, ray_direction, 3 * sizeof( float ) );

  // Normalize the ray
  norm = vtkMath::Normalize (ray_direction); // voxel coordinates

  memcpy( this->LocalUnitRayDirection, ray_direction, 3 * sizeof( float ) );

  // Pass the start ray origin (bottom left corner) through the
  // ViewRaysTransform to convert it from camera to volume coordinates
  start_pos_ptr = ray_caster->GetParallelStartPosition();
  memcpy( in, start_pos_ptr, 3 * sizeof( float ) );
  in[3] = 1.0;
  this->ViewRaysTransform.MultiplyPoint( in, start_ray );

  // Determine increment to move to next pixel along a row
  pos_inc_ptr = ray_caster->GetParallelIncrements();
  in[0] = start_pos_ptr[0] + pos_inc_ptr[0];
  this->ViewRaysTransform.MultiplyPoint( in, origin_inc_x );
  origin_inc_x[0] -= start_ray[0];
  origin_inc_x[1] -= start_ray[1];
  origin_inc_x[2] -= start_ray[2];

  // Determine increment to move to next pixel along a column
  in[0] = start_pos_ptr[0];
  in[1] = start_pos_ptr[1] + pos_inc_ptr[1];
  this->ViewRaysTransform.MultiplyPoint( in, origin_inc_y );

  origin_inc_y[0] -= start_ray[0];
  origin_inc_y[1] -= start_ray[1];
  origin_inc_y[2] -= start_ray[2];

  memcpy( this->LocalRayStart,    start_ray,    3 * sizeof( float ) );
  memcpy( this->XOriginIncrement, origin_inc_x, 3 * sizeof( float ) );
  memcpy( this->YOriginIncrement, origin_inc_y, 3 * sizeof( float ) );

  // Delete the objects we created
  transform->Delete();
  matrix->Delete();

}

// Description:
// Cast a ray for each pixel in the image plane using a parallel viewing
// transform.  The rays are obtained from the vtkRenderer and they define 
// the size of the image to be computed. At the end of this, we have an 
// RGBAImage and a ZImage.  This is a friend function and not a member
// function because it may be an argument to pthread_create or sproc for
// multi-threading.  Each row j of the image is computed if 
// j % thread_count is equal to the thread_id given to this routine.
// For thread_count = 1, the entire image will be computed by
// thread_id = 0. If, for example, this->ThreadCount = 2, the even rows
// will be computed by thread_id = 0 and the odd rows will be computed by
// thread_id = 1.
VTK_THREAD_RETURN_TYPE RenderParallelImage( void *arg )
{
  int                  i, j, last_i;
  float                unit_ray_direction[3], ray_origin[3];
  float                ray_direction[3];
  float                start_ray[4];
  float                *iptr;
  float                *zptr;
  int                  num_samples;
  float                near_z, far_z, tmp_z;
  float                zscale, zbias;
  float                *z_range_ptr;
  float                world_sample_distance;
  float                ray_increment[3];
  float                pixel_value[6];
  float                bounds[12];
  float		       *pixel_offset_x;
  float		       *pixel_offset_y;
  float		       *pixel_offset_z;
  float                *ren_z_ptr = NULL;
  int                  thread_id;
  int                  thread_count;
  vtkVolumeRayCaster   *mapper;
  int                  largest_increment_index;
  float                clipping_range[2];
  float                ray_info[12];
  int                  noAbort = 1;
  vtkRenderWindow      *renWin;
  
  // Get the info out of the input structure
  thread_id = ((ThreadInfoStruct *)(arg))->ThreadID;
  thread_count = ((ThreadInfoStruct *)(arg))->ThreadCount;
  mapper = ((vtkVolumeRayCasterInfo *)
	    ((ThreadInfoStruct *)arg)->UserData)->Caster;
  renWin = ((vtkVolumeRayCasterInfo *)
	    ((ThreadInfoStruct *)arg)->UserData)->RenderWindow;

  // Initialize some statistics
  mapper->TotalRaysCastPerId[thread_id]   = 0;
  mapper->TotalStepsTakenPerId[thread_id] = 0;

  // Pull some info out of instance variables into local variables

  // These are the pointers to the image (RGBA and Z) and all the
  // z buffers
  iptr = mapper->RGBAImage;
  zptr = mapper->ZImage;
  z_range_ptr = mapper->DepthRangeBufferPointer;
  ren_z_ptr = mapper->RenderZData;

  // This is the scale factor used to convert z buffer values to 
  // z distance values
  zscale = mapper->ParallelZScale;
  zbias  = mapper->ParallelZBias;

  // This is the length of the ray in the local volume space
  world_sample_distance = mapper->WorldSampleDistance;

  // This is the unit direction of the ray in the local volume space
  memcpy( unit_ray_direction, mapper->LocalUnitRayDirection, 
	  3 * sizeof( float ) );

  // This is the direction of the ray in the local volume space 
  memcpy( ray_direction, mapper->LocalRayDirection, 3 * sizeof( float ) );

  // The clipping range of the camera is used to clip the rays
  memcpy( clipping_range, mapper->CameraClippingRange, 2 * sizeof( float) );

  memcpy( ray_info + 6, ray_direction, 3 * sizeof( float ) );
  memcpy( ray_info + 9, unit_ray_direction, 3 * sizeof( float ) );

  // This is the local ray start
  start_ray[0] = mapper->LocalRayStart[0];
  start_ray[1] = mapper->LocalRayStart[1];
  start_ray[2] = mapper->LocalRayStart[2];

  // Compute the ray increments in x, y, and z 
  // accounted for interaction scale, 
  // volume scale, and word/volume transformation
  ray_increment[0] = ray_direction[0] * world_sample_distance;
  ray_increment[1] = ray_direction[1] * world_sample_distance;
  ray_increment[2] = ray_direction[2] * world_sample_distance;

  if ( fabs((double) ray_increment[0]) >= fabs((double) ray_increment[1]) &&
       fabs((double) ray_increment[0]) >= fabs((double) ray_increment[2]) )
    largest_increment_index = 0;
  else if (fabs((double) ray_increment[1]) >= fabs((double) ray_increment[2]))
    largest_increment_index = 1;
  else
    largest_increment_index = 2;

  // Compute ray origin offsets for a scan line
  pixel_offset_x = new float[(mapper->ViewRaysSize[0])];
  pixel_offset_y = new float[(mapper->ViewRaysSize[0])];
  pixel_offset_z = new float[(mapper->ViewRaysSize[0])];

  for( i = 0; i < mapper->ViewRaysSize[0]; i++ )
    {
    pixel_offset_x[i] = mapper->XOriginIncrement[0] * (float)i;
    pixel_offset_y[i] = mapper->XOriginIncrement[1] * (float)i;
    pixel_offset_z[i] = mapper->XOriginIncrement[2] * (float)i;
    }

  // Set the bounds of the volume
  for ( i = 0; i < 3; i++ )
    {
    bounds[i] = 0.0;
    bounds[i+3] = mapper->ScalarDataSize[i] - 1;
    }

  if ( mapper->Clipping )
    {
    for ( i = 0; i < 3; i++ )
      {
      if ( mapper->ClippingPlanes[i] > bounds[i] )
	bounds[i] = mapper->ClippingPlanes[i];
      if ( mapper->ClippingPlanes[i+3] < bounds[i+3] )
	bounds[i+3] = mapper->ClippingPlanes[i+3];
      }
    }

  for ( i = 0; i < 3; i++ )
    {
    bounds[i+6] = bounds[i]   + 0.001;
    bounds[i+9] = bounds[i+3] - 0.001;
    }

  // Loop through all pixels and cast rays where necessary
  for ( j = 0; j < mapper->ViewRaysSize[1]; j++ )
    {
    // If we want this row to be computed by this thread_id
    // also need to check on abort status
    if (!thread_id)
      {
      if (noAbort && renWin->CheckAbortStatus())
	{
	noAbort = 0;
	}
      }
    else
      {
      noAbort = !(renWin->GetAbortRender());
      }
    if (noAbort && (( j % thread_count ) == thread_id ))
      {
      ray_origin[0] = start_ray[0] + (float)j * mapper->YOriginIncrement[0];
      ray_origin[1] = start_ray[1] + (float)j * mapper->YOriginIncrement[1];
      ray_origin[2] = start_ray[2] + (float)j * mapper->YOriginIncrement[2];

      last_i = 0;

      for ( i = 0; i < mapper->ViewRaysSize[0]; i++ )
	{
	// If there is no z range buffer or it points to a value other than
	// 0.0 for this pixel, then we need to cast this ray
	if ( z_range_ptr == NULL || *z_range_ptr > 0.0 )
	  {

	  ray_origin[0] += pixel_offset_x[(i-last_i)];
	  ray_origin[1] += pixel_offset_y[(i-last_i)];
	  ray_origin[2] += pixel_offset_z[(i-last_i)];
	  last_i = i;
	  
	  // Decode the near zbuffer values at this pixel
	  // Add/Subtract one step to account for precision errors
	  near_z = clipping_range[0];
	  if ( z_range_ptr )
	    {
	    if ( *z_range_ptr > near_z )
	      near_z = *z_range_ptr;
	    z_range_ptr++;
	    }

	  far_z = clipping_range[1];
	  if ( ren_z_ptr )
	    {
	    tmp_z = -(((*ren_z_ptr)*2.0 -1.0) * zscale + zbias);
	    if ( tmp_z < far_z )
	      far_z = tmp_z;
	    }
	    
	  if ( z_range_ptr )
	    {
	    if ( *z_range_ptr < far_z )
	      far_z = *z_range_ptr;
	    
	    z_range_ptr++;
	    }

	  // Compute the initial position along the ray
	  ray_info[0] = ray_origin[0] + near_z * ray_direction[0];
	  ray_info[1] = ray_origin[1] + near_z * ray_direction[1];
	  ray_info[2] = ray_origin[2] + near_z * ray_direction[2];

	  ray_info[3] = ray_origin[0] + far_z * ray_direction[0];
	  ray_info[4] = ray_origin[1] + far_z * ray_direction[1];
	  ray_info[5] = ray_origin[2] + far_z * ray_direction[2];

	  if ( mapper->ClipRayAgainstVolume( ray_info, bounds ) )
	    {
	    num_samples = (int)( ( ray_info[3+largest_increment_index] - 
				   ray_info[largest_increment_index] ) /
				 ray_increment[largest_increment_index] ) + 1;

	    mapper->TotalRaysCastPerId[thread_id]++;
	    
	    mapper->CastARay( mapper->ScalarDataType, 
			      mapper->ScalarDataPointer,
			      ray_info, ray_increment, 
			      num_samples, pixel_value );
	    
	    // Set the pixel value to the value returned by the ray cast
	    *(iptr++) = pixel_value[0];
	    *(iptr++) = pixel_value[1];
	    *(iptr++) = pixel_value[2];
	    *(iptr++) = pixel_value[3];
	    *(zptr++) = pixel_value[4];

	    // Increment the number of samples taken
	    mapper->TotalStepsTakenPerId[thread_id] += (int)pixel_value[5];
	    
	    }
	  else
	    {
	    *(iptr++) = 0.0;
	    *(iptr++) = 0.0;
	    *(iptr++) = 0.0;
	    *(iptr++) = 0.0;
	    *(zptr++) = 1.0;
	    }
	  }
	// The near value was 1.0 so we didn't intersect the volume
	else
	  {
	  *(iptr++) = 0.0;
	  *(iptr++) = 0.0;
	  *(iptr++) = 0.0;
	  *(iptr++) = 0.0;
	  *(zptr++) = 1.0;
	  
	  if ( z_range_ptr )
	    z_range_ptr += 2;
	  }
	
	
	if( ren_z_ptr )
	  ren_z_ptr++;
	}
      }
    else  // This is the wrong thread to compute this row of the image
      {
      iptr        += 4 * mapper->ViewRaysSize[0];
      zptr        +=     mapper->ViewRaysSize[0];

      if ( z_range_ptr )
	z_range_ptr += 2 * mapper->ViewRaysSize[0];

      if ( ren_z_ptr )
	ren_z_ptr +=  mapper->ViewRaysSize[0];
      }
    } // For each pixel loop

  // Delete the objects we created
  delete pixel_offset_x;
  delete pixel_offset_y;
  delete pixel_offset_z;

  // Bogus return statement because the SUN compiler won't compile this
  // code unless I return something
  return VTK_THREAD_RETURN_VALUE;
}

// Description:
// Cast a ray for each pixel in the image plane.  The rays are obtained
// from the vtkRenderer and they define the size of the image to be
// computed.  At the end of this, we have an RGBAImage and a ZImage.
void vtkVolumeRayCaster::InitializePerspectiveImage( vtkRenderer *ren, 
						     vtkVolume *vol )
{
  float                  in[4];
  vtkTransform           *transform;
  vtkMatrix4x4           *matrix;
  float                  ren_aspect[2], aspect;
  vtkRayCaster           *ray_caster;

  // Create some objects that we will need later
  transform       = vtkTransform::New();
  matrix          = vtkMatrix4x4::New();

  // Get a pointer to the volume renderer from the renderer
  ray_caster = ren->GetRayCaster();

  // Get the precomputed view rays since we are using a perspective transform
  this->ViewRays = ray_caster->GetPerspectiveViewRays();

  // Get the aspect ratio of the renderer
  ren->GetAspect( ren_aspect );
  aspect = ren_aspect[0]/ren_aspect[1];

  // Create the perspective matrix for the camera.  This will be used
  // to decode z values, so we will need to invert it
  transform->SetMatrix( ren->GetActiveCamera()->GetPerspectiveTransform(
    aspect, -1, 1 ) );
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
      vtkErrorMacro( << "Oh no! They aren't 0 like they are supposed to be!");
    }

  // These are the important elements of the matrix.  We will decode
  // z values by taking the znum1 and dividing by the zbuffer z value times
  // zdenom1 plus zdenom2.
  this->ZNumerator   = matrix->Element[2][3];
  this->ZDenomMult   = matrix->Element[3][2];
  this->ZDenomAdd    = matrix->Element[3][3];

  // Pass the ray origin through the ViewRaysTransform to convert it
  // from camera to volume coordinates
  in[0] = in[1] = in[2] = 0.0;
  in[3] = 1.0;
  this->ViewRaysTransform.MultiplyPoint( in, this->LocalRayOrigin );

  // Delete the objects we created
  transform->Delete();
  matrix->Delete();

}

// Description:
// Cast a ray for each pixel in the image plane.  The rays are obtained
// from the vtkRenderer and they define the size of the image to be
// computed.  At the end of this, we have an RGBAImage and a ZImage.
VTK_THREAD_RETURN_TYPE RenderPerspectiveImage( void *arg )
{
  int                   i, j;
  float                 *ray_ptr, in[4];
  float                 ray_direction[4], ray_origin[4];
  float                 unit_ray_direction[3];
  int                   largest_increment_index;
  float                 *iptr;
  float                 *zptr;
  int                   num_samples;
  float                 *z_range_ptr;
  float                 znum1, zdenom1, zdenom2;
  float                 near_z, far_z, tmp_z;
  float                 ray_increment[3];
  float                 pixel_value[6];
  float                 *ren_z_ptr = NULL;
  int                   thread_id;
  int                   thread_count;
  vtkVolumeRayCaster    *mapper;
  float                 world_sample_distance;
  float                 bounds[12];
  float                 ray_info[12];
  float                 clipping_range[2];
  int                   noAbort = 1;
  vtkRenderWindow       *renWin;

  // Get the info out of the input structure
  thread_id = ((ThreadInfoStruct *)(arg))->ThreadID;
  thread_count = ((ThreadInfoStruct *)(arg))->ThreadCount;
  mapper = ((vtkVolumeRayCasterInfo *)
	    ((ThreadInfoStruct *)arg)->UserData)->Caster;
  renWin = ((vtkVolumeRayCasterInfo *)
	    ((ThreadInfoStruct *)arg)->UserData)->RenderWindow;

  // Initialize some statistics
  mapper->TotalRaysCastPerId[thread_id]   = 0;
  mapper->TotalStepsTakenPerId[thread_id] = 0;

  // Pull some info out of instance variables into local variables

  // These are the pointers to the image (RGBA and Z) and all the
  // z buffers
  iptr        = mapper->RGBAImage;
  zptr        = mapper->ZImage;
  z_range_ptr = mapper->DepthRangeBufferPointer;
  ren_z_ptr   = mapper->RenderZData;

  // These are the values used to  convert z buffer values to 
  // z distance values
  znum1   = mapper->ZNumerator;
  zdenom1 = mapper->ZDenomMult;
  zdenom2 = mapper->ZDenomAdd;

  // This is the length of the ray in the local volume space
  world_sample_distance = mapper->WorldSampleDistance;

  ray_ptr = mapper->ViewRays;

  memcpy( ray_origin, mapper->LocalRayOrigin, 3 * sizeof(float) );

  // The clipping range of the camera is used to clip the rays
  memcpy( clipping_range, mapper->CameraClippingRange, 2 * sizeof( float) );

  // Set the bounds of the volume
  for ( i = 0; i < 3; i++ )
    {
    bounds[i] = 0.0;
    bounds[i+3] = mapper->ScalarDataSize[i] - 1;
    }

  if ( mapper->Clipping )
    {
    for ( i = 0; i < 3; i++ )
      {
      if ( mapper->ClippingPlanes[i] > bounds[i] )
	bounds[i] = mapper->ClippingPlanes[i];
      if ( mapper->ClippingPlanes[i+3] < bounds[i+3] )
	bounds[i+3] = mapper->ClippingPlanes[i+3];
      }
    }

  for ( i = 0; i < 3; i++ )
    {
    bounds[i+6] = bounds[i]   + 0.001;
    bounds[i+9] = bounds[i+3] - 0.001;
    }

  // Loop through all pixel    
  for ( j = 0; j < mapper->ViewRaysSize[1]; j++ )
    {
    // If we want this row to be computed by this thread_id
    // also need to check on abort status
    if (!thread_id)
      {
      if (noAbort && renWin->CheckAbortStatus())
	{
	noAbort = 0;
	}
      }
    else
      {
      noAbort = !(renWin->GetAbortRender());
      }
    if (noAbort && (( j % thread_count ) == thread_id ))
      {
      for ( i = 0; i < mapper->ViewRaysSize[0]; i++ )
	{
	// If there is no z range buffer or it points to a value other than
	// 0.0 for this pixel, then we need to cast this ray
	if ( z_range_ptr == NULL || *z_range_ptr > 0.0 ) 
	  {
	  // Transform this ray into volume coordinates
	  in[0] = *(ray_ptr++);
	  in[1] = *(ray_ptr++);
	  in[2] = *(ray_ptr++);
	  in[3] = 1.0;
	  mapper->ViewRaysTransform.MultiplyPoint( in, ray_direction );
	  
	  ray_direction[0] -= ray_origin[0];
	  ray_direction[1] -= ray_origin[1];
	  ray_direction[2] -= ray_origin[2];
	  
	  memcpy( unit_ray_direction, ray_direction, 3 * sizeof(float) );

	  vtkMath::Normalize (unit_ray_direction);

	  memcpy( ray_info + 6, ray_direction, 3 * sizeof( float ) );
	  memcpy( ray_info + 9, unit_ray_direction, 3 * sizeof( float ) );

	  // Compute the ray increments in x, y, and z 
	  // accounted for interaction scale, 
	  // volume scale, and word/volume transformation
	  ray_increment[0] = ray_direction[0] * world_sample_distance;
	  ray_increment[1] = ray_direction[1] * world_sample_distance;
	  ray_increment[2] = ray_direction[2] * world_sample_distance;

	  if ( fabs((double) ray_increment[0]) >= 
	       fabs((double) ray_increment[1]) &&
	       fabs((double) ray_increment[0]) >= 
	       fabs((double) ray_increment[2]) )
	    largest_increment_index = 0;
	  else if (fabs((double) ray_increment[1]) >= 
		   fabs((double) ray_increment[2]))
	    largest_increment_index = 1;
	  else
	    largest_increment_index = 2;

	  // Decode the near zbuffer values at this pixel
	  // Add/Subtract one step to account for precision errors
	  near_z = clipping_range[0];
	  if ( z_range_ptr )
	    {
	    if ( *z_range_ptr > near_z )
	      near_z = *z_range_ptr;
	    z_range_ptr++;
	    }

	  far_z = clipping_range[1];
	  if ( ren_z_ptr )
	    {
	    tmp_z  = (-znum1 / ( ((*ren_z_ptr)*2.0 -1.0)  *
				 zdenom1 + zdenom2 ))/(-(*(ray_ptr-1)));
	    if ( tmp_z < far_z )
	      far_z = tmp_z;
	    }
	    
	  if ( z_range_ptr )
	    {
	    if ( *z_range_ptr < far_z )
	      far_z = *z_range_ptr;
	    
	    z_range_ptr++;
	    }

	  // Compute the initial position along the ray
	  ray_info[0] = ray_origin[0] + near_z * ray_direction[0];
	  ray_info[1] = ray_origin[1] + near_z * ray_direction[1];
	  ray_info[2] = ray_origin[2] + near_z * ray_direction[2];

	  ray_info[3] = ray_origin[0] + far_z * ray_direction[0];
	  ray_info[4] = ray_origin[1] + far_z * ray_direction[1];
	  ray_info[5] = ray_origin[2] + far_z * ray_direction[2];

	  if ( mapper->ClipRayAgainstVolume( ray_info, bounds ) )
	    {
  	    num_samples = (int)( ( ray_info[3+largest_increment_index] - 
				   ray_info[largest_increment_index] ) /
				 ray_increment[largest_increment_index] ) + 1;

	    mapper->TotalRaysCastPerId[thread_id]++;
	    
	    mapper->CastARay( mapper->ScalarDataType, 
			      mapper->ScalarDataPointer,
			      ray_info, 
			      ray_increment, 
			      num_samples, pixel_value );
	    
	    // Increment the number of samples taken
	    mapper->TotalStepsTakenPerId[thread_id] += (int)pixel_value[5];
	    
	    // Set the pixel value to the value returned by the ray cast
	    memcpy( iptr, pixel_value, 4*sizeof(float) );
	    iptr+=4;
	    *(zptr++) = pixel_value[4];
	    
	    }
	  else
	    {
	    *(iptr++) = 0.0;
	    *(iptr++) = 0.0;
	    *(iptr++) = 0.0;
	    *(iptr++) = 0.0;
	    *(zptr++) = 1.0;
	    }
	  }
	// The near value was < 0 so we didn't intersect the volume
	else
	  {
	  *(iptr++) = 0.0;
	  *(iptr++) = 0.0;
	  *(iptr++) = 0.0;
	  *(iptr++) = 0.0;
	  *(zptr++) = 1.0;

	  if ( z_range_ptr )
	    z_range_ptr += 2;

	  ray_ptr += 3;
	  }
	
	
	if( ren_z_ptr )
	  ren_z_ptr++;
	}
      }
    else  // This is the wrong thread to compute this row of the image
      {
      ray_ptr     += 3 * mapper->ViewRaysSize[0];
      iptr        += 4 * mapper->ViewRaysSize[0];
      zptr        +=     mapper->ViewRaysSize[0];
      if ( z_range_ptr )
	z_range_ptr += 2 * mapper->ViewRaysSize[0];
      if ( ren_z_ptr )
	ren_z_ptr +=  mapper->ViewRaysSize[0];
      }

    } // For each pixel loop

  // Bogus return statement because the SUN compiler won't compile this
  // code unless I return something
  return VTK_THREAD_RETURN_VALUE;

}

// Description:
// Print method for vtkVolumeRayCaster
void vtkVolumeRayCaster::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Sample Distance: " << this->SampleDistance << "\n";

  os << indent << "Interpolation Type: " << this->GetInterpolationType() 
    << "\n";

  os << indent << "Thread Count: " << this->ThreadCount << "\n";

  os << indent << "Total Steps Taken: " << this->TotalStepsTaken << "\n";

  os << indent << "Total Rays Cast: " << this->TotalRaysCast << "\n";

  os << indent << "Time To Draw: " << this->DrawTime << "\n";

  os << indent << "Time To Ray Cast: " << this->CastTime << "\n";

  vtkVolumeMapper::PrintSelf(os,indent);
}

