/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiBlockUnstructuredGridVolumeMapper.cxx

  copyright (c) ken martin, will schroeder, bill lorensen
  all rights reserved.
  see copyright.txt or http://www.kitware.com/copyright.htm for details.

  this software is distributed without any warranty; without even
  the implied warranty of merchantability or fitness for a particular
  purpose.  see the above copyright notice for more information.

=========================================================================*/
#include <algorithm>

// uncomment the following line to add a lot of debugging
// code to the sorting process
// #define MB_DEBUG

#include "vtkBlockSortHelper.h"
#include "vtkBoundingBox.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiBlockUnstructuredGridVolumeMapper.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLProjectedTetrahedraMapper.h"
#include "vtkProjectedTetrahedraMapper.h"
#include "vtkRenderWindow.h"
#include "vtkUnstructuredGrid.h"

#include "vtkProperty.h"

namespace vtkBlockSortHelper
{
template <>
inline void GetBounds(vtkProjectedTetrahedraMapper* first, double bds[6])
{
  first->GetInput()->GetBounds(bds);
}
}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkMultiBlockUnstructuredGridVolumeMapper);

//------------------------------------------------------------------------------
vtkMultiBlockUnstructuredGridVolumeMapper::vtkMultiBlockUnstructuredGridVolumeMapper()
  : UseFloatingPointFrameBuffer(false)
  , BlockLoadingTime(0)
  , BoundsComputeTime(0)
{
#ifdef MB_DEBUG
  this->DebugWin = vtkRenderWindow::New();
  this->DebugRen = vtkRenderer::New();
  this->DebugWin->AddRenderer(this->DebugRen);
#else
  this->DebugWin = nullptr;
  this->DebugRen = nullptr;
#endif
}

//------------------------------------------------------------------------------
vtkMultiBlockUnstructuredGridVolumeMapper::~vtkMultiBlockUnstructuredGridVolumeMapper()
{
  this->ClearMappers();
  if (this->DebugRen)
  {
    this->DebugRen->Delete();
  }
  if (this->DebugWin)
  {
    this->DebugWin->Delete();
  }
}

//------------------------------------------------------------------------------
void vtkMultiBlockUnstructuredGridVolumeMapper::Render(vtkRenderer* ren, vtkVolume* vol)
{
  vtkDataObject* dataObj = this->GetDataObjectInput();
  if (dataObj->GetMTime() != this->BlockLoadingTime)
  {
    vtkDebugMacro("Reloading data blocks!");
    this->LoadDataSet();
    this->BlockLoadingTime = dataObj->GetMTime();
  }

  vol->GetModelToWorldMatrix(this->TempMatrix4x4);
  this->SortMappers(ren, this->TempMatrix4x4);

  for (auto& mapper : this->Mappers)
  {
    int unused = 0;
    auto input = mapper->GetInput();
    auto scalars = vtkAbstractMapper::GetScalars(
      input, this->ScalarMode, this->ArrayAccessMode, this->ArrayId, this->ArrayName, unused);
    if (scalars != nullptr)
    {
      mapper->Render(ren, vol);
    }
  }
}

//------------------------------------------------------------------------------
void vtkMultiBlockUnstructuredGridVolumeMapper::SortMappers(
  vtkRenderer* ren, vtkMatrix4x4* volumeMat)
{
  vtkBlockSortHelper::BackToFront<vtkUnstructuredGridVolumeMapper> sortMappers(ren, volumeMat);
  vtkBlockSortHelper::Sort(this->Mappers.begin(), this->Mappers.end(), sortMappers);
}

