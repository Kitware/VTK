// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUCompositePolyDataMapperDelegator.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkObjectFactory.h"
#include "vtkWebGPUBatchedPolyDataMapper.h"

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkWebGPUCompositePolyDataMapperDelegator);

//------------------------------------------------------------------------------
vtkWebGPUCompositePolyDataMapperDelegator::vtkWebGPUCompositePolyDataMapperDelegator()
{
  this->WebGPUDelegate = vtkWebGPUBatchedPolyDataMapper::New();
  this->Delegate = vtk::TakeSmartPointer(this->WebGPUDelegate);
}

//------------------------------------------------------------------------------
vtkWebGPUCompositePolyDataMapperDelegator::~vtkWebGPUCompositePolyDataMapperDelegator() = default;

//------------------------------------------------------------------------------
void vtkWebGPUCompositePolyDataMapperDelegator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkWebGPUCompositePolyDataMapperDelegator::ShallowCopy(vtkCompositePolyDataMapper* cpdm)
{
  this->Superclass::ShallowCopy(cpdm);
  // this->WebGPUDelegate->SetCellIdArrayName(cpdm->GetCellIdArrayName());
  // this->WebGPUDelegate->SetCompositeIdArrayName(cpdm->GetCompositeIdArrayName());
  // this->WebGPUDelegate->SetPointIdArrayName(cpdm->GetPointIdArrayName());
  // this->WebGPUDelegate->SetProcessIdArrayName(cpdm->GetProcessIdArrayName());
}

//------------------------------------------------------------------------------
void vtkWebGPUCompositePolyDataMapperDelegator::ClearUnmarkedBatchElements()
{
  this->WebGPUDelegate->ClearUnmarkedBatchElements();
}

//------------------------------------------------------------------------------
void vtkWebGPUCompositePolyDataMapperDelegator::UnmarkBatchElements()
{
  this->WebGPUDelegate->UnmarkBatchElements();
}

//------------------------------------------------------------------------------
std::vector<vtkPolyData*> vtkWebGPUCompositePolyDataMapperDelegator::GetRenderedList() const
{
  return this->WebGPUDelegate->GetRenderedList();
}

//------------------------------------------------------------------------------
void vtkWebGPUCompositePolyDataMapperDelegator::SetParent(vtkCompositePolyDataMapper* mapper)
{
  this->WebGPUDelegate->SetParent(mapper);
}

//------------------------------------------------------------------------------
void vtkWebGPUCompositePolyDataMapperDelegator::Insert(BatchElement&& batchElement)
{
  this->WebGPUDelegate->AddBatchElement(batchElement.FlatIndex, std::move(batchElement));
}

//------------------------------------------------------------------------------
vtkCompositePolyDataMapperDelegator::BatchElement* vtkWebGPUCompositePolyDataMapperDelegator::Get(
  vtkPolyData* polydata)
{
  return this->WebGPUDelegate->GetBatchElement(polydata);
}

//------------------------------------------------------------------------------
void vtkWebGPUCompositePolyDataMapperDelegator::Clear()
{
  this->WebGPUDelegate->ClearBatchElements();
}

VTK_ABI_NAMESPACE_END
