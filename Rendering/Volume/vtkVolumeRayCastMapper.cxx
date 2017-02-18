/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeRayCastMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVolumeRayCastMapper.h"

#if !defined(VTK_LEGACY_REMOVE)

#include "vtkCamera.h"
#include "vtkDataArray.h"
#include "vtkEncodedGradientEstimator.h"
#include "vtkEncodedGradientShader.h"
#include "vtkFiniteDifferenceGradientEstimator.h"
#include "vtkGarbageCollector.h"
#include "vtkGraphicsFactory.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkMultiThreader.h"
#include "vtkObjectFactory.h"
#include "vtkPlaneCollection.h"
#include "vtkPointData.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkTimerLog.h"
#include "vtkTransform.h"
#include "vtkVolumeProperty.h"
#include "vtkVolumeRayCastFunction.h"
#include "vtkRayCastImageDisplayHelper.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <cmath>

vtkStandardNewMacro(vtkVolumeRayCastMapper);

vtkCxxSetObjectMacro(vtkVolumeRayCastMapper,VolumeRayCastFunction,
                     vtkVolumeRayCastFunction );

// A tolerance for bounds, historically equal to 2^(-23) and used
// to counter a small numerical precision issue with the old
// QuickFloor() function.  It should not be needed anymore.
// #define VTK_RAYCAST_FLOOR_TOL 1.1920928955078125e-07
#define VTK_RAYCAST_FLOOR_TOL 0

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

// Construct a new vtkVolumeRayCastMapper with default values
vtkVolumeRayCastMapper::vtkVolumeRayCastMapper()
{
  this->SampleDistance             =  1.0;
  this->ImageSampleDistance        =  1.0;
  this->MinimumImageSampleDistance =  1.0;
  this->MaximumImageSampleDistance = 10.0;
  this->AutoAdjustSampleDistances  =  1;
  this->VolumeRayCastFunction      = NULL;

  this->GradientEstimator  = vtkFiniteDifferenceGradientEstimator::New();
  this->GradientShader     = vtkEncodedGradientShader::New();

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


  this->ImageMemorySize[0]     = 0;
  this->ImageMemorySize[1]     = 0;

  this->Threader               = vtkMultiThreader::New();

  this->Image                  = NULL;
  this->RowBounds              = NULL;
  this->OldRowBounds           = NULL;

  this->RenderTimeTable        = NULL;
  this->RenderVolumeTable      = NULL;
  this->RenderRendererTable    = NULL;
  this->RenderTableSize        = 0;
  this->RenderTableEntries     = 0;

  this->ZBuffer                = NULL;
  this->ZBufferSize[0]         = 0;
  this->ZBufferSize[1]         = 0;
  this->ZBufferOrigin[0]       = 0;
  this->ZBufferOrigin[1]       = 0;

  this->ImageDisplayHelper     = vtkRayCastImageDisplayHelper::New();

  this->IntermixIntersectingGeometry = 1;

  VTK_LEGACY_BODY(vtkVolumeRayCastMapper::vtkVolumeRayCastMapper,"VTK 7.0");
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

  this->SetVolumeRayCastFunction(NULL);

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

  this->ImageDisplayHelper->Delete();

  this->Threader->Delete();

  delete [] this->Image;

  if ( this->RenderTableSize )
  {
    delete [] this->RenderTimeTable;
    delete [] this->RenderVolumeTable;
    delete [] this->RenderRendererTable;
  }

  if ( this->RowBounds )
  {
    delete [] this->RowBounds;
    delete [] this->OldRowBounds;
  }
}

