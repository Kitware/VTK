/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWendlandQuinticKernel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkWendlandQuinticKernel.h"
#include "vtkAbstractPointLocator.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"

vtkStandardNewMacro(vtkWendlandQuinticKernel);

//----------------------------------------------------------------------------
vtkWendlandQuinticKernel::vtkWendlandQuinticKernel()
{
  this->CutoffFactor = 2.0;
}

//----------------------------------------------------------------------------
vtkWendlandQuinticKernel::~vtkWendlandQuinticKernel()
{
}

//----------------------------------------------------------------------------
// At this point, the spatial step, the dimension of the kernel, and the cutoff
// factor should be known.
void vtkWendlandQuinticKernel::
Initialize(vtkAbstractPointLocator *loc, vtkDataSet *ds, vtkPointData *attr)
{
  if ( this->Dimension == 1 )
  {
    vtkErrorMacro("Wendland kernel defined for dimensions >2");
  }
  else if ( this->Dimension == 2 )
  {
    this->Sigma = 7.0 / (4.0*vtkMath::Pi());
  }
  else //if ( this->Dimension == 3 )
  {
    this->Sigma = 21.0 / (16.0*vtkMath::Pi());
  }

  // Sigma must be set before vtkSPHKernel::Initialize is invoked
  this->Superclass::Initialize(loc, ds, attr);
}

//----------------------------------------------------------------------------
void vtkWendlandQuinticKernel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
