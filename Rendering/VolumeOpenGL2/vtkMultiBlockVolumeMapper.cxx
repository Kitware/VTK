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
#include "vtkBoundingBox.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiBlockVolumeMapper.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLGPUVolumeRayCastMapper.h"
#include "vtkPerlinNoise.h"
#include "vtkRenderWindow.h"
#include "vtkSmartVolumeMapper.h"

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkMultiBlockVolumeMapper);

//------------------------------------------------------------------------------
vtkMultiBlockVolumeMapper::vtkMultiBlockVolumeMapper()
  : FallBackMapper(nullptr)
  , BlockLoadingTime(0)
  , BoundsComputeTime(0)
  , VectorMode(vtkSmartVolumeMapper::DISABLED)
  , VectorComponent(0)
  , RequestedRenderMode(vtkSmartVolumeMapper::DefaultRenderMode)
{
}

//------------------------------------------------------------------------------
vtkMultiBlockVolumeMapper::~vtkMultiBlockVolumeMapper()
{
  this->ClearMappers();
}

//------------------------------------------------------------------------------
void vtkMultiBlockVolumeMapper::Render(vtkRenderer* ren, vtkVolume* vol)
{
  vtkDataObject* dataObj = this->GetDataObjectInput();
  if (dataObj->GetMTime() != this->BlockLoadingTime)
  {
    vtkDebugMacro("Reloading data blocks!");
    this->LoadDataSet(ren, vol);
    this->BlockLoadingTime = dataObj->GetMTime();
  }

  this->SortMappers(ren, vol->GetMatrix());

  MapperVec::const_iterator end = this->Mappers.end();
  for (MapperVec::const_iterator it = this->Mappers.begin(); it != end; ++it)
  {
    if (this->FallBackMapper)
    {
      vtkImageData* image = (*it)->GetInput();
      image->Modified();
      this->FallBackMapper->SetInputData(image);
      this->FallBackMapper->Render(ren, vol);
      continue;
    }

    (*it)->Render(ren, vol);
  }
}

//------------------------------------------------------------------------------
void vtkMultiBlockVolumeMapper::SortMappers(vtkRenderer* ren, vtkMatrix4x4* volumeMat)
{
  vtkBlockSortHelper::BackToFront<vtkVolumeMapper> sortMappers(ren, volumeMat);
  std::sort(this->Mappers.begin(), this->Mappers.end(), sortMappers);
}

//------------------------------------------------------------------------------
double* vtkMultiBlockVolumeMapper::GetBounds()
{
  if (!this->GetDataObjectTreeInput())
  {
    return this->Superclass::GetBounds();
  }
  else
  {
    this->Update();
    this->ComputeBounds();
    return this->Bounds;
  }
}

//------------------------------------------------------------------------------
void vtkMultiBlockVolumeMapper::ComputeBounds()
{
  auto input = this->GetDataObjectTreeInput();
  assert(input != nullptr);
  if (input->GetMTime() == this->BoundsComputeTime)
  {
    // don't need to recompute bounds.
    return;
  }

  // Loop over the hierarchy of data objects to compute bounds
  vtkBoundingBox bbox;
  vtkCompositeDataIterator* iter = input->NewIterator();
  for (iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    if (vtkImageData* img = vtkImageData::SafeDownCast(iter->GetCurrentDataObject()))
      if (img)
      {
        double bds[6];
        img->GetBounds(bds);
        bbox.AddBounds(bds);
      }
  }
  iter->Delete();

  vtkMath::UninitializeBounds(this->Bounds);
  if (bbox.IsValid())
  {
    bbox.GetBounds(this->Bounds);
  }

  this->BoundsComputeTime = input->GetMTime();
}

//------------------------------------------------------------------------------
vtkDataObjectTree* vtkMultiBlockVolumeMapper::GetDataObjectTreeInput()
{
  if (this->GetNumberOfInputConnections(0) < 1)
  {
    return nullptr;
  }
  return vtkDataObjectTree::SafeDownCast(this->GetInputDataObject(0, 0));
}

//------------------------------------------------------------------------------
void vtkMultiBlockVolumeMapper::LoadDataSet(vtkRenderer* ren, vtkVolume* vol)
{
  this->ClearMappers();

  auto input = this->GetDataObjectInput();
  if (auto inputTree = vtkDataObjectTree::SafeDownCast(input))
  {
    this->CreateMappers(inputTree, ren, vol);
  }
  else if (auto inputImage = vtkImageData::SafeDownCast(input))
  {
    vtkSmartVolumeMapper* mapper = this->CreateMapper();
    mapper->SetInputData(inputImage);
    this->Mappers.push_back(mapper);
  }
  else
  {
    vtkErrorMacro(
      "Cannot handle input of type '" << (input ? input->GetClassName() : "(nullptr)") << "'.");
  }
}

