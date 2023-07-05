// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSPHCubicKernel.h"
#include "vtkAbstractPointLocator.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkSPHCubicKernel);

//------------------------------------------------------------------------------
vtkSPHCubicKernel::vtkSPHCubicKernel()
{
  this->CutoffFactor = 2.0;

  if (this->Dimension == 1)
  {
    this->Sigma = 2.0 / 3.0;
  }
  else if (this->Dimension == 2)
  {
    this->Sigma = 10.0 / (7.0 * vtkMath::Pi());
  }
  else // if ( this->Dimension == 3 )
  {
    this->Sigma = 1.0 / vtkMath::Pi();
  }
}

//------------------------------------------------------------------------------
vtkSPHCubicKernel::~vtkSPHCubicKernel() = default;

//------------------------------------------------------------------------------
// At this point, the spatial step, the dimension of the kernel, and the cutoff
// factor should be known.
void vtkSPHCubicKernel::Initialize(vtkAbstractPointLocator* loc, vtkDataSet* ds, vtkPointData* attr)
{
  if (this->Dimension == 1)
  {
    this->Sigma = 2.0 / 3.0;
  }
  else if (this->Dimension == 2)
  {
    this->Sigma = 10.0 / (7.0 * vtkMath::Pi());
  }
  else // if ( this->Dimension == 3 )
  {
    this->Sigma = 1.0 / vtkMath::Pi();
  }

  // Sigma must be set before vtkSPHKernel::Initialize is invoked
  this->Superclass::Initialize(loc, ds, attr);
}

//------------------------------------------------------------------------------
void vtkSPHCubicKernel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
