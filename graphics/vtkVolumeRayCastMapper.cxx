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
#include "vtkFiniteDifferenceGradientEstimator.h"

#define vtkRayCastMatrixMultiplyPointMacro( A, B, M ) \
  B[0] = A[0]*M[0]  + A[1]*M[1]  + A[2]*M[2]  + M[3]; \
  B[1] = A[0]*M[4]  + A[1]*M[5]  + A[2]*M[6]  + M[7]; \
  B[2] = A[0]*M[8]  + A[1]*M[9]  + A[2]*M[10] + M[11]; \
  B[3] = A[0]*M[12] + A[1]*M[13] + A[2]*M[14] + M[15]; \
  if ( B[3] != 1.0 ) { B[0] /= B[3]; B[1] /= B[3]; B[2] /= B[3]; }

// Construct a new vtkVolumeRayCastMapper with default values
vtkVolumeRayCastMapper::vtkVolumeRayCastMapper()
{
  this->SampleDistance                = 1.0;
  this->RayBounder                    = NULL;
  this->VolumeRayCastFunction         = NULL;
  this->GradientEstimator             = vtkFiniteDifferenceGradientEstimator::New();
  this->GradientShader                = vtkEncodedGradientShader::New();
}

// Destruct a vtkVolumeRayCastMapper - clean up any memory used
vtkVolumeRayCastMapper::~vtkVolumeRayCastMapper()
{
  if ( this->GradientEstimator )
    {
    this->GradientEstimator->UnRegister(this);
    this->GradientEstimator = NULL;
    }

  this->GradientShader->Delete();

  this->SetRayBounder(NULL);
  this->SetVolumeRayCastFunction(NULL);
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


void vtkVolumeRayCastMapper::ReleaseGraphicsResources(vtkWindow *renWin)
{
  // pass this information onto the ray bounder
  if (this->RayBounder)
    {
    this->RayBounder->ReleaseGraphicsResources(renWin);
    }
}

void vtkVolumeRayCastMapper::InitializeRender( vtkRenderer *ren, vtkVolume *vol,
					       VTKRayCastVolumeInfo *volumeInfo )
{
  // make sure that we have scalar input and update the scalar input
  if ( this->GetInput() == NULL ) 
    {
    vtkErrorMacro(<< "No Input!");
    return;
    }
  else
    {
    this->GetInput()->Update();
    } 

  if ( this->GetRGBTextureInput() )
    {
    this->GetRGBTextureInput()->Update();
    }

  this->UpdateShadingTables( ren, vol );

  if ( this->RayBounder )
    {
    this->DepthRangeBufferPointer = this->RayBounder->GetRayBounds( ren );
    }
  else
    {
    this->DepthRangeBufferPointer = NULL;
    }

  this->GeneralImageInitialization( ren, vol );

  this->VolumeRayCastFunction->FunctionInitialize( ren, vol, volumeInfo, this );

  memcpy( volumeInfo->WorldToVolumeMatrix, this->WorldToVolumeMatrix, 16*sizeof(float) );
  memcpy( volumeInfo->ViewToVolumeMatrix, this->ViewToVolumeMatrix, 16*sizeof(float) );

  volumeInfo->ScalarDataType = this->ScalarDataType;
  volumeInfo->ScalarDataPointer = this->ScalarDataPointer;    
}

void vtkVolumeRayCastMapper::CastViewRay( VTKRayCastRayInfo *rayInfo,
					  VTKRayCastVolumeInfo *volumeInfo )
{
  int     i;
  float   *volumeRayIncrement;
  float   rayStart[4], rayEnd[4];
  float   *rayOrigin, *rayDirection;
  float   *volumeRayStart, *volumeRayEnd, *volumeRayDirection;
  float   incrementLength, rayLength;
  float   *viewToVolumeMatrix;
  float   nearplane, farplane, bounderNear, bounderFar;
  float   oneStep[4], volumeOneStep[4];
  int     bitFlag, bitLoop;
  float   bounds[6];
  float   savedRayStart[3];
  float   savedRayEnd[3];
  float   savedRayDirection[3];
  float   rgbaArray[10*4];
  float   distanceArray[10];
  int     arrayCount;
  float   tmp, tmpArray[4];

  rayOrigin = rayInfo->Origin;
  rayDirection = rayInfo->Direction;
  volumeRayStart = rayInfo->TransformedStart;
  volumeRayEnd = rayInfo->TransformedEnd;
  volumeRayDirection = rayInfo->TransformedDirection;
  volumeRayIncrement = rayInfo->TransformedIncrement;
  viewToVolumeMatrix = volumeInfo->ViewToVolumeMatrix;

  // In case we don't encounter anything, initialize everything
  rayInfo->Color[0] = 
    rayInfo->Color[1] = 
    rayInfo->Color[2] = 
    rayInfo->Color[3] = 0.0;
  rayInfo->Depth = VTK_LARGE_FLOAT;
  rayInfo->NumberOfStepsTaken = 0;

  nearplane = rayInfo->NearClip;
  farplane  = rayInfo->FarClip;

  if ( rayInfo->Pixel[0] > 0 && this->DepthRangeBufferPointer )
    {
    bounderNear = *( this->DepthRangeBufferPointer + 
		     2 * (rayInfo->Pixel[1] * rayInfo->ImageSize[0] +
			  rayInfo->Pixel[0]) );
    bounderFar  = *( this->DepthRangeBufferPointer + 
		     2 * (rayInfo->Pixel[1] * rayInfo->ImageSize[0] +
			  rayInfo->Pixel[0]) + 1 );
    if ( bounderNear > 0.0 )
      {
      if ( bounderNear > nearplane )
	{
	nearplane = bounderNear;
	}
      if ( bounderFar  < farplane )
	{
	farplane  = bounderFar; 	
	}
      }

    if ( bounderNear <= 0.0 || nearplane >= farplane )
      {
      return;
      }
    }

  rayStart[0] = rayOrigin[0] + nearplane * rayDirection[0];
  rayStart[1] = rayOrigin[1] + nearplane * rayDirection[1];
  rayStart[2] = rayOrigin[2] + nearplane * rayDirection[2];

  rayEnd[0]   = rayOrigin[0] + farplane  * rayDirection[0];
  rayEnd[1]   = rayOrigin[1] + farplane  * rayDirection[1];
  rayEnd[2]   = rayOrigin[2] + farplane  * rayDirection[2];

  oneStep[0]  = rayStart[0] + rayDirection[0] * this->WorldSampleDistance;
  oneStep[1]  = rayStart[1] + rayDirection[1] * this->WorldSampleDistance;
  oneStep[2]  = rayStart[2] + rayDirection[2] * this->WorldSampleDistance;
  
  // Transform the ray start from view to volume coordinates
  vtkRayCastMatrixMultiplyPointMacro( rayStart, volumeRayStart, viewToVolumeMatrix );

  // Transform the ray end from view to volume coordinates
  vtkRayCastMatrixMultiplyPointMacro( rayEnd, volumeRayEnd, viewToVolumeMatrix );

  // Transform a point one step from the rayStart
  vtkRayCastMatrixMultiplyPointMacro( oneStep, volumeOneStep, viewToVolumeMatrix );

  // Compute the ray direction
  volumeRayIncrement[0] = 
    volumeOneStep[0] - volumeRayStart[0];
  volumeRayIncrement[1] = 
    volumeOneStep[1] - volumeRayStart[1];
  volumeRayIncrement[2] = 
    volumeOneStep[2] - volumeRayStart[2];

  incrementLength = sqrt( (double) (volumeRayIncrement[0]*volumeRayIncrement[0] +
				    volumeRayIncrement[1]*volumeRayIncrement[1] +
				    volumeRayIncrement[2]*volumeRayIncrement[2]) );

  volumeRayDirection[0] = volumeRayEnd[0] - volumeRayStart[0];
  volumeRayDirection[1] = volumeRayEnd[1] - volumeRayStart[1];
  volumeRayDirection[2] = volumeRayEnd[2] - volumeRayStart[2];

  rayLength = sqrt( (double) (volumeRayDirection[0]*volumeRayDirection[0] +
			      volumeRayDirection[1]*volumeRayDirection[1] +
			      volumeRayDirection[2]*volumeRayDirection[2]) );

  if ( rayLength )
    {
    volumeRayDirection[0] /= rayLength;
    volumeRayDirection[1] /= rayLength;
    volumeRayDirection[2] /= rayLength;
    }

  // If we are not cropping, or we are only clipping with a subvolume 
  // between the clipping planes, do the simple thing
  if ( incrementLength && rayLength && 
       (!this->Cropping || this->CroppingRegionFlags == 0x2000) )
    {
    if ( this->ClipRayAgainstVolume( rayInfo, volumeInfo, this->VolumeBounds ) )
      {
      // Recompute the ray length since the start and end may have been
      // modified by ClipRayAgainstVolume() 
      volumeRayDirection[0] = volumeRayEnd[0] - volumeRayStart[0];
      volumeRayDirection[1] = volumeRayEnd[1] - volumeRayStart[1];
      volumeRayDirection[2] = volumeRayEnd[2] - volumeRayStart[2];
      
      rayLength = sqrt( (double) (volumeRayDirection[0]*volumeRayDirection[0] +
				  volumeRayDirection[1]*volumeRayDirection[1] +
				  volumeRayDirection[2]*volumeRayDirection[2]) );
      if ( rayLength )
	{
	volumeRayDirection[0] /= rayLength;
	volumeRayDirection[1] /= rayLength;
	volumeRayDirection[2] /= rayLength;
	}

      volumeRayIncrement[0] = incrementLength * volumeRayDirection[0];
      volumeRayIncrement[1] = incrementLength * volumeRayDirection[1];
      volumeRayIncrement[2] = incrementLength * volumeRayDirection[2];

      rayInfo->NumberOfStepsToTake = (rayLength / incrementLength) + 1;
      
      for ( i = 0; i < 3; i++ )
	{
	if ( ( volumeRayIncrement[i] > 0.0 && 
	       ( volumeRayStart[i] + 
		 (float)(rayInfo->NumberOfStepsToTake-1) * volumeRayIncrement[i]) >
	       volumeRayEnd[i] ) ||
	     ( volumeRayIncrement[i] < 0.0 && 
	       ( volumeRayStart[i] + 
		 (float)(rayInfo->NumberOfStepsToTake-1) * volumeRayIncrement[i]) <
	       volumeRayEnd[i] ) )
	  {
	  rayInfo->NumberOfStepsToTake--;
	  }
	}

      if ( rayInfo->NumberOfStepsToTake > 0 )
	{
	this->VolumeRayCastFunction->CastRay( rayInfo, volumeInfo );
	}
      }
    }
  // Otherwise, we are cropping, and our flags are more complex than just a
  // center crop box - so loop through all 27 subregions, cast the rays, and
  // merge the results.
  else
    {
    memcpy( savedRayStart, volumeRayStart, 3*sizeof(float) );
    memcpy( savedRayEnd, volumeRayEnd, 3*sizeof(float) );
    memcpy( savedRayDirection, volumeRayDirection, 3*sizeof(float) );
    
    arrayCount = 0;

    for ( bitLoop = 0; bitLoop < 27; bitLoop++ )
      {
      bitFlag = 1 << bitLoop;
      if ( this->CroppingRegionFlags & bitFlag )
	{
	memcpy( volumeRayStart, savedRayStart, 3*sizeof(float) );
	memcpy( volumeRayEnd, savedRayEnd, 3*sizeof(float) );
	memcpy( volumeRayDirection, savedRayDirection, 3*sizeof(float) );

	switch ( bitLoop % 3 )
	  {
	  case 0:
	    bounds[0] = 0;
	    bounds[1] = this->CroppingBounds[0];
	    break;
	  case 1:
	    bounds[0] = this->CroppingBounds[0];
	    bounds[1] = this->CroppingBounds[1];
	    break;
	  case 2:
	    bounds[0] = this->CroppingBounds[1];
	    bounds[1] = volumeInfo->DataSize[0] - 1;
	    break;
	  }

	switch ( (bitLoop % 9) / 3 )
	  {
	  case 0:
	    bounds[2] = 0;
	    bounds[3] = this->CroppingBounds[2];
	    break;
	  case 1:
	    bounds[2] = this->CroppingBounds[2];
	    bounds[3] = this->CroppingBounds[3];
	    break;
	  case 2:
	    bounds[2] = this->CroppingBounds[3];
	    bounds[3] = volumeInfo->DataSize[1] - 1;
	    break;
	  }

	switch ( bitLoop / 9 )
	  {
	  case 0:
	    bounds[4] = 0;
	    bounds[5] = this->CroppingBounds[4];
	    break;
	  case 1:
	    bounds[4] = this->CroppingBounds[4];
	    bounds[5] = this->CroppingBounds[5];
	    break;
	  case 2:
	    bounds[4] = this->CroppingBounds[5];
	    bounds[5] = volumeInfo->DataSize[2] - 1;
	    break;
	  }

	for ( i = 0; i < 3; i++ )
	  {
	  if ( bounds[2*i] < 0 )
	    {
	    bounds[2*i] = 0;
	    }
	  if ( bounds[2*i + 1] > (volumeInfo->DataSize[i]-1) )
	    {
	    bounds[2*i + 1] = (volumeInfo->DataSize[i] - 1);
	    }
	  }

	if ( this->ClipRayAgainstVolume( rayInfo, volumeInfo, bounds ) )
	  {
	  // Recompute the ray length since the start and end may have been
	  // modified by ClipRayAgainstVolume() 
	  volumeRayDirection[0] = volumeRayEnd[0] - volumeRayStart[0];
	  volumeRayDirection[1] = volumeRayEnd[1] - volumeRayStart[1];
	  volumeRayDirection[2] = volumeRayEnd[2] - volumeRayStart[2];
      
	  rayLength = sqrt( (double) (volumeRayDirection[0]*volumeRayDirection[0] +
				      volumeRayDirection[1]*volumeRayDirection[1] +
				      volumeRayDirection[2]*volumeRayDirection[2]) );
	  if ( rayLength > 0.01 )
	    {
	    volumeRayDirection[0] /= rayLength;
	    volumeRayDirection[1] /= rayLength;
	    volumeRayDirection[2] /= rayLength;

	    volumeRayIncrement[0] = incrementLength * volumeRayDirection[0];
	    volumeRayIncrement[1] = incrementLength * volumeRayDirection[1];
	    volumeRayIncrement[2] = incrementLength * volumeRayDirection[2];

	    rayInfo->NumberOfStepsToTake = (rayLength / incrementLength) + 1;
      
	    for ( i = 0; i < 3; i++ )
	      {
	      if ( ( volumeRayIncrement[i] > 0.0 && 
		     ( volumeRayStart[i] + (float)(rayInfo->NumberOfStepsToTake-1) * volumeRayIncrement[i]) >
		     volumeRayEnd[i] ) ||
		   ( volumeRayIncrement[i] < 0.0 && 
		     ( volumeRayStart[i] + (float)(rayInfo->NumberOfStepsToTake-1) * volumeRayIncrement[i]) <
		     volumeRayEnd[i] ) )
		{
		rayInfo->NumberOfStepsToTake--;
		}
	      }
	    }
	  else
	    {
	    rayInfo->NumberOfStepsToTake = 0;
	    }

	  if ( rayInfo->NumberOfStepsToTake > 0 )
	    {
	    this->VolumeRayCastFunction->CastRay( rayInfo, volumeInfo );

	    for ( i = 0; i < 3; i++ )
	      {
	      if ( volumeRayDirection[i] >= volumeRayDirection[(i+1)%3] && 
		   volumeRayDirection[i] >= volumeRayDirection[(i+2)%3] )
		{
		distanceArray[arrayCount] = (volumeRayStart[i] - savedRayStart[i]) / volumeRayDirection[i];
		break;
		}
	      }
	    memcpy( (rgbaArray + 4*arrayCount), rayInfo->Color, 4*sizeof(float) );
	    for ( i = arrayCount; i > 0 && distanceArray[i] > distanceArray[i-1]; i-- )
	      {
	      tmp = distanceArray[i];
	      distanceArray[i] = distanceArray[i-1];
	      distanceArray[i-1] = tmp;

	      memcpy( tmpArray, (rgbaArray + 4*i), 4*sizeof(float) );
	      memcpy( (rgbaArray + 4*i), (rgbaArray + 4*(i-1)), 4*sizeof(float) );
	      memcpy( (rgbaArray + 4*(i-1)), tmpArray, 4*sizeof(float) );
	      }
	    
	    arrayCount++;
	    }
	  }
	}
      }

    rayInfo->Color[0] = 
      rayInfo->Color[1] =
      rayInfo->Color[2] = 0.0;
    rayInfo->Color[3] = 1.0;

    for ( i = 0; i < arrayCount; i++ )
      {
      rayInfo->Color[0] = rayInfo->Color[0] * (1.0 - rgbaArray[i*4 + 3]) + rgbaArray[i*4 + 0];
      rayInfo->Color[1] = rayInfo->Color[1] * (1.0 - rgbaArray[i*4 + 3]) + rgbaArray[i*4 + 1];
      rayInfo->Color[2] = rayInfo->Color[2] * (1.0 - rgbaArray[i*4 + 3]) + rgbaArray[i*4 + 2];
      rayInfo->Color[3] *= 1.0 - rgbaArray[i*4 + 3];
      }
    rayInfo->Color[3] = 1.0 - rayInfo->Color[3];

    }
}

int vtkVolumeRayCastMapper::ClipRayAgainstVolume( VTKRayCastRayInfo *rayInfo, 
						  VTKRayCastVolumeInfo *volumeInfo,
						  float bounds[6] )
{
  int    loop;
  float  diff;
  float  t;
  float  *rayStart, *rayEnd, *rayDirection;

  rayStart = rayInfo->TransformedStart;
  rayEnd = rayInfo->TransformedEnd;
  rayDirection = rayInfo->TransformedDirection;

  if ( rayStart[0] >= bounds[1] ||
       rayStart[1] >= bounds[3] ||
       rayStart[2] >= bounds[5] ||
       rayStart[0] < bounds[0] || 
       rayStart[1] < bounds[2] || 
       rayStart[2] < bounds[4] )
    {
    for ( loop = 0; loop < 3; loop++ )
      {
      diff = 0;

      if ( rayStart[loop] < (bounds[2*loop]+0.01) )
	{
	diff = (bounds[2*loop]+0.01) - rayStart[loop];
	}
      else if ( rayStart[loop] > (bounds[2*loop+1]-0.01) )
	{
	diff = (bounds[2*loop+1]-0.01) - rayStart[loop];
	}
      
      if ( diff )
	{
	if ( rayDirection[loop] != 0.0 ) 
	  {
	  t = diff / rayDirection[loop];
	  }
	else
	  {
	  t = -1.0;
	  }
	
	if ( t > 0.0 )
	  {
	  rayStart[0] += rayDirection[0] * t;
	  rayStart[1] += rayDirection[1] * t;
	  rayStart[2] += rayDirection[2] * t;	  	  
	  }
	}
      }
    }

  // If the voxel still isn't inside the volume, then this ray
  // doesn't really intersect the volume
	  
  if ( rayStart[0] >= bounds[1] ||
       rayStart[1] >= bounds[3] ||
       rayStart[2] >= bounds[5] ||
       rayStart[0] < bounds[0] || 
       rayStart[1] < bounds[2] || 
       rayStart[2] < bounds[4] )
    {
    return 0;
    }

  // The ray does intersect the volume, and we have a starting
  // position that is inside the volume
  if ( rayEnd[0] >= bounds[1] ||
       rayEnd[1] >= bounds[3] ||
       rayEnd[2] >= bounds[5] ||
       rayEnd[0] < bounds[0] || 
       rayEnd[1] < bounds[2] || 
       rayEnd[2] < bounds[4] )
    {
    for ( loop = 0; loop < 3; loop++ )
      {
      diff = 0;
      
      if ( rayEnd[loop] < (bounds[2*loop]+0.01) )
	{
	diff = (bounds[2*loop]+0.01) - rayEnd[loop];
	}
      else if ( rayEnd[loop] > (bounds[2*loop+1]-0.01) )
	{
	diff = (bounds[2*loop+1]-0.01) - rayEnd[loop];
	}
      
      if ( diff )
	{
	if ( rayDirection[loop] != 0.0 ) 
	  {
	  t = diff / rayDirection[loop];
	  }
	else
	  {
	  t = 1.0;
	  }
	
	if ( t < 0.0 )
	  {
	  rayEnd[0] += rayDirection[0] * t;
	  rayEnd[1] += rayDirection[1] * t;
	  rayEnd[2] += rayDirection[2] * t;

	  // Because the end clipping range might be far out, and
	  // the 0.01 might be too small compared to this number (and
	  // therefore is lost in the precision of a float) add a bit more
	  // (one hundredth of a step) so that we are sure we are inside the
	  // volume.
	  rayEnd[0] -= rayDirection[0] * 0.01;
	  rayEnd[1] -= rayDirection[1] * 0.01;
	  rayEnd[2] -= rayDirection[2] * 0.01;
	  }
	}
      }
    }
  
  if ( rayEnd[0] >= bounds[1] ||
       rayEnd[1] >= bounds[3] ||
       rayEnd[2] >= bounds[5] ||
       rayEnd[0] < bounds[0] || 
       rayEnd[1] < bounds[2] || 
       rayEnd[2] < bounds[4] )
    {
      return 0;
    }
    
  return 1;
}

void vtkVolumeRayCastMapper::GeneralImageInitialization( vtkRenderer *ren, 
							 vtkVolume *vol )
{
  vtkTransform           *scalarTransform;
  vtkTransform           *worldToVolumeTransform;
  vtkTransform           *viewToVolumeTransform;
  vtkRayCaster           *ray_caster;
  vtkStructuredPoints    *input = this->GetInput();
  float                  spacing[3], data_origin[3];
  int                    i, j;
  int                    scalarDataSize[3];

  // Create some objects that we will need later
  scalarTransform = vtkTransform::New();
  worldToVolumeTransform = vtkTransform::New();
  viewToVolumeTransform = vtkTransform::New();

  // Get a pointer to the volume renderer from the renderer
  ray_caster = ren->GetRayCaster();

  // Compute the transformation that will map the view rays (currently
  // in camera coordinates) into volume coordinates.
  // First, get the active camera transformation matrix
  viewToVolumeTransform->SetMatrix(
	     *ren->GetActiveCamera()->GetViewTransformMatrix() );

  // Now invert it so that we go from camera to world instead of
  // world to camera coordinates
  viewToVolumeTransform->Inverse();

  // Store the matrix of the volume in a temporary transformation matrix
  worldToVolumeTransform->SetMatrix(*( vol->vtkProp3D::GetMatrixPointer()) );

  // Get the origin of the data.  This translation is not accounted for in
  // the volume's matrix, so we must add it in.
  input->GetOrigin( data_origin );

  // Get the data spacing.  This scaling is not accounted for in
  // the volume's matrix, so we must add it in.
  input->GetSpacing( spacing );

  // Create a transform that will account for the scaling and translation of
  // the scalar data
  scalarTransform->Identity();
  scalarTransform->Translate(data_origin[0], data_origin[1], data_origin[2]);
  scalarTransform->Scale( spacing[0], spacing[1], spacing[2] );

  // Now concatenate the volume's matrix with this scalar data matrix
  worldToVolumeTransform->PostMultiply();
  worldToVolumeTransform->Concatenate( scalarTransform->GetMatrixPointer() );

  // Invert this matrix so that we have world to volume instead of
  // volume to world coordinates
  worldToVolumeTransform->Inverse();

  // Now concatenate the camera to world matrix with the world to volume
  // matrix to get the camera to volume matrix
  viewToVolumeTransform->PostMultiply();
  viewToVolumeTransform->Concatenate( worldToVolumeTransform->GetMatrixPointer() );

  for ( j = 0; j < 4; j++ )
    {
    for ( i = 0; i < 4; i++ )
      {
      this->WorldToVolumeMatrix[j*4 + i] = 
        worldToVolumeTransform->GetMatrixPointer()->Element[j][i];
      }
    }

  for ( j = 0; j < 4; j++ )
    {
    for ( i = 0; i < 4; i++ )
      {
      this->ViewToVolumeMatrix[j*4 + i] = 
        viewToVolumeTransform->GetMatrixPointer()->Element[j][i];
      }
    }
  // Get the size of the data for limit checks and compute increments
  input->GetDimensions( scalarDataSize );

  this->WorldSampleDistance = 
    this->SampleDistance * ray_caster->GetViewportStepSize();

  this->ScalarDataPointer = 
    input->GetPointData()->GetScalars()->GetVoidPointer(0);
  this->ScalarDataType = 
    input->GetPointData()->GetScalars()->GetDataType();
    
  if( (this->ScalarDataType != VTK_UNSIGNED_SHORT ) && 
      (this->ScalarDataType != VTK_UNSIGNED_CHAR ) )
    {
    vtkErrorMacro( << "The scalar data type: " << this->ScalarDataType <<
      " is not supported when volume rendering. Please convert the " <<
      " data to unsigned char or unsigned short.\n" );
    }

  // Set the bounds of the volume
  for ( i = 0; i < 3; i++ )
    {
    this->VolumeBounds[2*i] = 0.0;
    this->VolumeBounds[2*i+1] = scalarDataSize[i] - 1;
    }

  if ( this->Cropping )
    {
    for ( i = 0; i < 3; i++ )
      {
      if ( this->CroppingBounds[2*i] > this->VolumeBounds[2*i] )
	{
	this->VolumeBounds[2*i] = this->CroppingBounds[2*i];
	}
      if ( this->CroppingBounds[2*i+1] < this->VolumeBounds[2*i+1] )
	{
	this->VolumeBounds[2*i+1] = this->CroppingBounds[2*i+1];
	}
      }
    }

  // Delete the objects we created
  scalarTransform->Delete();
  worldToVolumeTransform->Delete();
  viewToVolumeTransform->Delete();

}



void vtkVolumeRayCastMapper::UpdateShadingTables( vtkRenderer *ren, 
						  vtkVolume *vol )
{
  int                   shading;
  vtkVolumeProperty     *volume_property;

  volume_property = vol->GetProperty();

  shading = volume_property->GetShade();

  this->GradientEstimator->SetInput( this->GetInput() );

  if ( shading )
    {
    this->GradientShader->UpdateShadingTable( ren, vol, 
					      this->GradientEstimator );
    }
}

float vtkVolumeRayCastMapper::GetZeroOpacityThreshold( vtkVolume *vol )
{
  return( this->VolumeRayCastFunction->GetZeroOpacityThreshold( vol ) );
}

// Print method for vtkVolumeRayCastMapper
void vtkVolumeRayCastMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkVolumeMapper::PrintSelf(os,indent);

  os << indent << "Sample Distance: " << this->SampleDistance << "\n";

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

}

