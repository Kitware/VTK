/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitHalo.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImplicitHalo.h"

#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include <assert.h>

vtkStandardNewMacro(vtkImplicitHalo);

// ----------------------------------------------------------------------------
vtkImplicitHalo::vtkImplicitHalo()
{
  this->Center[0]=0.0;
  this->Center[1]=0.0;
  this->Center[2]=0.0;
  this->Radius=1.0;
  this->FadeOut=0.01;
}

// ----------------------------------------------------------------------------
vtkImplicitHalo::~vtkImplicitHalo()
{
}

// ----------------------------------------------------------------------------
double vtkImplicitHalo::EvaluateFunction(double x[3])
{ 
  double result;
  double distance=sqrt(vtkMath::Distance2BetweenPoints(this->Center,x));
  if(distance>this->Radius)
    {
    result=0.0;
    }
  else
    {
    double smallRadius=this->Radius*(1.0-this->FadeOut);
    if(distance<=smallRadius)
      {
      result=1.0;
      }
    else
      {
      // here this->FadeOut and this->Radius cannot be 0.0 as they are handled
      // by the cases seen above.
      result=(1.0-distance/this->Radius)/this->FadeOut;
      }
    }
  return result;
}

// ----------------------------------------------------------------------------
void vtkImplicitHalo::EvaluateGradient(double vtkNotUsed(x)[3],
                                       double vtkNotUsed(g)[3])
{
  assert("check: TODO" && false);
}

// ----------------------------------------------------------------------------
void vtkImplicitHalo::PrintSelf(ostream &os,
                                vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Center: " << this->Center[0] << "," << this->Center[1]
     << "," << this->Center[2] << endl;
  os << indent << "Radius: " << this->Radius << endl;
  os << indent << "FadeOut: " << this->FadeOut << endl;
}