//------------------------------------------------------------------------------
void vtkMultiBlockVolumeMapper::CreateMappers(
  vtkDataObjectTree* input, vtkRenderer* ren, vtkVolume* vol)
{
  // Hierarchical case
  vtkCompositeDataIterator* it = input->NewIterator();
  it->GoToFirstItem();

  bool warnedOnce = false;
  bool allBlocksLoaded = true;
  while (!it->IsDoneWithTraversal())
  {
    vtkImageData* currentIm = vtkImageData::SafeDownCast(it->GetCurrentDataObject());
    if (!warnedOnce && !currentIm)
    {
      vtkErrorMacro("At least one block in the data object is not of type"
                    " vtkImageData.  These blocks will be ignored.");
      warnedOnce = true;
      it->GoToNextItem();
      continue;
    }

    vtkSmartVolumeMapper* mapper = this->CreateMapper();
    this->Mappers.push_back(mapper);

    vtkImageData* im = vtkImageData::New();
    im->ShallowCopy(currentIm);
    mapper->SetInputData(im);

    // Try allocating GPU memory only while succeeding
    if (allBlocksLoaded)
    {
      vtkOpenGLGPUVolumeRayCastMapper* glMapper =
        vtkOpenGLGPUVolumeRayCastMapper::SafeDownCast(mapper->GetGPUMapper());

      if (glMapper)
      {
        vtkImageData* imageInternal = vtkImageData::New();
        imageInternal->ShallowCopy(currentIm);

        glMapper->SetInputData(imageInternal);
        glMapper->SelectScalarArray(this->ArrayName);
        glMapper->SelectScalarArray(this->ArrayId);
        glMapper->SetScalarMode(this->ScalarMode);
        glMapper->SetArrayAccessMode(this->ArrayAccessMode);

        allBlocksLoaded &= glMapper->PreLoadData(ren, vol);
        imageInternal->Delete();
      }
    }
    im->Delete();
    it->GoToNextItem();
  }
  it->Delete();

  // If loading all of the blocks failed, fall back to using a single mapper.
  // Use a separate instance in order to keep using the Mappers vector for
  // sorting.
  if (!allBlocksLoaded)
  {
    vtkRenderWindow* win = ren->GetRenderWindow();
    this->ReleaseGraphicsResources(win);

    this->FallBackMapper = this->CreateMapper();
  }
}

//------------------------------------------------------------------------------
vtkSmartVolumeMapper* vtkMultiBlockVolumeMapper::CreateMapper()
{
  vtkSmartVolumeMapper* mapper = vtkSmartVolumeMapper::New();

  mapper->SetRequestedRenderMode(this->RequestedRenderMode);
  mapper->SelectScalarArray(this->ArrayName);
  mapper->SelectScalarArray(this->ArrayId);
  mapper->SetScalarMode(this->ScalarMode);
  mapper->SetArrayAccessMode(this->ArrayAccessMode);
  mapper->SetVectorMode(this->VectorMode);
  mapper->SetVectorComponent(this->VectorComponent);
  mapper->SetBlendMode(this->GetBlendMode());
  mapper->SetCropping(this->GetCropping());
  mapper->SetCroppingRegionFlags(this->GetCroppingRegionFlags());
  mapper->SetCroppingRegionPlanes(this->GetCroppingRegionPlanes());

  vtkOpenGLGPUVolumeRayCastMapper* glMapper =
    vtkOpenGLGPUVolumeRayCastMapper::SafeDownCast(mapper->GetGPUMapper());

  if (glMapper != nullptr)
  {
    glMapper->UseJitteringOn();
  }
  return mapper;
}

//------------------------------------------------------------------------------
void vtkMultiBlockVolumeMapper::ReleaseGraphicsResources(vtkWindow* window)
{
  MapperVec::const_iterator end = this->Mappers.end();
  for (MapperVec::const_iterator it = this->Mappers.begin(); it != end; ++it)
  {
    (*it)->ReleaseGraphicsResources(window);
  }

  if (this->FallBackMapper)
  {
    this->FallBackMapper->ReleaseGraphicsResources(window);
  }
}

//------------------------------------------------------------------------------
int vtkMultiBlockVolumeMapper::FillInputPortInformation(int port, vtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObjectTree");
  return 1;
}

//------------------------------------------------------------------------------
void vtkMultiBlockVolumeMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);

  os << "Number Of Mappers: " << this->Mappers.size() << "\n";
  os << "BlockLoadingTime: " << this->BlockLoadingTime << "\n";
  os << "BoundsComputeTime: " << this->BoundsComputeTime << "\n";
  os << "VectorMode: " << this->VectorMode << "\n";
  os << "VectorComponent: " << this->VectorComponent << "\n";
}

//------------------------------------------------------------------------------
void vtkMultiBlockVolumeMapper::ClearMappers()
{
  MapperVec::const_iterator end = this->Mappers.end();
  for (MapperVec::const_iterator it = this->Mappers.begin(); it != end; ++it)
  {
    (*it)->Delete();
  }
  this->Mappers.clear();

  if (this->FallBackMapper)
  {
    this->FallBackMapper->Delete();
    this->FallBackMapper = nullptr;
  }
}

