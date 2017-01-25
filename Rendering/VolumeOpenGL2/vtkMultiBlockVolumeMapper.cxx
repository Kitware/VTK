/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiBlockVolumeMapper.cxx

  copyright (c) ken martin, will schroeder, bill lorensen
  all rights reserved.
  see copyright.txt or http://www.kitware.com/copyright.htm for details.

  this software is distributed without any warranty; without even
  the implied warranty of merchantability or fitness for a particular
  purpose.  see the above copyright notice for more information.

=========================================================================*/
#include <algorithm>

#include "vtkBlockSortHelper.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataIterator.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiBlockVolumeMapper.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLGPUVolumeRayCastMapper.h"
#include "vtkPerlinNoise.h"
#include "vtkSmartVolumeMapper.h"


//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkMultiBlockVolumeMapper)

//------------------------------------------------------------------------------
vtkMultiBlockVolumeMapper::vtkMultiBlockVolumeMapper()
: RenderingMapper(vtkSmartVolumeMapper::New())
{
  this->RenderingMapper->SetRequestedRenderModeToGPU();
}

//------------------------------------------------------------------------------
vtkMultiBlockVolumeMapper::~vtkMultiBlockVolumeMapper()
{
  this->RenderingMapper->Delete();
  this->RenderingMapper = NULL;

  this->ClearBlocks();
}

//------------------------------------------------------------------------------
void vtkMultiBlockVolumeMapper::Render(vtkRenderer* ren, vtkVolume* vol)
{
  vtkCompositeDataPipeline* exec =
    vtkCompositeDataPipeline::SafeDownCast(this->GetExecutive());

  if (exec->GetPipelineMTime() > this->BlockLoadingTime)
  {
    vtkDebugMacro("Reloading data blocks!");
    this->ClearBlocks();
    this->LoadBlocks();
    this->BlockLoadingTime.Modified();
  }

  this->SortBlocks(ren, vol->GetMatrix());

  BlockVec::const_iterator end = this->DataBlocks.end();
  for (BlockVec::const_iterator it = this->DataBlocks.begin(); it != end; ++it)
  {
    (*it)->Modified();
    this->RenderingMapper->SetInputData((*it));
    this->RenderingMapper->Render(ren, vol);
  }
}

//------------------------------------------------------------------------------
void vtkMultiBlockVolumeMapper::SortBlocks(vtkRenderer* ren,
  vtkMatrix4x4* volumeMat)
{
  vtkBlockSortHelper::BackToFront sortBlocks(ren, volumeMat);
  std::sort(this->DataBlocks.begin(), this->DataBlocks.end(), sortBlocks);
}

//------------------------------------------------------------------------------
double* vtkMultiBlockVolumeMapper::GetBounds()
{
  vtkDataObjectTree* data = this->GetDataObjectTreeInput();
  if (!data)
  {
    vtkMath::UninitializeBounds(this->Bounds);
    return this->Bounds;
  }
  else
  {
    this->Update();

    vtkCompositeDataPipeline* exec =
      vtkCompositeDataPipeline::SafeDownCast(this->GetExecutive());
    if (exec->GetPipelineMTime() > this->BoundsComputeTime)
    {
      this->ComputeBounds();
    }

    return this->Bounds;
  }
}

