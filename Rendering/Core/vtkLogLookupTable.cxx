// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkLogLookupTable.h"
#include "vtkObjectFactory.h"

#include <cmath>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkLogLookupTable);

// Construct with (minimum,maximum) range 1 to 10 (based on
// logarithmic values).
vtkLogLookupTable::vtkLogLookupTable(int sze, int ext)
  : vtkLookupTable(sze, ext)
{
  this->Scale = VTK_SCALE_LOG10;

  this->TableRange[0] = 1;
  this->TableRange[1] = 10;
}

void vtkLogLookupTable::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
