/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeRayCastMapper.cxx
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

#include "vtkVolumeRayCastMapper.h"
#include "vtkTimerLog.h"
#include "vtkRenderer.h"
#include "vtkMath.h"
#include "vtkRenderWindow.h"
#include "vtkRayCaster.h"
#include "vtkVolumeRayCastFunction.h"
#include "vtkRecursiveSphereDirectionEncoder.h"
#include "vtkFiniteDifferenceGradientEstimator.h"

// Description:
// Construct a new vtkVolumeRayCastMapper with default values
vtkVolumeRayCastMapper::vtkVolumeRayCastMapper()
{
  this->ViewRays                      = NULL;
  this->ViewRaysSize[0]               = 0;
  this->ViewRaysSize[1]               = 0;
  this->RGBAImage                     = NULL;
  this->ZImage                        = NULL;
  this->SampleDistance                = 1.0;
  this->RayBounder                    = NULL;
  this->VolumeRayCastFunction         = NULL;
  this->ScalarOpacityTFArray          = NULL;
  this->RGBTFArray                    = NULL;
  this->GrayTFArray                   = NULL;
  this->CorrectedScalarOpacityTFArray = NULL;
  this->CorrectedStepSize             = -1;
  this->CastTime                      = 0.0;
  this->DrawTime                      = 0.0;
  this->TotalStepsTaken               = 0;
  this->TotalRaysCast                 = 0;
  this->DirectionEncoder              = vtkRecursiveSphereDirectionEncoder::New();
  this->GradientEstimator             = vtkFiniteDifferenceGradientEstimator::New();
  this->GradientShader                = vtkEncodedGradientShader::New();
}

// Description:
// Destruct a vtkVolumeRayCastMapper - clean up any memory used
vtkVolumeRayCastMapper::~vtkVolumeRayCastMapper()
{
  if ( this->RGBAImage )
    delete [] this->RGBAImage;

  if ( this->ZImage )
    delete [] this->ZImage;

  if ( this->ScalarOpacityTFArray )
    delete [] this->ScalarOpacityTFArray;

  if ( this->RGBTFArray )
    delete [] this->RGBTFArray;

  if ( this->GrayTFArray )
    delete [] this->GrayTFArray;

  if ( this->CorrectedScalarOpacityTFArray )
    delete [] this->CorrectedScalarOpacityTFArray;

  if ( this->DirectionEncoder )
    {
    this->DirectionEncoder->UnRegister(this);
    this->DirectionEncoder = NULL;
    }

  if ( this->GradientEstimator )
    {
    this->GradientEstimator->UnRegister(this);
    this->GradientEstimator = NULL;
    }

  this->GradientShader->Delete();
}

void vtkVolumeRayCastMapper::SetDirectionEncoder( vtkDirectionEncoder *direnc )
{

  // If we are setting it to its current value, don't do anything
  if ( this->DirectionEncoder == direnc )
    {
    return;
    }

  // If we already have a direction encoder, unregister it.
  if ( this->DirectionEncoder )
    {
    this->DirectionEncoder->UnRegister(this);
    this->DirectionEncoder = NULL;
    }

  // If we are passing in a non-NULL encoder, register it
  if ( direnc )
    {
    direnc->Register( this );
    }

  // Actually set the encoder, and consider the object Modified
  this->DirectionEncoder = direnc;
  this->Modified();
}

void vtkVolumeRayCastMapper::SetGradientEstimator( vtkEncodedGradientEstimator *gradest )
{

  // If we are setting it to its current value, don't do anything
  if ( this->GradientEstimator == gradest )
    {
    return;
    }

  // If we already have a gradient estimator, unregister it.
  if ( this->GradientEstimator )
    {
    this->GradientEstimator->UnRegister(this);
    this->GradientEstimator = NULL;
    }

  // If we are passing in a non-NULL estimator, register it
  if ( gradest )
    {
    gradest->Register( this );
    }

  // Actually set the estimator, and consider the object Modified
  this->GradientEstimator = gradest;
  this->Modified();
}

