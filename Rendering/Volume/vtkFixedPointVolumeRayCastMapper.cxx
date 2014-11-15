/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFixedPointVolumeRayCastMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkFixedPointVolumeRayCastMapper.h"

#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkDataArray.h"
#include "vtkEncodedGradientShader.h"
#include "vtkFiniteDifferenceGradientEstimator.h"
#include "vtkImageData.h"
#include "vtkCommand.h"
#include "vtkGraphicsFactory.h"
#include "vtkSphericalDirectionEncoder.h"
#include "vtkFixedPointVolumeRayCastCompositeGOHelper.h"
#include "vtkFixedPointVolumeRayCastCompositeGOShadeHelper.h"
#include "vtkFixedPointVolumeRayCastCompositeHelper.h"
#include "vtkFixedPointVolumeRayCastCompositeShadeHelper.h"
#include "vtkFixedPointVolumeRayCastMIPHelper.h"
#include "vtkLight.h"
#include "vtkMath.h"
#include "vtkMultiThreader.h"
#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPlaneCollection.h"
#include "vtkPointData.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkTimerLog.h"
#include "vtkTransform.h"
#include "vtkVolumeProperty.h"
#include "vtkRayCastImageDisplayHelper.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkFixedPointRayCastImage.h"
#include "vtkVolumeRayCastSpaceLeapingImageFilter.h"

#include <exception>
#include <math.h>

vtkStandardNewMacro(vtkFixedPointVolumeRayCastMapper);
vtkCxxSetObjectMacro(vtkFixedPointVolumeRayCastMapper, RayCastImage, vtkFixedPointRayCastImage);

#define vtkVRCMultiplyPointMacro( A, B, M ) \
  B[0] = A[0]*M[0]  + A[1]*M[1]  + A[2]*M[2]  + M[3]; \
  B[1] = A[0]*M[4]  + A[1]*M[5]  + A[2]*M[6]  + M[7]; \
  B[2] = A[0]*M[8]  + A[1]*M[9]  + A[2]*M[10] + M[11]; \
  B[3] = A[0]*M[12] + A[1]*M[13] + A[2]*M[14] + M[15]; \
  if ( B[3] != 1.0 ) { B[0] /= B[3]; B[1] /= B[3]; B[2] /= B[3]; }

#define vtkVRCMultiplyNormalMacro( A, B, M ) \
  B[0] = A[0]*M[0]  + A[1]*M[4]  + A[2]*M[8]; \
  B[1] = A[0]*M[1]  + A[1]*M[5]  + A[2]*M[9]; \
  B[2] = A[0]*M[2]  + A[1]*M[6]  + A[2]*M[10]


//----------------------------------------------------------------------------
template <class T>
void vtkFixedPointVolumeRayCastMapperComputeCS1CGradients( T *dataPtr,
                                                           int dim[3],
                                                           double spacing[3],
                                                           double scalarRange[2],
                                                           unsigned short **gradientNormal,
                                                           unsigned char  **gradientMagnitude,
                                                           vtkDirectionEncoder *directionEncoder,
                                                           int thread_id, int thread_count,
                                                           vtkFixedPointVolumeRayCastMapper *me )
{
  int                 x, y, z;
  vtkIdType           yinc, zinc;
  int                 x_start, x_limit;
  int                 y_start, y_limit;
  int                 z_start, z_limit;
  T                   *dptr;
  float               n[3], t;
  float               gvalue=0;
  int                 xlow, xhigh;
  double              aspect[3];
  float               scale;
  unsigned short      *dirPtr;
  unsigned char       *magPtr;

  if ( thread_id == 0 )
    {
    me->InvokeEvent( vtkCommand::VolumeMapperComputeGradientsStartEvent, NULL );
    }

  double avgSpacing = (spacing[0]+spacing[1]+spacing[2])/3.0;

  // adjust the aspect
  aspect[0] = spacing[0] * 2.0 / avgSpacing;
  aspect[1] = spacing[1] * 2.0 / avgSpacing;
  aspect[2] = spacing[2] * 2.0 / avgSpacing;

  // compute the increments
  yinc = static_cast<vtkIdType>(dim[0]);
  zinc = yinc*static_cast<vtkIdType>(dim[1]);

  if ( scalarRange[1] - scalarRange[0] )
    {
    scale = 255.0 / (0.25*(scalarRange[1] - scalarRange[0]));
    }
  else
    {
    scale = 1.0;
    }

  x_start = 0;
  x_limit = dim[0];
  y_start = 0;
  y_limit = dim[1];
  z_start = (int)(( (float)thread_id / (float)thread_count ) *
                  dim[2] );
  z_limit = (int)(( (float)(thread_id + 1) / (float)thread_count ) *
                  dim[2] );

  // Do final error checking on limits - make sure they are all within bounds
  // of the scalar input

  x_start = (x_start<0)?(0):(x_start);
  y_start = (y_start<0)?(0):(y_start);
  z_start = (z_start<0)?(0):(z_start);

  x_limit = (x_limit>dim[0])?(dim[0]):(x_limit);
  y_limit = (y_limit>dim[1])?(dim[1]):(y_limit);
  z_limit = (z_limit>dim[2])?(dim[2]):(z_limit);


  int *dxBuffer = new int[dim[0]];
  int *dyBuffer = new int[dim[0]];
  int *dzBuffer = new int[dim[0]];

  for ( z = z_start; z < z_limit; z++ )
    {
    unsigned short *gradientDirPtr = gradientNormal[z];
    unsigned char *gradientMagPtr = gradientMagnitude[z];

    for ( y = y_start; y < y_limit; y++ )
      {
      xlow = x_start;
      xhigh = x_limit;

      dirPtr  = gradientDirPtr + y * yinc + xlow;
      magPtr  = gradientMagPtr + y * yinc + xlow;

      // Working on dx - that is this row
      dptr = dataPtr + z * zinc + y * yinc + xlow;
      // add this into our dxBuffer
      dxBuffer[0] = *dptr;
      for ( x = xlow+1; x < xhigh; x++ )
        {
        *(dxBuffer+x) = *(dptr+x-1);
        }

      // subtract this from our dxBuffer
      for ( x = xlow; x < xhigh-1; x++ )
        {
        *(dxBuffer+x) -= *(dptr+x+1);
        }
      dxBuffer[xhigh-1] -= dptr[xhigh-1];

      // working on dy - need the row before and the row after.
      // first, the row before, or this row if we are at the edge
      if ( y > 0 )
        {
        dptr = dataPtr + z * zinc + (y-1) * yinc + xlow;
        }
      else
        {
        dptr = dataPtr + z * zinc + y * yinc + xlow;
        }
      // add this into our dyBuffer
      for ( x = xlow; x < xhigh; x++ )
        {
        dyBuffer[x] = dptr[x];
        }

      // now the row after
      if ( y < y_limit-1 )
        {
        dptr = dataPtr + z * zinc + (y+1) * yinc + xlow;
        }
      else
        {
        dptr = dataPtr + z * zinc + y * yinc + xlow;
        }
      // subtract this from our dyBuffer
      for ( x = xlow; x < xhigh; x++ )
        {
        dyBuffer[x] -= dptr[x];
        }

      // Find the pointer for the slice before - use this if there is
      // no slice before
      if ( z > 0 )
        {
        dptr = dataPtr + (z-1) * zinc + y * yinc + xlow;
        }
      else
        {
        dptr = dataPtr + z * zinc + y * yinc + xlow;
        }

      // add this into our dzBuffer
      for ( x = xlow; x < xhigh; x++ )
        {
        dzBuffer[x] = dptr[x];
        }

      // Find the pointer for the slice after - use this if there is
      // no slice after
      if ( z < z_limit-1 )
        {
        dptr = dataPtr + (z+1) * zinc + y * yinc + xlow;
        }
      else
        {
        dptr = dataPtr + z * zinc + y * yinc + xlow;
        }

      // add this into our dzBuffer
      for ( x = xlow; x < xhigh; x++ )
        {
        dzBuffer[x] -= dptr[x];
        }

      // now one more loop to generate the normals
      for ( x = xlow; x < xhigh; x++ )
        {
        n[0] = dxBuffer[x];
        n[1] = dyBuffer[x];
        n[2] = dzBuffer[x];

        // Take care of the aspect ratio of the data
        // Scaling in the vtkVolume is isotropic, so this is the
        // only place we have to worry about non-isotropic scaling.
        n[0] /= aspect[0];
        n[1] /= aspect[1];
        n[2] /= aspect[2];

        // Compute the gradient magnitude
        t = sqrt( (double)( n[0]*n[0] +
                            n[1]*n[1] +
                            n[2]*n[2] ) );


        // Encode this into an 8 bit value
        gvalue = t * scale;

        gvalue = (gvalue<0.0)?(0.0):(gvalue);
        gvalue = (gvalue>255.0)?(255.0):(gvalue);

        // Normalize the gradient direction
        if ( t > 0.0 )
          {
          n[0] /= t;
          n[1] /= t;
          n[2] /= t;
          }
        else
          {
          n[0] = n[1] = n[2] = 0.0;
          }

        *(magPtr++) = static_cast<unsigned char>(gvalue + 0.5);
        *(dirPtr++) = directionEncoder->GetEncodedDirection( n );
        }
      }

    if ( (z/thread_count)%8 == 7 && thread_id == 0)
      {
      double args[1];
      args[0] =
        static_cast<float>(z - z_start) /
        static_cast<float>(z_limit - z_start - 1);
      me->InvokeEvent( vtkCommand::VolumeMapperComputeGradientsProgressEvent, args );
      }
    }

  delete[] dxBuffer;
  delete[] dyBuffer;
  delete[] dzBuffer;

  if ( thread_id == 0 )
    {
    me->InvokeEvent( vtkCommand::VolumeMapperComputeGradientsEndEvent, NULL );
    }

}

VTK_THREAD_RETURN_TYPE vtkFPVRCMSwitchOnDataType( void *arg )
{
  vtkFixedPointVolumeRayCastMapper   *mapper;
  int                                 thread_count;
  int                                 thread_id;

  thread_id = ((vtkMultiThreader::ThreadInfo *)(arg))->ThreadID;
  thread_count = ((vtkMultiThreader::ThreadInfo *)(arg))->NumberOfThreads;
  mapper = (vtkFixedPointVolumeRayCastMapper *)
    (((vtkMultiThreader::ThreadInfo *)(arg))->UserData);

  vtkImageData *input = mapper->GetInput();

  void *dataPtr = mapper->GetCurrentScalars()->GetVoidPointer(0);
  int scalarType   = mapper->GetCurrentScalars()->GetDataType();

  int dim[3];
  double spacing[3];
  input->GetDimensions(dim);
  input->GetSpacing(spacing);

  // Find the scalar range
  double scalarRange[2];
  mapper->GetCurrentScalars()->GetRange(scalarRange, 0);

  if ( scalarType == VTK_UNSIGNED_CHAR )
    {
    vtkFixedPointVolumeRayCastMapperComputeCS1CGradients(
      (unsigned char *)(dataPtr), dim, spacing, scalarRange,
      mapper->GradientNormal,
      mapper->GradientMagnitude,
      mapper->DirectionEncoder,
      thread_id, thread_count,
      mapper);
    }
  else if ( scalarType == VTK_UNSIGNED_SHORT )
    {
    vtkFixedPointVolumeRayCastMapperComputeCS1CGradients(
      (unsigned short *)(dataPtr), dim, spacing, scalarRange,
      mapper->GradientNormal,
      mapper->GradientMagnitude,
      mapper->DirectionEncoder,
      thread_id, thread_count,
      mapper);
    }
  else if ( scalarType == VTK_CHAR )
    {
    vtkFixedPointVolumeRayCastMapperComputeCS1CGradients(
      (char *)(dataPtr), dim, spacing, scalarRange,
      mapper->GradientNormal,
      mapper->GradientMagnitude,
      mapper->DirectionEncoder,
      thread_id, thread_count,
      mapper);
    }
  else if ( scalarType == VTK_SHORT )
    {
    vtkFixedPointVolumeRayCastMapperComputeCS1CGradients(
      (short *)(dataPtr), dim, spacing, scalarRange,
      mapper->GradientNormal,
      mapper->GradientMagnitude,
      mapper->DirectionEncoder,
      thread_id, thread_count,
      mapper);
    }

  return VTK_THREAD_RETURN_VALUE;
}


