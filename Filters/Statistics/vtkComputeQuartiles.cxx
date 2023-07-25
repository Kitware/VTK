// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkComputeQuartiles.h"

#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkComputeQuartiles);
//------------------------------------------------------------------------------
vtkComputeQuartiles::vtkComputeQuartiles()
{
  //
  // Note:
  // Implementation of this class has moved to new super class vtkComputeNtiles.
  // This class is kept for backwards compatibility.
  //

  this->SetNumberOfIntervals(4);
}

//------------------------------------------------------------------------------
void vtkComputeQuartiles::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
