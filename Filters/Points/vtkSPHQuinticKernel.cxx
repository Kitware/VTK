// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSPHQuinticKernel.h"
#include "vtkAbstractPointLocator.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkSPHQuinticKernel);

//------------------------------------------------------------------------------
vtkSPHQuinticKernel::vtkSPHQuinticKernel()
{
  this->CutoffFactor = 3.0;
}

//------------------------------------------------------------------------------
vtkSPHQuinticKernel::~vtkSPHQuinticKernel() = default;

//------------------------------------------------------------------------------
// At this point, the spatial step, the dimension of the kernel, and the cutoff
// factor should be known.
void vtkSPHQuinticKernel::Initialize(
  vtkAbstractPointLocator* loc, vtkDataSet* ds, vtkPointData* attr)
{
  if (this->Dimension == 1)
  {
    this->Sigma = 1.0 / 120.0;
  }
  else if (this->Dimension == 2)
  {
    this->Sigma = 7.0 / (478.0 * vtkMath::Pi());
  }
  else // if ( this->Dimension == 3 )
  {
    this->Sigma = 1.0 / (120.0 * vtkMath::Pi());
  }

  // Sigma must be set before vtkSPHKernel::Initialize is invoked
  this->Superclass::Initialize(loc, ds, attr);
}

//------------------------------------------------------------------------------
void vtkSPHQuinticKernel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