//------------------------------------------------------------------------------
void vtkMultiBlockVolumeMapper::ComputeBounds()
{
  vtkMath::UninitializeBounds(this->Bounds);

  vtkInformation* inInfo = this->GetExecutive()->GetInputInformation(0,0);
  vtkCompositeDataSet *input = vtkCompositeDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Not hierarchical, just get the bounds of the data
  if(!input)
  {
    vtkImageData* img = vtkImageData::SafeDownCast(
      this->GetExecutive()->GetInputData(0, 0));

    if (img)
    {
      img->GetBounds(this->Bounds);
    }
    this->BoundsComputeTime.Modified();
    return;
  }

  // Loop over the hierarchy of data objects to compute bounds
  vtkCompositeDataIterator* iter = input->NewIterator();
  iter->GoToFirstItem();
  double bounds[6];
  while (!iter->IsDoneWithTraversal())
  {
    vtkImageData* img = vtkImageData::SafeDownCast(iter->GetCurrentDataObject());
    if (img)
    {
      if (vtkMath::AreBoundsInitialized(this->Bounds))
      {
        // Expand current bounds
        img->GetBounds(bounds);
        for (int i = 0; i < 3; i++)
        {
          this->Bounds[i * 2] = (bounds[i * 2] < this->Bounds[i * 2]) ?
            bounds[i * 2] : this->Bounds[i*2];

          this->Bounds[i * 2 + 1] = (bounds[i * 2 + 1] > this->Bounds[i * 2 + 1]) ?
            bounds[i * 2 + 1] : this->Bounds[i * 2 + 1];
        }
      }
      else
      {
        // Init bounds
        img->GetBounds(this->Bounds);
      }
    }
    iter->GoToNextItem();
  }
  iter->Delete();
  this->BoundsComputeTime.Modified();
}

//------------------------------------------------------------------------------
vtkDataObjectTree* vtkMultiBlockVolumeMapper::GetDataObjectTreeInput()
{
  if (this->GetNumberOfInputConnections(0) < 1)
  {
    return NULL;
  }
  return vtkDataObjectTree::SafeDownCast(this->GetInputDataObject(0, 0));
}

//------------------------------------------------------------------------------
void vtkMultiBlockVolumeMapper::LoadBlocks()
{
  vtkExecutive* exec = this->GetExecutive();
  vtkInformation* info = exec->GetInputInformation(0, 0);
  vtkDataObjectTree* input = vtkDataObjectTree::SafeDownCast(
    info->Get(vtkDataObject::DATA_OBJECT()));

  if (!input)
  {
    vtkDataObject* dataObj = exec->GetInputData(0, 0);
    vtkImageData* currentIm = vtkImageData::SafeDownCast(dataObj);

    if (currentIm)
    {
      vtkImageData* im = vtkImageData::New();
      im->ShallowCopy(currentIm);
      this->DataBlocks.push_back(im);
      return;
    }

    char const* name = dataObj ? dataObj->GetClassName() : "NULL";
    vtkErrorMacro("Cannot handle input of type: " << name);
    return;
  }
  else
  {
    // Hierarchical case
    vtkCompositeDataIterator* it = input->NewIterator();
    it->GoToFirstItem();

    bool warnedOnce = false;
    while (!it->IsDoneWithTraversal())
    {
      vtkImageData* currentIm = vtkImageData::SafeDownCast(it->GetCurrentDataObject());
      if (currentIm)
      {
        vtkImageData* im = vtkImageData::New();
        im->ShallowCopy(currentIm);
        this->DataBlocks.push_back(im);
      }
      else
      {
        if (!warnedOnce)
        {
          vtkErrorMacro("All blocks in the hierarchical dataset must be vtkImageData.");
          warnedOnce = true;
        }
      }
      it->GoToNextItem();
    }
    it->Delete();
  }
}

//------------------------------------------------------------------------------
void vtkMultiBlockVolumeMapper::ReleaseGraphicsResources(vtkWindow* window)
{
  this->RenderingMapper->ReleaseGraphicsResources(window);
}

//------------------------------------------------------------------------------
int vtkMultiBlockVolumeMapper::FillInputPortInformation(int vtkNotUsed(port),
  vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
  return 1;
}

//------------------------------------------------------------------------------
void vtkMultiBlockVolumeMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkMultiBlockVolumeMapper::ClearBlocks()
{
  BlockVec::const_iterator end = this->DataBlocks.end();
  for (BlockVec::const_iterator it = this->DataBlocks.begin(); it != end; ++it)
  {
    (*it)->Delete();
  }
  this->DataBlocks.clear();
}

//------------------------------------------------------------------------------
void vtkMultiBlockVolumeMapper::SelectScalarArray(int arrayNum)
{
  this->RenderingMapper->SelectScalarArray(arrayNum);
  Superclass::SelectScalarArray(arrayNum);
}

//------------------------------------------------------------------------------
void vtkMultiBlockVolumeMapper::SelectScalarArray(char const* arrayName)
{
  this->RenderingMapper->SelectScalarArray(arrayName);
  Superclass::SelectScalarArray(arrayName);
}

