// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSPHQuarticKernel.h"
#include "vtkAbstractPointLocator.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkSPHQuarticKernel);

//------------------------------------------------------------------------------
vtkSPHQuarticKernel::vtkSPHQuarticKernel()
{
  this->CutoffFactor = 2.5;

  if (this->Dimension == 1)
  {
    this->Sigma = 1.0 / 24.0;
  }
  else if (this->Dimension == 2)
  {
    this->Sigma = 96.0 / (1199.0 * vtkMath::Pi());
  }
  else // if ( this->Dimension == 3 )
  {
    this->Sigma = 1.0 / (20.0 * vtkMath::Pi());
  }
}

//------------------------------------------------------------------------------
vtkSPHQuarticKernel::~vtkSPHQuarticKernel() = default;

//------------------------------------------------------------------------------
// At this point, the spatial step, the dimension of the kernel, and the cutoff
// factor should be known.
void vtkSPHQuarticKernel::Initialize(
  vtkAbstractPointLocator* loc, vtkDataSet* ds, vtkPointData* attr)
{
  if (this->Dimension == 1)
  {
    this->Sigma = 1.0 / 24.0;
  }
  else if (this->Dimension == 2)
  {
    this->Sigma = 96.0 / (1199.0 * vtkMath::Pi());
  }
  else // if ( this->Dimension == 3 )
  {
    this->Sigma = 1.0 / (20.0 * vtkMath::Pi());
  }

  // Sigma must be set before vtkSPHKernel::Initialize is invoked
  this->Superclass::Initialize(loc, ds, attr);
}

//------------------------------------------------------------------------------
void vtkSPHQuarticKernel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
