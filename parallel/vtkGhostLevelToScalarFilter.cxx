/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGhostLevelToScalarFilter.cxx
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
#include "vtkGhostLevelToScalarFilter.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"



//----------------------------------------------------------------------------
vtkGhostLevelToScalarFilter* vtkGhostLevelToScalarFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkGhostLevelToScalarFilter");
  if(ret)
    {
    return (vtkGhostLevelToScalarFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkGhostLevelToScalarFilter;
}

//----------------------------------------------------------------------------
void vtkGhostLevelToScalarFilter::CopyLevelsToScalars(vtkGhostLevels *levels,
						      vtkScalars *scalars)
{
  int i, num;

  num = levels->GetNumberOfGhostLevels();
  scalars->Allocate(num);
  scalars->SetNumberOfScalars(num);
  for (i = 0; i < num; ++i)
    {
    scalars->InsertScalar(i, levels->GetGhostLevel(i));
    }
}

//----------------------------------------------------------------------------
void vtkGhostLevelToScalarFilter::Execute()
{
  vtkScalars *newScalars;
  vtkGhostLevels *ghostLevels;
  vtkDataSet *input = this->GetInput();
  
  // First, copy the input to the output as a starting point
  this->GetOutput()->CopyStructure( input );
  this->GetOutput()->GetPointData()->CopyScalarsOff();
  this->GetOutput()->GetPointData()->PassData(input->GetPointData());
  this->GetOutput()->GetCellData()->CopyScalarsOff();
  this->GetOutput()->GetCellData()->PassData(input->GetCellData());

  ghostLevels = input->GetPointData()->GetGhostLevels();
  if (ghostLevels)
    {
    newScalars = vtkScalars::New();
    this->CopyLevelsToScalars(ghostLevels, newScalars);
    this->GetOutput()->GetPointData()->SetScalars(newScalars);
    newScalars->Delete();
    newScalars = NULL;
    }
  
  ghostLevels = input->GetCellData()->GetGhostLevels();
  if (ghostLevels)
    {
    newScalars = vtkScalars::New();
    this->CopyLevelsToScalars(ghostLevels, newScalars);
    this->GetOutput()->GetCellData()->SetScalars(newScalars);
    newScalars->Delete();
    newScalars = NULL;
    }
}

