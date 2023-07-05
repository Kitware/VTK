// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2003 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
#include "vtkVisibilitySort.h"

#include "vtkCamera.h"
#include "vtkDataSet.h"
#include "vtkGarbageCollector.h"
#include "vtkIdList.h"
#include "vtkMatrix4x4.h"

//------------------------------------------------------------------------------

VTK_ABI_NAMESPACE_BEGIN
vtkCxxSetObjectMacro(vtkVisibilitySort, Camera, vtkCamera);
vtkCxxSetObjectMacro(vtkVisibilitySort, Input, vtkDataSet);

//------------------------------------------------------------------------------

vtkVisibilitySort::vtkVisibilitySort()
{
  this->ModelTransform = vtkMatrix4x4::New();
  this->ModelTransform->Identity();
  this->InverseModelTransform = vtkMatrix4x4::New();
  this->InverseModelTransform->Identity();

  this->Camera = nullptr;
  this->Input = nullptr;

  this->Direction = vtkVisibilitySort::BACK_TO_FRONT;

  this->MaxCellsReturned = VTK_INT_MAX;
}

//------------------------------------------------------------------------------

vtkVisibilitySort::~vtkVisibilitySort()
{
  this->ModelTransform->Delete();
  this->InverseModelTransform->Delete();

  this->SetCamera(nullptr);
  this->SetInput(nullptr);
}

//------------------------------------------------------------------------------

void vtkVisibilitySort::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->Input, "Input");
}

//------------------------------------------------------------------------------

void vtkVisibilitySort::SetModelTransform(vtkMatrix4x4* mat)
{
  // Less efficient than vtkMatrix4x4::DeepCopy, but only sets Modified if
  // there is a real change.
  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 4; j++)
    {
      this->ModelTransform->SetElement(i, j, mat->GetElement(i, j));
    }
  }

  if (this->ModelTransform->GetMTime() > this->InverseModelTransform->GetMTime())
  {
    this->InverseModelTransform->DeepCopy(this->ModelTransform);
    this->InverseModelTransform->Invert();
  }
}

//------------------------------------------------------------------------------

void vtkVisibilitySort::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Input: (" << this->Input << ")" << endl;
  os << indent << "Direction: ";
  switch (this->Direction)
  {
    case vtkVisibilitySort::BACK_TO_FRONT:
      os << "back to front" << endl;
      break;
    case vtkVisibilitySort::FRONT_TO_BACK:
      os << "front to back" << endl;
      break;
    default:
      os << "unknown" << endl;
      break;
  }

  os << indent << "MaxCellsReturned: " << this->MaxCellsReturned << endl;

  os << indent << "ModelTransform:" << endl;
  this->ModelTransform->PrintSelf(os, indent.GetNextIndent());
  os << indent << "InverseModelTransform:" << endl;
  this->InverseModelTransform->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Camera: (" << this->Camera << ")" << endl;
}
VTK_ABI_NAMESPACE_END
