/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridVolumeRayCastMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkUnstructuredGridVolumeRayCastMapper.h"

#include "vtkCamera.h"
#include "vtkEncodedGradientEstimator.h"
#include "vtkEncodedGradientShader.h"
#include "vtkFiniteDifferenceGradientEstimator.h"
#include "vtkUnstructuredGrid.h"
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
#include "vtkUnstructuredGridBunykRayCastFunction.h"
#include "vtkUnstructuredGridVolumeRayCastIterator.h"
#include "vtkUnstructuredGridPreIntegration.h"
#include "vtkUnstructuredGridPartialPreIntegration.h"
#include "vtkUnstructuredGridHomogeneousRayIntegrator.h"
#include "vtkRayCastImageDisplayHelper.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"

#include <math.h>

VTK_THREAD_RETURN_TYPE UnstructuredGridVolumeRayCastMapper_CastRays( void *arg );


vtkStandardNewMacro(vtkUnstructuredGridVolumeRayCastMapper);

vtkCxxSetObjectMacro(vtkUnstructuredGridVolumeRayCastMapper, RayCastFunction,
                     vtkUnstructuredGridVolumeRayCastFunction);
vtkCxxSetObjectMacro(vtkUnstructuredGridVolumeRayCastMapper, RayIntegrator,
                     vtkUnstructuredGridVolumeRayIntegrator);

// Construct a new vtkUnstructuredGridVolumeRayCastMapper with default values
vtkUnstructuredGridVolumeRayCastMapper::vtkUnstructuredGridVolumeRayCastMapper()
{
  this->ImageSampleDistance        =  1.0;
  this->MinimumImageSampleDistance =  1.0;
  this->MaximumImageSampleDistance = 10.0;
  this->AutoAdjustSampleDistances  =  1;

  this->ImageMemorySize[0]     = 0;
  this->ImageMemorySize[1]     = 0;

  this->Threader               = vtkMultiThreader::New();
  this->NumberOfThreads        = this->Threader->GetNumberOfThreads();

  this->Image                  = NULL;

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

  this->IntermixIntersectingGeometry = 1;

  this->ImageDisplayHelper     = vtkRayCastImageDisplayHelper::New();

  this->RayCastFunction = vtkUnstructuredGridBunykRayCastFunction::New();
  this->RayIntegrator = NULL;
  this->RealRayIntegrator = NULL;
}

// Destruct a vtkUnstructuredGridVolumeRayCastMapper - clean up any memory used
vtkUnstructuredGridVolumeRayCastMapper::~vtkUnstructuredGridVolumeRayCastMapper()
{
  this->Threader->Delete();

  if ( this->Image )
    {
    delete [] this->Image;
    }

  if ( this->RenderTableSize )
    {
    delete [] this->RenderTimeTable;
    delete [] this->RenderVolumeTable;
    delete [] this->RenderRendererTable;
    }

  this->ImageDisplayHelper->Delete();

  this->SetRayCastFunction(NULL);
  this->SetRayIntegrator(NULL);
  if (this->RealRayIntegrator)
    {
    this->RealRayIntegrator->UnRegister(this);
    }
}