template <class T>
void vtkFixedPointVolumeRayCastMapperComputeGradients( T *dataPtr,
                                                       int dim[3],
                                                       double spacing[3],
                                                       int components,
                                                       int independent,
                                                       double scalarRange[4][2],
                                                       unsigned short **gradientNormal,
                                                       unsigned char  **gradientMagnitude,
                                                       vtkDirectionEncoder *directionEncoder,
                                                       vtkFixedPointVolumeRayCastMapper *me )
{
  int                 x, y, z, c;
  vtkIdType           yinc, zinc;
  int                 x_start, x_limit;
  int                 y_start, y_limit;
  int                 z_start, z_limit;
  T                   *dptr, *cdptr;
  float               n[3], t;
  float               gvalue=0;
  int                 xlow, xhigh;
  double              aspect[3];
  vtkIdType           xstep, ystep, zstep;
  float               scale[4];
  unsigned short      *dirPtr, *cdirPtr;
  unsigned char       *magPtr, *cmagPtr;

  int thread_id = 0;
  int thread_count = 1;

  if ( thread_id == 0 )
    {
    me->InvokeEvent( vtkCommand::VolumeMapperComputeGradientsStartEvent, NULL );
    }

  double avgSpacing = (spacing[0]+spacing[1]+spacing[2])/3.0;

  // adjust the aspect
  aspect[0] = spacing[0] * 2.0 / avgSpacing;
  aspect[1] = spacing[1] * 2.0 / avgSpacing;
  aspect[2] = spacing[2] * 2.0 / avgSpacing;

  std::cerr << "spacing is " << spacing[0] << " " << spacing[1] << " " << spacing[2] << std::endl;
  std::cerr << "aspect is " << aspect[0] << " " << aspect[1] << " " << aspect[2] << std::endl;

  // compute the increments
  yinc = static_cast<vtkIdType>(dim[0]);
  zinc = yinc*static_cast<vtkIdType>(dim[1]);

  // Compute steps through the volume in x, y, and z
  xstep = components;
  ystep = components*yinc;
  zstep = components*zinc;

  if ( !independent )
    {
    if ( scalarRange[components-1][1] - scalarRange[components-1][0] )
      {
      scale[0] = 255.0 / (0.25*(scalarRange[components-1][1] - scalarRange[components-1][0]));
      }
    else
      {
      scale[0] = 0.0;
      }
    }
  else
    {
    for (c = 0; c < components; c++ )
      {
      if ( scalarRange[c][1] - scalarRange[c][0] )
        {
        scale[c] = 255.0 / (0.25*(scalarRange[c][1] - scalarRange[c][0]));
        }
      else
        {
        scale[c] = 1.0;
        }
      }
    }

  x_start = 0;
  x_limit = dim[0];
  y_start = 0;
  y_limit = dim[1];
  z_start = (int)(( (float)thread_id / (float)thread_count ) *
                  dim[2] );
  z_limit = (int)(( (float)(thread_id + 1) / (float)thread_count ) *
                  dim[2] );

  // Do final error checking on limits - make sure they are all within bounds
  // of the scalar input

  x_start = (x_start<0)?(0):(x_start);
  y_start = (y_start<0)?(0):(y_start);
  z_start = (z_start<0)?(0):(z_start);

  x_limit = (x_limit>dim[0])?(dim[0]):(x_limit);
  y_limit = (y_limit>dim[1])?(dim[1]):(y_limit);
  z_limit = (z_limit>dim[2])?(dim[2]):(z_limit);


  int increment = (independent)?(components):(1);

  float tolerance[4];
  for ( c = 0; c < components; c++ )
    {
    tolerance[c] = .00001 * (scalarRange[c][1] - scalarRange[c][0]);
    }

  // Loop through all the data and compute the encoded normal and
  // gradient magnitude for each scalar location
  for ( z = z_start; z < z_limit; z++ )
    {
    unsigned short *gradientDirPtr = gradientNormal[z];
    unsigned char *gradientMagPtr = gradientMagnitude[z];

    for ( y = y_start; y < y_limit; y++ )
      {
      xlow = x_start;
      xhigh = x_limit;

      dptr = dataPtr + components*(z * zinc + y * yinc + xlow);

      dirPtr  = gradientDirPtr    + (y * yinc + xlow)*increment;
      magPtr  = gradientMagPtr    + (y * yinc + xlow)*increment;

      for ( x = xlow; x < xhigh; x++ )
        {
        for ( c = 0; ( independent && c < components ) || c == 0; c++ )
          {
          cdptr   = dptr   + ((independent)?(c):(components-1));
          cdirPtr = dirPtr + ((independent)?(c):(0));
          cmagPtr = magPtr + ((independent)?(c):(0));

          // Allow up to 3 tries to find the gadient - looking out at a distance of
          // 1, 2, and 3 units.
          int foundGradient = 0;
          for ( int d = 1; d <= 3 && !foundGradient; d++ )
            {
            // Use a central difference method if possible,
            // otherwise use a forward or backward difference if
            // we are on the edge
            // Compute the X component
            if ( x < d )
              {
              n[0] = 2.0*((float)*(cdptr) - (float)*(cdptr+d*xstep));
              }
            else if ( x >= dim[0] - d )
              {
              n[0] = 2.0*((float)*(cdptr-d*xstep) - (float)*(cdptr));
              }
            else
              {
            n[0] = (float)*(cdptr-d*xstep) - (float)*(cdptr+d*xstep);
              }

            // Compute the Y component
            if ( y < d )
              {
              n[1] = 2.0*((float)*(cdptr) - (float)*(cdptr+d*ystep));
              }
            else if ( y >= dim[1] - d )
              {
              n[1] = 2.0*((float)*(cdptr-d*ystep) - (float)*(cdptr));
              }
            else
              {
              n[1] = (float)*(cdptr-d*ystep) - (float)*(cdptr+d*ystep);
              }

            // Compute the Z component
            if ( z < d )
              {
              n[2] = 2.0*((float)*(cdptr) - (float)*(cdptr+d*zstep));
              }
            else if ( z >= dim[2] - d )
              {
              n[2] = 2.0*((float)*(cdptr-d*zstep) - (float)*(cdptr));
              }
            else
              {
              n[2] = (float)*(cdptr-d*zstep) - (float)*(cdptr+d*zstep);
              }

            // Take care of the aspect ratio of the data
            // Scaling in the vtkVolume is isotropic, so this is the
            // only place we have to worry about non-isotropic scaling.
            n[0] /= d*aspect[0];
            n[1] /= d*aspect[1];
            n[2] /= d*aspect[2];

            // Compute the gradient magnitude
            t = sqrt( (double)( n[0]*n[0] +
                                n[1]*n[1] +
                                n[2]*n[2] ) );


            // Encode this into an 8 bit value
            gvalue = t * scale[c];

            if ( d > 1 )
              {
              gvalue = 0;
              }

            gvalue = (gvalue<0.0)?(0.0):(gvalue);
            gvalue = (gvalue>255.0)?(255.0):(gvalue);

            // Normalize the gradient direction
            if ( t > tolerance[c] )
              {
              n[0] /= t;
              n[1] /= t;
              n[2] /= t;
              foundGradient = 1;
              }
            else
              {
              n[0] = n[1] = n[2] = 0.0;
            }
          }


          *cmagPtr = static_cast<unsigned char>(gvalue + 0.5);
          *cdirPtr = directionEncoder->GetEncodedDirection( n );
          }

        dptr    +=   components;
        dirPtr  +=   increment;
        magPtr  +=   increment;
        }
      }
    if ( (z/thread_count)%8 == 7 )
      {
      double args[1];
      args[0] =
        static_cast<float>(z - z_start) /
        static_cast<float>(z_limit - z_start - 1);
      me->InvokeEvent( vtkCommand::VolumeMapperComputeGradientsProgressEvent, args );
      }
    }

  if ( thread_id == 0 )
    {
    me->InvokeEvent( vtkCommand::VolumeMapperComputeGradientsEndEvent, NULL );
    }
}

//----------------------------------------------------------------------------
// Construct a new vtkFixedPointVolumeRayCastMapper with default values
vtkFixedPointVolumeRayCastMapper::vtkFixedPointVolumeRayCastMapper()
{
  this->SampleDistance                   =  1.0;
  this->InteractiveSampleDistance        =  2.0;
  this->ImageSampleDistance              =  1.0;
  this->MinimumImageSampleDistance       =  1.0;
  this->MaximumImageSampleDistance       = 10.0;
  this->AutoAdjustSampleDistances        =  1;
  this->LockSampleDistanceToInputSpacing =  0;

  // Should never be used without initialization, but
  // set here to avoid compiler warnings
  this->OldSampleDistance          =  1.0;
  this->OldImageSampleDistance     =  1.0;

  this->PerspectiveMatrix      = vtkMatrix4x4::New();
  this->ViewToWorldMatrix      = vtkMatrix4x4::New();
  this->ViewToVoxelsMatrix     = vtkMatrix4x4::New();
  this->VoxelsToViewMatrix     = vtkMatrix4x4::New();
  this->WorldToVoxelsMatrix    = vtkMatrix4x4::New();
  this->VoxelsToWorldMatrix    = vtkMatrix4x4::New();

  this->VolumeMatrix           = vtkMatrix4x4::New();

  this->PerspectiveTransform   = vtkTransform::New();
  this->VoxelsTransform        = vtkTransform::New();
  this->VoxelsToViewTransform  = vtkTransform::New();

  this->Threader               = vtkMultiThreader::New();
  this->ThreadWarning          = true;
  this->RayCastImage           = vtkFixedPointRayCastImage::New();

  this->RowBounds              = NULL;
  this->OldRowBounds           = NULL;

  this->RenderTimeTable        = NULL;
  this->RenderVolumeTable      = NULL;
  this->RenderRendererTable    = NULL;
  this->RenderTableSize        = 0;
  this->RenderTableEntries     = 0;

  this->RenderWindow           = NULL;

  this->MIPHelper              = vtkFixedPointVolumeRayCastMIPHelper::New();
  this->CompositeHelper        = vtkFixedPointVolumeRayCastCompositeHelper::New();
  this->CompositeGOHelper      = vtkFixedPointVolumeRayCastCompositeGOHelper::New();
  this->CompositeShadeHelper   = vtkFixedPointVolumeRayCastCompositeShadeHelper::New();
  this->CompositeGOShadeHelper = vtkFixedPointVolumeRayCastCompositeGOShadeHelper::New();

  this->IntermixIntersectingGeometry = 1;

  int i;
  for ( i = 0; i < 4; i++ )
    {
    this->SavedRGBFunction[i]             = NULL;
    this->SavedGrayFunction[i]            = NULL;
    this->SavedScalarOpacityFunction[i]   = NULL;
    this->SavedGradientOpacityFunction[i] = NULL;
    this->SavedColorChannels[i]           = 0;
    this->SavedScalarOpacityDistance[i]   = 0;
    this->TableSize[i]                    = 0;
    }

  this->SavedSampleDistance          = 0;
  this->SavedBlendMode               = -1;

  this->SavedGradientsInput          = NULL;
  this->SavedParametersInput         = NULL;

  this->NumberOfGradientSlices       = 0;
  this->GradientNormal               = NULL;
  this->GradientMagnitude            = NULL;
  this->ContiguousGradientNormal     = NULL;
  this->ContiguousGradientMagnitude  = NULL;

  this->DirectionEncoder             = vtkSphericalDirectionEncoder::New();
  this->GradientShader               = vtkEncodedGradientShader::New();
  this->GradientEstimator            = vtkFiniteDifferenceGradientEstimator::New();

  this->GradientEstimator->SetDirectionEncoder( this->DirectionEncoder );

  this->ShadingRequired              = 0;
  this->GradientOpacityRequired      = 0;

  this->CroppingRegionMask[0] = 1;
  for ( i = 1; i < 27; i++ )
    {
    this->CroppingRegionMask[i] = this->CroppingRegionMask[i-1]*2;
    }

  this->NumTransformedClippingPlanes = 0;
  this->TransformedClippingPlanes    = NULL;

  // Which scalar field are we rendering this time, and which
  // did we render last time (so we can check if it is changing)
  this->CurrentScalars = NULL;
  this->PreviousScalars = NULL;

  this->ImageDisplayHelper  = vtkRayCastImageDisplayHelper::New();
  this->ImageDisplayHelper->PreMultipliedColorsOn();
  this->ImageDisplayHelper->SetPixelScale( 2.0 );

  // This is the min max volume used for space leaping. Each 4x4x4 cell from
  // the original input volume has three values per component - a minimum scalar
  // index, maximum scalar index, and a values used for both the maximum gradient
  // magnitude and a flag. The flag is used to indicate for the
  // current transfer function whether any non-zero opacity exists between the
  // minimum and maximum scalar values and up to the maximum gradient magnitude
  this->MinMaxVolume = NULL;
  this->MinMaxVolumeSize[0] = 0;
  this->MinMaxVolumeSize[1] = 0;
  this->MinMaxVolumeSize[2] = 0;
  this->MinMaxVolumeSize[3] = 0;
  this->SavedMinMaxInput = NULL;

  this->Volume = NULL;

  this->FinalColorWindow           = 1.0;
  this->FinalColorLevel            = 0.5;

  this->FlipMIPComparison = 0;

  this->TableShift[0] = 0;
  this->TableShift[1] = 0;
  this->TableShift[2] = 0;
  this->TableShift[3] = 0;

  this->TableScale[0] = 1;
  this->TableScale[1] = 1;
  this->TableScale[2] = 1;
  this->TableScale[3] = 1;

  this->SpaceLeapFilter = vtkVolumeRayCastSpaceLeapingImageFilter::New();

  // Cached space leaping output. This is shared between runs. The output
  // of the last run is passed back to the SpaceLeapFilter and its reused
  // since we may not be updating every flag in this structure.
  this->MinMaxVolumeCache = vtkImageData::New();
}

//----------------------------------------------------------------------------
// Destruct a vtkFixedPointVolumeRayCastMapper - clean up any memory used
vtkFixedPointVolumeRayCastMapper::~vtkFixedPointVolumeRayCastMapper()
{
  this->SpaceLeapFilter->Delete();

  this->PerspectiveMatrix->Delete();
  this->ViewToWorldMatrix->Delete();
  this->ViewToVoxelsMatrix->Delete();
  this->VoxelsToViewMatrix->Delete();
  this->WorldToVoxelsMatrix->Delete();
  this->VoxelsToWorldMatrix->Delete();

  this->VolumeMatrix->Delete();

  this->VoxelsTransform->Delete();
  this->VoxelsToViewTransform->Delete();
  this->PerspectiveTransform->Delete();

  this->Threader->Delete();

  this->MIPHelper->Delete();
  this->CompositeHelper->Delete();
  this->CompositeGOHelper->Delete();
  this->CompositeShadeHelper->Delete();
  this->CompositeGOShadeHelper->Delete();

  if ( this->RayCastImage )
    {
    this->RayCastImage->Delete();
    this->RayCastImage = NULL;
    }

  delete [] this->RenderTimeTable;
  delete [] this->RenderVolumeTable;
  delete [] this->RenderRendererTable;

  delete [] this->RowBounds;
  delete [] this->OldRowBounds;

  int i;
  if ( this->GradientNormal )
    {
    // Contiguous? Delete in one chunk otherwise delete slice by slice
    if ( this->ContiguousGradientNormal )
      {
      delete [] this->ContiguousGradientNormal;
      this->ContiguousGradientNormal = NULL;
      }
    else
      {
      for ( i = 0; i < this->NumberOfGradientSlices; i++ )
        {
        delete [] this->GradientNormal[i];
        }
      }
    delete [] this->GradientNormal;
    this->GradientNormal = NULL;
    }

  if ( this->GradientMagnitude )
    {
    // Contiguous? Delete in one chunk otherwise delete slice by slice
    if ( this->ContiguousGradientMagnitude )
      {
      delete [] this->ContiguousGradientMagnitude;
      this->ContiguousGradientMagnitude = NULL;
      }
    else
      {
      for ( i = 0; i < this->NumberOfGradientSlices; i++ )
        {
        delete [] this->GradientMagnitude[i];
        }
      }
    delete [] this->GradientMagnitude;
    this->GradientMagnitude = NULL;
    }

  this->DirectionEncoder->Delete();
  this->GradientShader->Delete();
  this->GradientEstimator->Delete();

  delete [] this->TransformedClippingPlanes;

  this->ImageDisplayHelper->Delete();

  this->MinMaxVolumeCache->Delete();
}

float vtkFixedPointVolumeRayCastMapper::ComputeRequiredImageSampleDistance( float desiredTime,
                                                                            vtkRenderer *ren )
{
  return this->ComputeRequiredImageSampleDistance( desiredTime, ren, NULL );
}

float vtkFixedPointVolumeRayCastMapper::ComputeRequiredImageSampleDistance( float desiredTime,
                                                                            vtkRenderer *ren,
                                                                            vtkVolume *vol )
{
  float result;

  float oldTime;

  if ( vol )
    {
    oldTime = this->RetrieveRenderTime( ren, vol );
    }
  else
    {
    oldTime = this->RetrieveRenderTime( ren );
    }

  float newTime = desiredTime;

  if ( oldTime == 0.0 )
    {
    if ( newTime > 10 )
      {
      result = this->MinimumImageSampleDistance;
      }
    else
      {
      result = this->MaximumImageSampleDistance / 2.0;
      }
    }
  else
    {
    oldTime /= (this->ImageSampleDistance * this->ImageSampleDistance);
    result = this->ImageSampleDistance * sqrt(oldTime / newTime);
    result = (result > this->MaximumImageSampleDistance)?
      (this->MaximumImageSampleDistance):(result);
    result =
      (result<this->MinimumImageSampleDistance)?
      (this->MinimumImageSampleDistance):(result);
    }

  return result;
}

