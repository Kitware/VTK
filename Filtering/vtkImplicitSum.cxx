/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitSum.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
    
#include <math.h>
#include "vtkImplicitSum.h"
#include "vtkObjectFactory.h"

#ifdef vtkCxxRevisionMacro
vtkCxxRevisionMacro(vtkImplicitSum, "1.1");
#endif

//------------------------------------------------------------------------------
vtkImplicitSum* vtkImplicitSum::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImplicitSum");
  if(ret)
    {
    return (vtkImplicitSum*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImplicitSum;
}

// Constructor.
vtkImplicitSum::vtkImplicitSum()
{
  this->FunctionList = vtkImplicitFunctionCollection::New();
  this->Weights = vtkFloatArray::New();
  this->Weights->SetNumberOfComponents(1);
  this->TotalWeight = 0.0;
  this->NormalizeByWeight = 0;
}

vtkImplicitSum::~vtkImplicitSum()
{
  this->FunctionList->Delete();
  this->Weights->Delete();
}

unsigned long int vtkImplicitSum::GetMTime()
{
  unsigned long int fMtime;
  unsigned long int mtime = this->vtkImplicitFunction::GetMTime();
  vtkImplicitFunction *f;

  fMtime = this->Weights->GetMTime();
  if ( fMtime > mtime )
    {
    mtime = fMtime;
    }

  for (this->FunctionList->InitTraversal(); 
       (f=this->FunctionList->GetNextItem()); )
    {
    fMtime = f->GetMTime();
    if ( fMtime > mtime )
      {
      mtime = fMtime;
      }
    }
  return mtime;
}

// Add another implicit function to the list of functions.
void vtkImplicitSum::AddFunction(vtkImplicitFunction *f, float scale)
{
  this->Modified();
  this->FunctionList->AddItem(f);
  this->Weights->InsertNextValue(scale);
  this->CalculateTotalWeight();
}

void vtkImplicitSum::SetFunctionWeight(vtkImplicitFunction *f, float scale)
{
  int loc = this->FunctionList->IsItemPresent(f);
  if (! loc)
    {
    vtkWarningMacro("Function not found in function list");
    return;
    }
  loc--; // IsItemPresent returns index+1. 

  if ( this->Weights->GetValue(loc) != scale )
    {
    this->Modified();
    this->Weights->SetValue(loc, scale);
    this->CalculateTotalWeight();
    }
}

void vtkImplicitSum::RemoveAllFunctions()
{
  this->Modified();
  this->FunctionList->RemoveAllItems();
  this->Weights->Initialize();
  this->TotalWeight = 0.0;
}

void vtkImplicitSum::CalculateTotalWeight(void) 
{
  this->TotalWeight = 0.0;

  for(int i = this->Weights->GetNumberOfTuples() - 1; i >= 0; i--)
    {
    this->TotalWeight += this->Weights->GetValue(i);
    }
}


// Evaluate sum of implicit functions.
float vtkImplicitSum::EvaluateFunction(float x[3])
{
  float sum = 0;
  float c;
  int i;
  vtkImplicitFunction *f;
  float *weights = this->Weights->GetPointer(0);

  for (i = 0, this->FunctionList->InitTraversal(); 
       (f=this->FunctionList->GetNextItem()); i++)
    { 
    c = weights[i];
    if (c != 0.0)
      {
      sum += f->FunctionValue(x)*c;
      }
    }
  if (this->NormalizeByWeight && this->TotalWeight != 0.0) 
    {
    sum /= this->TotalWeight;
    }
  return sum;
}

// Evaluate gradient of sum of functions (valid only if linear)
void vtkImplicitSum::EvaluateGradient(float x[3], float g[3])
{
  float c;
  int i;
  float gtmp[3];
  vtkImplicitFunction *f;
  float *weights = this->Weights->GetPointer(0);

  g[0] = g[1] = g[2] = 0.0;
  for (i = 0, this->FunctionList->InitTraversal(); 
       (f=this->FunctionList->GetNextItem()); i++)
    {
    c = weights[i];
    if ( c != 0.0 ) 
      {
      f->FunctionGradient(x, gtmp);
      g[0] += gtmp[0]*c;
      g[1] += gtmp[1]*c;
      g[2] += gtmp[2]*c;
      }
    }

  if (this->NormalizeByWeight && this->TotalWeight != 0.0) 
    {
    g[0] /= this->TotalWeight;
    g[1] /= this->TotalWeight;
    g[2] /= this->TotalWeight;
    }
}

void vtkImplicitSum::PrintSelf(ostream& os, vtkIndent indent)
{
#if VTK_MAJOR_VERSION < 4
  vtkImplicitFunction::PrintSelf(os,indent);
#else
  this->Superclass::PrintSelf(os,indent);
#endif

  os << indent << "NormalizeByWeight: " 
     << (this->NormalizeByWeight ? "On\n" : "Off\n");

  os << indent << "Function List:\n";
  this->FunctionList->PrintSelf(os,indent.GetNextIndent());

  os << indent << "Weights:\n";
  this->Weights->PrintSelf(os,indent.GetNextIndent());
}