void vtkVolumeRayCastMapper::Render( vtkRenderer *ren, vtkVolume *vol )
{
  vtkTimerLog        *timer;
  
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
    this->InitializeParallelImage( ren );
    this->VolumeRayCastFunction->FunctionInitialize( 
				       ren, vol, this,
				       this->ScalarOpacityTFArray,
				       this->CorrectedScalarOpacityTFArray,
				       this->GradientOpacityTFArray,
				       this->GradientOpacityConstant,
				       this->RGBTFArray,
				       this->GrayTFArray,
				       this->TFArraySize );

    this->RenderParallelImage( ren );
    }
  else
    {
    // Do the initialization for this image
    // This is all the stuff that happens before the rays
    // are cast
    this->GeneralImageInitialization( ren, vol );
    this->InitializePerspectiveImage( ren );
    this->VolumeRayCastFunction->FunctionInitialize( 
				       ren, vol, this,
				       this->ScalarOpacityTFArray,
				       this->CorrectedScalarOpacityTFArray,
				       this->GradientOpacityTFArray,
				       this->GradientOpacityConstant,
				       this->RGBTFArray,
				       this->GrayTFArray,
				       this->TFArraySize );
    
    this->RenderPerspectiveImage( ren );
    }

  this->TotalRaysCast   = this->TotalRaysCastPerId[0];
  this->TotalStepsTaken = this->TotalStepsTakenPerId[0];

  timer->StopTimer();

  this->CastTime = timer->GetElapsedTime();

  delete timer;
}