float vtkFixedPointVolumeRayCastMapper::RetrieveRenderTime( vtkRenderer *ren,
                                                            vtkVolume   *vol )
{
  int i;

  for ( i = 0; i < this->RenderTableEntries; i++ )
    {
    if ( this->RenderVolumeTable[i] == vol &&
         this->RenderRendererTable[i] == ren )
      {
      return this->RenderTimeTable[i];
      }
    }

  return 0.0;
}

float vtkFixedPointVolumeRayCastMapper::RetrieveRenderTime( vtkRenderer *ren )
{
  int i;

  for ( i = 0; i < this->RenderTableEntries; i++ )
    {
    if ( this->RenderRendererTable[i] == ren )
      {
      return this->RenderTimeTable[i];
      }
    }

  return 0.0;
}

void vtkFixedPointVolumeRayCastMapper::StoreRenderTime( vtkRenderer *ren,
                                                        vtkVolume   *vol,
                                                        float       time )
{
  int i;
  for ( i = 0; i < this->RenderTableEntries; i++ )
    {
    if ( this->RenderVolumeTable[i] == vol &&
         this->RenderRendererTable[i] == ren )
      {
      this->RenderTimeTable[i] = time;
      return;
      }
    }


  // Need to increase size
  if ( this->RenderTableEntries >= this->RenderTableSize )
    {
    if ( this->RenderTableSize == 0 )
      {
      this->RenderTableSize = 10;
      }
    else
      {
      this->RenderTableSize *= 2;
      }

    float        *oldTimePtr     = this->RenderTimeTable;
    vtkVolume   **oldVolumePtr   = this->RenderVolumeTable;
    vtkRenderer **oldRendererPtr = this->RenderRendererTable;

    this->RenderTimeTable     = new float [this->RenderTableSize];
    this->RenderVolumeTable   = new vtkVolume *[this->RenderTableSize];
    this->RenderRendererTable = new vtkRenderer *[this->RenderTableSize];

    for (i = 0; i < this->RenderTableEntries; i++ )
      {
      this->RenderTimeTable[i] = oldTimePtr[i];
      this->RenderVolumeTable[i] = oldVolumePtr[i];
      this->RenderRendererTable[i] = oldRendererPtr[i];
      }

    delete [] oldTimePtr;
    delete [] oldVolumePtr;
    delete [] oldRendererPtr;
    }

  this->RenderTimeTable[this->RenderTableEntries] = time;
  this->RenderVolumeTable[this->RenderTableEntries] = vol;
  this->RenderRendererTable[this->RenderTableEntries] = ren;

  this->RenderTableEntries++;
}

void vtkFixedPointVolumeRayCastMapper::SetNumberOfThreads( int num )
{
  this->Threader->SetNumberOfThreads( num );
}

int vtkFixedPointVolumeRayCastMapper::GetNumberOfThreads()
{
  if (this->Threader)
    {
    return this->Threader->GetNumberOfThreads();
    }
  return 0;
}

//----------------------------------------------------------------------------
// This method should be called after UpdateColorTables since it
// relies on some information (shift and scale) computed in that method,
// as well as the last built time for the color tables.
void vtkFixedPointVolumeRayCastMapper::UpdateMinMaxVolume( vtkVolume *vol )
{
  // A three bit variable:
  //   first bit indicates need to update flags
  //   second bit indicates need to update scalars
  //   third bit indicates need to update gradient magnitudes
  int needToUpdate = 0;

  // Get the image data
  vtkImageData *input = this->GetInput();

  // We'll need this info later
  int dim[3];
  input->GetDimensions( dim );


  // Has the data itself changed?
  if ( input != this->SavedMinMaxInput ||
       input->GetMTime() > this->SpaceLeapFilter->GetLastMinMaxBuildTime() ||
       this->CurrentScalars != this->PreviousScalars )
    {
    needToUpdate |= 0x03;
    }

  // Do the gradient magnitudes need to be filled in?
  if ( this->GradientOpacityRequired && // done
       ( needToUpdate&0x02 ||  // done
         this->SavedGradientsMTime.GetMTime() >
         this->SpaceLeapFilter->GetLastMinMaxBuildTime() ) )
    {
    needToUpdate |= 0x05;
    }

  // Have the parameters changed which means the flags need
  // to be recomputed. Actually, we could be checking just
  // a subset of these parameters (we don't need to recompute
  // the flags if the colors change, but unless these seems
  // like a significant performance problem, I'd rather not
  // complicate the code)
  if ( !(needToUpdate&0x01) &&
       this->SavedParametersMTime.GetMTime() >
       this->SpaceLeapFilter->GetLastMinMaxFlagTime() )
    {
    needToUpdate |= 0x01;
    }

  if ( !needToUpdate )
    {
    return;
    }


  // Set the update flags, telling the filter what to update...
  this->SpaceLeapFilter->SetInputConnection(this->GetInputConnection(0, 0));
  this->SpaceLeapFilter->SetCurrentScalars(this->CurrentScalars);
  this->SpaceLeapFilter->SetIndependentComponents(
      vol->GetProperty()->GetIndependentComponents());
  this->SpaceLeapFilter->SetComputeMinMax((needToUpdate&0x02) ? 1 : 0);
  this->SpaceLeapFilter->SetComputeGradientOpacity((needToUpdate&0x04)? 1 : 0);
  this->SpaceLeapFilter->SetUpdateGradientOpacityFlags(
      (this->GradientOpacityRequired && (needToUpdate&0x01)) ? 1 : 0 );
  this->SpaceLeapFilter->SetGradientMagnitude(this->GradientMagnitude);
  this->SpaceLeapFilter->SetTableSize(this->TableSize);
  this->SpaceLeapFilter->SetTableShift(this->TableShift);
  this->SpaceLeapFilter->SetTableScale(this->TableScale);
  for (unsigned int compIdx = 0; compIdx < 4; ++compIdx)
    {
    this->SpaceLeapFilter->SetScalarOpacityTable(
          compIdx, this->ScalarOpacityTable[compIdx]);
    this->SpaceLeapFilter->SetGradientOpacityTable(
          compIdx, this->GradientOpacityTable[compIdx]);
    }
  this->SpaceLeapFilter->SetCache(this->MinMaxVolumeCache);
  this->SpaceLeapFilter->Update();
  this->MinMaxVolume =
    this->SpaceLeapFilter->GetMinMaxVolume(this->MinMaxVolumeSize);

  // Cached space leaping output. This is shared between runs. The output
  // of the last run is passed back to the SpaceLeapFilter and its reused
  // since we may not be updating every flag in this structure.
  this->MinMaxVolumeCache->ShallowCopy(this->SpaceLeapFilter->GetOutput());

  // For debugging if necessary, write the min-max-volume components.
  //vtkVolumeRayCastSpaceLeapingImageFilter::WriteMinMaxVolume(
  //  0,this->MinMaxVolume,this->MinMaxVolumeSize,
  //  "MinMaxVolumeNewComponent0.mha");

  // If the line below is commented out, we get reference counting loops
  this->SpaceLeapFilter->SetInputConnection(NULL);


  if ( needToUpdate&0x02 )
    {
    this->SavedMinMaxInput = input;
    }
}

//----------------------------------------------------------------------------
void vtkFixedPointVolumeRayCastMapper::UpdateCroppingRegions()
{
  this->ConvertCroppingRegionPlanesToVoxels();

  int i;
  for ( i = 0; i < 6; i++ )
    {
    this->FixedPointCroppingRegionPlanes[i] =
      this->ToFixedPointPosition( this->VoxelCroppingRegionPlanes[i] );
    }

}

// This is the initialization that should be done once per image
// The render has been broken into several parts to support AMR
// volume rendering. Basically, this is done by having the AMR
// mapper call the PerImageInitialization once, then the
// PerVolumeInitialization once for each volume in the hierarchical
// structure. Finally, the AMR mapper divides all the volumes
// into subvolumes in order to render everything in a back-to-front
// order. The PerSubVolumeInitialization is called for each subvolume,
// then the RenderSubVolume is called. Finally, the DisplayImage method
// is called to map the image onto the screen. When this class is used
// directly as the mapper, the Render method calls these initialization
// methods and the RenderSubVolumeMethod. The AMR mapper will set the
// multiRender flag to 1 indicating that the PerImageInitialization
// should fully polulate the RayCastImage class based on the
// origin, spacing, and extent passed in. This will result in computing
// some things twice - once for the "full" volume (the extent bounding
// all volumes in the hierarchy), then once for each volume in the
// hierarchy. This does not make sense when rendering just a single
// volume so the multiRender flag indicates whether to do this
// computation here or skip it for later.
int vtkFixedPointVolumeRayCastMapper::PerImageInitialization( vtkRenderer *ren,
                                                              vtkVolume *vol,
                                                              int multiRender,
                                                              double inputOrigin[3],
                                                              double inputSpacing[3],
                                                              int    inputExtent[6] )
{
  // Save this so that we can restore it if the image is cancelled
  this->OldImageSampleDistance = this->ImageSampleDistance;
  this->OldSampleDistance      = this->SampleDistance;

  // If we are automatically adjusting the size to achieve a desired frame
  // rate, then do that adjustment here. Base the new image sample distance
  // on the previous one and the previous render time. Don't let
  // the adjusted image sample distance be less than the minimum image sample
  // distance or more than the maximum image sample distance.
  if ( this->AutoAdjustSampleDistances )
    {
    this->ImageSampleDistance =
      this->ComputeRequiredImageSampleDistance( vol->GetAllocatedRenderTime(), ren, vol );

    // If this is an interactive render (faster than 1 frame per second) then we'll
    // increase the sample distance along the ray to improve performance
    if ( vol->GetAllocatedRenderTime() < 1.0 )
      {
      this->SampleDistance = this->InteractiveSampleDistance;
      }
    }

  // Pass the ImageSampleDistance on the RayCastImage
  this->RayCastImage->SetImageSampleDistance( this->ImageSampleDistance );

  // The full image fills the viewport. First, compute the actual viewport
  // size, then divide by the ImageSampleDistance to find the full image
  // size in pixels
  int width, height;
  ren->GetTiledSize(&width, &height);
  this->RayCastImage->SetImageViewportSize(
    static_cast<int>(width/this->ImageSampleDistance),
    static_cast<int>(height/this->ImageSampleDistance) );

  if ( multiRender )
    {
    this->UpdateCroppingRegions();
    this->ComputeMatrices( inputOrigin,
                           inputSpacing,
                           inputExtent,
                           ren, vol );

    if ( !this->ComputeRowBounds( ren, 1, 0, inputExtent )  )
      {
      return 0;
      }
    }

  return 1;
}

// This is the initialization that should be done once per volume
void vtkFixedPointVolumeRayCastMapper::PerVolumeInitialization( vtkRenderer *ren, vtkVolume *vol )
{
  // This is the input of this mapper
  vtkImageData *input = this->GetInput();
  this->PreviousScalars = this->CurrentScalars;


  // make sure that we have scalar input and update the scalar input
  if ( input == NULL )
    {
    vtkErrorMacro(<< "No Input!");
    return;
    }
  else
    {
    vtkAlgorithm* inAlg = this->GetInputAlgorithm();
    inAlg->UpdateInformation();
    vtkStreamingDemandDrivenPipeline::SetUpdateExtentToWholeExtent(
      this->GetInputInformation());
    inAlg->Update();
    }

  int usingCellColors;
  this->CurrentScalars =
    this->GetScalars( input, this->ScalarMode,
                      this->ArrayAccessMode,
                      this->ArrayId,
                      this->ArrayName,
                      usingCellColors );

  if ( usingCellColors )
    {
    vtkErrorMacro("Cell Scalars not supported");
    return;
    }

  // Compute some matrices from voxels to view and vice versa based
  // on the whole input
  double inputSpacing[3];
  double inputOrigin[3];
  int    inputExtent[6];
  input->GetSpacing( inputSpacing );
  input->GetOrigin( inputOrigin );
  input->GetExtent( inputExtent );

  this->ComputeMatrices( inputOrigin,
                         inputSpacing,
                         inputExtent,
                         ren, vol );

  this->RenderWindow = ren->GetRenderWindow();
  this->Volume = vol;

  // Adjust the sample spacing if necessary
  if ( this->LockSampleDistanceToInputSpacing )
    {
    // compute 1/2 the average spacing
    double dist =
      (inputSpacing[0] + inputSpacing[1] + inputSpacing[2])/6.0;
    double avgNumVoxels =
      pow(static_cast<double>((inputExtent[1] - inputExtent[0]) *
                              (inputExtent[3] - inputExtent[2]) *
                              (inputExtent[5] - inputExtent[4])),
          static_cast<double>(0.333));

    if (avgNumVoxels < 100)
      {
      dist *= 0.01 + (1 - 0.01) * avgNumVoxels / 100;
      }

    // Need to treat interactive renders differently, because if
    // AutoAdjustSampleDistances is on, then we doubled the sample
    // distance
    if ( this->AutoAdjustSampleDistances &&
         vol->GetAllocatedRenderTime() < 1.0 )
      {
      if ( this->SampleDistance / (dist*2) < 0.999 ||
           this->SampleDistance / (dist*2) > 1.001)
        {
        this->OldSampleDistance         = dist;
        this->SampleDistance            = dist*2;
        this->InteractiveSampleDistance = dist*2;
        }
      }
    else if ( this->SampleDistance / dist < 0.999 ||
              this->SampleDistance / dist > 1.001 )
      {
      this->OldSampleDistance         = dist;
      this->SampleDistance            = dist;
      this->InteractiveSampleDistance = dist*2;
      }
    }

  this->UpdateColorTable( vol );
  this->UpdateGradients( vol );
  this->UpdateShadingTable( ren, vol );

  // Calls SpaceLeapFilter for a multi-threaded optimized computation of the
  // min-max structure.
  this->UpdateMinMaxVolume( vol );

}

