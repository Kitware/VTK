// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGOperation.h"

#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkDGCell.h"
#include "vtkDGOperationEvaluator.h"
#include "vtkDoubleArray.h"
#include "vtkMatrix3x3.h"

VTK_ABI_NAMESPACE_BEGIN

bool vtkDGOperationBase::RangeKey::Contains(vtkTypeUInt64 cellId) const
{
  return cellId >= this->Begin && cellId < this->End;
}

bool vtkDGOperationBase::RangeKey::ContainedBy(const RangeKey& other) const
{
  return other.Begin >= this->Begin && other.End < this->Begin;
}

bool vtkDGOperationBase::RangeKey::operator<(const RangeKey& other) const
{
  return this->Begin < other.Begin;
}

VTK_ABI_NAMESPACE_END
