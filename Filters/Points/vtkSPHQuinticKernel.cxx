/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSPHQuinticKernel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSPHQuinticKernel.h"
#include "vtkAbstractPointLocator.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"

vtkStandardNewMacro(vtkSPHQuinticKernel);

//----------------------------------------------------------------------------
vtkSPHQuinticKernel::vtkSPHQuinticKernel()
{
  this->CutoffFactor = 3.0;
}

//----------------------------------------------------------------------------
vtkSPHQuinticKernel::~vtkSPHQuinticKernel()
{
}

//----------------------------------------------------------------------------
// At this point, the spatial step, the dimension of the kernel, and the cutoff
// factor should be known.
void vtkSPHQuinticKernel::
Initialize(vtkAbstractPointLocator *loc, vtkDataSet *ds, vtkPointData *attr)
{
  if ( this->Dimension == 1 )
  {
    this->Sigma = 1.0 / 120.0;
  }
  else if ( this->Dimension == 2 )
  {
    this->Sigma = 7.0 / (478.0*vtkMath::Pi());
  }
  else //if ( this->Dimension == 3 )
  {
    this->Sigma = 1.0 / (120.0*vtkMath::Pi());
  }

  // Sigma must be set before vtkSPHKernel::Initialize is invoked
  this->Superclass::Initialize(loc, ds, attr);
}

//----------------------------------------------------------------------------
void vtkSPHQuinticKernel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
