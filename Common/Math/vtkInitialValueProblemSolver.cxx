/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInitialValueProblemSolver.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInitialValueProblemSolver.h"

#include "vtkFunctionSet.h"


vtkInitialValueProblemSolver::vtkInitialValueProblemSolver()
{
  this->FunctionSet = 0;
  this->Vals = 0;
  this->Derivs = 0;
  this->Initialized = 0;
  this->Adaptive = 0;
}

vtkInitialValueProblemSolver::~vtkInitialValueProblemSolver()
{
  this->SetFunctionSet(0);
  delete[] this->Vals;
  this->Vals = 0;
  delete[] this->Derivs;
  this->Derivs = 0;
  this->Initialized = 0;
}

void vtkInitialValueProblemSolver::SetFunctionSet(vtkFunctionSet* fset)
{
  if (this->FunctionSet != fset)
    {
    if (this->FunctionSet != 0) { this->FunctionSet->UnRegister(this); }
    if (fset != 0 && (fset->GetNumberOfFunctions() !=
                      fset->GetNumberOfIndependentVariables() - 1))
      {
      vtkErrorMacro("Invalid function set!");
      this->FunctionSet = 0;
      return;
      }
    this->FunctionSet = fset;
    if (this->FunctionSet != 0) { this->FunctionSet->Register(this); }
    this->Modified();
    }
  this->Initialize();
}

void vtkInitialValueProblemSolver::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Function set : " << this->FunctionSet << endl;
  os << indent << "Function values : " << this->Vals << endl;
  os << indent << "Function derivatives: " << this->Derivs << endl;
  os << indent << "Initialized: " << (this->Initialized ? "Yes" : "No") << endl;
}

void vtkInitialValueProblemSolver::Initialize()
{
  if (!this->FunctionSet || this->Initialized)
    {
    return;
    }
  this->Vals =
    new double[this->FunctionSet->GetNumberOfIndependentVariables()];
  this->Derivs =
    new double[this->FunctionSet->GetNumberOfFunctions()];
  this->Initialized = 1;
}

