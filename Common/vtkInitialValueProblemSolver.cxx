/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInitialValueProblemSolver.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "vtkInitialValueProblemSolver.h"

vtkInitialValueProblemSolver::vtkInitialValueProblemSolver() 
{
  this->FunctionSet = 0;
  this->Vals = 0;
  this->Derivs = 0;
  this->Initialized = 0;
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
  this->vtkObject::PrintSelf(os,indent);
  os << indent << "Function set : " << this->FunctionSet << endl;
  os << indent << "Function values : " << this->Vals << endl;
  os << indent << "Function derivatives: " << this->Derivs << endl;
  os << indent << "Initialized: ";
  if (this->Initialized)
    {
    os << "yes." << endl;
    }
  else
    { 
    os << "no." << endl;
    }
}

void vtkInitialValueProblemSolver::Initialize()
{
  if (!FunctionSet || this->Initialized)
    {
    return;
    }
  this->Vals = new float[this->FunctionSet->GetNumberOfIndependentVariables()];
  this->Derivs = new float[this->FunctionSet->GetNumberOfFunctions()];
  this->Initialized = 1;
}