// This is the initialization that should be done once per subvolume
void vtkFixedPointVolumeRayCastMapper::PerSubVolumeInitialization( vtkRenderer *ren, vtkVolume *vol, int multiRender )
{
  this->UpdateCroppingRegions();

  // Compute row bounds. This will also compute the size of the image to
  // render, allocate the space if necessary, and clear the image where
  // required. If no rays need to be cast, restore the old image sample
  // distance and return
  int inputExtent[6];
  vtkImageData *input = this->GetInput();
  input->GetExtent( inputExtent );

  // If this is part of a multirender (AMR volume rendering) then
  // the image parameters have already been computed and we can skip
  // that. In all cases we need to compute the row bounds so pass in
  // a 1 for that flag
  int imageFlag = (multiRender)?(0):(1);
  if ( !this->ComputeRowBounds( ren, imageFlag, 1, inputExtent )  )
    {
    this->AbortRender();
    return;
    }

  // If this is part of a multiRender, then we've already captured the z buffer,
  // otherwise we need to do it here
  if ( !multiRender )
    {
    this->CaptureZBuffer( ren );
    }

  this->InitializeRayInfo( vol );
}

// This is the render method for the subvolume
void vtkFixedPointVolumeRayCastMapper::RenderSubVolume()
{
  // Set the number of threads to use for ray casting,
  // then set the execution method and do it.
  this->InvokeEvent( vtkCommand::VolumeMapperRenderStartEvent, NULL );
  this->Threader->SetSingleMethod( FixedPointVolumeRayCastMapper_CastRays,
                                   (void *)this);
  this->Threader->SingleMethodExecute();
  this->InvokeEvent( vtkCommand::VolumeMapperRenderEndEvent, NULL );
}

// This method displays the image that has been created
void vtkFixedPointVolumeRayCastMapper::DisplayRenderedImage( vtkRenderer *ren,
                                                             vtkVolume   *vol )
{
  float depth;
  if ( this->IntermixIntersectingGeometry )
    {
    depth = this->MinimumViewDistance;
    }
  else
    {
    depth = -1;
    }


  if( this->FinalColorWindow != 1.0 ||
      this->FinalColorLevel != 0.5 )
    {
    this->ApplyFinalColorWindowLevel();
    }

  this->ImageDisplayHelper->
    RenderTexture( vol, ren,
                   this->RayCastImage,
                   depth );
}

void vtkFixedPointVolumeRayCastMapper::ApplyFinalColorWindowLevel()
{
  double scale=1.0/this->FinalColorWindow;
  double bias=0.5-this->FinalColorLevel/this->FinalColorWindow;

  unsigned short *image = this->RayCastImage->GetImage();
  unsigned short *iptr;

  int fullSize[2];
  this->RayCastImage->GetImageMemorySize(fullSize);

  int size[2];
  this->RayCastImage->GetImageInUseSize(size);

  int i, j;

  for ( j = 0; j < fullSize[1]; j++ )
    {
    iptr = image + 4*j*fullSize[0];
    for ( i = 0; i < size[0]; i++ )
      {
      int tmp;

      // Red component
      tmp = (int)((float)(*iptr)*scale + bias * (float)(*(iptr+3)));
      tmp = (tmp<0)?(0):(tmp);
      tmp = (tmp>32767)?(32767):(tmp);
      *iptr = tmp;

      // Green component
      iptr++;
      tmp = (int)((float)(*iptr)*scale + bias * (float)(*(iptr+2)));
      tmp = (tmp<0)?(0):(tmp);
      tmp = (tmp>32767)?(32767):(tmp);
      *iptr = tmp;

      // Green component
      iptr++;
      tmp = (int)((float)(*iptr)*scale + bias * (float)(*(iptr+1)));
      tmp = (tmp<0)?(0):(tmp);
      tmp = (tmp>32767)?(32767):(tmp);
      *iptr = tmp;

      // alpha - do nothing
      iptr++;
      iptr++;
      }
    }
}

// This method should be called when the render is aborted to restore previous values.
// Otherwise, the old time is still stored, with the newly computed sample distances,
// and that will cause problems on the next render.
void vtkFixedPointVolumeRayCastMapper::AbortRender()
{
  // Restore values
  this->ImageSampleDistance = this->OldImageSampleDistance;
  this->SampleDistance      = this->OldSampleDistance;
}

// Capture the ZBuffer to use for intermixing with opaque geometry
// that has already been rendered
void vtkFixedPointVolumeRayCastMapper::CaptureZBuffer( vtkRenderer *ren )
{
  // How big is the viewport in pixels?
  double *viewport   =  ren->GetViewport();
  int *renWinSize   =  ren->GetRenderWindow()->GetSize();

  // Do we need to capture the z buffer to intermix intersecting
  // geometry? If so, do it here
  if ( this->IntermixIntersectingGeometry &&
       ren->GetNumberOfPropsRendered() )
    {
    int x1, x2, y1, y2;

    // turn ImageOrigin into (x1,y1) in window (not viewport!)
    // coordinates.
    int imageOrigin[2];
    int imageInUseSize[2];
    this->RayCastImage->GetImageOrigin( imageOrigin );
    this->RayCastImage->GetImageInUseSize( imageInUseSize );

    x1 = static_cast<int> (
      viewport[0] * static_cast<float>(renWinSize[0]) +
      static_cast<float>(imageOrigin[0]) * this->ImageSampleDistance );
    y1 = static_cast<int> (
      viewport[1] * static_cast<float>(renWinSize[1]) +
      static_cast<float>(imageOrigin[1]) * this->ImageSampleDistance);

    int zbufferSize[2];
    int zbufferOrigin[2];

    // compute z buffer size
    zbufferSize[0] = static_cast<int>(
      static_cast<float>(imageInUseSize[0]) * this->ImageSampleDistance);
    zbufferSize[1] = static_cast<int>(
      static_cast<float>(imageInUseSize[1]) * this->ImageSampleDistance);

    // Use the size to compute (x2,y2) in window coordinates
    x2 = x1 + zbufferSize[0] - 1;
    y2 = y1 + zbufferSize[1] - 1;

    // This is the z buffer origin (in viewport coordinates)
    zbufferOrigin[0] = static_cast<int>(
      static_cast<float>(imageOrigin[0]) * this->ImageSampleDistance);
    zbufferOrigin[1] = static_cast<int>(
      static_cast<float>(imageOrigin[1]) * this->ImageSampleDistance);

    this->RayCastImage->SetZBufferSize( zbufferSize );
    this->RayCastImage->SetZBufferOrigin( zbufferOrigin );
    this->RayCastImage->AllocateZBuffer();

    // Capture the z buffer
    ren->GetRenderWindow()->GetZbufferData( x1, y1, x2, y2,
                                            this->RayCastImage->GetZBuffer() );

    this->RayCastImage->UseZBufferOn();
    }
  else
    {
    this->RayCastImage->UseZBufferOff();
    }
}


void vtkFixedPointVolumeRayCastMapper::Render( vtkRenderer *ren, vtkVolume *vol )
{
//  if (this->Threader->GetNumberOfThreads() > 1 && this->ThreadWarning)
//    {
//    vtkWarningMacro("Number of threads "
//                    << this->Threader->GetNumberOfThreads()
//                    << " is  > 1. This class does not produce repeatable results when the number of threads exceeds 1.");
//    this->ThreadWarning = false;
//    }
  this->Timer->StartTimer();

  // Since we are passing in a value of 0 for the multiRender flag
  // (this is a single render pass - not part of a multipass AMR render)
  // then we know the origin, spacing, and extent values will not
  // be used so just initialize everything to 0. No need to check
  // the return value of the PerImageInitialization method - since this
  // is not a multirender it will always return 1.
  double dummyOrigin[3]  = {0.0, 0.0, 0.0};
  double dummySpacing[3] = {0.0, 0.0, 0.0};
  int dummyExtent[6] = {0, 0, 0, 0, 0, 0};
  this->PerImageInitialization( ren, vol, 0,
                                dummyOrigin,
                                dummySpacing,
                                dummyExtent );

  this->PerVolumeInitialization( ren, vol );

  vtkRenderWindow *renWin=ren->GetRenderWindow();

  if ( renWin && renWin->CheckAbortStatus() )
    {
    this->AbortRender();
    return;
    }

  this->PerSubVolumeInitialization( ren, vol, 0 );
  if ( renWin && renWin->CheckAbortStatus() )
    {
    this->AbortRender();
    return;
    }

  this->RenderSubVolume();

  if ( renWin && renWin->CheckAbortStatus() )
    {
    this->AbortRender();
    return;
    }

  this->DisplayRenderedImage( ren, vol );

  this->Timer->StopTimer();
  this->TimeToDraw = this->Timer->GetElapsedTime();
  // If we've increased the sample distance, account for that in the stored time. Since we
  // don't get linear performance improvement, use a factor of .66
  this->StoreRenderTime( ren, vol,
                         this->TimeToDraw *
                         this->ImageSampleDistance *
                         this->ImageSampleDistance *
                         ( 1.0 + 0.66*
                           (this->SampleDistance - this->OldSampleDistance) /
                           this->OldSampleDistance ) );

  this->SampleDistance = this->OldSampleDistance;
}

VTK_THREAD_RETURN_TYPE FixedPointVolumeRayCastMapper_CastRays( void *arg )
{
  // Get the info out of the input structure
  int threadID    = ((vtkMultiThreader::ThreadInfo *)(arg))->ThreadID;
  int threadCount = ((vtkMultiThreader::ThreadInfo *)(arg))->NumberOfThreads;

  vtkFixedPointVolumeRayCastMapper *me = (vtkFixedPointVolumeRayCastMapper *)(((vtkMultiThreader::ThreadInfo *)arg)->UserData);

  if ( !me )
    {
    vtkGenericWarningMacro("Irrecoverable error: no mapper specified");
    return VTK_THREAD_RETURN_VALUE;
    }

  vtkVolume *vol = me->GetVolume();

  if ( me->GetBlendMode() == vtkVolumeMapper::MAXIMUM_INTENSITY_BLEND ||
       me->GetBlendMode() == vtkVolumeMapper::MINIMUM_INTENSITY_BLEND )
    {
    me->GetMIPHelper()->GenerateImage( threadID, threadCount, vol, me );
    }
  else
    {
    if ( me->GetShadingRequired() == 0 )
      {
      if ( me->GetGradientOpacityRequired() == 0 )
        {
        me->GetCompositeHelper()->GenerateImage( threadID, threadCount, vol, me );
        }
      else
        {
        me->GetCompositeGOHelper()->GenerateImage( threadID, threadCount, vol, me );
        }
      }
    else
      {
      if ( me->GetGradientOpacityRequired() == 0 )
        {
        me->GetCompositeShadeHelper()->GenerateImage( threadID, threadCount, vol, me );
        }
      else
        {
        me->GetCompositeGOShadeHelper()->GenerateImage( threadID, threadCount, vol, me );
        }
      }
    }

  return VTK_THREAD_RETURN_VALUE;
}

// Create an image into the vtkImageData argmument. Used generally for
// creating thumbnail images
void vtkFixedPointVolumeRayCastMapper::CreateCanonicalView( vtkVolume *vol,
                                                            vtkImageData *image,
                                                            int blend_mode,
                                                            double direction[3],
                                                            double viewUp[3] )
{
  // Make sure we have as long as we'd like so that the
  // image sample distance will be 1.0
  vol->SetAllocatedRenderTime(VTK_DOUBLE_MAX, NULL);

  // Create a renderer / camera with the right parameters
  // These will never be mapped to the screen - just used
  // to hold parameters such as size, view direction, aspect,
  // etc.
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  vtkRenderer *ren = vtkRenderer::New();
  vtkCamera *cam = ren->GetActiveCamera();

  renWin->AddRenderer(ren);
  int dim[3];
  image->GetDimensions(dim);

  // The size of the window is the size of the image
  renWin->SetSize( dim[0], dim[1] );

  double *center =  vol->GetCenter();

  double bnds[6];
  vol->GetBounds(bnds);
  double d = sqrt((bnds[1]-bnds[0])*(bnds[1]-bnds[0]) +
                  (bnds[3]-bnds[2])*(bnds[3]-bnds[2]) +
                  (bnds[5]-bnds[4])*(bnds[5]-bnds[4]));

  // For now use x distance - need to change this
  d = bnds[1]-bnds[0];

  // Set up the camera in parallel
  cam->SetFocalPoint( center );
  cam->ParallelProjectionOn();
  cam->SetPosition( center[0] - d*direction[0],
                    center[1] - d*direction[1],
                    center[2] - d*direction[2] );
  cam->SetViewUp(viewUp);

  cam->SetParallelScale(d/2);

  // Add a light
  vtkLight *light = vtkLight::New();
  light->SetPosition( center[0] - d*direction[0],
                      center[1] - d*direction[1],
                      center[2] - d*direction[2] );
  light->SetFocalPoint( center );
  ren->AddLight(light);

  int savedBlendMode = this->BlendMode;
  this->BlendMode = blend_mode;
  int savedCropping = this->Cropping;
  this->Cropping = 0;

  // Do all the initialization. This is not a multipass image
  // so just pass in dummy origin, spacing, and extent here
  double dummyOrigin[3]  = {0.0, 0.0, 0.0};
  double dummySpacing[3] = {0.0, 0.0, 0.0};
  int dummyExtent[6] = {0, 0, 0, 0, 0, 0};
  this->PerImageInitialization( ren, vol, 0,
                                dummyOrigin,
                                dummySpacing,
                                dummyExtent );
  this->PerVolumeInitialization( ren, vol );
  this->PerSubVolumeInitialization( ren, vol, 0 );

  // Render the image
  this->RenderSubVolume();

  // Now copy the image into the vtkImageData
  unsigned char *outPtr = (unsigned char *)image->GetScalarPointer();
  unsigned short *inPtr = this->RayCastImage->GetImage();

  int viewportSize[2];
  int inUseSize[2];
  int memorySize[2];
  int origin[2];

  this->RayCastImage->GetImageViewportSize( viewportSize );
  this->RayCastImage->GetImageInUseSize( inUseSize );
  this->RayCastImage->GetImageMemorySize( memorySize );
  this->RayCastImage->GetImageOrigin( origin );

  int i, j;
  for ( j = 0; j < dim[1]; j++ )
    {
    for ( i = 0; i < dim[0]; i++ )
      {
      if ( j < origin[1] || (j-origin[1]) >= inUseSize[1] ||
           i < origin[0] || (i-origin[0]) >= inUseSize[0] )
        {
        *(outPtr++) = 0;
        *(outPtr++) = 0;
        *(outPtr++) = 0;
        }
      else
        {
        unsigned short *tmp = inPtr + (j-origin[1])*memorySize[0]*4 + (i-origin[0])*4;
        *(outPtr++) = (*(tmp++))>>7;
        *(outPtr++) = (*(tmp++))>>7;
        *(outPtr++) = (*(tmp++))>>7;
        }
      }
    }

  // Restore

  this->SampleDistance = this->OldSampleDistance;
  this->BlendMode = savedBlendMode;
  this->Cropping = savedCropping;

  //Clean up
  renWin->RemoveRenderer(ren);
  ren->RemoveLight(light);

  renWin->Delete();
  ren->Delete();
  light->Delete();
}

