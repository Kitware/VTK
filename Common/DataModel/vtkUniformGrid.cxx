// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// VTK_DEPRECATED_IN_9_6_0()
#define VTK_DEPRECATION_LEVEL 0

#include "vtkUniformGrid.h"

#include "vtkAMRBox.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkUnsignedCharArray.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkUniformGrid);

//------------------------------------------------------------------------------
vtkUniformGrid::vtkUniformGrid() = default;

//------------------------------------------------------------------------------
vtkUniformGrid::~vtkUniformGrid() = default;

//------------------------------------------------------------------------------
vtkImageData* vtkUniformGrid::NewImageDataCopy()
{
  vtkImageData* copy = vtkImageData::New();

  copy->ShallowCopy(this);

  double origin[3];
  double spacing[3];
  this->GetOrigin(origin);
  this->GetSpacing(spacing);
  // First set the extent of the copy to empty so that
  // the next call computes the DataDescription for us
  copy->SetExtent(0, -1, 0, -1, 0, -1);
  copy->SetExtent(this->GetExtent());
  copy->SetOrigin(origin);
  copy->SetSpacing(spacing);

  return copy;
}

//------------------------------------------------------------------------------
vtkUniformGrid* vtkUniformGrid::GetData(vtkInformation* info)
{
  return info ? vtkUniformGrid::SafeDownCast(info->Get(DATA_OBJECT())) : nullptr;
}

//------------------------------------------------------------------------------
vtkUniformGrid* vtkUniformGrid::GetData(vtkInformationVector* v, int i)
{
  return vtkUniformGrid::GetData(v->GetInformationObject(i));
}
VTK_ABI_NAMESPACE_END