float vtkVolumeRayCastMapper::RetrieveRenderTime( vtkRenderer *ren,
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

void vtkVolumeRayCastMapper::StoreRenderTime( vtkRenderer *ren,
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

    float       *oldTimePtr     = this->RenderTimeTable;
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

void vtkVolumeRayCastMapper::SetNumberOfThreads( int num )
{
  this->Threader->SetNumberOfThreads( num );
}

int vtkVolumeRayCastMapper::GetNumberOfThreads()
{
  if (this->Threader)
  {
    return this->Threader->GetNumberOfThreads();
  }
  return 0;
}

void vtkVolumeRayCastMapper::SetGradientEstimator(
                                       vtkEncodedGradientEstimator *gradest )
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

float vtkVolumeRayCastMapper::GetGradientMagnitudeScale()
{
  if ( !this->GradientEstimator )
  {
    vtkErrorMacro( "You must have a gradient estimator set to get the scale" );
    return 1.0;
  }

  return this->GradientEstimator->GetGradientMagnitudeScale();
}

float vtkVolumeRayCastMapper::GetGradientMagnitudeBias()
{
  if ( !this->GradientEstimator )
  {
    vtkErrorMacro( "You must have a gradient estimator set to get the bias" );
    return 1.0;
  }

  return this->GradientEstimator->GetGradientMagnitudeBias();
}

void vtkVolumeRayCastMapper::ReleaseGraphicsResources(vtkWindow *)
{
}

void vtkVolumeRayCastMapper::Render( vtkRenderer *ren, vtkVolume *vol )
{
  // make sure that we have scalar input and update the scalar input
  if ( this->GetInput() == NULL )
  {
    vtkErrorMacro(<< "No Input!");
    return;
  }
  else
  {
    this->GetInputAlgorithm()->UpdateWholeExtent();
  }


  int scalarType = this->GetInput()->GetPointData()->GetScalars()->GetDataType();
  if (scalarType != VTK_UNSIGNED_SHORT && scalarType != VTK_UNSIGNED_CHAR)
  {
    vtkErrorMacro ("Cannot volume render data of type "
                   << vtkImageScalarTypeNameMacro(scalarType)
                   << ", only unsigned char or unsigned short.");
    return;
  }
  // Start timing now. We didn't want to capture the update of the
  // input data in the times
  this->Timer->StartTimer();

  this->ConvertCroppingRegionPlanesToVoxels();

  this->UpdateShadingTables( ren, vol );

  // This is the input of this mapper
  vtkImageData *input = this->GetInput();

  // Get the camera from the renderer
  vtkCamera *cam = ren->GetActiveCamera();

  // Get the aspect ratio from the renderer. This is needed for the
  // computation of the perspective matrix
  ren->ComputeAspect();
  double *aspect = ren->GetAspect();

  // Keep track of the projection matrix - we'll need it in a couple of
  // places Get the projection matrix. The method is called perspective, but
  // the matrix is valid for perspective and parallel viewing transforms.
  // Don't replace this with the GetCompositePerspectiveTransformMatrix
  // because that turns off stereo rendering!!!
  this->PerspectiveTransform->Identity();
  this->PerspectiveTransform->Concatenate(
    cam->GetProjectionTransformMatrix(aspect[0]/aspect[1],0.0, 1.0 ));
  this->PerspectiveTransform->Concatenate(cam->GetViewTransformMatrix());
  this->PerspectiveMatrix->DeepCopy(this->PerspectiveTransform->GetMatrix());

  // Compute some matrices from voxels to view and vice versa based
  // on the whole input
  this->ComputeMatrices( input, vol );


  // How big is the viewport in pixels?
  double *viewport   =  ren->GetViewport();
  int *renWinSize   =  ren->GetRenderWindow()->GetSize();

  // Save this so that we can restore it if the image is cancelled
  double oldImageSampleDistance = this->ImageSampleDistance;

  // If we are automatically adjusting the size to achieve a desired frame
  // rate, then do that adjustment here. Base the new image sample distance
  // on the previous one and the previous render time. Don't let
  // the adjusted image sample distance be less than the minimum image sample
  // distance or more than the maximum image sample distance.
  if ( this->AutoAdjustSampleDistances )
  {
    float oldTime = this->RetrieveRenderTime( ren, vol );
    float newTime = vol->GetAllocatedRenderTime();
    this->ImageSampleDistance *= sqrt(oldTime / newTime);
    this->ImageSampleDistance =
      (this->ImageSampleDistance>this->MaximumImageSampleDistance)?
      (this->MaximumImageSampleDistance):(this->ImageSampleDistance);
    this->ImageSampleDistance =
      (this->ImageSampleDistance<this->MinimumImageSampleDistance)?
      (this->MinimumImageSampleDistance):(this->ImageSampleDistance);
  }

  // The full image fills the viewport. First, compute the actual viewport
  // size, then divide by the ImageSampleDistance to find the full image
  // size in pixels
  int width, height;
  ren->GetTiledSize(&width, &height);
  this->ImageViewportSize[0] =
    static_cast<int>(width/this->ImageSampleDistance);
  this->ImageViewportSize[1] =
    static_cast<int>(height/this->ImageSampleDistance);

  // Compute row bounds. This will also compute the size of the image to
  // render, allocate the space if necessary, and clear the image where
  // required
  if ( this->ComputeRowBounds( vol, ren ) )
  {
    vtkVolumeRayCastStaticInfo *staticInfo = new vtkVolumeRayCastStaticInfo;
    staticInfo->ClippingPlane = NULL;
    staticInfo->Volume = vol;
    staticInfo->Renderer = ren;
    staticInfo->ScalarDataPointer =
      this->GetInput()->GetPointData()->GetScalars()->GetVoidPointer(0);
    staticInfo->ScalarDataType =
      this->GetInput()->GetPointData()->GetScalars()->GetDataType();

    // Do we need to capture the z buffer to intermix intersecting
    // geometry? If so, do it here
    if ( this->IntermixIntersectingGeometry &&
         ren->GetNumberOfPropsRendered() )
    {
      int x1, x2, y1, y2;

      // turn this->ImageOrigin into (x1,y1) in window (not viewport!)
      // coordinates.
      x1 = static_cast<int> (
        viewport[0] * static_cast<double>(renWinSize[0]) +
        static_cast<double>(this->ImageOrigin[0]) * this->ImageSampleDistance );
      y1 = static_cast<int> (
        viewport[1] * static_cast<double>(renWinSize[1]) +
        static_cast<double>(this->ImageOrigin[1]) * this->ImageSampleDistance);

      // compute z buffer size
      this->ZBufferSize[0] = static_cast<int>(
        static_cast<double>(this->ImageInUseSize[0]) * this->ImageSampleDistance);
      this->ZBufferSize[1] = static_cast<int>(
        static_cast<double>(this->ImageInUseSize[1]) * this->ImageSampleDistance);

      // Use the size to compute (x2,y2) in window coordinates
      x2 = x1 + this->ZBufferSize[0] - 1;
      y2 = y1 + this->ZBufferSize[1] - 1;

      // This is the z buffer origin (in viewport coordinates)
      this->ZBufferOrigin[0] = static_cast<int>(
        static_cast<double>(this->ImageOrigin[0]) * this->ImageSampleDistance);
      this->ZBufferOrigin[1] = static_cast<int>(
        static_cast<double>(this->ImageOrigin[1]) * this->ImageSampleDistance);

      // Capture the z buffer
      this->ZBuffer = ren->GetRenderWindow()->GetZbufferData(x1,y1,x2,y2);
    }

    // This must be done before FunctionInitialize since FunctionInitialize
    // depends on the gradient opacity constant (computed in here) to
    // determine whether to save the gradient magnitudes
    vol->UpdateTransferFunctions( ren );

    // Requires UpdateTransferFunctions to have been called first
    this->VolumeRayCastFunction->FunctionInitialize( ren, vol,
                                                     staticInfo );

    double scalarOpacityUnitDistance =
      vol->GetProperty()->GetScalarOpacityUnitDistance();
    vol->UpdateScalarOpacityforSampleSize( ren, this->SampleDistance /
                                           scalarOpacityUnitDistance);

    staticInfo->CameraThickness =
      static_cast<float>(ren->GetActiveCamera()->GetThickness());

    // Copy the viewToVoxels matrix to 16 floats
    int i, j;
    for ( j = 0; j < 4; j++ )
    {
      for ( i = 0; i < 4; i++ )
      {
        staticInfo->ViewToVoxelsMatrix[j*4+i] =
          static_cast<float>(this->ViewToVoxelsMatrix->GetElement(j,i));
      }
    }

    // Copy the worldToVoxels matrix to 16 floats
    for ( j = 0; j < 4; j++ )
    {
      for ( i = 0; i < 4; i++ )
      {
        staticInfo->WorldToVoxelsMatrix[j*4+i] =
          static_cast<float>(this->WorldToVoxelsMatrix->GetElement(j,i));
      }
    }

    // Copy the voxelsToWorld matrix to 16 floats
    for ( j = 0; j < 4; j++ )
    {
      for ( i = 0; i < 4; i++ )
      {
        staticInfo->VoxelsToWorldMatrix[j*4+i] =
          static_cast<float>(this->VoxelsToWorldMatrix->GetElement(j,i));
      }
    }

    if ( this->ClippingPlanes )
    {
      this->InitializeClippingPlanes( staticInfo, this->ClippingPlanes );
    }
    else
    {
      staticInfo->NumberOfClippingPlanes = 0;
    }


    // Copy in the image info
    staticInfo->ImageInUseSize[0]    = this->ImageInUseSize[0];
    staticInfo->ImageInUseSize[1]    = this->ImageInUseSize[1];
    staticInfo->ImageMemorySize[0]   = this->ImageMemorySize[0];
    staticInfo->ImageMemorySize[1]   = this->ImageMemorySize[1];
    staticInfo->ImageViewportSize[0] = this->ImageViewportSize[0];
    staticInfo->ImageViewportSize[1] = this->ImageViewportSize[1];

    staticInfo->ImageOrigin[0] = this->ImageOrigin[0];
    staticInfo->ImageOrigin[1] = this->ImageOrigin[1];

    staticInfo->Image     = this->Image;
    staticInfo->RowBounds = this->RowBounds;

    // Set the number of threads to use for ray casting,
    // then set the execution method and do it.
    this->Threader->SetSingleMethod( VolumeRayCastMapper_CastRays,
                                     static_cast<void *>(staticInfo));
    this->Threader->SingleMethodExecute();

    if ( !ren->GetRenderWindow()->GetAbortRender() )
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

      this->ImageDisplayHelper->
        RenderTexture( vol, ren,
                       this->ImageMemorySize,
                       this->ImageViewportSize,
                       this->ImageInUseSize,
                       this->ImageOrigin,
                       depth,
                       this->Image );

      this->Timer->StopTimer();
      this->TimeToDraw = this->Timer->GetElapsedTime();
      this->StoreRenderTime( ren, vol, this->TimeToDraw );
    }
    // Restore the image sample distance so that automatic adjustment
    // will work correctly
    else
    {
      this->ImageSampleDistance = oldImageSampleDistance;
    }

    delete [] staticInfo->ClippingPlane;
    delete staticInfo;
    delete [] this->ZBuffer;
    this->ZBuffer = NULL;
  }
}
VTK_THREAD_RETURN_TYPE VolumeRayCastMapper_CastRays( void *arg )
{
  // Get the info out of the input structure
  int threadID    = ((vtkMultiThreader::ThreadInfo *)(arg))->ThreadID;
  int threadCount = ((vtkMultiThreader::ThreadInfo *)(arg))->NumberOfThreads;
  vtkVolumeRayCastStaticInfo *staticInfo  =
    (vtkVolumeRayCastStaticInfo *)((vtkMultiThreader::ThreadInfo *)arg)->UserData;

  int i, j, k;
  unsigned char *ucptr;

  vtkVolumeRayCastMapper *me =
    vtkVolumeRayCastMapper::SafeDownCast( staticInfo->Volume->GetMapper() );

  if ( !me )
  {
    vtkGenericWarningMacro("The volume does not have a ray cast mapper!");
    return VTK_THREAD_RETURN_VALUE;
  }

  vtkVolumeRayCastDynamicInfo *dynamicInfo = new vtkVolumeRayCastDynamicInfo;

  // Initialize this to avoid purify problems
  dynamicInfo->ScalarValue = 0;

  float *rayStart     = dynamicInfo->TransformedStart;
  float *rayEnd       = dynamicInfo->TransformedEnd;
  float *rayDirection = dynamicInfo->TransformedDirection;
  float *rayStep      = dynamicInfo->TransformedIncrement;

  float norm;
  float viewRay[3];
  float rayCenter[3];
  float absStep[3];
  float voxelPoint[4];

  // We need to know what the center ray is (in voxel coordinates) to
  // compute sampling distance later on. Save it in an instance variable.

  // This is tbe near end of the center ray in view coordinates
  // Convert it to voxel coordinates
  viewRay[0] = viewRay[1] = viewRay[2] = 0.0;
  vtkVRCMultiplyPointMacro( viewRay, rayStart,
                            staticInfo->ViewToVoxelsMatrix );

  // This is the far end of the center ray in view coordinates
  // Convert it to voxel coordiantes
  viewRay[2] = 1.0;
  vtkVRCMultiplyPointMacro( viewRay, voxelPoint,
                            staticInfo->ViewToVoxelsMatrix );

  // Turn these two points into a vector
  rayCenter[0] = voxelPoint[0] - rayStart[0];
  rayCenter[1] = voxelPoint[1] - rayStart[1];
  rayCenter[2] = voxelPoint[2] - rayStart[2];

  // normalize the vector based on world coordinate distance
  // This way we can scale by sample distance and it will work
  // out even though we are in voxel coordinates
  rayCenter[0] /= staticInfo->CameraThickness;
  rayCenter[1] /= staticInfo->CameraThickness;
  rayCenter[2] /= staticInfo->CameraThickness;

  float centerScale = sqrt( (rayCenter[0] * rayCenter[0]) +
                            (rayCenter[1] * rayCenter[1]) +
                            (rayCenter[2] * rayCenter[2]) );

  rayCenter[0] /= centerScale;
  rayCenter[1] /= centerScale;
  rayCenter[2] /= centerScale;

  float bounds[6];
  int dim[3];

  me->GetInput()->GetDimensions(dim);
  bounds[0] = bounds[2] = bounds[4] = 0.0;
  bounds[1] = dim[0]-1;
  bounds[3] = dim[1]-1;
  bounds[5] = dim[2]-1;

  // If we have a simple crop box then we can tighten the bounds
  if ( me->Cropping && me->CroppingRegionFlags == 0x2000 )
  {
    bounds[0] = me->VoxelCroppingRegionPlanes[0];
    bounds[1] = me->VoxelCroppingRegionPlanes[1];
    bounds[2] = me->VoxelCroppingRegionPlanes[2];
    bounds[3] = me->VoxelCroppingRegionPlanes[3];
    bounds[4] = me->VoxelCroppingRegionPlanes[4];
    bounds[5] = me->VoxelCroppingRegionPlanes[5];
  }

  bounds[0] = (bounds[0] < 0)?(0):(bounds[0]);
  bounds[0] = (bounds[0] > dim[0]-1)?(dim[0]-1):(bounds[0]);
  bounds[1] = (bounds[1] < 0)?(0):(bounds[1]);
  bounds[1] = (bounds[1] > dim[0]-1)?(dim[0]-1):(bounds[1]);
  bounds[2] = (bounds[2] < 0)?(0):(bounds[2]);
  bounds[2] = (bounds[2] > dim[1]-1)?(dim[1]-1):(bounds[2]);
  bounds[3] = (bounds[3] < 0)?(0):(bounds[3]);
  bounds[3] = (bounds[3] > dim[1]-1)?(dim[1]-1):(bounds[3]);
  bounds[4] = (bounds[4] < 0)?(0):(bounds[4]);
  bounds[4] = (bounds[4] > dim[2]-1)?(dim[2]-1):(bounds[4]);
  bounds[5] = (bounds[5] < 0)?(0):(bounds[5]);
  bounds[5] = (bounds[5] > dim[2]-1)?(dim[2]-1):(bounds[5]);

  bounds[1] -= VTK_RAYCAST_FLOOR_TOL;
  bounds[3] -= VTK_RAYCAST_FLOOR_TOL;
  bounds[5] -= VTK_RAYCAST_FLOOR_TOL;

  int *imageInUseSize     = staticInfo->ImageInUseSize;
  int *imageMemorySize    = staticInfo->ImageMemorySize;
  int *imageViewportSize  = staticInfo->ImageViewportSize;
  int *imageOrigin        = staticInfo->ImageOrigin;
  int *rowBounds          = staticInfo->RowBounds;

  unsigned char *imagePtr = staticInfo->Image;

  float sampleDistance    = me->GetSampleDistance();

  float val;

  vtkRenderWindow *renWin = staticInfo->Renderer->GetRenderWindow();

  // Compute the offset valuex for viewing rays - this is the 1 / fullSize
  // value to add to the computed location so that they falls between
  // -1 + 1/fullSize and 1 - 1/fullSize and are each 2/fullSize apart.
  // fullSize is the viewport size along the corresponding direction (in
  // pixels)
  float offsetX = 1.0 / static_cast<float>(imageViewportSize[0]);
  float offsetY = 1.0 / static_cast<float>(imageViewportSize[1]);

  // Some variables needed for non-subvolume cropping
  float fullRayStart[3];
  float fullRayEnd[3];
  float fullRayDirection[3];
  int bitLoop, bitFlag;
  float tmp, tmpArray[4];
  int arrayCount;

  // Need room for potentially 27 subvolumes.
  float rgbaArray[27*4] = {};
  float distanceArray[27] = {};
  float scalarArray[27] = {};

  for ( j = 0; j < imageInUseSize[1]; j++ )
  {
    if ( j%threadCount != threadID )
    {
      continue;
    }

    if ( !threadID )
    {
      if ( renWin->CheckAbortStatus() )
      {
        break;
      }
    }
    else if ( renWin->GetAbortRender() )
    {
      break;
    }

    ucptr = imagePtr + 4*(j*imageMemorySize[0] +
                             rowBounds[j*2]);

    // compute the view point y value for this row. Do this by
    // taking our pixel position, adding the image origin then dividing
    // by the full image size to get a number from 0 to 1-1/fullSize. Then,
    // multiply by two and subtract one to get a number from
    // -1 to 1 - 2/fullSize. Then ass offsetX (which is 1/fullSize) to
    // center it.
    viewRay[1] = ((static_cast<float>(j) + static_cast<float>(imageOrigin[1])) /
                  imageViewportSize[1]) * 2.0 - 1.0 + offsetY;

    for ( i = rowBounds[j*2]; i <= rowBounds[j*2+1]; i++ )
    {
      // Initialize for the cases where the ray doesn't intersect anything
      ucptr[0] = 0;
      ucptr[1] = 0;
      ucptr[2] = 0;
      ucptr[3] = 0;

      // Convert the view coordinates for the start and end of the
      // ray into voxel coordinates, then compute the origin and direction.

      // compute the view point x value for this pixel. Do this by
      // taking our pixel position, adding the image origin then dividing
      // by the full image size to get a number from 0 to 1-1/fullSize. Then,
      // multiply by two and subtract one to get a number from
      // -1 to 1 - 2/fullSize. Then ass offsetX (which is 1/fullSize) to
      // center it.
      viewRay[0] = ((static_cast<float>(i) + static_cast<float>(imageOrigin[0])) /
                    imageViewportSize[0]) * 2.0 - 1.0 + offsetX;

      // Now transform this point with a z value of 0 for the ray start, and
      // a z value of 1 for the ray end. This corresponds to the near and far
      // plane locations. If IntermixIntersectingGeometry is on, then use
      // the zbuffer value instead of 1.0
      viewRay[2] = 0.0;
      vtkVRCMultiplyPointMacro( viewRay, rayStart,
                                staticInfo->ViewToVoxelsMatrix );

      viewRay[2] =
        (me->ZBuffer)?(me->GetZBufferValue(i,j)):(1.0);
      vtkVRCMultiplyPointMacro( viewRay, rayEnd,
                                staticInfo->ViewToVoxelsMatrix );

      rayDirection[0] = rayEnd[0] - rayStart[0];
      rayDirection[1] = rayEnd[1] - rayStart[1];
      rayDirection[2] = rayEnd[2] - rayStart[2];

      // If cropping is off, or we are just doing a subvolume, we can
      // do the easy thing here
      if ( !me->Cropping || me->CroppingRegionFlags == 0x2000 )
      {
        if ( me->ClipRayAgainstVolume( dynamicInfo, bounds ) &&
             ( staticInfo->NumberOfClippingPlanes == 0 ||
               me->ClipRayAgainstClippingPlanes( dynamicInfo, staticInfo ) ) )
        {
          rayDirection[0] = rayEnd[0] - rayStart[0];
          rayDirection[1] = rayEnd[1] - rayStart[1];
          rayDirection[2] = rayEnd[2] - rayStart[2];

          // Find the length of the input ray. It is not normalized
          norm = sqrt( rayDirection[0] * rayDirection[0] +
                       rayDirection[1] * rayDirection[1] +
                       rayDirection[2] * rayDirection[2] );

          // Normalize this ray into rayStep
          rayStep[0] = rayDirection[0] / norm;
          rayStep[1] = rayDirection[1] / norm;
          rayStep[2] = rayDirection[2] / norm;

          // Correct for perspective in the sample distance. 1.0 over the
          // dot product between this ray and the center ray is the
          // correction factor to allow samples to be taken on parallel
          // planes rather than concentric hemispheres. This factor will
          // compute to be 1.0 for parallel.
          val =  ( rayStep[0] * rayCenter[0] +
                   rayStep[1] * rayCenter[1] +
                   rayStep[2] * rayCenter[2] );
          norm = (val != 0)?(1.0/val):(1.0);

          // Now multiple the normalized step by the sample distance and this
          // correction factor to find the actual step
          rayStep[0] *= norm * sampleDistance * centerScale;
          rayStep[1] *= norm * sampleDistance * centerScale;
          rayStep[2] *= norm * sampleDistance * centerScale;

          absStep[0] = ( rayStep[0] < 0.0 )?(-rayStep[0]):(rayStep[0]);
          absStep[1] = ( rayStep[1] < 0.0 )?(-rayStep[1]):(rayStep[1]);
          absStep[2] = ( rayStep[2] < 0.0 )?(-rayStep[2]):(rayStep[2]);

          if ( absStep[0] >= absStep[1] && absStep[0] >= absStep[2] )
          {
            dynamicInfo->NumberOfStepsToTake = static_cast<int>
              ((rayEnd[0]-rayStart[0]) / rayStep[0]);
          }
          else if ( absStep[1] >= absStep[2] && absStep[1] >= absStep[0] )
          {
            dynamicInfo->NumberOfStepsToTake = static_cast<int>
              ((rayEnd[1]-rayStart[1]) / rayStep[1]);
          }
          else
          {
            dynamicInfo->NumberOfStepsToTake = static_cast<int>
              ((rayEnd[2]-rayStart[2]) / rayStep[2]);
          }

          me->VolumeRayCastFunction->CastRay( dynamicInfo, staticInfo );
          if ( dynamicInfo->Color[3] > 0.0 )
          {
            val = dynamicInfo->Color[0]*255.0;
            val = (val > 255.0)?(255.0):(val);
            val = (val <   0.0)?(  0.0):(val);
            ucptr[0] = static_cast<unsigned char>(val);

            val = dynamicInfo->Color[1]*255.0;
            val = (val > 255.0)?(255.0):(val);
            val = (val <   0.0)?(  0.0):(val);
            ucptr[1] = static_cast<unsigned char>(val);

            val = dynamicInfo->Color[2]*255.0;
            val = (val > 255.0)?(255.0):(val);
            val = (val <   0.0)?(  0.0):(val);
            ucptr[2] = static_cast<unsigned char>(val);

            val = dynamicInfo->Color[3]*255.0;
            val = (val > 255.0)?(255.0):(val);
            val = (val <   0.0)?(  0.0):(val);
            ucptr[3] = static_cast<unsigned char>(val);
          }
        }
      }
      // Otherwise, cropping is on and we don't have a simple subvolume.
      // We'll have to cast a ray for each of the 27 regions that is on
      // and composite the results.
      else
      {
        // We'll keep an array of regions that we intersect, the arrayCount
        // variable will count how many of them we have
        arrayCount = 0;

        // Save the ray start, end, and direction. We will modify this
        // during each iteration of the loop for the current cropping region.
        fullRayStart[0] = rayStart[0];
        fullRayStart[1] = rayStart[1];
        fullRayStart[2] = rayStart[2];

        fullRayEnd[0] = rayEnd[0];
        fullRayEnd[1] = rayEnd[1];
        fullRayEnd[2] = rayEnd[2];

        fullRayDirection[0] = rayDirection[0];
        fullRayDirection[1] = rayDirection[1];
        fullRayDirection[2] = rayDirection[2];

        // Loop through the twenty seven cropping regions
        bitFlag = 1;
        for ( bitLoop = 0; bitLoop < 27; bitLoop++ )
        {
          // Check if this cropping region is on
          if ( !(me->CroppingRegionFlags & bitFlag) )
          {
            bitFlag = bitFlag << 1;
            continue;
          }

          // Restore the ray information
          rayStart[0] = fullRayStart[0];
          rayStart[1] = fullRayStart[1];
          rayStart[2] = fullRayStart[2];

          rayEnd[0] = fullRayEnd[0];
          rayEnd[1] = fullRayEnd[1];
          rayEnd[2] = fullRayEnd[2];

          rayDirection[0] = fullRayDirection[0];
          rayDirection[1] = fullRayDirection[1];
          rayDirection[2] = fullRayDirection[2];

          // Figure out the bounds of the cropping region
          // along the X axis
          switch ( bitLoop % 3 )
          {
            case 0:
              bounds[0] = 0;
              bounds[1] = me->VoxelCroppingRegionPlanes[0];
              break;
            case 1:
              bounds[0] = me->VoxelCroppingRegionPlanes[0];
              bounds[1] = me->VoxelCroppingRegionPlanes[1];
              break;
            case 2:
              bounds[0] = me->VoxelCroppingRegionPlanes[1];
              bounds[1] = (staticInfo->DataSize[0] - 1)  - VTK_RAYCAST_FLOOR_TOL;
              break;
          }

          // Figure out the bounds of the cropping region
          // along the Y axis
          switch ( (bitLoop % 9) / 3 )
          {
            case 0:
              bounds[2] = 0;
              bounds[3] = me->VoxelCroppingRegionPlanes[2];
              break;
            case 1:
              bounds[2] = me->VoxelCroppingRegionPlanes[2];
              bounds[3] = me->VoxelCroppingRegionPlanes[3];
              break;
            case 2:
              bounds[2] = me->VoxelCroppingRegionPlanes[3];
              bounds[3] = (staticInfo->DataSize[1] - 1)  - VTK_RAYCAST_FLOOR_TOL;
              break;
          }

          // Figure out the bounds of the cropping region
          // along the Z axis
          switch ( bitLoop / 9 )
          {
            case 0:
              bounds[4] = 0;
              bounds[5] = me->VoxelCroppingRegionPlanes[4];
              break;
            case 1:
              bounds[4] = me->VoxelCroppingRegionPlanes[4];
              bounds[5] = me->VoxelCroppingRegionPlanes[5];
              break;
            case 2:
              bounds[4] = me->VoxelCroppingRegionPlanes[5];
              bounds[5] = (staticInfo->DataSize[2] - 1)  - VTK_RAYCAST_FLOOR_TOL;
              break;
          }

          // Check against the bounds of the volume
          for ( k = 0; k < 3; k++ )
          {
            if ( bounds[2*k] < 0 )
            {
              bounds[2*k] = 0;
            }
            if ( bounds[2*k + 1] > ((staticInfo->DataSize[k]-1) - VTK_RAYCAST_FLOOR_TOL) )
            {
              bounds[2*k + 1] = ((staticInfo->DataSize[k] - 1)  - VTK_RAYCAST_FLOOR_TOL);
            }
          }

          // Clip against the volume and the clipping planes
          if ( me->ClipRayAgainstVolume( dynamicInfo, bounds ) &&
               ( staticInfo->NumberOfClippingPlanes == 0 ||
                 me->ClipRayAgainstClippingPlanes( dynamicInfo,
                                                   staticInfo ) ) )
          {
            // The ray start and end may have changed - recompute
            // the direction
            rayDirection[0] = rayEnd[0] - rayStart[0];
            rayDirection[1] = rayEnd[1] - rayStart[1];
            rayDirection[2] = rayEnd[2] - rayStart[2];

            // Find the length of the ray. It is not normalized yet
            norm = sqrt( rayDirection[0] * rayDirection[0] +
                         rayDirection[1] * rayDirection[1] +
                         rayDirection[2] * rayDirection[2] );

            // Normalize this ray into rayStep
            rayStep[0] = rayDirection[0] / norm;
            rayStep[1] = rayDirection[1] / norm;
            rayStep[2] = rayDirection[2] / norm;

            // Correct for perspective in the sample distance. 1.0 over the
            // dot product between this ray and the center ray is the
            // correction factor to allow samples to be taken on parallel
            // planes rather than concentric hemispheres. This factor will
            // compute to be 1.0 for parallel.
            val =  ( rayStep[0] * rayCenter[0] +
                     rayStep[1] * rayCenter[1] +
                     rayStep[2] * rayCenter[2] );
            norm = (val != 0)?(1.0/val):(1.0);

            // Now multiple the normalized step by the sample distance and this
            // correction factor to find the actual step
            rayStep[0] *= norm * sampleDistance * centerScale;
            rayStep[1] *= norm * sampleDistance * centerScale;
            rayStep[2] *= norm * sampleDistance * centerScale;

            // Find the major direction to determine the number of
            // steps to take
            absStep[0] = ( rayStep[0] < 0.0 )?(-rayStep[0]):(rayStep[0]);
            absStep[1] = ( rayStep[1] < 0.0 )?(-rayStep[1]):(rayStep[1]);
            absStep[2] = ( rayStep[2] < 0.0 )?(-rayStep[2]):(rayStep[2]);
            if ( absStep[0] >= absStep[1] && absStep[0] >= absStep[2] )
            {
              dynamicInfo->NumberOfStepsToTake = static_cast<int>
                ((rayEnd[0]-rayStart[0]) / rayStep[0]);
            }
            else if ( absStep[1] >= absStep[2] && absStep[1] >= absStep[0] )
            {
              dynamicInfo->NumberOfStepsToTake = static_cast<int>
                ((rayEnd[1]-rayStart[1]) / rayStep[1]);
            }
            else
            {
              dynamicInfo->NumberOfStepsToTake = static_cast<int>
                ((rayEnd[2]-rayStart[2]) / rayStep[2]);
            }

            // Cast the ray
            me->VolumeRayCastFunction->CastRay( dynamicInfo, staticInfo );

            // If the ray returns a non-transparent color, store this
            // in our arrays of distances and colors
            if ( dynamicInfo->Color[3] > 0.0 )
            {
              // Figure out the distance from this ray start to the full ray
              // start and use this to sort the ray segments
              for ( k = 0; k < 3; k++ )
              {
                if ( absStep[k] >= absStep[(k+1)%3] &&
                     absStep[k] >= absStep[(k+2)%3] )
                {
                  distanceArray[arrayCount] =
                    (rayStart[k] - fullRayStart[k]) / rayStep[k];
                  break;
                }
              }

              // Store the ray color
              rgbaArray[4*arrayCount  ] = dynamicInfo->Color[0];
              rgbaArray[4*arrayCount+1] = dynamicInfo->Color[1];
              rgbaArray[4*arrayCount+2] = dynamicInfo->Color[2];
              rgbaArray[4*arrayCount+3] = dynamicInfo->Color[3];
              scalarArray[arrayCount] = dynamicInfo->ScalarValue;

              if ( !staticInfo->MIPFunction )
              {
                // Do a sort pass (one iteration of bubble sort each time an
                // element is added. The array stores element from farthest
                // to closest
                for ( k = arrayCount;
                      k > 0 && distanceArray[k] > distanceArray[k-1]; k-- )
                {
                  tmp = distanceArray[k];
                  distanceArray[k] = distanceArray[k-1];
                  distanceArray[k-1] = tmp;

                  tmpArray[0] = rgbaArray[4*k  ];
                  tmpArray[1] = rgbaArray[4*k+1];
                  tmpArray[2] = rgbaArray[4*k+2];
                  tmpArray[3] = rgbaArray[4*k+3];

                  rgbaArray[4*k  ] = rgbaArray[4*(k-1)  ];
                  rgbaArray[4*k+1] = rgbaArray[4*(k-1)+1];
                  rgbaArray[4*k+2] = rgbaArray[4*(k-1)+2];
                  rgbaArray[4*k+3] = rgbaArray[4*(k-1)+3];

                  rgbaArray[4*(k-1)  ] = tmpArray[0];
                  rgbaArray[4*(k-1)+1] = tmpArray[1];
                  rgbaArray[4*(k-1)+2] = tmpArray[2];
                  rgbaArray[4*(k-1)+3] = tmpArray[3];
                }
              }

              arrayCount++;
            }
          }

          // Move the bit over by one
          bitFlag = bitFlag << 1;
        }

        // We have encountered something in at least one crop region -
        // merge all results into one RGBA value
        if ( arrayCount )
        {
          // Is this MIP compositing? We need to treat this differently
          if ( staticInfo->MIPFunction )
          {
            dynamicInfo->Color[0] = 0.0;
            dynamicInfo->Color[1] = 0.0;
            dynamicInfo->Color[2] = 0.0;
            dynamicInfo->Color[3] = 0.0;
            dynamicInfo->ScalarValue = 0.0;

            // If we are maximizing the opacity, find the max Color[3]
            if ( staticInfo->MaximizeOpacity )
            {
              for ( k = 0; k < arrayCount; k++ )
              {
                if ( rgbaArray[k*4+3] > dynamicInfo->Color[3] )
                {
                  dynamicInfo->Color[0] = rgbaArray[k*4  ];
                  dynamicInfo->Color[1] = rgbaArray[k*4+1];
                  dynamicInfo->Color[2] = rgbaArray[k*4+2];
                  dynamicInfo->Color[3] = rgbaArray[k*4+3];
                }
              }
            }
            // Otherwise we are maximizing scalar value
            else
            {
              for ( k = 0; k < arrayCount; k++ )
              {
                if ( scalarArray[k] > dynamicInfo->ScalarValue )
                {
                  dynamicInfo->Color[0] = rgbaArray[k*4  ];
                  dynamicInfo->Color[1] = rgbaArray[k*4+1];
                  dynamicInfo->Color[2] = rgbaArray[k*4+2];
                  dynamicInfo->Color[3] = rgbaArray[k*4+3];
                  dynamicInfo->ScalarValue = scalarArray[k];
                }
              }
            }
          }
          else
          {
            // Now we have the sorted distances / colors, put them together
            // in a back-to-front order. First, initialize the color to black
            // and the remaining opacity (color[3]) to 1.0
            dynamicInfo->Color[0] = 0.0;
            dynamicInfo->Color[1] = 0.0;
            dynamicInfo->Color[2] = 0.0;
            dynamicInfo->Color[3] = 1.0;

           // Now do alpha blending, keeping remaining opacity in color[3]
            for ( k = 0; k < arrayCount; k++ )
            {
              dynamicInfo->Color[0] = dynamicInfo->Color[0] *
                (1.0 - rgbaArray[k*4 + 3]) + rgbaArray[k*4 + 0];
              dynamicInfo->Color[1] = dynamicInfo->Color[1] *
                (1.0 - rgbaArray[k*4 + 3]) + rgbaArray[k*4 + 1];
              dynamicInfo->Color[2] = dynamicInfo->Color[2] *
                (1.0 - rgbaArray[k*4 + 3]) + rgbaArray[k*4 + 2];
              dynamicInfo->Color[3] *= 1.0 - rgbaArray[k*4 + 3];
            }

            // Take 1.0 - color[3] to convert from remaining opacity to alpha.
            dynamicInfo->Color[3] = 1.0 - dynamicInfo->Color[3];
          }

          val = dynamicInfo->Color[0]*255.0;
          val = (val > 255.0)?(255.0):(val);
          val = (val <   0.0)?(  0.0):(val);
          ucptr[0] = static_cast<unsigned char>(val);

          val = dynamicInfo->Color[1]*255.0;
          val = (val > 255.0)?(255.0):(val);
          val = (val <   0.0)?(  0.0):(val);
          ucptr[1] = static_cast<unsigned char>(val);

          val = dynamicInfo->Color[2]*255.0;
          val = (val > 255.0)?(255.0):(val);
          val = (val <   0.0)?(  0.0):(val);
          ucptr[2] = static_cast<unsigned char>(val);

          val = dynamicInfo->Color[3]*255.0;
          val = (val > 255.0)?(255.0):(val);
          val = (val <   0.0)?(  0.0):(val);
          ucptr[3] = static_cast<unsigned char>(val);
        }
      }

      // Increment the image pointer
      ucptr+=4;
    }
  }

  delete dynamicInfo;

  return VTK_THREAD_RETURN_VALUE;
}

double vtkVolumeRayCastMapper::GetZBufferValue(int x, int y)
{
  int xPos, yPos;

  xPos = static_cast<int>(static_cast<float>(x) * this->ImageSampleDistance);
  yPos = static_cast<int>(static_cast<float>(y) * this->ImageSampleDistance);

  xPos = (xPos >= this->ZBufferSize[0])?(this->ZBufferSize[0]-1):(xPos);
  yPos = (yPos >= this->ZBufferSize[1])?(this->ZBufferSize[1]-1):(yPos);

  return *(this->ZBuffer + yPos*this->ZBufferSize[0] + xPos);
}

int vtkVolumeRayCastMapper::ComputeRowBounds(vtkVolume   *vol,
                                             vtkRenderer *ren)
{
  float voxelPoint[3];
  float viewPoint[8][4];
  int i, j, k;
  unsigned char *ucptr;
  float minX, minY, maxX, maxY, minZ, maxZ;

  minX =  1.0;
  minY =  1.0;
  maxX = -1.0;
  maxY = -1.0;
  minZ =  1.0;
  maxZ =  0.0;

  float bounds[6];
  int dim[3];

  this->GetInput()->GetDimensions(dim);
  bounds[0] = bounds[2] = bounds[4] = 0.0;
  bounds[1] = static_cast<float>(dim[0]-1) - VTK_RAYCAST_FLOOR_TOL;
  bounds[3] = static_cast<float>(dim[1]-1) - VTK_RAYCAST_FLOOR_TOL;
  bounds[5] = static_cast<float>(dim[2]-1) - VTK_RAYCAST_FLOOR_TOL;

  double camPos[3];
  double worldBounds[6];
  vol->GetBounds( worldBounds );
  int insideFlag = 0;
  ren->GetActiveCamera()->GetPosition( camPos );
  if ( camPos[0] >= worldBounds[0] &&
       camPos[0] <= worldBounds[1] &&
       camPos[1] >= worldBounds[2] &&
       camPos[1] <= worldBounds[3] &&
       camPos[2] >= worldBounds[4] &&
       camPos[2] <= worldBounds[5] )
  {
    insideFlag = 1;
  }


  // If we have a simple crop box then we can tighten the bounds
  // See prior explanation of RoundingTieBreaker
  if ( this->Cropping && this->CroppingRegionFlags == 0x2000 )
  {
    bounds[0] = this->VoxelCroppingRegionPlanes[0];
    bounds[1] = this->VoxelCroppingRegionPlanes[1] - VTK_RAYCAST_FLOOR_TOL;
    bounds[2] = this->VoxelCroppingRegionPlanes[2];
    bounds[3] = this->VoxelCroppingRegionPlanes[3] - VTK_RAYCAST_FLOOR_TOL;
    bounds[4] = this->VoxelCroppingRegionPlanes[4];
    bounds[5] = this->VoxelCroppingRegionPlanes[5] - VTK_RAYCAST_FLOOR_TOL;
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

  // We have min/max values from -1.0 to 1.0 now - we want to convert
  // these to pixel locations. Give a couple of pixels of breathing room
  // on each side if possible
  minX = ( minX + 1.0 ) * 0.5 * this->ImageViewportSize[0] - 2;
  minY = ( minY + 1.0 ) * 0.5 * this->ImageViewportSize[1] - 2;
  maxX = ( maxX + 1.0 ) * 0.5 * this->ImageViewportSize[0] + 2;
  maxY = ( maxY + 1.0 ) * 0.5 * this->ImageViewportSize[1] + 2;

  // If we are outside the view frustum return 0 - there is no need
  // to render anything
  if ( ( minX < 0 && maxX < 0 ) ||
       ( minY < 0 && maxY < 0 ) ||
       ( minX > this->ImageViewportSize[0]-1 &&
         maxX > this->ImageViewportSize[0]-1 ) ||
       ( minY > this->ImageViewportSize[1]-1 &&
         maxY > this->ImageViewportSize[1]-1 ) )
  {
    return 0;
  }

  int oldImageMemorySize[2];
  oldImageMemorySize[0] = this->ImageMemorySize[0];
  oldImageMemorySize[1] = this->ImageMemorySize[1];

  // Swap the row bounds
  int *tmpptr;
  tmpptr = this->RowBounds;
  this->RowBounds = this->OldRowBounds;
  this->OldRowBounds = tmpptr;


  // Check the bounds - the volume might project outside of the
  // viewing box / frustum so clip it if necessary
  minX = (minX<0)?(0):(minX);
  minY = (minY<0)?(0):(minY);
  maxX = (maxX>this->ImageViewportSize[0]-1)?
    (this->ImageViewportSize[0]-1):(maxX);
  maxY = (maxY>this->ImageViewportSize[1]-1)?
    (this->ImageViewportSize[1]-1):(maxY);

  // Create the new image, and set its size and position
  this->ImageInUseSize[0] = static_cast<int>(maxX - minX + 1.0);
  this->ImageInUseSize[1] = static_cast<int>(maxY - minY + 1.0);

  // What is a power of 2 size big enough to fit this image?
  this->ImageMemorySize[0] = 32;
  this->ImageMemorySize[1] = 32;
  while ( this->ImageMemorySize[0] < this->ImageInUseSize[0] )
  {
    this->ImageMemorySize[0] *= 2;
  }
  while ( this->ImageMemorySize[1] < this->ImageInUseSize[1] )
  {
    this->ImageMemorySize[1] *= 2;
  }

  this->ImageOrigin[0] = static_cast<int>(minX);
  this->ImageOrigin[1] = static_cast<int>(minY);

  // If the old image size is much too big (more than twice in
  // either direction) then set the old width to 0 which will
  // cause the image to be recreated
  if ( oldImageMemorySize[0] > 2*this->ImageMemorySize[0] ||
       oldImageMemorySize[1] > 2*this->ImageMemorySize[1] )
  {
    oldImageMemorySize[0] = 0;
  }

  // If the old image is big enough (but not too big - we handled
  // that above) then we'll bump up our required size to the
  // previous one. This will keep us from thrashing.
  if ( oldImageMemorySize[0] >= this->ImageMemorySize[0] &&
       oldImageMemorySize[1] >= this->ImageMemorySize[1] )
  {
    this->ImageMemorySize[0] = oldImageMemorySize[0];
    this->ImageMemorySize[1] = oldImageMemorySize[1];
  }

  // Do we already have a texture big enough? If not, create a new one and
  // clear it.
  if ( !this->Image ||
       this->ImageMemorySize[0] > oldImageMemorySize[0] ||
       this->ImageMemorySize[1] > oldImageMemorySize[1] )
  {
    // If there is an image there must be row bounds
    if ( this->Image )
    {
      delete [] this->Image;
      delete [] this->RowBounds;
      delete [] this->OldRowBounds;
    }

    this->Image = new unsigned char[(this->ImageMemorySize[0] *
                                     this->ImageMemorySize[1] * 4)];

    // Create the row bounds array. This will store the start / stop pixel
    // for each row. This helps eleminate work in areas outside the bounding
    // hexahedron since a bounding box is not very tight. We keep the old ones
    // too to help with only clearing where required
    this->RowBounds = new int [2*this->ImageMemorySize[1]];
    this->OldRowBounds = new int [2*this->ImageMemorySize[1]];

    for ( i = 0; i < this->ImageMemorySize[1]; i++ )
    {
      this->RowBounds[i*2]      = this->ImageMemorySize[0];
      this->RowBounds[i*2+1]    = -1;
      this->OldRowBounds[i*2]   = this->ImageMemorySize[0];
      this->OldRowBounds[i*2+1] = -1;
    }

    ucptr = this->Image;

    for ( i = 0; i < this->ImageMemorySize[0]*this->ImageMemorySize[1]; i++ )
    {
      *(ucptr++) = 0;
      *(ucptr++) = 0;
      *(ucptr++) = 0;
      *(ucptr++) = 0;
    }
  }

  // If we are inside the volume our row bounds indicate every ray must be
  // cast - we don't need to intersect with the 12 lines
  if ( insideFlag )
  {
    for ( j = 0; j < this->ImageInUseSize[1]; j++ )
    {
      this->RowBounds[j*2] = 0;
      this->RowBounds[j*2+1] = this->ImageInUseSize[0] - 1;
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
        0.5*this->ImageViewportSize[0] - this->ImageOrigin[0];

      y1 = (viewPoint[lineIndex[i][0]][1]+1.0) *
        0.5*this->ImageViewportSize[1] - this->ImageOrigin[1];

      x2 = (viewPoint[lineIndex[i][1]][0]+1.0) *
        0.5*this->ImageViewportSize[0] - this->ImageOrigin[0];

      y2 = (viewPoint[lineIndex[i][1]][1]+1.0) *
        0.5*this->ImageViewportSize[1] - this->ImageOrigin[1];

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
    for ( j = 0; j < this->ImageInUseSize[1]; j++ )
    {
      this->RowBounds[j*2] = this->ImageMemorySize[0];
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
          xlow = (xlow>this->ImageInUseSize[0]-1)?
            (this->ImageInUseSize[0]-1):(xlow);

          xhigh = (xhigh<0)?(0):(xhigh);
          xhigh = (xhigh>this->ImageInUseSize[0]-1)?(
            this->ImageInUseSize[0]-1):(xhigh);

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
        this->RowBounds[j*2] = this->ImageMemorySize[0];
        this->RowBounds[j*2+1] = -1;
      }
    }
  }

  for ( j = this->ImageInUseSize[1]; j < this->ImageMemorySize[1]; j++ )
  {
    this->RowBounds[j*2] = this->ImageMemorySize[0];
    this->RowBounds[j*2+1] = -1;
  }

  for ( j = 0; j < this->ImageMemorySize[1]; j++ )
  {
    // New bounds are not overlapping with old bounds - clear between
    // old bounds only
    if ( this->RowBounds[j*2+1] < this->OldRowBounds[j*2] ||
         this->RowBounds[j*2]   > this->OldRowBounds[j*2+1] )
    {
      ucptr = this->Image + 4*( j*this->ImageMemorySize[0] +
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
      ucptr = this->Image + 4*( j*this->ImageMemorySize[0] +
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
      ucptr = this->Image + 4*( j*this->ImageMemorySize[0] +
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


void vtkVolumeRayCastMapper::ComputeMatrices( vtkImageData *data,
                                                 vtkVolume *vol )
{
  // Get the data spacing. This scaling is not accounted for in
  // the volume's matrix, so we must add it in.
  double volumeSpacing[3];
  data->GetSpacing( volumeSpacing );

  // Get the origin of the data.  This translation is not accounted for in
  // the volume's matrix, so we must add it in.
  float volumeOrigin[3];
  const double *bds = data->GetBounds();
  volumeOrigin[0] = bds[0];
  volumeOrigin[1] = bds[2];
  volumeOrigin[2] = bds[4];

  // Get the dimensions of the data.
  int volumeDimensions[3];
  data->GetDimensions( volumeDimensions );

  vtkTransform *voxelsTransform = this->VoxelsTransform;
  vtkTransform *voxelsToViewTransform = this->VoxelsToViewTransform;

  // Get the volume matrix. This is a volume to world matrix right now.
  // We'll need to invert it, translate by the origin and scale by the
  // spacing to change it to a world to voxels matrix.
  this->VolumeMatrix->DeepCopy( vol->GetMatrix() );
  voxelsToViewTransform->SetMatrix( VolumeMatrix );

  // Create a transform that will account for the scaling and translation of
  // the scalar data. The is the volume to voxels matrix.
  voxelsTransform->Identity();
  voxelsTransform->Translate(volumeOrigin[0],
                             volumeOrigin[1],
                             volumeOrigin[2] );

  voxelsTransform->Scale( volumeSpacing[0],
                          volumeSpacing[1],
                          volumeSpacing[2] );

  // Now concatenate the volume's matrix with this scalar data matrix
  voxelsToViewTransform->PreMultiply();
  voxelsToViewTransform->Concatenate( voxelsTransform->GetMatrix() );

  // Now we actually have the world to voxels matrix - copy it out
  this->WorldToVoxelsMatrix->DeepCopy( voxelsToViewTransform->GetMatrix() );
  this->WorldToVoxelsMatrix->Invert();

  // We also want to invert this to get voxels to world
  this->VoxelsToWorldMatrix->DeepCopy( voxelsToViewTransform->GetMatrix() );

  // Compute the voxels to view transform by concatenating the
  // voxels to world matrix with the projection matrix (world to view)
  voxelsToViewTransform->PostMultiply();
  voxelsToViewTransform->Concatenate( this->PerspectiveMatrix );

  this->VoxelsToViewMatrix->DeepCopy( voxelsToViewTransform->GetMatrix() );

  this->ViewToVoxelsMatrix->DeepCopy( this->VoxelsToViewMatrix );
  this->ViewToVoxelsMatrix->Invert();
}

void vtkVolumeRayCastMapper::InitializeClippingPlanes(
                                           vtkVolumeRayCastStaticInfo *staticInfo,
                                           vtkPlaneCollection *planes )
{
  vtkPlane *onePlane;
  double    worldNormal[3], worldOrigin[3];
  double    volumeOrigin[4];
  int      i;
  float    *worldToVoxelsMatrix;
  float    *voxelsToWorldMatrix;
  int      count;
  float    *clippingPlane;
  float    t;

  count = planes->GetNumberOfItems();
  staticInfo->NumberOfClippingPlanes = count;

  if ( !count )
  {
    return;
  }

  worldToVoxelsMatrix = staticInfo->WorldToVoxelsMatrix;
  voxelsToWorldMatrix = staticInfo->VoxelsToWorldMatrix;

  staticInfo->ClippingPlane = new float [4*count];

  // loop through all the clipping planes
  for ( i = 0; i < count; i++ )
  {
    onePlane = static_cast<vtkPlane *>(planes->GetItemAsObject(i));
    onePlane->GetNormal(worldNormal);
    onePlane->GetOrigin(worldOrigin);
    clippingPlane = staticInfo->ClippingPlane + 4*i;
    vtkVRCMultiplyNormalMacro( worldNormal,
                               clippingPlane,
                               voxelsToWorldMatrix );
    vtkVRCMultiplyPointMacro( worldOrigin, volumeOrigin,
                              worldToVoxelsMatrix );

    t = sqrt( clippingPlane[0]*clippingPlane[0] +
              clippingPlane[1]*clippingPlane[1] +
              clippingPlane[2]*clippingPlane[2] );
    if ( t )
    {
      clippingPlane[0] /= t;
      clippingPlane[1] /= t;
      clippingPlane[2] /= t;
    }

    clippingPlane[3] = -(clippingPlane[0]*volumeOrigin[0] +
                         clippingPlane[1]*volumeOrigin[1] +
                         clippingPlane[2]*volumeOrigin[2]);
  }
}


int vtkVolumeRayCastMapper::ClipRayAgainstClippingPlanes(
                                           vtkVolumeRayCastDynamicInfo *dynamicInfo,
                                           vtkVolumeRayCastStaticInfo *staticInfo )
{
  float    *clippingPlane;
  int      i;
  float    rayDir[3];
  float    t, point[3], dp;
  float    *rayStart, *rayEnd;

  rayStart = dynamicInfo->TransformedStart;
  rayEnd = dynamicInfo->TransformedEnd;

  rayDir[0] = rayEnd[0] - rayStart[0];
  rayDir[1] = rayEnd[1] - rayStart[1];
  rayDir[2] = rayEnd[2] - rayStart[2];

  // loop through all the clipping planes
  for ( i = 0; i < staticInfo->NumberOfClippingPlanes; i++ )
  {
    clippingPlane = staticInfo->ClippingPlane + 4*i;

    dp =
      clippingPlane[0]*rayDir[0] +
      clippingPlane[1]*rayDir[1] +
      clippingPlane[2]*rayDir[2];

    if ( dp != 0.0 )
    {
      t =
        -( clippingPlane[0]*rayStart[0] +
           clippingPlane[1]*rayStart[1] +
           clippingPlane[2]*rayStart[2] + clippingPlane[3]) / dp;

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
  }

  return 1;
}

int vtkVolumeRayCastMapper::ClipRayAgainstVolume(
                                            vtkVolumeRayCastDynamicInfo *dynamicInfo,
                                            float bounds[6] )
{
  int    loop;
  float  diff;
  float  t;
  float  *rayStart, *rayEnd, *rayDirection;

  rayStart     = dynamicInfo->TransformedStart;
  rayEnd       = dynamicInfo->TransformedEnd;
  rayDirection = dynamicInfo->TransformedDirection;

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

  return 1;
}

void vtkVolumeRayCastMapper::UpdateShadingTables( vtkRenderer *ren,
                                                  vtkVolume *vol )
{
  int                   shading;
  vtkVolumeProperty     *volume_property;

  volume_property = vol->GetProperty();

  shading = volume_property->GetShade();

  this->GradientEstimator->SetInputData( this->GetInput() );

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
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Sample Distance: " << this->SampleDistance << "\n";
  os << indent << "Image Sample Distance: "
     << this->ImageSampleDistance << "\n";
  os << indent << "Minimum Image Sample Distance: "
     << this->MinimumImageSampleDistance << "\n";
  os << indent << "Maximum Image Sample Distance: "
     << this->MaximumImageSampleDistance << "\n";
  os << indent << "Auto Adjust Sample Distances: "
     << this->AutoAdjustSampleDistances << "\n";
  os << indent << "Intermix Intersecting Geometry: "
    << (this->IntermixIntersectingGeometry ? "On\n" : "Off\n");

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

//----------------------------------------------------------------------------
void vtkVolumeRayCastMapper::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  // These filters share our input and are therefore involved in a
  // reference loop.
  vtkGarbageCollectorReport(collector, this->GradientEstimator,
                            "GradientEstimator");
}

#endif // VTK_LEGACY_REMOVE