void vtkFixedPointVolumeRayCastMapper::ComputeRayInfo( int x, int y, unsigned int pos[3],
                                                       unsigned int dir[3],
                                                       unsigned int *numSteps )
{
  float viewRay[3];
  float rayDirection[3];
  float rayStart[4], rayEnd[4];

  int imageViewportSize[2];
  int imageOrigin[2];
  this->RayCastImage->GetImageViewportSize( imageViewportSize );
  this->RayCastImage->GetImageOrigin( imageOrigin );

  float offsetX = 1.0 / static_cast<float>(imageViewportSize[0]);
  float offsetY = 1.0 / static_cast<float>(imageViewportSize[1]);


  // compute the view point y value for this row. Do this by
  // taking our pixel position, adding the image origin then dividing
  // by the full image size to get a number from 0 to 1-1/fullSize. Then,
  // multiply by two and subtract one to get a number from
  // -1 to 1 - 2/fullSize. Then add offsetX (which is 1/fullSize) to
  // center it.
  viewRay[1] = ((static_cast<float>(y) +
                 static_cast<float>(imageOrigin[1])) /
                imageViewportSize[1]) * 2.0 - 1.0 + offsetY;

  // compute the view point x value for this pixel. Do this by
  // taking our pixel position, adding the image origin then dividing
  // by the full image size to get a number from 0 to 1-1/fullSize. Then,
  // multiply by two and subtract one to get a number from
  // -1 to 1 - 2/fullSize. Then add offsetX (which is 1/fullSize) to
  // center it.
  viewRay[0] = ((static_cast<float>(x) +
                 static_cast<float>(imageOrigin[0])) /
                imageViewportSize[0]) * 2.0 - 1.0 + offsetX;

  // Now transform this point with a z value of 0 for the ray start, and
  // a z value of 1 for the ray end. This corresponds to the near and far
  // plane locations. If IntermixIntersectingGeometry is on, then use
  // the zbuffer value instead of 1.0
  viewRay[2] = 0.0;
  vtkVRCMultiplyPointMacro( viewRay, rayStart,
                            this->ViewToVoxelsArray );

  viewRay[2] = this->RayCastImage->GetZBufferValue(x,y);

  vtkVRCMultiplyPointMacro( viewRay, rayEnd,
                            this->ViewToVoxelsArray );

  rayDirection[0] = rayEnd[0] - rayStart[0];
  rayDirection[1] = rayEnd[1] - rayStart[1];
  rayDirection[2] = rayEnd[2] - rayStart[2];

  float originalRayStart[3];
  originalRayStart[0] = rayStart[0];
  originalRayStart[1] = rayStart[1];
  originalRayStart[2] = rayStart[2];


  // Initialize with 0, fill in with actual number of steps
  // if necessary
  *numSteps = 0;

  if ( this->ClipRayAgainstVolume( rayStart,
                                   rayEnd,
                                   rayDirection,
                                   this->CroppingBounds ) &&
       ( this->NumTransformedClippingPlanes == 0 ||
         this->ClipRayAgainstClippingPlanes( rayStart,
                                             rayEnd,
                                             this->NumTransformedClippingPlanes,
                                             this->TransformedClippingPlanes ) ) )
    {
    double worldRayDirection[3];
    worldRayDirection[0] = rayDirection[0]*this->SavedSpacing[0];
    worldRayDirection[1] = rayDirection[1]*this->SavedSpacing[1];
    worldRayDirection[2] = rayDirection[2]*this->SavedSpacing[2];
    double worldLength =
      vtkMath::Normalize( worldRayDirection ) / this->SampleDistance;

    rayDirection[0] /= worldLength;
    rayDirection[1] /= worldLength;
    rayDirection[2] /= worldLength;

    float diff[3];
    diff[0] = (rayStart[0] - originalRayStart[0])*((rayDirection[0]<0)?(-1):(1));
    diff[1] = (rayStart[1] - originalRayStart[1])*((rayDirection[1]<0)?(-1):(1));
    diff[2] = (rayStart[2] - originalRayStart[2])*((rayDirection[2]<0)?(-1):(1));

    int steps = -1;

    if ( diff[0] >= diff[1] && diff[0] >= diff[2] && rayDirection[0])
      {
      steps = 1 + static_cast<int>( diff[0] /
                                    ((rayDirection[0]<0)?(-rayDirection[0]):(rayDirection[0])) );
      }

    if ( diff[1] >= diff[0] && diff[1] >= diff[2] && rayDirection[1])
      {
      steps = 1 + static_cast<int>( diff[1] /
                                    ((rayDirection[1]<0)?(-rayDirection[1]):(rayDirection[1])) );
      }

    if ( diff[2] >= diff[0] && diff[2] >= diff[1] && rayDirection[2])
      {
      steps = 1 + static_cast<int>( diff[2] /
                                    ((rayDirection[2]<0)?(-rayDirection[2]):(rayDirection[2])) );
      }

    if ( steps > 0 )
      {
      rayStart[0] = originalRayStart[0] + steps*rayDirection[0];
      rayStart[1] = originalRayStart[1] + steps*rayDirection[1];
      rayStart[2] = originalRayStart[2] + steps*rayDirection[2];
      }

    if ( rayStart[0] > 0.0 && rayStart[1] > 0.0 && rayStart[2] > 0.0 )
      {
      pos[0] = this->ToFixedPointPosition(rayStart[0]);
      pos[1] = this->ToFixedPointPosition(rayStart[1]);
      pos[2] = this->ToFixedPointPosition(rayStart[2]);
      dir[0] = this->ToFixedPointDirection(rayDirection[0]);
      dir[1] = this->ToFixedPointDirection(rayDirection[1]);
      dir[2] = this->ToFixedPointDirection(rayDirection[2]);

      int stepLoop;
      int stepsValid = 0;
      unsigned int currSteps;
      for ( stepLoop = 0; stepLoop < 3; stepLoop++ )
        {
        if ( !( dir[stepLoop]&0x7fffffff ) )
          {
          continue;
          }

        unsigned int endVal = this->ToFixedPointPosition(rayEnd[stepLoop]);

        if ( dir[stepLoop]&0x80000000 )
          {
          if ( endVal > pos[stepLoop] )
            {
            currSteps = static_cast<unsigned int>(
              1 + (endVal - pos[stepLoop])/(dir[stepLoop]&0x7fffffff));
            }
          else
            {
            currSteps = 0;
            }
          }
        else
          {
          if ( pos[stepLoop] > endVal )
            {
            currSteps = 1 + (pos[stepLoop]- endVal)/dir[stepLoop];
            }
          else
            {
            currSteps = 0;
            }
          }

        if ( !stepsValid || currSteps < *numSteps )
          {
          *numSteps = currSteps;
          stepsValid = 1;
          }
        }
      }
    }
}

void vtkFixedPointVolumeRayCastMapper::InitializeRayInfo( vtkVolume   *vol )
{
  if ( !vol )
    {
    return;
    }

  // Copy the viewToVoxels matrix to 16 floats
  int i, j;
  for ( j = 0; j < 4; j++ )
    {
    for ( i = 0; i < 4; i++ )
      {
      this->ViewToVoxelsArray[j*4+i] =
        static_cast<float>(this->ViewToVoxelsMatrix->GetElement(j,i));
      }
    }

  // Copy the worldToVoxels matrix to 16 floats
  for ( j = 0; j < 4; j++ )
    {
    for ( i = 0; i < 4; i++ )
      {
      this->WorldToVoxelsArray[j*4+i] =
        static_cast<float>(this->WorldToVoxelsMatrix->GetElement(j,i));
      }
    }

  // Copy the voxelsToWorld matrix to 16 floats
  for ( j = 0; j < 4; j++ )
    {
    for ( i = 0; i < 4; i++ )
      {
      this->VoxelsToWorldArray[j*4+i] =
        static_cast<float>(this->VoxelsToWorldMatrix->GetElement(j,i));
      }
    }

  int dim[3];
  this->GetInput()->GetDimensions(dim);
  this->CroppingBounds[0] = this->CroppingBounds[2] = this->CroppingBounds[4] = 0.0;
  this->CroppingBounds[1] = dim[0]-1;
  this->CroppingBounds[3] = dim[1]-1;
  this->CroppingBounds[5] = dim[2]-1;


  // Do some initialization of the clipping planes
  this->NumTransformedClippingPlanes = (this->ClippingPlanes)?(this->ClippingPlanes->GetNumberOfItems()):(0);

  // Clear out old clipping planes
  delete [] this->TransformedClippingPlanes;
  this->TransformedClippingPlanes = NULL;

  // Do we have any clipping planes
  if ( this->NumTransformedClippingPlanes > 0 )
    {
    // Allocate some space to store the plane equations
    this->TransformedClippingPlanes = new float [4*this->NumTransformedClippingPlanes];

    // loop through all the clipping planes
    for ( i = 0; i < this->NumTransformedClippingPlanes; i++ )
      {
      // Convert plane into voxel coordinate system
      double worldNormal[3], worldOrigin[3];
      double inputOrigin[4];
      vtkPlane *onePlane = (vtkPlane *)this->ClippingPlanes->GetItemAsObject(i);
      onePlane->GetNormal(worldNormal);
      onePlane->GetOrigin(worldOrigin);
      float *planePtr = this->TransformedClippingPlanes + 4*i;
      vtkVRCMultiplyNormalMacro( worldNormal,
                                 planePtr,
                                 this->VoxelsToWorldArray );
      vtkVRCMultiplyPointMacro( worldOrigin, inputOrigin,
                                this->WorldToVoxelsArray );

      float t = sqrt( planePtr[0]*planePtr[0] +
                      planePtr[1]*planePtr[1] +
                      planePtr[2]*planePtr[2] );
      if ( t )
        {
        planePtr[0] /= t;
        planePtr[1] /= t;
        planePtr[2] /= t;
        }

      planePtr[3] = -(planePtr[0]*inputOrigin[0] +
                      planePtr[1]*inputOrigin[1] +
                      planePtr[2]*inputOrigin[2]);
      }
    }

  // If we have a simple crop box then we can tighten the bounds
  if ( this->Cropping && this->CroppingRegionFlags == 0x2000 )
    {
    this->CroppingBounds[0] = this->VoxelCroppingRegionPlanes[0];
    this->CroppingBounds[1] = this->VoxelCroppingRegionPlanes[1];
    this->CroppingBounds[2] = this->VoxelCroppingRegionPlanes[2];
    this->CroppingBounds[3] = this->VoxelCroppingRegionPlanes[3];
    this->CroppingBounds[4] = this->VoxelCroppingRegionPlanes[4];
    this->CroppingBounds[5] = this->VoxelCroppingRegionPlanes[5];
    }

  this->CroppingBounds[0] = (this->CroppingBounds[0] < 0)?(0):(this->CroppingBounds[0]);
  this->CroppingBounds[0] = (this->CroppingBounds[0] > dim[0]-1)?(dim[0]-1):(this->CroppingBounds[0]);
  this->CroppingBounds[1] = (this->CroppingBounds[1] < 0)?(0):(this->CroppingBounds[1]);
  this->CroppingBounds[1] = (this->CroppingBounds[1] > dim[0]-1)?(dim[0]-1):(this->CroppingBounds[1]);
  this->CroppingBounds[2] = (this->CroppingBounds[2] < 0)?(0):(this->CroppingBounds[2]);
  this->CroppingBounds[2] = (this->CroppingBounds[2] > dim[1]-1)?(dim[1]-1):(this->CroppingBounds[2]);
  this->CroppingBounds[3] = (this->CroppingBounds[3] < 0)?(0):(this->CroppingBounds[3]);
  this->CroppingBounds[3] = (this->CroppingBounds[3] > dim[1]-1)?(dim[1]-1):(this->CroppingBounds[3]);
  this->CroppingBounds[4] = (this->CroppingBounds[4] < 0)?(0):(this->CroppingBounds[4]);
  this->CroppingBounds[4] = (this->CroppingBounds[4] > dim[2]-1)?(dim[2]-1):(this->CroppingBounds[4]);
  this->CroppingBounds[5] = (this->CroppingBounds[5] < 0)?(0):(this->CroppingBounds[5]);
  this->CroppingBounds[5] = (this->CroppingBounds[5] > dim[2]-1)?(dim[2]-1):(this->CroppingBounds[5]);

  // Save spacing because for some reason this call is really really slow!
  this->GetInput()->GetSpacing(this->SavedSpacing);
}

