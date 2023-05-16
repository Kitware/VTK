/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLCompositePolyDataMapperDelegator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLCompositePolyDataMapperDelegator.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLBatchedPolyDataMapper.h"

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLCompositePolyDataMapperDelegator);

//------------------------------------------------------------------------------
vtkOpenGLCompositePolyDataMapperDelegator::vtkOpenGLCompositePolyDataMapperDelegator()
{
  this->GLDelegate = vtkOpenGLBatchedPolyDataMapper::New();
  this->Delegate = vtk::TakeSmartPointer(this->GLDelegate);
}

//------------------------------------------------------------------------------
vtkOpenGLCompositePolyDataMapperDelegator::~vtkOpenGLCompositePolyDataMapperDelegator() = default;

//------------------------------------------------------------------------------
void vtkOpenGLCompositePolyDataMapperDelegator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkOpenGLCompositePolyDataMapperDelegator::ShallowCopy(vtkCompositePolyDataMapper* cpdm)
{
  this->Superclass::ShallowCopy(cpdm);
  this->GLDelegate->SetCellIdArrayName(cpdm->GetCellIdArrayName());
  this->GLDelegate->SetCompositeIdArrayName(cpdm->GetCompositeIdArrayName());
  this->GLDelegate->SetPointIdArrayName(cpdm->GetPointIdArrayName());
  this->GLDelegate->SetProcessIdArrayName(cpdm->GetProcessIdArrayName());
}

//------------------------------------------------------------------------------
void vtkOpenGLCompositePolyDataMapperDelegator::ClearUnmarkedBatchElements()
{
  this->GLDelegate->ClearUnmarkedBatchElements();
}

//------------------------------------------------------------------------------
void vtkOpenGLCompositePolyDataMapperDelegator::UnmarkBatchElements()
{
  this->GLDelegate->UnmarkBatchElements();
}

//------------------------------------------------------------------------------
std::vector<vtkPolyData*> vtkOpenGLCompositePolyDataMapperDelegator::GetRenderedList() const
{
  return this->GLDelegate->GetRenderedList();
}

//------------------------------------------------------------------------------
void vtkOpenGLCompositePolyDataMapperDelegator::SetParent(vtkCompositePolyDataMapper* mapper)
{
  this->GLDelegate->SetParent(mapper);
}

//------------------------------------------------------------------------------
void vtkOpenGLCompositePolyDataMapperDelegator::Insert(BatchElement&& batchElement)
{
  this->GLDelegate->AddBatchElement(batchElement.FlatIndex, std::move(batchElement));
}

//------------------------------------------------------------------------------
vtkCompositePolyDataMapperDelegator::BatchElement* vtkOpenGLCompositePolyDataMapperDelegator::Get(
  vtkPolyData* polydata)
{
  return this->GLDelegate->GetBatchElement(polydata);
}

//------------------------------------------------------------------------------
void vtkOpenGLCompositePolyDataMapperDelegator::Clear()
{
  this->GLDelegate->ClearBatchElements();
}

VTK_ABI_NAMESPACE_END