float vtkUnstructuredGridVolumeRayCastMapper::RetrieveRenderTime( vtkRenderer *ren,
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

void vtkUnstructuredGridVolumeRayCastMapper::StoreRenderTime( vtkRenderer *ren,
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

void vtkUnstructuredGridVolumeRayCastMapper::ReleaseGraphicsResources(vtkWindow *)
{
}

void vtkUnstructuredGridVolumeRayCastMapper::Render( vtkRenderer *ren, vtkVolume *vol )
{
  int i;

  // Check for input
  if ( this->GetInput() == NULL )
    {
    vtkErrorMacro(<< "No Input!");
    return;
    }

  this->Scalars = this->GetScalars(this->GetInput(), this->ScalarMode,
                                   this->ArrayAccessMode,
                                   this->ArrayId, this->ArrayName,
                                   this->CellScalars);

  if (this->Scalars == NULL)
    {
    vtkErrorMacro("Can't use the ray cast mapper without scalars!");
    return;
    }

  int inputAlgPort;
  vtkAlgorithm* inputAlg = this->GetInputAlgorithm(0, 0, inputAlgPort);
  inputAlg->UpdateInformation();
  inputAlg->SetUpdateExtentToWholeExtent(inputAlgPort);
  inputAlg->Update();

  // Check to make sure we have an appropriate integrator.
  if (this->RayIntegrator)
    {
    if (this->RealRayIntegrator != this->RayIntegrator)
      {
      if (this->RealRayIntegrator)
        {
        this->RealRayIntegrator->UnRegister(this);
        }
      this->RealRayIntegrator = this->RayIntegrator;
      this->RealRayIntegrator->Register(this);
      }
    }
  else
    {

#define ESTABLISH_INTEGRATOR(classname)                                        \
  if (   !this->RealRayIntegrator                                              \
      || (!this->RealRayIntegrator->IsA(#classname)) )                         \
    {                                                                          \
    if (this->RealRayIntegrator) this->RealRayIntegrator->UnRegister(this);    \
    this->RealRayIntegrator = classname::New();                                \
    this->RealRayIntegrator->Register(this);                                   \
    this->RealRayIntegrator->Delete();                                         \
    }                                                                          \

    if (this->CellScalars)
      {
      ESTABLISH_INTEGRATOR(vtkUnstructuredGridHomogeneousRayIntegrator);
      }
    else
      {
      if (vol->GetProperty()->GetIndependentComponents())
        {
        ESTABLISH_INTEGRATOR(vtkUnstructuredGridPreIntegration);
        }
      else
        {
        ESTABLISH_INTEGRATOR(vtkUnstructuredGridPartialPreIntegration);
        }
      }
    }

#undef ESTABLISH_INTEGRATOR

  // Start timing now. We didn't want to capture the update of the
  // input data in the times
  this->Timer->StartTimer();

  int oldImageMemorySize[2];
  oldImageMemorySize[0] = this->ImageMemorySize[0];
  oldImageMemorySize[1] = this->ImageMemorySize[1];

  // If we are automatically adjusting the size to achieve a desired frame
  // rate, then do that adjustment here. Base the new image sample distance
  // on the previous one and the previous render time. Don't let
  // the adjusted image sample distance be less than the minimum image sample
  // distance or more than the maximum image sample distance.
  float oldImageSampleDistance = this->ImageSampleDistance;
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

  this->ImageInUseSize[0] = this->ImageViewportSize[0];
  this->ImageInUseSize[1] = this->ImageViewportSize[1];
  this->ImageOrigin[0] = 0;
  this->ImageOrigin[1] = 0;

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
      }

    this->Image = new unsigned char[(this->ImageMemorySize[0] *
                                     this->ImageMemorySize[1] * 4)];

    unsigned char *ucptr = this->Image;

    for ( i = 0; i < this->ImageMemorySize[0]*this->ImageMemorySize[1]; i++ )
      {
      *(ucptr++) = 0;
      *(ucptr++) = 0;
      *(ucptr++) = 0;
      *(ucptr++) = 0;
      }
    }

  // Capture the zbuffer if necessary
  if ( this->IntermixIntersectingGeometry &&
       ren->GetNumberOfPropsRendered() )
    {
    int x1, x2, y1, y2;
    double *viewport   =  ren->GetViewport();
    int *renWinSize   =  ren->GetRenderWindow()->GetSize();

    // turn this->ImageOrigin into (x1,y1) in window (not viewport!)
    // coordinates.
    x1 = static_cast<int> (
      viewport[0] * static_cast<float>(renWinSize[0]) +
      static_cast<float>(this->ImageOrigin[0]) * this->ImageSampleDistance );
    y1 = static_cast<int> (
      viewport[1] * static_cast<float>(renWinSize[1]) +
      static_cast<float>(this->ImageOrigin[1]) * this->ImageSampleDistance);

    // compute z buffer size
    this->ZBufferSize[0] = static_cast<int>(
      static_cast<float>(this->ImageInUseSize[0]) * this->ImageSampleDistance);
    this->ZBufferSize[1] = static_cast<int>(
      static_cast<float>(this->ImageInUseSize[1]) * this->ImageSampleDistance);

    // Use the size to compute (x2,y2) in window coordinates
    x2 = x1 + this->ZBufferSize[0] - 1;
    y2 = y1 + this->ZBufferSize[1] - 1;

    // This is the z buffer origin (in viewport coordinates)
    this->ZBufferOrigin[0] = static_cast<int>(
      static_cast<float>(this->ImageOrigin[0]) * this->ImageSampleDistance);
    this->ZBufferOrigin[1] = static_cast<int>(
      static_cast<float>(this->ImageOrigin[1]) * this->ImageSampleDistance);

    // Capture the z buffer
    this->ZBuffer = ren->GetRenderWindow()->GetZbufferData(x1,y1,x2,y2);
    }

  this->RayCastFunction->Initialize( ren, vol );

  this->RealRayIntegrator->Initialize(vol, this->Scalars);

  // Save the volume and mapper temporarily so that they can be accessed later
  this->CurrentVolume   = vol;
  this->CurrentRenderer = ren;

  // Create iterators and buffers here to prevent race conditions.
  this->RayCastIterators
    = new vtkUnstructuredGridVolumeRayCastIterator*[this->NumberOfThreads];
  this->IntersectedCellsBuffer    = new vtkIdList*[this->NumberOfThreads];
  this->IntersectionLengthsBuffer = new vtkDoubleArray*[this->NumberOfThreads];
  this->NearIntersectionsBuffer   = new vtkDataArray*[this->NumberOfThreads];
  this->FarIntersectionsBuffer    = new vtkDataArray*[this->NumberOfThreads];
  for (i = 0; i < this->NumberOfThreads; i++)
    {
    this->RayCastIterators[i] = this->RayCastFunction->NewIterator();
    this->IntersectionLengthsBuffer[i] = vtkDoubleArray::New();
    this->IntersectionLengthsBuffer[i]
      ->Allocate(this->RayCastIterators[i]->GetMaxNumberOfIntersections());
    this->NearIntersectionsBuffer[i]
      = vtkDataArray::CreateDataArray(this->Scalars->GetDataType());
    this->NearIntersectionsBuffer[i]
      ->Allocate(this->RayCastIterators[i]->GetMaxNumberOfIntersections());
    if (this->CellScalars)
      {
      this->IntersectedCellsBuffer[i] = vtkIdList::New();
      this->IntersectedCellsBuffer[i]
        ->Allocate(this->RayCastIterators[i]->GetMaxNumberOfIntersections());
      this->FarIntersectionsBuffer[i] = this->NearIntersectionsBuffer[i];
      }
    else
      {
      this->IntersectedCellsBuffer[i] = NULL;
      this->FarIntersectionsBuffer[i]
        = vtkDataArray::CreateDataArray(this->Scalars->GetDataType());
      this->FarIntersectionsBuffer[i]
        ->Allocate(this->RayCastIterators[i]->GetMaxNumberOfIntersections());
      }
    }

  // Set the number of threads to use for ray casting,
  // then set the execution method and do it.
  this->Threader->SetNumberOfThreads( this->NumberOfThreads );
  this->Threader->SetSingleMethod( UnstructuredGridVolumeRayCastMapper_CastRays,
                                   (void *)this);
  this->Threader->SingleMethodExecute();

  // We don't need these anymore
  this->CurrentVolume   = NULL;
  this->CurrentRenderer = NULL;
  for (i = 0; i < this->NumberOfThreads; i++)
    {
    this->RayCastIterators[i]->Delete();
    this->IntersectionLengthsBuffer[i]->Delete();
    this->NearIntersectionsBuffer[i]->Delete();
    if (this->CellScalars)
      {
      this->IntersectedCellsBuffer[i]->Delete();
      }
    else
      {
      this->FarIntersectionsBuffer[i]->Delete();
      }
    }
  delete[] this->RayCastIterators;
  delete[] this->IntersectedCellsBuffer;
  delete[] this->IntersectionLengthsBuffer;
  delete[] this->NearIntersectionsBuffer;
  delete[] this->FarIntersectionsBuffer;

  if ( !ren->GetRenderWindow()->GetAbortRender() )
    {
    float depth;
    if ( this->IntermixIntersectingGeometry )
      {
      depth = this->GetMinimumBoundsDepth( ren, vol );
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
  else
    {
    this->ImageSampleDistance = oldImageSampleDistance;
    }


  if ( this->ZBuffer )
    {
    delete [] this->ZBuffer;
    this->ZBuffer = NULL;
    }

  this->UpdateProgress(1.0);
}

VTK_THREAD_RETURN_TYPE UnstructuredGridVolumeRayCastMapper_CastRays( void *arg )
{
  // Get the info out of the input structure
  int threadID    = ((vtkMultiThreader::ThreadInfo *)(arg))->ThreadID;
  int threadCount = ((vtkMultiThreader::ThreadInfo *)(arg))->NumberOfThreads;
  vtkUnstructuredGridVolumeRayCastMapper *me =
    (vtkUnstructuredGridVolumeRayCastMapper *)((vtkMultiThreader::ThreadInfo *)arg)->UserData;

  if ( !me )
    {
    vtkGenericWarningMacro("The volume does not have a ray cast mapper!");
    return VTK_THREAD_RETURN_VALUE;
    }

  me->CastRays( threadID, threadCount );

  return VTK_THREAD_RETURN_VALUE;
}

template<class T>
inline void vtkUGVRCMLookupCopy(const T *src, T *dest, vtkIdType *lookup,
                                int numcomponents, int numtuples)
{
  for (vtkIdType i = 0; i < numtuples; i++)
    {
    const T *srctuple = src + lookup[i] * numcomponents;
    for (int j = 0; j < numcomponents; j++)
      {
      *dest = *srctuple;
      dest++;  srctuple++;
      }
    }
}

void vtkUnstructuredGridVolumeRayCastMapper::CastRays( int threadID, int threadCount )
{
  int i, j;
  unsigned char *ucptr;

  vtkRenderWindow *renWin = this->CurrentRenderer->GetRenderWindow();
  vtkUnstructuredGridVolumeRayCastIterator *iterator
    = this->RayCastIterators[threadID];

  vtkIdList *intersectedCells = this->IntersectedCellsBuffer[threadID];
  vtkDoubleArray *intersectionLengths=this->IntersectionLengthsBuffer[threadID];
  vtkDataArray *nearIntersections = this->NearIntersectionsBuffer[threadID];
  vtkDataArray *farIntersections = this->FarIntersectionsBuffer[threadID];

  for ( j = 0; j < this->ImageInUseSize[1]; j++ )
    {
    if ( j%threadCount != threadID )
      {
      continue;
      }

    if ( !threadID )
      {
      this->UpdateProgress((double)j/this->ImageInUseSize[1]);
      if ( renWin->CheckAbortStatus() )
        {
        break;
        }
      }
    else if ( renWin->GetAbortRender() )
      {
      break;
      }

    ucptr = this->Image + 4*j*this->ImageMemorySize[0];

    for ( i = 0; i < this->ImageInUseSize[0]; i++ )
      {
      int x = i + this->ImageOrigin[0];
      int y = j + this->ImageOrigin[1];

      double bounds[2] = {0.0,1.0};
      float color[4] = {0.0f, 0.0f, 0.0f, 0.0f};

      if ( this->ZBuffer )
        {
        bounds[1] = this->GetZBufferValue( x, y );
        }

      iterator->SetBounds(bounds);
      iterator->Initialize(x, y);

      vtkIdType numIntersections;
      do
        {
        if (this->CellScalars)
          {
          numIntersections = iterator->GetNextIntersections(intersectedCells,
                                                            intersectionLengths,
                                                            NULL,
                                                            NULL, NULL);
          nearIntersections
            ->SetNumberOfComponents(this->Scalars->GetNumberOfComponents());
          nearIntersections->SetNumberOfTuples(numIntersections);
          switch (this->Scalars->GetDataType())
            {
            vtkTemplateMacro(vtkUGVRCMLookupCopy
                             ((const VTK_TT*)this->Scalars->GetVoidPointer(0),
                              (VTK_TT*)nearIntersections->GetVoidPointer(0),
                              intersectedCells->GetPointer(0),
                              this->Scalars->GetNumberOfComponents(),
                              numIntersections));
            }
          }
        else
          {
          numIntersections = iterator->GetNextIntersections(NULL,
                                                            intersectionLengths,
                                                            this->Scalars,
                                                            nearIntersections,
                                                            farIntersections);
          }
        if (numIntersections < 1) break;
        this->RealRayIntegrator->Integrate(intersectionLengths,
                                           nearIntersections,
                                           farIntersections,
                                           color);
        } while (color[3] < 0.99);

      if ( color[3] > 0.0 )
        {
        int val;
        val = static_cast<int>(color[0]*255.0);
        val = (val > 255)?(255):(val);
        val = (val <   0)?(  0):(val);
        ucptr[0] = static_cast<unsigned char>(val);

        val = static_cast<int>(color[1]*255.0);
        val = (val > 255)?(255):(val);
        val = (val <   0)?(  0):(val);
        ucptr[1] = static_cast<unsigned char>(val);

        val = static_cast<int>(color[2]*255.0);
        val = (val > 255)?(255):(val);
        val = (val <   0)?(  0):(val);
        ucptr[2] = static_cast<unsigned char>(val);

        val = static_cast<int>(color[3]*255.0);
        val = (val > 255)?(255):(val);
        val = (val <   0)?(  0):(val);
        ucptr[3] = static_cast<unsigned char>(val);
        }
      else
        {
        ucptr[0] = 0;
        ucptr[1] = 0;
        ucptr[2] = 0;
        ucptr[3] = 0;
        }
      ucptr+=4;
      }
    }
}

double vtkUnstructuredGridVolumeRayCastMapper::
GetMinimumBoundsDepth( vtkRenderer *ren, vtkVolume   *vol )
{
  double bounds[6];
  vol->GetBounds( bounds );

  vtkTransform *perspectiveTransform = vtkTransform::New();
  vtkMatrix4x4 *perspectiveMatrix = vtkMatrix4x4::New();

  ren->ComputeAspect();
  double *aspect = ren->GetAspect();

  // Get the view matrix in two steps - there is a one step method in camera
  // but it turns off stereo so we do not want to use that one
  vtkCamera *cam = ren->GetActiveCamera();
  perspectiveTransform->Identity();
  perspectiveTransform->Concatenate(
    cam->GetProjectionTransformMatrix(aspect[0]/aspect[1], 0.0, 1.0 ));
  perspectiveTransform->Concatenate(cam->GetViewTransformMatrix());
  perspectiveMatrix->DeepCopy(perspectiveTransform->GetMatrix());

  double minZ = 1.0;

  for ( int k = 0; k < 2; k++ )
    {
    for ( int j = 0; j < 2; j++ )
      {
      for ( int i = 0; i < 2; i++ )
        {
        double inPoint[4];
        inPoint[0] = bounds[  i];
        inPoint[1] = bounds[2+j];
        inPoint[2] = bounds[4+k];
        inPoint[3] = 1.0;

        double outPoint[4];
        perspectiveMatrix->MultiplyPoint( inPoint, outPoint );
        double testZ = outPoint[2] / outPoint[3];
        minZ = ( testZ < minZ ) ? (testZ) : (minZ);
        }
      }
    }

  perspectiveTransform->Delete();
  perspectiveMatrix->Delete();

  return minZ;
}

double vtkUnstructuredGridVolumeRayCastMapper::GetZBufferValue(int x, int y)
{
  int xPos, yPos;

  xPos = static_cast<int>(static_cast<float>(x) * this->ImageSampleDistance);
  yPos = static_cast<int>(static_cast<float>(y) * this->ImageSampleDistance);

  xPos = (xPos >= this->ZBufferSize[0])?(this->ZBufferSize[0]-1):(xPos);
  yPos = (yPos >= this->ZBufferSize[1])?(this->ZBufferSize[1]-1):(yPos);

  return *(this->ZBuffer + yPos*this->ZBufferSize[0] + xPos);
}

// Print method for vtkUnstructuredGridVolumeRayCastMapper
void vtkUnstructuredGridVolumeRayCastMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

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

  os << indent << "Number Of Threads: " << this->NumberOfThreads << "\n";

  if (this->RayCastFunction)
    {
    os << indent << "RayCastFunction: " <<
      this->RayCastFunction->GetClassName() << "\n";
    }
  else
    {
    os << indent << "RayCastFunction: (none)\n";
    }

  if (this->RayIntegrator)
    {
    os << indent << "RayIntegrator: "
       << this->RayIntegrator->GetClassName() << endl;
    }
  else
    {
    os << indent << "RayIntegrator: (automatic)" << endl;
    }

  // Do not want to print this->ImageOrigin, this->ImageViewportSize or
  // this->ImageInUseSize since these are just internal variables with Get
  // methods for access from the ray cast function (not part of the public
  // API)
}