// Return 0 if our volume is outside the view frustum, 1 if it
// is in the view frustum.
int vtkFixedPointVolumeRayCastMapper::ComputeRowBounds(vtkRenderer *ren,
                                                       int imageFlag,
                                                       int rowBoundsFlag,
                                                       int inputExtent[6] )
{
  float voxelPoint[3];
  float viewPoint[8][4];
  int i, j, k;
  unsigned short *ucptr;
  float minX, minY, maxX, maxY, minZ, maxZ;

  minX =  1.0;
  minY =  1.0;
  maxX = -1.0;
  maxY = -1.0;
  minZ =  1.0;
  maxZ =  0.0;

  float bounds[6];
  int dim[3];
  dim[0] = inputExtent[1] - inputExtent[0] + 1;
  dim[1] = inputExtent[3] - inputExtent[2] + 1;
  dim[2] = inputExtent[5] - inputExtent[4] + 1;


  bounds[0] = bounds[2] = bounds[4] = 0.0;
  bounds[1] = static_cast<float>(dim[0]-1);
  bounds[3] = static_cast<float>(dim[1]-1);
  bounds[5] = static_cast<float>(dim[2]-1);

  int insideFlag = 0;
  double camPos[4];
  ren->GetActiveCamera()->GetPosition( camPos );
  camPos[3] = 1.0;
  this->WorldToVoxelsMatrix->MultiplyPoint( camPos, camPos );
  if ( camPos[3] )
    {
    camPos[0] /= camPos[3];
    camPos[1] /= camPos[3];
    camPos[2] /= camPos[3];
    }


  // If we have a simple crop box then we can tighten the bounds
  if ( this->Cropping && this->CroppingRegionFlags == 0x2000 )
    {
    bounds[0] = this->VoxelCroppingRegionPlanes[0];
    bounds[1] = this->VoxelCroppingRegionPlanes[1];
    bounds[2] = this->VoxelCroppingRegionPlanes[2];
    bounds[3] = this->VoxelCroppingRegionPlanes[3];
    bounds[4] = this->VoxelCroppingRegionPlanes[4];
    bounds[5] = this->VoxelCroppingRegionPlanes[5];
    }


  if ( camPos[0] >= bounds[0] &&
       camPos[0] <= bounds[1] &&
       camPos[1] >= bounds[2] &&
       camPos[1] <= bounds[3] &&
       camPos[2] >= bounds[4] &&
       camPos[2] <= bounds[5] )
    {
    insideFlag = 1;
    }

  // Copy the voxelsToView matrix to 16 floats
  float voxelsToViewMatrix[16];
  for ( j = 0; j < 4; j++ )
    {
    for ( i = 0; i < 4; i++ )
      {
      voxelsToViewMatrix[j*4+i] =
        static_cast<float>(this->VoxelsToViewMatrix->GetElement(j,i));
      }
    }

  // Convert the voxel bounds to view coordinates to find out the
  // size and location of the image we need to generate.
  int idx = 0;
  if ( insideFlag )
    {
    minX = -1.0;
    maxX =  1.0;
    minY = -1.0;
    maxY =  1.0;
    minZ =  0.001;
    maxZ =  0.001;
    }
  else
    {
    for ( k = 0; k < 2; k++ )
      {
      voxelPoint[2] = bounds[4+k];
      for ( j = 0; j < 2; j++ )
        {
        voxelPoint[1] = bounds[2+j];
        for ( i = 0; i < 2; i++ )
          {
          voxelPoint[0] = bounds[i];
          vtkVRCMultiplyPointMacro( voxelPoint, viewPoint[idx],
                                    voxelsToViewMatrix );

          minX = (viewPoint[idx][0]<minX)?(viewPoint[idx][0]):(minX);
          minY = (viewPoint[idx][1]<minY)?(viewPoint[idx][1]):(minY);
          maxX = (viewPoint[idx][0]>maxX)?(viewPoint[idx][0]):(maxX);
          maxY = (viewPoint[idx][1]>maxY)?(viewPoint[idx][1]):(maxY);
          minZ = (viewPoint[idx][2]<minZ)?(viewPoint[idx][2]):(minZ);
          maxZ = (viewPoint[idx][2]>maxZ)?(viewPoint[idx][2]):(maxZ);
          idx++;
          }
        }
      }
    }

  if ( minZ < 0.001 || maxZ > 0.9999 )
    {
    minX = -1.0;
    maxX =  1.0;
    minY = -1.0;
    maxY =  1.0;
    insideFlag = 1;
    }

  this->MinimumViewDistance =
    (minZ<0.001)?(0.001):((minZ>0.999)?(0.999):(minZ));

  int imageViewportSize[2];
  int imageOrigin[2];
  int imageMemorySize[2];
  int imageInUseSize[2];
  this->RayCastImage->GetImageViewportSize( imageViewportSize );
  this->RayCastImage->GetImageOrigin( imageOrigin );
  this->RayCastImage->GetImageMemorySize( imageMemorySize );

  // We have min/max values from -1.0 to 1.0 now - we want to convert
  // these to pixel locations. Give a couple of pixels of breathing room
  // on each side if possible
  minX = ( minX + 1.0 ) * 0.5 * imageViewportSize[0] - 2;
  minY = ( minY + 1.0 ) * 0.5 * imageViewportSize[1] - 2;
  maxX = ( maxX + 1.0 ) * 0.5 * imageViewportSize[0] + 2;
  maxY = ( maxY + 1.0 ) * 0.5 * imageViewportSize[1] + 2;

  // If we are outside the view frustum return 0 - there is no need
  // to render anything
  if ( ( minX < 0 && maxX < 0 ) ||
       ( minY < 0 && maxY < 0 ) ||
       ( minX > imageViewportSize[0]-1 &&
         maxX > imageViewportSize[0]-1 ) ||
       ( minY > imageViewportSize[1]-1 &&
         maxY > imageViewportSize[1]-1 ) )
    {
    return 0;
    }

  int oldImageMemorySize[2];
  oldImageMemorySize[0] = imageMemorySize[0];
  oldImageMemorySize[1] = imageMemorySize[1];

  // Check the bounds - the volume might project outside of the
  // viewing box / frustum so clip it if necessary
  minX = (minX<0)?(0):(minX);
  minY = (minY<0)?(0):(minY);
  maxX = (maxX>imageViewportSize[0]-1)?
    (imageViewportSize[0]-1):(maxX);
  maxY = (maxY>imageViewportSize[1]-1)?
    (imageViewportSize[1]-1):(maxY);

  // Create the new image, and set its size and position
  imageInUseSize[0] = static_cast<int>(maxX - minX + 1.0);
  imageInUseSize[1] = static_cast<int>(maxY - minY + 1.0);

  // What is a power of 2 size big enough to fit this image?
  imageMemorySize[0] = 32;
  imageMemorySize[1] = 32;
  while ( imageMemorySize[0] < imageInUseSize[0] )
    {
    imageMemorySize[0] *= 2;
    }
  while ( imageMemorySize[1] < imageInUseSize[1] )
    {
    imageMemorySize[1] *= 2;
    }

  imageOrigin[0] = static_cast<int>(minX);
  imageOrigin[1] = static_cast<int>(minY);

  // If the old image size is much too big (more than twice in
  // either direction) then set the old width to 0 which will
  // cause the image to be recreated
  if ( oldImageMemorySize[0] > 4*imageMemorySize[0] ||
       oldImageMemorySize[1] > 4*imageMemorySize[1] )
    {
    oldImageMemorySize[0] = 0;
    }

  // If the old image is big enough (but not too big - we handled
  // that above) then we'll bump up our required size to the
  // previous one. This will keep us from thrashing.
  if ( oldImageMemorySize[0] >= imageMemorySize[0] &&
       oldImageMemorySize[1] >= imageMemorySize[1] )
    {
    imageMemorySize[0] = oldImageMemorySize[0];
    imageMemorySize[1] = oldImageMemorySize[1];
    }

  if ( imageFlag )
    {
    this->RayCastImage->SetImageOrigin( imageOrigin );
    this->RayCastImage->SetImageMemorySize( imageMemorySize );
    this->RayCastImage->SetImageInUseSize( imageInUseSize );

    // Do we already have a texture big enough? If not, create a new one and
    // clear it.
    if ( imageMemorySize[0] > oldImageMemorySize[0] ||
         imageMemorySize[1] > oldImageMemorySize[1] )
      {
      this->RayCastImage->AllocateImage();
      delete [] this->RowBounds;
      delete [] this->OldRowBounds;

      this->RayCastImage->ClearImage();

      if ( rowBoundsFlag )
        {
        // Create the row bounds array. This will store the start / stop pixel
        // for each row. This helps eleminate work in areas outside the bounding
        // hexahedron since a bounding box is not very tight. We keep the old ones
        // too to help with only clearing where required
        this->RowBounds = new int [2*imageMemorySize[1]];
        this->OldRowBounds = new int [2*imageMemorySize[1]];

        for ( i = 0; i < imageMemorySize[1]; i++ )
          {
          this->RowBounds[i*2]      = imageMemorySize[0];
          this->RowBounds[i*2+1]    = -1;
          this->OldRowBounds[i*2]   = imageMemorySize[0];
          this->OldRowBounds[i*2+1] = -1;
          }
        }
      }
    }

  if ( !rowBoundsFlag )
    {
    return 1;
    }


  // Swap the row bounds
  int *tmpptr;
  tmpptr = this->RowBounds;
  this->RowBounds = this->OldRowBounds;
  this->OldRowBounds = tmpptr;

  // If we are inside the volume our row bounds indicate every ray must be
  // cast - we don't need to intersect with the 12 lines
  if ( insideFlag )
    {
    for ( j = 0; j < imageInUseSize[1]; j++ )
      {
      this->RowBounds[j*2] = 0;
      this->RowBounds[j*2+1] = imageInUseSize[0] - 1;
      }
    }
  else
    {
    // create an array of lines where the y value of the first vertex is less
    // than or equal to the y value of the second vertex. There are 12 lines,
    // each containing x1, y1, x2, y2 values.
    float lines[12][4];
    float x1, y1, x2, y2;
    int xlow, xhigh;
    int lineIndex[12][2] = {{0,1}, {2,3}, {4,5}, {6,7},
                            {0,2}, {1,3} ,{4,6}, {5,7},
                            {0,4}, {1,5}, {2,6}, {3,7}};

    for ( i = 0; i < 12; i++ )
      {
      x1 = (viewPoint[lineIndex[i][0]][0]+1.0) *
        0.5*imageViewportSize[0] - imageOrigin[0];

      y1 = (viewPoint[lineIndex[i][0]][1]+1.0) *
        0.5*imageViewportSize[1] - imageOrigin[1];

      x2 = (viewPoint[lineIndex[i][1]][0]+1.0) *
        0.5*imageViewportSize[0] - imageOrigin[0];

      y2 = (viewPoint[lineIndex[i][1]][1]+1.0) *
        0.5*imageViewportSize[1] - imageOrigin[1];

      if ( y1 < y2 )
        {
        lines[i][0] = x1;
        lines[i][1] = y1;
        lines[i][2] = x2;
        lines[i][3] = y2;
        }
      else
        {
        lines[i][0] = x2;
        lines[i][1] = y2;
        lines[i][2] = x1;
        lines[i][3] = y1;
        }
      }

    // Now for each row in the image, find out the start / stop pixel
    // If min > max, then no intersection occurred
    for ( j = 0; j < imageInUseSize[1]; j++ )
      {
      this->RowBounds[j*2] = imageMemorySize[0];
      this->RowBounds[j*2+1] = -1;
      for ( i = 0; i < 12; i++ )
        {
        if ( j >= lines[i][1] && j <= lines[i][3] &&
             ( lines[i][1] != lines[i][3] ) )
          {
          x1 = lines[i][0] +
            (static_cast<float>(j) - lines[i][1])/(lines[i][3] - lines[i][1]) *
            (lines[i][2] - lines[i][0] );

          xlow  = static_cast<int>(x1 + 1.5);
          xhigh = static_cast<int>(x1 - 1.0);

          xlow = (xlow<0)?(0):(xlow);
          xlow = (xlow>imageInUseSize[0]-1)?
            (imageInUseSize[0]-1):(xlow);

          xhigh = (xhigh<0)?(0):(xhigh);
          xhigh = (xhigh>imageInUseSize[0]-1)?(
            imageInUseSize[0]-1):(xhigh);

          if ( xlow < this->RowBounds[j*2] )
            {
            this->RowBounds[j*2] = xlow;
            }
          if ( xhigh > this->RowBounds[j*2+1] )
            {
            this->RowBounds[j*2+1] = xhigh;
            }
          }
        }
      // If they are the same this is either a point on the cube or
      // all lines were out of bounds (all on one side or the other)
      // It is safe to ignore the point (since the ray isn't likely
      // to travel through it enough to actually take a sample) and it
      // must be ignored in the case where all lines are out of range
      if ( this->RowBounds[j*2] == this->RowBounds[j*2+1] )
        {
        this->RowBounds[j*2] = imageMemorySize[0];
        this->RowBounds[j*2+1] = -1;
        }
      }
    }

  for ( j = imageInUseSize[1]; j < imageMemorySize[1]; j++ )
    {
    this->RowBounds[j*2] = imageMemorySize[0];
    this->RowBounds[j*2+1] = -1;
    }

  unsigned short *image = this->RayCastImage->GetImage();

  for ( j = 0; j < imageMemorySize[1]; j++ )
    {
    if ( j%64 == 1 &&
         this->RenderWindow && this->RenderWindow->CheckAbortStatus() )
      {
      return 0;
      }

    // New bounds are not overlapping with old bounds - clear between
    // old bounds only
    if ( this->RowBounds[j*2+1] < this->OldRowBounds[j*2] ||
         this->RowBounds[j*2]   > this->OldRowBounds[j*2+1] )
      {
      ucptr = image + 4*( j*imageMemorySize[0] +
                                this->OldRowBounds[j*2] );
      for ( i = 0;
            i <= (this->OldRowBounds[j*2+1] - this->OldRowBounds[j*2]);
            i++ )
        {
        *(ucptr++) = 0;
        *(ucptr++) = 0;
        *(ucptr++) = 0;
        *(ucptr++) = 0;
        }
      }
    // New bounds do overlap with old bounds
    else
      {
      // Clear from old min to new min
      ucptr = image + 4*( j*imageMemorySize[0] +
                          this->OldRowBounds[j*2] );
      for ( i = 0;
            i < (this->RowBounds[j*2] - this->OldRowBounds[j*2]);
            i++ )
        {
        *(ucptr++) = 0;
        *(ucptr++) = 0;
        *(ucptr++) = 0;
        *(ucptr++) = 0;
        }

      // Clear from new max to old max
      ucptr = image + 4*( j*imageMemorySize[0] +
                          this->RowBounds[j*2+1]+1 );
      for ( i = 0;
            i < (this->OldRowBounds[j*2+1] - this->RowBounds[j*2+1]);
            i++ )
        {
        *(ucptr++) = 0;
        *(ucptr++) = 0;
        *(ucptr++) = 0;
        *(ucptr++) = 0;
        }

      }
    }

  return 1;
}


void vtkFixedPointVolumeRayCastMapper::ComputeMatrices( double inputOrigin[3],
                                                        double inputSpacing[3],
                                                        int inputExtent[6],
                                                        vtkRenderer *ren,
                                                        vtkVolume *vol )
{
  // Get the camera from the renderer
  vtkCamera *cam = ren->GetActiveCamera();

  // Get the aspect ratio from the renderer. This is needed for the
  // computation of the perspective matrix
  ren->ComputeAspect();
  double *aspect = ren->GetAspect();

  // Keep track of the projection matrix - we'll need it in a couple of places
  // Get the projection matrix. The method is called perspective, but
  // the matrix is valid for perspective and parallel viewing transforms.
  // Don't replace this with the GetCompositePerspectiveTransformMatrix
  // because that turns off stereo rendering!!!
  this->PerspectiveTransform->Identity();
  this->PerspectiveTransform->
    Concatenate(cam->GetProjectionTransformMatrix(aspect[0]/aspect[1],
                                                   0.0, 1.0 ));
  this->PerspectiveTransform->Concatenate(cam->GetViewTransformMatrix());
  this->PerspectiveMatrix->DeepCopy(this->PerspectiveTransform->GetMatrix());


  // Compute the origin of the extent the volume origin is at voxel (0,0,0)
  // but we want to consider (0,0,0) in voxels to be at
  // (inputExtent[0], inputExtent[2], inputExtent[4]).
  double extentOrigin[3];
  extentOrigin[0] = inputOrigin[0] + inputExtent[0]*inputSpacing[0];
  extentOrigin[1] = inputOrigin[1] + inputExtent[2]*inputSpacing[1];
  extentOrigin[2] = inputOrigin[2] + inputExtent[4]*inputSpacing[2];

  // Get the volume matrix. This is a volume to world matrix right now.
  // We'll need to invert it, translate by the origin and scale by the
  // spacing to change it to a world to voxels matrix.
  this->VolumeMatrix->DeepCopy( vol->GetMatrix() );

  this->VoxelsToViewTransform->SetMatrix( this->VolumeMatrix );

  // Create a transform that will account for the scaling and translation of
  // the scalar data. The is the volume to voxels matrix.
  this->VoxelsTransform->Identity();
  this->VoxelsTransform->Translate(extentOrigin[0],
                                   extentOrigin[1],
                                   extentOrigin[2] );

  this->VoxelsTransform->Scale( inputSpacing[0],
                                inputSpacing[1],
                                inputSpacing[2] );

  // Now concatenate the volume's matrix with this scalar data matrix
  this->VoxelsToViewTransform->PreMultiply();
  this->VoxelsToViewTransform->Concatenate( this->VoxelsTransform->GetMatrix() );

  // Now we actually have the world to voxels matrix - copy it out
  this->WorldToVoxelsMatrix->DeepCopy( this->VoxelsToViewTransform->GetMatrix() );
  this->WorldToVoxelsMatrix->Invert();

  // We also want to invert this to get voxels to world
  this->VoxelsToWorldMatrix->DeepCopy( this->VoxelsToViewTransform->GetMatrix() );

  // Compute the voxels to view transform by concatenating the
  // voxels to world matrix with the projection matrix (world to view)
  this->VoxelsToViewTransform->PostMultiply();
  this->VoxelsToViewTransform->Concatenate( this->PerspectiveMatrix );

  this->VoxelsToViewMatrix->DeepCopy( this->VoxelsToViewTransform->GetMatrix() );

  this->ViewToVoxelsMatrix->DeepCopy( this->VoxelsToViewMatrix );
  this->ViewToVoxelsMatrix->Invert();
}