//------------------------------------------------------------------------------
double* vtkMultiBlockUnstructuredGridVolumeMapper::GetBounds()
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
void vtkMultiBlockUnstructuredGridVolumeMapper::ComputeBounds()
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
    if (vtkUnstructuredGrid* grid = vtkUnstructuredGrid::SafeDownCast(iter->GetCurrentDataObject()))
      if (grid)
      {
        double bds[6];
        grid->GetBounds(bds);
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
vtkDataObjectTree* vtkMultiBlockUnstructuredGridVolumeMapper::GetDataObjectTreeInput()
{
  if (this->GetNumberOfInputConnections(0) < 1)
  {
    return nullptr;
  }
  return vtkDataObjectTree::SafeDownCast(this->GetInputDataObject(0, 0));
}

//------------------------------------------------------------------------------
void vtkMultiBlockUnstructuredGridVolumeMapper::LoadDataSet()
{
  this->ClearMappers();

  auto input = this->GetDataObjectInput();
  if (auto inputTree = vtkDataObjectTree::SafeDownCast(input))
  {
    this->CreateMappers(inputTree);
  }
  else if (auto inputGrid = vtkUnstructuredGrid::SafeDownCast(input))
  {
    vtkProjectedTetrahedraMapper* mapper = this->CreateMapper();
    mapper->SetInputData(inputGrid);
    this->Mappers.push_back(mapper);
  }
  else
  {
    vtkErrorMacro(
      "Cannot handle input of type '" << (input ? input->GetClassName() : "(nullptr)") << "'.");
  }
}

//------------------------------------------------------------------------------
void vtkMultiBlockUnstructuredGridVolumeMapper::CreateMappers(vtkDataObjectTree* input)
{
  // Hierarchical case
  vtkCompositeDataIterator* it = input->NewIterator();
  it->GoToFirstItem();

  bool warnedOnce = false;
  while (!it->IsDoneWithTraversal())
  {
    vtkUnstructuredGrid* currentGrid =
      vtkUnstructuredGrid::SafeDownCast(it->GetCurrentDataObject());
    if (!warnedOnce && !currentGrid)
    {
      vtkErrorMacro("At least one block in the data object is not of type"
                    " vtkUnstructuredGrid. These blocks will be ignored.");
      warnedOnce = true;
      it->GoToNextItem();
      continue;
    }

    vtkProjectedTetrahedraMapper* mapper = this->CreateMapper();
    this->Mappers.push_back(mapper);

    mapper->SetInputData(currentGrid);

    it->GoToNextItem();
  }
  it->Delete();
}

//------------------------------------------------------------------------------
vtkProjectedTetrahedraMapper* vtkMultiBlockUnstructuredGridVolumeMapper::CreateMapper()
{
  vtkProjectedTetrahedraMapper* mapper = vtkProjectedTetrahedraMapper::New();

  mapper->SelectScalarArray(this->ArrayName);
  mapper->SelectScalarArray(this->ArrayId);
  mapper->SetScalarMode(this->ScalarMode);
  mapper->SetArrayAccessMode(this->ArrayAccessMode);
  mapper->SetBlendMode(this->GetBlendMode());
  if (auto glmapper = vtkOpenGLProjectedTetrahedraMapper::SafeDownCast(mapper))
  {
    glmapper->SetUseFloatingPointFrameBuffer(this->UseFloatingPointFrameBuffer);
  }

  return mapper;
}

//------------------------------------------------------------------------------
void vtkMultiBlockUnstructuredGridVolumeMapper::ReleaseGraphicsResources(vtkWindow* window)
{
  for (auto& mapper : this->Mappers)
  {
    mapper->ReleaseGraphicsResources(window);
  }
}

//------------------------------------------------------------------------------
int vtkMultiBlockUnstructuredGridVolumeMapper::FillInputPortInformation(
  int port, vtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObjectTree");
  return 1;
}

//------------------------------------------------------------------------------
void vtkMultiBlockUnstructuredGridVolumeMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << "Number Of Mappers: " << this->Mappers.size() << "\n";
  os << "BlockLoadingTime: " << this->BlockLoadingTime << "\n";
  os << "BoundsComputeTime: " << this->BoundsComputeTime << "\n";
}

//------------------------------------------------------------------------------
void vtkMultiBlockUnstructuredGridVolumeMapper::ClearMappers()
{
  for (auto& mapper : this->Mappers)
  {
    mapper->Delete();
  }
  this->Mappers.clear();
}

//------------------------------------------------------------------------------
void vtkMultiBlockUnstructuredGridVolumeMapper::SelectScalarArray(int arrayNum)
{
  for (auto& mapper : this->Mappers)
  {
    mapper->SelectScalarArray(arrayNum);
  }
  this->Superclass::SelectScalarArray(arrayNum);
}

//------------------------------------------------------------------------------
void vtkMultiBlockUnstructuredGridVolumeMapper::SelectScalarArray(char const* arrayName)
{
  for (auto& mapper : this->Mappers)
  {
    mapper->SelectScalarArray(arrayName);
  }
  this->Superclass::SelectScalarArray(arrayName);
}

//------------------------------------------------------------------------------
void vtkMultiBlockUnstructuredGridVolumeMapper::SetScalarMode(int scalarMode)
{
  for (auto& mapper : this->Mappers)
  {
    mapper->SetScalarMode(scalarMode);
  }
  this->Superclass::SetScalarMode(scalarMode);
}

//------------------------------------------------------------------------------
void vtkMultiBlockUnstructuredGridVolumeMapper::SetArrayAccessMode(int accessMode)
{
  for (auto& mapper : this->Mappers)
  {
    mapper->SetArrayAccessMode(accessMode);
  }
  this->Superclass::SetArrayAccessMode(accessMode);
}

//------------------------------------------------------------------------------
void vtkMultiBlockUnstructuredGridVolumeMapper::SetUseFloatingPointFrameBuffer(bool use)
{
  for (auto& mapper : this->Mappers)
  {
    vtkOpenGLProjectedTetrahedraMapper* glmapper =
      vtkOpenGLProjectedTetrahedraMapper::SafeDownCast(mapper);
    if (glmapper)
    {
      glmapper->SetUseFloatingPointFrameBuffer(use);
    }
  }
  this->UseFloatingPointFrameBuffer = use;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkMultiBlockUnstructuredGridVolumeMapper::SetBlendMode(int mode)
{
  for (auto& mapper : this->Mappers)
  {
    mapper->SetBlendMode(mode);
  }
  this->Superclass::SetBlendMode(mode);
}
