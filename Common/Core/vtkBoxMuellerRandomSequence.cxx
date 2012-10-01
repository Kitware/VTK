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
  return this->Value;
}

// ----------------------------------------------------------------------------
void vtkBoxMuellerRandomSequence::Next()
{
  this->UniformSequence->Next();
  double x=this->UniformSequence->GetValue();
  // Make sure x is in (0,1]
  while(x==0.0)
    {
    this->UniformSequence->Next();
    x=this->UniformSequence->GetValue();
    }

  this->UniformSequence->Next();
  double y=this->UniformSequence->GetValue();

  // Make sure y is in (0,1]
  while(y==0.0)
    {
    this->UniformSequence->Next();
    y=this->UniformSequence->GetValue();
    }

  this->Value=sqrt(-2.0*log(x))*cos(2.0 * vtkMath::Pi()*y);
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
