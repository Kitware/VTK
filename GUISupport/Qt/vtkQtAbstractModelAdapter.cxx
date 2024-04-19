// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkQtAbstractModelAdapter.h"
#include "vtkObject.h" // For vtkGenericWarningMacro

VTK_ABI_NAMESPACE_BEGIN
int vtkQtAbstractModelAdapter::ModelColumnToFieldDataColumn(int col) const
{
  int result = -1;
  switch (this->ViewType)
  {
    case FULL_VIEW:
      result = col;
      break;
    case DATA_VIEW:
      result = this->DataStartColumn + col;
      break;
    default:
      vtkGenericWarningMacro("vtkQtAbstractModelAdapter: Bad view type.");
      break;
  };
  return result;
}
VTK_ABI_NAMESPACE_END