//------------------------------------------------------------------------------
void vtkMultiBlockVolumeMapper::SelectScalarArray(int arrayNum)
{
  MapperVec::const_iterator end = this->Mappers.end();
  for (MapperVec::const_iterator it = this->Mappers.begin(); it != end; ++it)
  {
    (*it)->SelectScalarArray(arrayNum);
  }
  Superclass::SelectScalarArray(arrayNum);
}

//------------------------------------------------------------------------------
void vtkMultiBlockVolumeMapper::SelectScalarArray(char const* arrayName)
{
  MapperVec::const_iterator end = this->Mappers.end();
  for (MapperVec::const_iterator it = this->Mappers.begin(); it != end; ++it)
  {
    (*it)->SelectScalarArray(arrayName);
  }
  Superclass::SelectScalarArray(arrayName);
}

//------------------------------------------------------------------------------
void vtkMultiBlockVolumeMapper::SetScalarMode(int scalarMode)
{
  MapperVec::const_iterator end = this->Mappers.end();
  for (MapperVec::const_iterator it = this->Mappers.begin(); it != end; ++it)
  {
    (*it)->SetScalarMode(scalarMode);
  }
  Superclass::SetScalarMode(scalarMode);
}

//------------------------------------------------------------------------------
void vtkMultiBlockVolumeMapper::SetArrayAccessMode(int accessMode)
{
  MapperVec::const_iterator end = this->Mappers.end();
  for (MapperVec::const_iterator it = this->Mappers.begin(); it != end; ++it)
  {
    (*it)->SetArrayAccessMode(accessMode);
  }
  Superclass::SetArrayAccessMode(accessMode);
}

//----------------------------------------------------------------------------
void vtkMultiBlockVolumeMapper::SetBlendMode(int mode)
{
  MapperVec::const_iterator end = this->Mappers.end();
  for (MapperVec::const_iterator it = this->Mappers.begin(); it != end; ++it)
  {
    (*it)->SetBlendMode(mode);
  }
  Superclass::SetBlendMode(mode);
}

//----------------------------------------------------------------------------
void vtkMultiBlockVolumeMapper::SetCropping(vtkTypeBool mode)
{
  MapperVec::const_iterator end = this->Mappers.end();
  for (MapperVec::const_iterator it = this->Mappers.begin(); it != end; ++it)
  {
    (*it)->SetCropping(mode);
  }
  Superclass::SetCropping(mode);
}

//----------------------------------------------------------------------------
void vtkMultiBlockVolumeMapper::SetCroppingRegionFlags(int mode)
{
  MapperVec::const_iterator end = this->Mappers.end();
  for (MapperVec::const_iterator it = this->Mappers.begin(); it != end; ++it)
  {
    (*it)->SetCroppingRegionFlags(mode);
  }
  Superclass::SetCroppingRegionFlags(mode);
}

//----------------------------------------------------------------------------
void vtkMultiBlockVolumeMapper::SetCroppingRegionPlanes(const double* planes)
{
  MapperVec::const_iterator end = this->Mappers.end();
  for (MapperVec::const_iterator it = this->Mappers.begin(); it != end; ++it)
  {
    (*it)->SetCroppingRegionPlanes(
      planes[0], planes[1], planes[2], planes[3], planes[4], planes[5]);
  }
  Superclass::SetCroppingRegionPlanes(planes);
}

//----------------------------------------------------------------------------
void vtkMultiBlockVolumeMapper::SetCroppingRegionPlanes(
  double arg1, double arg2, double arg3, double arg4, double arg5, double arg6)
{
  MapperVec::const_iterator end = this->Mappers.end();
  for (MapperVec::const_iterator it = this->Mappers.begin(); it != end; ++it)
  {
    (*it)->SetCroppingRegionPlanes(arg1, arg2, arg3, arg4, arg5, arg6);
  }
  Superclass::SetCroppingRegionPlanes(arg1, arg2, arg3, arg4, arg5, arg6);
}

//----------------------------------------------------------------------------
void vtkMultiBlockVolumeMapper::SetVectorMode(int mode)
{
  if (this->VectorMode != mode)
  {
    MapperVec::const_iterator end = this->Mappers.end();
    for (MapperVec::const_iterator it = this->Mappers.begin(); it != end; ++it)
    {
      (*it)->SetVectorMode(mode);
    }
    this->VectorMode = mode;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkMultiBlockVolumeMapper::SetVectorComponent(int component)
{
  if (this->VectorComponent != component)
  {
    MapperVec::const_iterator end = this->Mappers.end();
    for (MapperVec::const_iterator it = this->Mappers.begin(); it != end; ++it)
    {
      (*it)->SetVectorComponent(component);
    }
    this->VectorComponent = component;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkMultiBlockVolumeMapper::SetRequestedRenderMode(int mode)
{
  if (this->RequestedRenderMode != mode)
  {
    for (auto& mapper : this->Mappers)
    {
      mapper->SetRequestedRenderMode(mode);
    }
    this->RequestedRenderMode = mode;
    this->Modified();
  }
}
