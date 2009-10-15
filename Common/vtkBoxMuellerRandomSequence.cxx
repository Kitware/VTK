/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBoxMuellerRandomSequence.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "vtkBoxMuellerRandomSequence.h"

#include "vtkMinimalStandardRandomSequence.h"
#include <cassert>
#include "vtkMath.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkBoxMuellerRandomSequence, "1.1");
vtkStandardNewMacro(vtkBoxMuellerRandomSequence);


// ----------------------------------------------------------------------------
vtkBoxMuellerRandomSequence::vtkBoxMuellerRandomSequence()
{
  this->UniformSequence=vtkMinimalStandardRandomSequence::New();
  this->Value=0;
}

// ----------------------------------------------------------------------------
vtkBoxMuellerRandomSequence::~vtkBoxMuellerRandomSequence()
{
  this->UniformSequence->Delete();
}

// ----------------------------------------------------------------------------
double vtkBoxMuellerRandomSequence::GetValue()
{
  assert("post: unit_range" && this->Value>=0.0 && this->Value<=1.0);
  return this->Value;
}

// ----------------------------------------------------------------------------
void vtkBoxMuellerRandomSequence::Next()
{
  this->UniformSequence->Next();
  double x=this->UniformSequence->GetValue();
  this->UniformSequence->Next();
  double y=this->UniformSequence->GetValue();
  this->Value=sqrt(-2.0*log(x))*cos(vtkMath::DoubleTwoPi()*y);
}
  
// ----------------------------------------------------------------------------
vtkRandomSequence *vtkBoxMuellerRandomSequence::GetUniformSequence()
{
  assert("post: result_exists" && this->UniformSequence!=0);
  return this->UniformSequence;
}

// ----------------------------------------------------------------------------
// Description:
// Set the uniformly distributed sequence of random numbers.
// Default is a .
void vtkBoxMuellerRandomSequence::SetUniformSequence(
  vtkRandomSequence *uniformSequence)
{
  assert("pre: uniformSequence_exists" && uniformSequence!=0);
  
  if(this->UniformSequence!=uniformSequence)
    {
    this->UniformSequence->Delete();
    this->UniformSequence=uniformSequence;
    this->UniformSequence->Register(this);
    }
  
  assert("post: assigned" && uniformSequence==this->GetUniformSequence());
}

// ----------------------------------------------------------------------------
void vtkBoxMuellerRandomSequence::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