int vtkVolumeRayCastMapper::ClipRayAgainstVolume( float ray_info[12], 
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

  if ( ray_start[0] >= bounds[1] ||
       ray_start[1] >= bounds[3] ||
       ray_start[2] >= bounds[5] ||
       ray_start[0] < bounds[0] || 
       ray_start[1] < bounds[2] || 
       ray_start[2] < bounds[4] )
    {
    for ( loop = 0; loop < 3; loop++ )
      {
      diff = 0;

      if ( ray_start[loop] < inner_bounds[2*loop] )
	diff = inner_bounds[2*loop] - ray_start[loop];
      else if ( ray_start[loop] > inner_bounds[2*loop+1] )
	diff = inner_bounds[2*loop+1] - ray_start[loop];
      
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
	  
  if ( ray_start[0] >= bounds[1] ||
       ray_start[1] >= bounds[3] ||
       ray_start[2] >= bounds[5] ||
       ray_start[0] < bounds[0] || 
       ray_start[1] < bounds[2] || 
       ray_start[2] < bounds[4] )
    {
    return 0;
    }

  // The ray does intersect the volume, and we have a starting
  // position that is inside the volume
  if ( ray_end[0] >= bounds[1] ||
       ray_end[1] >= bounds[3] ||
       ray_end[2] >= bounds[5] ||
       ray_end[0] < bounds[0] || 
       ray_end[1] < bounds[2] || 
       ray_end[2] < bounds[4] )
    {
    for ( loop = 0; loop < 3; loop++ )
      {
      diff = 0;
      
      if ( ray_end[loop] < inner_bounds[2*loop] )
	diff = inner_bounds[2*loop] - ray_end[loop];
      else if ( ray_end[loop] > inner_bounds[2*loop+1] )
	diff = inner_bounds[2*loop+1] - ray_end[loop];
      
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
  
  if ( ray_end[0] >= bounds[1] ||
       ray_end[1] >= bounds[3] ||
       ray_end[2] >= bounds[5] ||
       ray_end[0] < bounds[0] || 
       ray_end[1] < bounds[2] || 
       ray_end[2] < bounds[4] )
    {
      return 0;
    }
    
  return 1;
}

void vtkVolumeRayCastMapper::GeneralImageInitialization( vtkRenderer *ren, 
						     vtkVolume *vol )
{
  vtkTransform           *transform;
  vtkTransform           *scalar_transform;
  vtkRayCaster           *ray_caster;
  float                  spacing[3], data_origin[3];

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
    this->SampleDistance * ray_caster->GetViewportStepSize() * vol->GetScale();

  // Delete the objects we created
  transform->Delete();
  scalar_transform->Delete();

  // If there'a a color image already allocated, delete it.  Create a new
  // image. This image is RGBA in floats.
  if ( this->RGBAImage )
    delete [] this->RGBAImage;
  this->RGBAImage = new float[ this->ViewRaysSize[0] * 
			       this->ViewRaysSize[1] * 4 ];

  if ( this->ZImage )
    delete [] this->ZImage;
  
  // If there is a depth image already allocated, delete it. Create a new
  // depth image.  The depth image is z values in perspective coordinates
  // as floats.
  this->ZImage = new float[ this->ViewRaysSize[0] * this->ViewRaysSize[1] ];

  // Get the previous zbuffer data
  this->RenderZData = ren->GetRayCaster()->GetCurrentZBuffer();

  this->ScalarDataPointer = 
    this->ScalarInput->GetPointData()->GetScalars()->GetVoidPointer(0);
  this->ScalarDataType = 
    this->ScalarInput->GetPointData()->GetScalars()->GetDataType();
    
  if( (this->ScalarDataType != VTK_UNSIGNED_SHORT ) && 
      (this->ScalarDataType != VTK_UNSIGNED_CHAR ) )
    {
    vtkErrorMacro( << "The scalar data type: " << this->ScalarDataType <<
      " is not supported when volume rendering. Please convert the " <<
      " data to unsigned char or unsigned short.\n" );
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
void vtkVolumeRayCastMapper::InitializeParallelImage( vtkRenderer *ren ) 
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

  // Just checking that our assumptions are correct. 
  if( this->Debug )
    {
    if (  matrix->Element[3][0] || matrix->Element[3][1]  ||
          matrix->Element[3][2] || (matrix->Element[3][3] != 1.0) )
      {
      vtkErrorMacro( << "Oh no! They aren't 0 like they are supposed to be!");
      cout << *transform;
      }
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
// RGBAImage and a ZImage.  
void vtkVolumeRayCastMapper::RenderParallelImage( vtkRenderer *ren )
{
  int                     i, j, last_i;
  float                   unit_ray_direction[3], ray_origin[3];
  float                   ray_direction[3];
  float                   start_ray[4];
  float                   *iptr;
  float                   *zptr;
  int                     num_samples;
  float                   near_z, far_z, tmp_z;
  float                   zscale, zbias;
  float                   *z_range_ptr;
  float                   world_sample_distance;
  float                   ray_increment[3];
  float                   pixel_value[6];
  float                   bounds[12];
  float		          *pixel_offset_x;
  float		          *pixel_offset_y;
  float		          *pixel_offset_z;
  float                   *ren_z_ptr = NULL;
  int                     largest_increment_index;
  float                   clipping_range[2];
  float                   ray_info[12];
  int                     noAbort = 1;
  vtkRenderWindow         *renWin;
  int                     count_loc = 0;
  
  renWin = ren->GetRenderWindow();
	   
  // Initialize some statistics
  this->TotalRaysCastPerId[count_loc]   = 0;
  this->TotalStepsTakenPerId[count_loc] = 0;

  // Pull some info out of instance variables into local variables

  // These are the pointers to the image (RGBA and Z) and all the
  // z buffers
  iptr = this->RGBAImage;
  zptr = this->ZImage;
  z_range_ptr = this->DepthRangeBufferPointer;
  ren_z_ptr = this->RenderZData;

  // This is the scale factor used to convert z buffer values to 
  // z distance values
  zscale = this->ParallelZScale;
  zbias  = this->ParallelZBias;

  // This is the length of the ray in the local volume space
  world_sample_distance = this->WorldSampleDistance;

  // This is the unit direction of the ray in the local volume space
  memcpy( unit_ray_direction, this->LocalUnitRayDirection, 
	  3 * sizeof( float ) );

  // This is the direction of the ray in the local volume space 
  memcpy( ray_direction, this->LocalRayDirection, 3 * sizeof( float ) );

  // The clipping range of the camera is used to clip the rays
  memcpy( clipping_range, this->CameraClippingRange, 2 * sizeof( float) );

  memcpy( ray_info + 6, ray_direction, 3 * sizeof( float ) );
  memcpy( ray_info + 9, unit_ray_direction, 3 * sizeof( float ) );

  // This is the local ray start
  start_ray[0] = this->LocalRayStart[0];
  start_ray[1] = this->LocalRayStart[1];
  start_ray[2] = this->LocalRayStart[2];

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
  pixel_offset_x = new float[(this->ViewRaysSize[0])];
  pixel_offset_y = new float[(this->ViewRaysSize[0])];
  pixel_offset_z = new float[(this->ViewRaysSize[0])];

  for( i = 0; i < this->ViewRaysSize[0]; i++ )
    {
    pixel_offset_x[i] = this->XOriginIncrement[0] * (float)i;
    pixel_offset_y[i] = this->XOriginIncrement[1] * (float)i;
    pixel_offset_z[i] = this->XOriginIncrement[2] * (float)i;
    }

  // Set the bounds of the volume
  for ( i = 0; i < 3; i++ )
    {
    bounds[2*i] = 0.0;
    bounds[2*i+1] = this->ScalarDataSize[i] - 1;
    }

  if ( this->Clipping )
    {
    for ( i = 0; i < 3; i++ )
      {
      if ( this->ClippingPlanes[2*i] > bounds[2*i] )
	bounds[2*i] = this->ClippingPlanes[2*i];
      if ( this->ClippingPlanes[2*i+1] < bounds[2*i+1] )
	bounds[2*i+1] = this->ClippingPlanes[2*i+1];
      }
    }

  for ( i = 0; i < 3; i++ )
    {
    bounds[2*i+6] = bounds[2*i]   + 0.001;
    bounds[2*i+7] = bounds[2*i+1] - 0.001;
    }

  // Just in case it is the wrong data type, we have valid default values
  // (CastARay will return without having filled this in)
  pixel_value[0] = 0.0;
  pixel_value[1] = 0.0;
  pixel_value[2] = 0.0;
  pixel_value[3] = 0.0;
  pixel_value[4] = 1.0;
  pixel_value[5] = 0.0;

  // Loop through all pixels and cast rays where necessary
  for ( j = 0; j < this->ViewRaysSize[1]; j++ )
    {
    noAbort = !(renWin->CheckAbortStatus());
    if (noAbort)
      {
      ray_origin[0] = start_ray[0] + (float)j * this->YOriginIncrement[0];
      ray_origin[1] = start_ray[1] + (float)j * this->YOriginIncrement[1];
      ray_origin[2] = start_ray[2] + (float)j * this->YOriginIncrement[2];

      last_i = 0;

      for ( i = 0; i < this->ViewRaysSize[0]; i++ )
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

	  if ( this->ClipRayAgainstVolume( ray_info, bounds ) )
	    {
	    num_samples = (int)( ( ray_info[3+largest_increment_index] - 
				   ray_info[largest_increment_index] ) /
				 ray_increment[largest_increment_index] ) + 1;

	    this->TotalRaysCastPerId[count_loc]++;
	    
	    this->VolumeRayCastFunction->CastARay( this->ScalarDataType, 
						   this->ScalarDataPointer,
						   ray_info, ray_increment, 
						   num_samples, 
						   pixel_value );
	    
	    // Set the pixel value to the value returned by the ray cast
	    *(iptr++) = pixel_value[0];
	    *(iptr++) = pixel_value[1];
	    *(iptr++) = pixel_value[2];
	    *(iptr++) = pixel_value[3];
	    *(zptr++) = pixel_value[4];

	    // Increment the number of samples taken
	    this->TotalStepsTakenPerId[count_loc] += (int)pixel_value[5];
	    
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
    } // For each pixel loop

  // Delete the objects we created
  delete [] pixel_offset_x;
  delete [] pixel_offset_y;
  delete [] pixel_offset_z;

}

// Description:
// Cast a ray for each pixel in the image plane.  The rays are obtained
// from the vtkRenderer and they define the size of the image to be
// computed.  At the end of this, we have an RGBAImage and a ZImage.
void vtkVolumeRayCastMapper::InitializePerspectiveImage( vtkRenderer *ren ) 
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
void vtkVolumeRayCastMapper::RenderPerspectiveImage( vtkRenderer *ren )
{
  int                      i, j;
  float                    *ray_ptr, in[4];
  float                    ray_direction[4], ray_origin[4];
  float                    unit_ray_direction[3];
  int                      largest_increment_index;
  float                    *iptr;
  float                    *zptr;
  int                      num_samples;
  float                    *z_range_ptr;
  float                    znum1, zdenom1, zdenom2;
  float                    near_z, far_z, tmp_z;
  float                    ray_increment[3];
  float                    pixel_value[6];
  float                    *ren_z_ptr = NULL;
  float                    world_sample_distance;
  float                    bounds[12];
  float                    ray_info[12];
  float                    clipping_range[2];
  int                      noAbort = 1;
  vtkRenderWindow          *renWin;
  int                      count_loc = 0;

  renWin = ren->GetRenderWindow();

  // Initialize some statistics
  this->TotalRaysCastPerId[count_loc]   = 0;
  this->TotalStepsTakenPerId[count_loc] = 0;

  // Pull some info out of instance variables into local variables

  // These are the pointers to the image (RGBA and Z) and all the
  // z buffers
  iptr        = this->RGBAImage;
  zptr        = this->ZImage;
  z_range_ptr = this->DepthRangeBufferPointer;
  ren_z_ptr   = this->RenderZData;

  // These are the values used to  convert z buffer values to 
  // z distance values
  znum1   = this->ZNumerator;
  zdenom1 = this->ZDenomMult;
  zdenom2 = this->ZDenomAdd;

  // This is the length of the ray in the local volume space
  world_sample_distance = this->WorldSampleDistance;

  ray_ptr = this->ViewRays;

  memcpy( ray_origin, this->LocalRayOrigin, 3 * sizeof(float) );

  // The clipping range of the camera is used to clip the rays
  memcpy( clipping_range, this->CameraClippingRange, 2 * sizeof( float) );

  // Set the bounds of the volume
  for ( i = 0; i < 3; i++ )
    {
    bounds[2*i] = 0.0;
    bounds[2*i+1] = this->ScalarDataSize[i] - 1;
    }

  if ( this->Clipping )
    {
    for ( i = 0; i < 3; i++ )
      {
      if ( this->ClippingPlanes[2*i] > bounds[2*i] )
	bounds[2*i] = this->ClippingPlanes[2*i];
      if ( this->ClippingPlanes[2*i+1] < bounds[2*i+1] )
	bounds[2*i+1] = this->ClippingPlanes[2*i+1];
      }
    }

  for ( i = 0; i < 3; i++ )
    {
    bounds[2*i+6] = bounds[2*i]   + 0.001;
    bounds[2*i+7] = bounds[2*i+1] - 0.001;
    }

  // Loop through all pixel    
  for ( j = 0; j < this->ViewRaysSize[1]; j++ )
    {
    noAbort = !(renWin->CheckAbortStatus());
    if (noAbort)
      {
      for ( i = 0; i < this->ViewRaysSize[0]; i++ )
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
	  this->ViewRaysTransform.MultiplyPoint( in, ray_direction );
	  
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

	  if ( this->ClipRayAgainstVolume( ray_info, bounds ) )
	    {
	      num_samples = (int)( ( ray_info[3+largest_increment_index] - 
				     ray_info[largest_increment_index] ) /
				  ray_increment[largest_increment_index] ) + 1;

	    this->TotalRaysCastPerId[count_loc]++;
	    
	    this->VolumeRayCastFunction->CastARay( this->ScalarDataType, 
						   this->ScalarDataPointer,
						   ray_info, 
						   ray_increment, 
						   num_samples, 
						   pixel_value );
	    
	    // Increment the number of samples taken
	    this->TotalStepsTakenPerId[count_loc] += (int)pixel_value[5];
	    
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
    } // For each pixel loop

}

void vtkVolumeRayCastMapper::CasterUpdate( vtkRenderer *ren, 
					   vtkVolume *vol )
{
  this->UpdateShadingTables( ren, vol );

  this->UpdateTransferFunctions( ren, vol );
}


void vtkVolumeRayCastMapper::UpdateShadingTables( vtkRenderer *ren, 
						  vtkVolume *vol )
{
  int                   shading;
  vtkVolumeProperty     *volume_property;
  float                 gradient_opacity_bias;
  float                 gradient_opacity_scale;

  volume_property = vol->GetVolumeProperty();

  shading = volume_property->GetShade();

  gradient_opacity_bias  = volume_property->GetGradientOpacityBias();
  gradient_opacity_scale = volume_property->GetGradientOpacityScale();

  this->GradientEstimator->SetGradientMagnitudeBias( gradient_opacity_bias );
  this->GradientEstimator->SetGradientMagnitudeScale( gradient_opacity_scale );
  this->GradientEstimator->SetScalarInput( this->ScalarInput );
  this->GradientEstimator->SetDirectionEncoder( this->DirectionEncoder );


  if ( shading )
    {
    this->GradientShader->UpdateShadingTable( ren, vol, 
					      this->GradientEstimator );
    }
}


void vtkVolumeRayCastMapper::UpdateTransferFunctions( vtkRenderer *ren, vtkVolume *vol )
{
  int                       data_type;
  vtkVolumeProperty         *volume_property;
  vtkPiecewiseFunction      *scalar_opacity_transfer_function;
  vtkPiecewiseFunction      *gradient_opacity_transfer_function;
  vtkPiecewiseFunction      *gray_transfer_function;
  vtkColorTransferFunction  *rgb_transfer_function;
  int                       color_channels;
  int                       scalar_opacity_tf_needs_updating = 0;
  int                       gradient_opacity_tf_needs_updating = 0;
  int                       rgb_tf_needs_updating = 0;
  int                       gray_tf_needs_updating = 0;

  volume_property = vol->GetVolumeProperty();

  // Update the ScalarOpacityTFArray if necessary.  This is necessary if
  // the ScalarOpacityTFArray does not exist, or the transfer function has
  // been modified more recently than the ScalarOpacityTFArray has.
  scalar_opacity_transfer_function   = volume_property->GetScalarOpacity();
  gradient_opacity_transfer_function = volume_property->GetGradientOpacity();
  rgb_transfer_function              = volume_property->GetRGBTransferFunction();
  gray_transfer_function             = volume_property->GetGrayTransferFunction();
  color_channels                     = volume_property->GetColorChannels();

  if ( this->ScalarInput->GetPointData()->GetScalars() == NULL )
    {
    vtkErrorMacro(<<"Need scalar data to volume render");
    return;
    }
    
  data_type = this->ScalarInput->GetPointData()->GetScalars()->GetDataType();

  if ( scalar_opacity_transfer_function == NULL )
    {
    vtkErrorMacro( << "Error: no transfer function!" );
    return;
    }
  else if ( this->ScalarOpacityTFArray == NULL ||
	    scalar_opacity_transfer_function->GetMTime() >
	    this->ScalarOpacityTFArrayMTime ||
	    volume_property->GetScalarOpacityMTime() >
	    this->ScalarOpacityTFArrayMTime )
    {
    scalar_opacity_tf_needs_updating = 1;
    }

  if ( gradient_opacity_transfer_function == NULL )
    {
    vtkErrorMacro( << "Error: no gradient magnitude opacity function!" );
    return;
    }
  else if ( gradient_opacity_transfer_function->GetMTime() >
	    this->GradientOpacityTFArrayMTime ||
	    volume_property->GetGradientOpacityMTime() >
	    this->GradientOpacityTFArrayMTime )
    {
    gradient_opacity_tf_needs_updating = 1;
    }
  
  switch ( color_channels )
    {
    case 1:
      if ( gray_transfer_function == NULL )
	{
	vtkErrorMacro( << "Error: no gray transfer function!" );
	}
      else if ( this->GrayTFArray == NULL ||
		gray_transfer_function->GetMTime() >
		this->GrayTFArrayMTime ||
		volume_property->GetGrayTransferFunctionMTime() >
		this->GrayTFArrayMTime )
	gray_tf_needs_updating = 1;
      break;
    case 3:
      if ( rgb_transfer_function == NULL )
	{
	vtkErrorMacro( << "Error: no color transfer function!" );
	}
      else if ( this->RGBTFArray == NULL ||
		rgb_transfer_function->GetMTime() >
		this->RGBTFArrayMTime ||
		volume_property->GetRGBTransferFunctionMTime() >
		this->RGBTFArrayMTime )
	rgb_tf_needs_updating = 1;
      break;
    }

  if ( gradient_opacity_tf_needs_updating )
    {
    // Get values 0-255 (256 values)
    gradient_opacity_transfer_function->GetTable(
					  (float)(0x00),
					  (float)(0xff),  
					  (int)(0x100), 
					  this->GradientOpacityTFArray );
    if ( !strcmp(gradient_opacity_transfer_function->GetType(), "Constant") )
      {
      this->GradientOpacityConstant = this->GradientOpacityTFArray[128];
      }
    else
      {
      this->GradientOpacityConstant = -1.0;
      }

    this->GradientOpacityTFArrayMTime.Modified();
    }


  if (data_type == VTK_UNSIGNED_CHAR)
    {
    this->TFArraySize = (int)(0x100);

    if ( scalar_opacity_tf_needs_updating )
      {
      // Get values 0-255 (256 values)
      if ( this->ScalarOpacityTFArray )
	delete [] this->ScalarOpacityTFArray;

      this->ScalarOpacityTFArray = new float[(int)(0x100)];
      scalar_opacity_transfer_function->GetTable( (float)(0x00),
					   (float)(0xff),  
					   (int)(0x100), 
					   this->ScalarOpacityTFArray );
      this->ScalarOpacityTFArrayMTime.Modified();
      }

    if ( gray_tf_needs_updating )
      {
      if ( this->GrayTFArray )
	delete [] this->GrayTFArray;

      this->GrayTFArray = new float[(int)(0x100)];
      gray_transfer_function->GetTable( (float)(0x00),
					(float)(0xff),  
					(int)(0x100), 
					this->GrayTFArray );
      this->GrayTFArrayMTime.Modified();
      }

    if ( rgb_tf_needs_updating )
      {
      if ( this->RGBTFArray )
	delete [] this->RGBTFArray;
      
      this->RGBTFArray = new float[3 * (int)(0x100)];
      rgb_transfer_function->GetTable( (float)(0x00),
				       (float)(0xff),  
				       (int)(0x100), 
				       this->RGBTFArray );
      this->RGBTFArrayMTime.Modified();
      }
    }
  else if ( data_type == VTK_UNSIGNED_SHORT )
    {
    this->TFArraySize = (int)(0x10000);

    if ( scalar_opacity_tf_needs_updating )
      {
      // Get values 0-65535 (65536 values)
      if ( this->ScalarOpacityTFArray )
	delete [] this->ScalarOpacityTFArray;

      this->ScalarOpacityTFArray = new float[(int)(0x10000)];
      scalar_opacity_transfer_function->GetTable( (float)(0x0000),
					   (float)(0xffff),  
					   (int)(0x10000), 
					   this->ScalarOpacityTFArray );
      this->ScalarOpacityTFArrayMTime.Modified();
      }

    if ( gray_tf_needs_updating )
      {
      if ( this->GrayTFArray )
	delete [] this->GrayTFArray;

      this->GrayTFArray = new float[(int)(0x10000)];
      gray_transfer_function->GetTable( (float)(0x0000),
					(float)(0xffff),  
					(int)(0x10000), 
					this->GrayTFArray );
      this->GrayTFArrayMTime.Modified();
      }

    if ( rgb_tf_needs_updating )
      {
      if ( this->RGBTFArray )
	delete [] this->RGBTFArray;
      
      this->RGBTFArray = new float[3 * (int)(0x10000)];
      rgb_transfer_function->GetTable( (float)(0x0000),
				       (float)(0xffff),  
				       (int)(0x10000), 
				       this->RGBTFArray );
      this->RGBTFArrayMTime.Modified();
      }
    }

  // check that the corrected scalar opacity transfer function
  // is update to date with the current step size.
  // Update CorrectedScalarOpacityTFArray if it is required.

  if ( scalar_opacity_tf_needs_updating )
    {
    if ( this->CorrectedScalarOpacityTFArray )
      delete [] this->CorrectedScalarOpacityTFArray;

    this->CorrectedScalarOpacityTFArray = new float[this->TFArraySize];
    }

  this->UpdateScalarOpacityTFforSampleSize(ren,vol);
}


// Description:
// This method computes the corrected alpha blending for a given
// step size.  The ScalarOpacityTFArray reflects step size 1.
// The CorrectedScalarOpacityTFArray reflects step size CorrectedStepSize.
void vtkVolumeRayCastMapper::UpdateScalarOpacityTFforSampleSize(
					vtkRenderer *ren,
				 	vtkVolume *vol) 
{
  int i;
  int needsRecomputing;
  float originalAlpha,correctedAlpha;
  float ray_scale;
  float volumeScale;
  float interactionScale;
  vtkRayCaster *ray_caster;

  ray_caster = ren->GetRayCaster();
  interactionScale = ray_caster->GetViewportStepSize();
  volumeScale = vol->GetScale();
  ray_scale = this->SampleDistance * interactionScale * volumeScale;


  // step size changed
  needsRecomputing =  
      this->CorrectedStepSize-ray_scale >  0.0001;
  
  needsRecomputing = needsRecomputing || 
      this->CorrectedStepSize-ray_scale < -0.0001;

  if (!needsRecomputing)
    {
    // updated scalar opacity xfer function
    needsRecomputing = needsRecomputing || 
	this->ScalarOpacityTFArrayMTime > this->CorrectedSOTFArrayMTime;
    }
  if (needsRecomputing)
    {
    this->CorrectedSOTFArrayMTime.Modified();
    this->CorrectedStepSize = ray_scale;
    for (i = 0;i < this->TFArraySize;i++)
      {
      originalAlpha = *(this->ScalarOpacityTFArray+i);

      // this test is to accelerate the Transfer function correction

      if (originalAlpha > 0.0001)
	{
	correctedAlpha = 
	  1.0-pow((double)(1.0-originalAlpha),double(this->CorrectedStepSize));
	}
      else
	{
	correctedAlpha = originalAlpha;
	}
      *(this->CorrectedScalarOpacityTFArray+i) = correctedAlpha;
      }
    }
}

// Description:
float vtkVolumeRayCastMapper::GetZeroOpacityThreshold( vtkVolume *vol )
{
  return( this->VolumeRayCastFunction->GetZeroOpacityThreshold( vol ) );
}

// Description:
// Print method for vtkVolumeRayCastMapper
void vtkVolumeRayCastMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkVolumeMapper::PrintSelf(os,indent);

  os << indent << "Sample Distance: " << this->SampleDistance << "\n";

  os << indent << "Total Steps Taken: " << this->TotalStepsTaken << "\n";

  os << indent << "Total Rays Cast: " << this->TotalRaysCast << "\n";

  os << indent << "Time To Draw: " << this->DrawTime << "\n";

  os << indent << "Time To Ray Cast: " << this->CastTime << "\n";

  if ( this->RayBounder )
    {
    os << indent << "Ray Bounder: " << this->RayBounder << "\n";
    }
  else
    {
    os << indent << "Ray Bounder: (none)\n";
    }

  if ( this->VolumeRayCastFunction )
    {
    os << indent << "Ray Cast Function: " << this->VolumeRayCastFunction<<"\n";
    }
  else
    {
    os << indent << "Ray Cast Function: (none)\n";
    }

  os << indent << "Scalar Data Size: " << "( " << 
    this->ScalarDataSize[0] << ", " << this->ScalarDataSize[1] << ", " <<
    this->ScalarDataSize[2] << " )\n";

  if ( this->DirectionEncoder )
    {
      os << indent << "Direction Encoder: " << (this->DirectionEncoder) <<
	endl;
    }
  else
    {
      os << indent << "Direction Encoder: (none)" << endl;
    }

  if ( this->GradientEstimator )
    {
      os << indent << "Gradient Estimator: " << (this->GradientEstimator) <<
	endl;
    }
  else
    {
      os << indent << "Gradient Estimator: (none)" << endl;
    }

  if ( this->GradientShader )
    {
      os << indent << "Gradient Shader: " << (this->GradientShader) << endl;
    }
  else
    {
      os << indent << "Gradient Shader: (none)" << endl;
    }

  // These variables should not be printed to the user:
  // this->DataIncrement[0] 
}