//------------------------------------------------------------------------------
void vtkMultiBlockVolumeMapper::SetScalarMode(int scalarMode)
{
  this->RenderingMapper->SetScalarMode(scalarMode);
  Superclass::SetScalarMode(scalarMode);
}

//------------------------------------------------------------------------------
void vtkMultiBlockVolumeMapper::SetVectorMode(int mode)
{
  this->RenderingMapper->SetVectorMode(mode);
}

//------------------------------------------------------------------------------
int vtkMultiBlockVolumeMapper::GetVectorMode()
{
  return this->RenderingMapper->GetVectorMode();
}

//------------------------------------------------------------------------------
void vtkMultiBlockVolumeMapper::SetVectorComponent(int component)
{
  this->RenderingMapper->SetVectorComponent(component);
}

//------------------------------------------------------------------------------
int vtkMultiBlockVolumeMapper::GetVectorComponent()
{
  return this->RenderingMapper->GetVectorComponent();
}

//------------------------------------------------------------------------------
void vtkMultiBlockVolumeMapper::SetJitteringResolution(int x, int y)
{
  vtkOpenGLGPUVolumeRayCastMapper* glMapper =
    vtkOpenGLGPUVolumeRayCastMapper::SafeDownCast(this->RenderingMapper->GetGPUMapper());

  if (glMapper == NULL)
  {
    vtkWarningMacro("Jittering is not supported in this RenderMode.");
    return;
  }

  if (x <= 0 || y <= 0)
  {
    glMapper->UseJitteringOff();
  }

  vtkPerlinNoise* generator = vtkPerlinNoise::New();
  generator->SetFrequency(static_cast<double>(x), static_cast<double>(y), 1.0);
  generator->SetAmplitude(0.5);

  int texSize[2] = {x, y};
  glMapper->SetNoiseTextureSize(texSize);
  glMapper->SetNoiseGenerator(generator);
  glMapper->UseJitteringOn();

  generator->Delete();
}

//----------------------------------------------------------------------------
void vtkMultiBlockVolumeMapper::SetBlendMode(int mode)
{
  this->RenderingMapper->SetBlendMode(mode);
}

//----------------------------------------------------------------------------
int vtkMultiBlockVolumeMapper::GetBlendMode()
{
  return this->RenderingMapper->GetBlendMode();
}

//----------------------------------------------------------------------------
void vtkMultiBlockVolumeMapper::SetCropping(int mode)
{
  this->RenderingMapper->SetCropping(mode);
}

//----------------------------------------------------------------------------
int vtkMultiBlockVolumeMapper::GetCropping()
{
  return this->RenderingMapper->GetCropping();
}

//----------------------------------------------------------------------------
void vtkMultiBlockVolumeMapper::SetCroppingRegionFlags(int mode)
{
  this->RenderingMapper->SetCroppingRegionFlags(mode);
}

//----------------------------------------------------------------------------
int vtkMultiBlockVolumeMapper::GetCroppingRegionFlags()
{
  return this->RenderingMapper->GetCroppingRegionFlags();
}

//----------------------------------------------------------------------------
void vtkMultiBlockVolumeMapper::SetCroppingRegionPlanes(double* planes)
{
  this->SetCroppingRegionPlanes(planes[0], planes[1], planes[2], planes[3],
    planes[4],planes[5]);
}

//----------------------------------------------------------------------------
void vtkMultiBlockVolumeMapper::SetCroppingRegionPlanes(double arg1, double arg2,
  double arg3, double arg4, double arg5, double arg6)
{
  this->RenderingMapper->SetCroppingRegionPlanes(arg1, arg2, arg3,
    arg4, arg5, arg6);
}

//----------------------------------------------------------------------------
void vtkMultiBlockVolumeMapper::GetCroppingRegionPlanes(double* planes)
{
  this->RenderingMapper->GetCroppingRegionPlanes(planes);
}

//----------------------------------------------------------------------------
double* vtkMultiBlockVolumeMapper::GetCroppingRegionPlanes()
{
  return this->RenderingMapper->GetCroppingRegionPlanes();
}
