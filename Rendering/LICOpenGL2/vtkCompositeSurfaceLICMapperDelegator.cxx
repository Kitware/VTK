// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCompositeSurfaceLICMapperDelegator.h"
#include "vtkBatchedSurfaceLICMapper.h"
#include "vtkCompositeSurfaceLICMapper.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkCompositeSurfaceLICMapperDelegator);

//------------------------------------------------------------------------------
vtkCompositeSurfaceLICMapperDelegator::vtkCompositeSurfaceLICMapperDelegator()
{
  if (this->Delegate != nullptr)
  {
    // delete the delegate created by parent class
    this->Delegate = nullptr;
  }
  // create our own.
  this->GLDelegate = vtkBatchedSurfaceLICMapper::New();
  this->Delegate = vtk::TakeSmartPointer(this->GLDelegate);
}

//------------------------------------------------------------------------------
vtkCompositeSurfaceLICMapperDelegator::~vtkCompositeSurfaceLICMapperDelegator() = default;

//------------------------------------------------------------------------------
void vtkCompositeSurfaceLICMapperDelegator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkCompositeSurfaceLICMapperDelegator::ShallowCopy(vtkCompositePolyDataMapper* cpdm)
{
  this->Superclass::ShallowCopy(cpdm);
  this->GLDelegate->SetInputArrayToProcess(0, cpdm->GetInputArrayInformation(0));
}

VTK_ABI_NAMESPACE_END