int vtkFixedPointVolumeRayCastMapper::ClipRayAgainstClippingPlanes( float rayStart[3],
                                                            float rayEnd[3],
                                                            int numClippingPlanes,
                                                            float *clippingPlanes )
{

  float    *planePtr;
  int      i;
  float    t, point[3], dp;
  float    rayDir[3];

  rayDir[0] = rayEnd[0] - rayStart[0];
  rayDir[1] = rayEnd[1] - rayStart[1];
  rayDir[2] = rayEnd[2] - rayStart[2];

  // loop through all the clipping planes
  for ( i = 0; i < numClippingPlanes; i++ )
    {
    planePtr = clippingPlanes + 4*i;

    dp =
      planePtr[0]*rayDir[0] +
      planePtr[1]*rayDir[1] +
      planePtr[2]*rayDir[2];

    if ( dp != 0.0 )
      {
      t =
        -( planePtr[0]*rayStart[0] +
           planePtr[1]*rayStart[1] +
           planePtr[2]*rayStart[2] + planePtr[3]) / dp;

      if ( t > 0.0 && t < 1.0 )
        {
        point[0] = rayStart[0] + t*rayDir[0];
        point[1] = rayStart[1] + t*rayDir[1];
        point[2] = rayStart[2] + t*rayDir[2];

        if ( dp > 0.0 )
          {
          rayStart[0] = point[0];
          rayStart[1] = point[1];
          rayStart[2] = point[2];
          }
        else
          {
          rayEnd[0] = point[0];
          rayEnd[1] = point[1];
          rayEnd[2] = point[2];
          }

        rayDir[0] = rayEnd[0] - rayStart[0];
        rayDir[1] = rayEnd[1] - rayStart[1];
        rayDir[2] = rayEnd[2] - rayStart[2];

        }
      // If the clipping plane is outside the ray segment, then
      // figure out if that means the ray segment goes to zero (if so
      // return 0) or doesn't affect it (if so do nothing)
      else
        {
        if ( dp >= 0.0 && t >= 1.0 )
          {
          return 0;
          }
        if ( dp <= 0.0 && t <= 0.0 )
          {
          return 0;
          }
        }
      }
    else
      {
      // rayDir is perpendicular to planePtr; i.e., the ray does not
      // intersect the plane. Determine which side of the plane the ray
      // is on.
      float side = planePtr[0]*rayStart[0] + planePtr[1]*rayStart[1] +
        planePtr[2]*rayStart[2] + planePtr[3];
      if (side < 0)
        {
        // clip
        return 0;
        }
      }
    }

  return 1;
}


int vtkFixedPointVolumeRayCastMapper::ClipRayAgainstVolume( float rayStart[3],
                                                    float rayEnd[3],
                                                    float rayDirection[3],
                                                    double bounds[6] )
{
  int    loop;
  float  diff;
  float  t;

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
          }
        }
      }
    }

  // To be absolutely certain our ray remains inside the volume,
  // recompute the ray direction (since it has changed - it is not
  // normalized and therefore changes when start/end change) and move
  // the start/end points in by 1/1000th of the distance.
  float offset;
  offset = (rayEnd[0] - rayStart[0])*0.001;
  rayStart[0] += offset;
  rayEnd[0]   -= offset;

  offset = (rayEnd[1] - rayStart[1])*0.001;
  rayStart[1] += offset;
  rayEnd[1]   -= offset;

  offset = (rayEnd[2] - rayStart[2])*0.001;
  rayStart[2] += offset;
  rayEnd[2]   -= offset;

  if ( rayEnd[0] >= bounds[1] ||
       rayEnd[1] >= bounds[3] ||
       rayEnd[2] >= bounds[5] ||
       rayEnd[0] < bounds[0] ||
       rayEnd[1] < bounds[2] ||
       rayEnd[2] < bounds[4] )
    {
      return 0;
    }

  if ( (rayEnd[0]-rayStart[0])*rayDirection[0] < 0.0 ||
       (rayEnd[1]-rayStart[1])*rayDirection[1] < 0.0 ||
       (rayEnd[2]-rayStart[2])*rayDirection[2] < 0.0 )
    {
    return 0;
    }

  return 1;
}


void vtkFixedPointVolumeRayCastMapper::ComputeGradients( vtkVolume *vol )
{
  vtkImageData *input = this->GetInput();

  void *dataPtr = this->CurrentScalars->GetVoidPointer(0);

 int scalarType   = this->CurrentScalars->GetDataType();
 int components   = this->CurrentScalars->GetNumberOfComponents();
 int independent  = vol->GetProperty()->GetIndependentComponents();

 int dim[3];
 double spacing[3];
 input->GetDimensions(dim);
 input->GetSpacing(spacing);

 // Find the scalar range
 double scalarRange[4][2];
 int c;
 for ( c = 0; c < components; c++ )
   {
   this->CurrentScalars->GetRange(scalarRange[c], c);
   }

 vtkIdType sliceSize = (static_cast<vtkIdType>(dim[0])*
                        static_cast<vtkIdType>(dim[1])*
                        ((independent)?(components):(1)));
 vtkIdType numSlices = dim[2];

 int i;

 // Delete the prior gradient normal information
 if ( this->GradientNormal )
   {
   // Contiguous? Delete in one chunk otherwise delete slice by slice
   if ( this->ContiguousGradientNormal )
     {
     delete [] this->ContiguousGradientNormal;
     this->ContiguousGradientNormal = NULL;
     }
   else
     {
     for ( i = 0; i < this->NumberOfGradientSlices; i++ )
       {
       delete [] this->GradientNormal[i];
       }
     }
   delete [] this->GradientNormal;
   this->GradientNormal = NULL;
   }

 // Delete the prior gradient magnitude information
 if ( this->GradientMagnitude )
   {
   // Contiguous? Delete in one chunk otherwise delete slice by slice
   if ( this->ContiguousGradientMagnitude )
     {
     delete [] this->ContiguousGradientMagnitude;
     this->ContiguousGradientMagnitude = NULL;
     }
   else
     {
     for ( i = 0; i < this->NumberOfGradientSlices; i++ )
       {
       delete [] this->GradientMagnitude[i];
       }
     }
   delete [] this->GradientMagnitude;
   this->GradientMagnitude = NULL;
   }

  this->NumberOfGradientSlices = numSlices;
  this->GradientNormal  = new unsigned short *[numSlices];
  this->GradientMagnitude = new unsigned char *[numSlices];

  // first, attempt contiguous memory. If this fails, then go
  // for non-contiguous
  // NOTE: Standard behavior is to catch std::bad_alloc, but it's
  // CMemoryException if hosted by MFC.
  try
    {
    this->ContiguousGradientNormal = new unsigned short [numSlices * sliceSize];
    }
  catch(...)
    {
    this->ContiguousGradientNormal = NULL;
    }
  try
    {
    this->ContiguousGradientMagnitude = new unsigned char [numSlices * sliceSize];
    }
  catch(...)
    {
    this->ContiguousGradientMagnitude = NULL;
    }

  if ( this->ContiguousGradientNormal )
    {
    // We were able to allocate contiguous space - we just need to set the
    // slice pointers here
    for ( i = 0; i < numSlices; i++ )
      {
      this->GradientNormal[i]  = this->ContiguousGradientNormal + i*sliceSize;
      }
    }
  else
    {
    // We were not able to allocate contigous space - allocate it slice by slice
    for ( i = 0; i < numSlices; i++ )
      {
      this->GradientNormal[i]  = new unsigned short [sliceSize];
      }
    }

  if ( this->ContiguousGradientMagnitude )
    {
    // We were able to allocate contiguous space - we just need to set the
    // slice pointers here
    for ( i = 0; i < numSlices; i++ )
      {
      this->GradientMagnitude[i]  = this->ContiguousGradientMagnitude + i*sliceSize;
      }
    }
  else
    {
    // We were not able to allocate contigous space - allocate it slice by slice
    for ( i = 0; i < numSlices; i++ )
      {
      this->GradientMagnitude[i] = new unsigned char [sliceSize];
      }
    }

  vtkTimerLog *timer = vtkTimerLog::New();
  timer->StartTimer();

  if ( components == 1 &&
       (scalarType == VTK_UNSIGNED_CHAR ||
        scalarType == VTK_CHAR ||
        scalarType == VTK_UNSIGNED_SHORT ||
        scalarType == VTK_SHORT ) )
    {
      this->Threader->SetSingleMethod( vtkFPVRCMSwitchOnDataType,
                                       (vtkObject *)this );
      this->Threader->SingleMethodExecute();
    }

  else
    {
    switch ( scalarType )
      {
      vtkTemplateMacro(
        vtkFixedPointVolumeRayCastMapperComputeGradients(
          (VTK_TT *)(dataPtr), dim, spacing, components,
          independent, scalarRange,
          this->GradientNormal,
          this->GradientMagnitude,
          this->DirectionEncoder,
          this) );
      }
    }

  timer->StopTimer();
  //cout << "Gradients computed in " << timer->GetElapsedTime() << " seconds " << endl;
  timer->Delete();
}

int vtkFixedPointVolumeRayCastMapper::UpdateShadingTable( vtkRenderer *ren,
                                                          vtkVolume *vol )
{
  if ( this->ShadingRequired == 0 )
    {
    return 0;
    }

  // How many components?
  int components = this->CurrentScalars->GetNumberOfComponents();

  int c;
  for ( c = 0; c < ((vol->GetProperty()->GetIndependentComponents())?(components):(1)); c++ )
    {
    this->GradientShader->SetActiveComponent( c );
    this->GradientShader->UpdateShadingTable( ren, vol, this->GradientEstimator );

    float *r = this->GradientShader->GetRedDiffuseShadingTable(vol);
    float *g = this->GradientShader->GetGreenDiffuseShadingTable(vol);
    float *b = this->GradientShader->GetBlueDiffuseShadingTable(vol);

    float *rptr = r;
    float *gptr = g;
    float *bptr = b;

    unsigned short *tablePtr = this->DiffuseShadingTable[c];

    int i;
    for ( i = 0; i < this->DirectionEncoder->GetNumberOfEncodedDirections(); i++ )
      {
      *(tablePtr++) = static_cast<unsigned short>((*(rptr++))*VTKKW_FP_SCALE + 0.5);
      *(tablePtr++) = static_cast<unsigned short>((*(gptr++))*VTKKW_FP_SCALE + 0.5);
      *(tablePtr++) = static_cast<unsigned short>((*(bptr++))*VTKKW_FP_SCALE + 0.5);
      }

    r = this->GradientShader->GetRedSpecularShadingTable(vol);
    g = this->GradientShader->GetGreenSpecularShadingTable(vol);
    b = this->GradientShader->GetBlueSpecularShadingTable(vol);

    rptr = r;
    gptr = g;
    bptr = b;

    tablePtr = this->SpecularShadingTable[c];


    for ( i = 0; i < this->DirectionEncoder->GetNumberOfEncodedDirections(); i++ )
      {
      *(tablePtr++) = static_cast<unsigned short>((*(rptr++))*VTKKW_FP_SCALE + 0.5);
      *(tablePtr++) = static_cast<unsigned short>((*(gptr++))*VTKKW_FP_SCALE + 0.5);
      *(tablePtr++) = static_cast<unsigned short>((*(bptr++))*VTKKW_FP_SCALE + 0.5);
      }
    }

  return 1;
}

int vtkFixedPointVolumeRayCastMapper::UpdateGradients( vtkVolume *vol )
{
  int needToUpdate = 0;

  this->GradientOpacityRequired = 0;
  this->ShadingRequired         = 0;

  // Get the image data
  vtkImageData *input = this->GetInput();

  if ( vol->GetProperty()->GetShade() )
    {
    needToUpdate = 1;
    this->ShadingRequired = 1;
    }

  for ( int c = 0; c < this->CurrentScalars->GetNumberOfComponents(); c++ )
    {
    vtkPiecewiseFunction *f = vol->GetProperty()->GetGradientOpacity(c);
    if ( strcmp(f->GetType(), "Constant") || f->GetValue(0.0) != 1.0 )
      {
      needToUpdate = 1;
      this->GradientOpacityRequired = 1;
      }
    }

  if ( !needToUpdate )
    {
    return 0;
    }

  // Check if the input has changed
  if ( input == this->SavedGradientsInput &&
       this->CurrentScalars == this->PreviousScalars &&
       input->GetMTime() < this->SavedGradientsMTime.GetMTime() )
    {
    return 0;
    }

  this->ComputeGradients( vol );

  // Time to save the input used to update the tabes
  this->SavedGradientsInput = this->GetInput();
  this->SavedGradientsMTime.Modified();

  return 1;
}

