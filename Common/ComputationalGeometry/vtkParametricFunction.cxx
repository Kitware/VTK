/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParametricFunction.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkParametricFunction.h"


//----------------------------------------------------------------------------
vtkParametricFunction::vtkParametricFunction() :
    MinimumU(0.0)
  , MaximumU(1.0)
  , MinimumV(0.0)
  , MaximumV(1.0)
  , MinimumW(0.0)
  , MaximumW(1.0)
  , JoinU(0)
  , JoinV(0)
  , JoinW(0)
  , TwistU(0)
  , TwistV(0)
  , TwistW(0)
  , ClockwiseOrdering(1)
  , DerivativesAvailable(1)
{
}


//----------------------------------------------------------------------------
vtkParametricFunction::~vtkParametricFunction()
{
}


//----------------------------------------------------------------------------
void vtkParametricFunction::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Minimum U: " << this->MinimumU << "\n";
  os << indent << "Maximum U: " << this->MaximumU << "\n";

  os << indent << "Minimum V: " << this->MinimumV << "\n";
  os << indent << "Maximum V: " << this->MaximumV << "\n";

  os << indent << "Minimum W: " << this->MinimumW << "\n";
  os << indent << "Maximum W: " << this->MaximumW << "\n";

  os << indent << "JoinU: " << this->JoinU << "\n";
  os << indent << "JoinV: " << this->JoinV << "\n";
  os << indent << "JoinW: " << this->JoinV << "\n";

  os << indent << "TwistU: " << this->TwistU << "\n";
  os << indent << "TwistV: " << this->TwistV << "\n";
  os << indent << "TwistW: " << this->TwistV << "\n";

  os << indent << "ClockwiseOrdering: " << this->ClockwiseOrdering << "\n";
  os << indent << "Derivatives Available: " << this->DerivativesAvailable << "\n";
}