int vtkFixedPointVolumeRayCastMapper::UpdateColorTable( vtkVolume *vol )
{
  int needToUpdate = 0;

  // Get the image data
  vtkImageData *input = this->GetInput();

  // Has the data itself changed?
  if ( input != this->SavedParametersInput ||
       this->CurrentScalars != this->PreviousScalars ||
       input->GetMTime() > this->SavedParametersMTime.GetMTime() )
    {
    needToUpdate = 1;
    }

  // What is the blending mode?
  int blendMode = this->GetBlendMode();
  if ( blendMode != this->SavedBlendMode )
    {
    needToUpdate = 1;

    if ( this->GetBlendMode() == vtkVolumeMapper::MINIMUM_INTENSITY_BLEND )
      {
      this->FlipMIPComparison = 1;
      }
    else
      {
      this->FlipMIPComparison = 0;
      }
    }

  // How many components?
  int components = this->CurrentScalars->GetNumberOfComponents();

  // Has the sample distance changed?
  if ( this->SavedSampleDistance != this->SampleDistance )
    {
    needToUpdate = 1;
    }

  vtkColorTransferFunction *rgbFunc[4];
  vtkPiecewiseFunction     *grayFunc[4];
  vtkPiecewiseFunction     *scalarOpacityFunc[4];
  vtkPiecewiseFunction     *gradientOpacityFunc[4];
  int                       colorChannels[4];
  float                     scalarOpacityDistance[4];

  int c;

  for ( c = 0; c < ((vol->GetProperty()->GetIndependentComponents())?(components):(1)); c++ )
    {
    colorChannels[c]         = vol->GetProperty()->GetColorChannels(c);
    if ( colorChannels[c] == 1 )
      {
      rgbFunc[c]               = NULL;
      grayFunc[c]              = vol->GetProperty()->GetGrayTransferFunction(c);
      }
    else
      {
      rgbFunc[c]               = vol->GetProperty()->GetRGBTransferFunction(c);
      grayFunc[c]              = NULL;
      }
    scalarOpacityFunc[c]     = vol->GetProperty()->GetScalarOpacity(c);
    gradientOpacityFunc[c]   = vol->GetProperty()->GetGradientOpacity(c);
    scalarOpacityDistance[c] = vol->GetProperty()->GetScalarOpacityUnitDistance(c);

    // Has the number of color channels changed?
    if ( this->SavedColorChannels[c] != colorChannels[c] )
      {
      needToUpdate = 1;
      }

    // Has the color transfer function changed in some way,
    // and we are using it?
    if ( colorChannels[c] == 3 )
      {
      if ( this->SavedRGBFunction[c] != rgbFunc[c] ||
           this->SavedParametersMTime.GetMTime() < rgbFunc[c]->GetMTime() )
        {
        needToUpdate = 1;
        }
      }

    // Has the gray transfer function changed in some way,
    // and we are using it?
    if ( colorChannels[c] == 1 )
      {
      if ( this->SavedGrayFunction[c] != grayFunc[c] ||
           this->SavedParametersMTime.GetMTime() < grayFunc[c]->GetMTime() )
        {
        needToUpdate = 1;
        }
      }

    // Has the scalar opacity transfer function changed in some way?
    if ( this->SavedScalarOpacityFunction[c] != scalarOpacityFunc[c] ||
         this->SavedParametersMTime.GetMTime() < scalarOpacityFunc[c]->GetMTime() )
      {
      needToUpdate = 1;
      }

    // Has the gradient opacity transfer function changed in some way?
    if ( this->SavedGradientOpacityFunction[c] != gradientOpacityFunc[c] ||
         this->SavedParametersMTime.GetMTime() < gradientOpacityFunc[c]->GetMTime() )
      {
      needToUpdate = 1;
      }

    // Has the distance over which the scalar opacity function is defined changed?
    if ( this->SavedScalarOpacityDistance[c] != scalarOpacityDistance[c] )
      {
      needToUpdate = 1;
      }
    }

  // If we have not found any need to update, return now
  if ( !needToUpdate )
    {
    return 0;
    }


  for ( c = 0; c < ((vol->GetProperty()->GetIndependentComponents())?(components):(1)); c++ )
    {
    this->SavedRGBFunction[c]             = rgbFunc[c];
    this->SavedGrayFunction[c]            = grayFunc[c];
    this->SavedScalarOpacityFunction[c]   = scalarOpacityFunc[c];
    this->SavedGradientOpacityFunction[c] = gradientOpacityFunc[c];
    this->SavedColorChannels[c]           = colorChannels[c];
    this->SavedScalarOpacityDistance[c]   = scalarOpacityDistance[c];
    }

  this->SavedSampleDistance          = this->SampleDistance;
  this->SavedBlendMode               = blendMode;
  this->SavedParametersInput         = input;

  this->SavedParametersMTime.Modified();

  int scalarType = this->CurrentScalars->GetDataType();

  int i;
  float tmpArray[3*32768];

  // Find the scalar range
  double scalarRange[4][2];
  for ( c = 0; c < components; c++ )
    {
    this->CurrentScalars->GetRange(scalarRange[c], c);

    // Is the difference between max and min less than 32768? If so, and if
    // the data is not of float or double type, use a simple offset mapping.
    // If the difference between max and min is 32768 or greater, or the data
    // is of type float or double, we must use an offset / scaling mapping.
    // In this case, the array size will be 32768 - we need to figure out the
    // offset and scale factor.
    float offset;
    float scale;

    int arraySizeNeeded;

    if ( scalarType == VTK_FLOAT ||
         scalarType == VTK_DOUBLE ||
         scalarRange[c][1] - scalarRange[c][0] > 32767 )
      {
      arraySizeNeeded = 32768;
      offset          = -scalarRange[c][0];

      if ( scalarRange[c][1] - scalarRange[c][0] )
        {
        scale = 32767.0 / (scalarRange[c][1] - scalarRange[c][0]);
        }
      else
        {
        scale = 1.0;
        }
      }
    else
      {
      arraySizeNeeded = (int)(scalarRange[c][1] - scalarRange[c][0] + 1);
      offset          = -scalarRange[c][0];
      scale           = 1.0;
      }

    this->TableSize[c]   = arraySizeNeeded;
    this->TableShift[c]  = offset;
    this->TableScale[c]  = scale;
    }

  if ( vol->GetProperty()->GetIndependentComponents() )
    {
    for ( c = 0; c < components; c++ )
      {
      // Sample the transfer functions between the min and max.
      if ( colorChannels[c] == 1 )
        {
        float tmpArray2[32768];
        grayFunc[c]->GetTable( scalarRange[c][0], scalarRange[c][1],
                               this->TableSize[c], tmpArray2 );
        for ( int index = 0; index < this->TableSize[c]; index++ )
          {
          tmpArray[3*index  ] = tmpArray2[index];
          tmpArray[3*index+1] = tmpArray2[index];
          tmpArray[3*index+2] = tmpArray2[index];
          }
        }
      else
        {
        rgbFunc[c]->GetTable( scalarRange[c][0], scalarRange[c][1],
                              this->TableSize[c], tmpArray );
        }
      // Convert color to short format
      for ( i = 0; i < this->TableSize[c]; i++ )
        {
        this->ColorTable[c][3*i  ] =
          static_cast<unsigned short>(tmpArray[3*i  ]*VTKKW_FP_SCALE + 0.5);
        this->ColorTable[c][3*i+1] =
          static_cast<unsigned short>(tmpArray[3*i+1]*VTKKW_FP_SCALE + 0.5);
        this->ColorTable[c][3*i+2] =
          static_cast<unsigned short>(tmpArray[3*i+2]*VTKKW_FP_SCALE + 0.5);
        }

      scalarOpacityFunc[c]->GetTable( scalarRange[c][0], scalarRange[c][1],
                                   this->TableSize[c], tmpArray );

      // Correct the opacity array for the spacing between the planes if we are
      // using a composite blending operation
      if ( this->BlendMode == vtkVolumeMapper::COMPOSITE_BLEND )
        {
        float *ptr = tmpArray;
        double factor;
          {
          factor = this->SampleDistance / vol->GetProperty()->GetScalarOpacityUnitDistance(c);
          }

        for ( i = 0; i < this->TableSize[c]; i++ )
          {
          if ( *ptr > 0.0001 )
            {
            *ptr =  1.0-pow((double)(1.0-(*ptr)),factor);
            }
          ptr++;
          }
        }

      // Convert tables to short format
      for ( i = 0; i < this->TableSize[c]; i++ )
        {
        this->ScalarOpacityTable[c][i] =
          static_cast<unsigned short>(tmpArray[i]*VTKKW_FP_SCALE + 0.5);
        }

      if ( scalarRange[c][1] - scalarRange[c][0] )
        {
        gradientOpacityFunc[c]->GetTable( 0,
                                          (scalarRange[c][1] - scalarRange[c][0])*0.25,
                                          256, tmpArray );

        for ( i = 0; i < 256; i++ )
          {
          this->GradientOpacityTable[c][i] =
            static_cast<unsigned short>(tmpArray[i]*VTKKW_FP_SCALE + 0.5);
          }
        }
      else
        {
        for ( i = 0; i < 256; i++ )
          {
          this->GradientOpacityTable[c][i] = 0x0000;
          }
        }
      }
    }
  else
    {
    if ( components ==  2 )
      {
      // Sample the transfer functions between the min and max.
      if ( colorChannels[0] == 1 )
        {
        float tmpArray2[32768];
        grayFunc[0]->GetTable( scalarRange[0][0], scalarRange[0][1],
                               this->TableSize[0], tmpArray2 );
        for ( int index = 0; index < this->TableSize[0]; index++ )
          {
          tmpArray[3*index  ] = tmpArray2[index];
          tmpArray[3*index+1] = tmpArray2[index];
          tmpArray[3*index+2] = tmpArray2[index];
          }
        }
      else
        {
        rgbFunc[0]->GetTable( scalarRange[0][0], scalarRange[0][1],
                           this->TableSize[0], tmpArray );
        }

      // Convert color to short format
      for ( i = 0; i < this->TableSize[0]; i++ )
        {
        this->ColorTable[0][3*i  ] =
          static_cast<unsigned short>(tmpArray[3*i  ]*VTKKW_FP_SCALE + 0.5);
        this->ColorTable[0][3*i+1] =
          static_cast<unsigned short>(tmpArray[3*i+1]*VTKKW_FP_SCALE + 0.5);
        this->ColorTable[0][3*i+2] =
          static_cast<unsigned short>(tmpArray[3*i+2]*VTKKW_FP_SCALE + 0.5);
        }
      }

    // The opacity table is indexed with the last component
    scalarOpacityFunc[0]->GetTable( scalarRange[components-1][0], scalarRange[components-1][1],
                                    this->TableSize[components-1], tmpArray );

    // Correct the opacity array for the spacing between the planes if we are
    // using a composite blending operation
    if ( this->BlendMode == vtkVolumeMapper::COMPOSITE_BLEND )
      {
      float *ptr = tmpArray;
      double factor =
        this->SampleDistance / vol->GetProperty()->GetScalarOpacityUnitDistance();
      for ( i = 0; i < this->TableSize[components-1]; i++ )
        {
        if ( *ptr > 0.0001 )
          {
          *ptr =  1.0-pow((double)(1.0-(*ptr)),factor);
          }
        ptr++;
        }
      }

      // Convert tables to short format
      for ( i = 0; i < this->TableSize[components-1]; i++ )
        {
        this->ScalarOpacityTable[0][i] =
          static_cast<unsigned short>(tmpArray[i]*VTKKW_FP_SCALE + 0.5);
        }

      if ( scalarRange[components-1][1] - scalarRange[components-1][0] )
        {
        gradientOpacityFunc[0]->GetTable( 0,
                                          (scalarRange[components-1][1] -
                                           scalarRange[components-1][0])*0.25,
                                          256, tmpArray );

        for ( i = 0; i < 256; i++ )
          {
          this->GradientOpacityTable[0][i] =
            static_cast<unsigned short>(tmpArray[i]*VTKKW_FP_SCALE + 0.5);
          }
        }
      else
        {
        for ( i = 0; i < 256; i++ )
          {
          this->GradientOpacityTable[0][i] = 0x0000;
          }
        }
    }

  return 1;
}


int vtkFixedPointVolumeRayCastMapper::ShouldUseNearestNeighborInterpolation( vtkVolume *vol )
{
//  return ( this->UseShortCuts ||
//           vol->GetProperty()->GetInterpolationType() == VTK_NEAREST_INTERPOLATION );
  return ( vol->GetProperty()->GetInterpolationType() == VTK_NEAREST_INTERPOLATION );
}

// Print method for vtkFixedPointVolumeRayCastMapper
void vtkFixedPointVolumeRayCastMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Sample Distance: " << this->SampleDistance << endl;
  os << indent << "Interactive Sample Distance: "
     << this->InteractiveSampleDistance << endl;
  os << indent << "Image Sample Distance: "
     << this->ImageSampleDistance << endl;
  os << indent << "Minimum Image Sample Distance: "
     << this->MinimumImageSampleDistance << endl;
  os << indent << "Maximum Image Sample Distance: "
     << this->MaximumImageSampleDistance << endl;
  os << indent << "Auto Adjust Sample Distances: "
     << this->AutoAdjustSampleDistances << endl;
  os << indent << "LockSampleDistanceToInputSpacing: "
    << (this->LockSampleDistanceToInputSpacing ? "On\n" : "Off\n");
  os << indent << "Intermix Intersecting Geometry: "
    << (this->IntermixIntersectingGeometry ? "On\n" : "Off\n");
  os << indent << "Final Color Window: " << this->FinalColorWindow << endl;
  os << indent << "Final Color Level: " << this->FinalColorLevel << endl;
  os << indent << "Space leaping filter: " << this->SpaceLeapFilter << endl;

  // These are all things that shouldn't be printed....
  //os << indent << "ShadingRequired: " << this->ShadingRequired << endl;
  //os << indent << "GradientOpacityRequired: " << this->GradientOpacityRequired
  //   << endl;

  //os << indent << "CurrentScalars: " << this->CurrentScalars << endl;
  //os << indent << "PreviousScalars: " << this->PreviousScalars << endl;

  //if ( this->RayCastImage )
  //  {
  //  os << indent << "Ray Cast Image:\n";
  //  this->RayCastImage->PrintSelf(os,indent.GetNextIndent());
  //  }
  //else
  //  {
  //  os << indent << "Ray Cast Image: (none)\n";
  //  }

  //os << indent << "RenderWindow: " << this->RenderWindow << endl;

  //os << indent << "CompositeHelper: " << this->CompositeHelper << endl;
  //os << indent << "CompositeShadeHelper: " << this->CompositeShadeHelper << endl;
  //os << indent << "CompositeGOHelper: " << this->CompositeGOHelper << endl;
  //os << indent << "CompositeGOShadeHelper: " << this->CompositeGOShadeHelper << endl;
  //os << indent << "MIPHelper: " << this->MIPHelper << endl;

  //os << indent << "TableShift: " << this->TableShift[0] << " "
  //   << this->TableShift[1] << " " << this->TableShift[2] << " "
  //   << this->TableShift[3] << endl;
  //os << indent << "TableScale: " << this->TableScale[0] << " "
  //   << this->TableScale[1] << " " << this->TableScale[2] << " "
  //   << this->TableScale[3] << endl;

  // os << indent << "Flip Mip Comparison" << this->FlipMIPComparison << end;"
}


void vtkFixedPointVolumeRayCastMapper::ReleaseGraphicsResources(vtkWindow *win)
{
  if (win && this->ImageDisplayHelper)
    {
    this->ImageDisplayHelper->ReleaseGraphicsResources(win);
    }
}
