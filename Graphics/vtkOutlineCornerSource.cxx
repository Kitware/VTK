/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOutlineCornerSource.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Sebastien Barre who developed this class.


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
#include "vtkOutlineCornerSource.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkOutlineCornerSource* vtkOutlineCornerSource::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkOutlineCornerSource");
  if(ret)
    {
    return (vtkOutlineCornerSource*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkOutlineCornerSource;
}




//----------------------------------------------------------------------------
vtkOutlineCornerSource::vtkOutlineCornerSource()
    : vtkOutlineSource()
{
  this->CornerFactor = 0.2;
}

//----------------------------------------------------------------------------
void vtkOutlineCornerSource::Execute()
{
  float *bounds;
  float inner_bounds[6];

  int i, j, k;

  vtkDebugMacro(<< "Generating outline");

  // Initialize

  float delta;

  bounds = this->Bounds;
  for (i = 0; i < 3; i++)
  {
      delta = (bounds[2*i + 1] - bounds[2*i]) * this->CornerFactor;
      inner_bounds[2*i] = bounds[2*i] + delta;
      inner_bounds[2*i + 1] = bounds[2*i + 1] - delta;
  }

  // Allocate storage and create outline

  vtkPoints *newPts;
  vtkCellArray *newLines;
  vtkPolyData *output = this->GetOutput();
  
  newPts = vtkPoints::New();
  newPts->Allocate(32);
  newLines = vtkCellArray::New();
  newLines->Allocate(newLines->EstimateSize(24,2));

  float x[3];
  vtkIdType pts[2];

  int pid = 0;

  // 32 points and 24 lines

  for (i = 0; i <= 1; i++)
  {
      for (j = 2; j <= 3; j++)
      {
          for (k = 4; k <= 5; k++)
          {
              pts[0] = pid;
              x[0] = bounds[i]; x[1] = bounds[j]; x[2] = bounds[k];
              newPts->InsertPoint(pid++, x);

              pts[1] = pid;
              x[0] = inner_bounds[i]; x[1] = bounds[j]; x[2] = bounds[k];
              newPts->InsertPoint(pid++, x);
              newLines->InsertNextCell(2,pts);

              pts[1] = pid;
              x[0] = bounds[i]; x[1] = inner_bounds[j]; x[2] = bounds[k];
              newPts->InsertPoint(pid++, x);
              newLines->InsertNextCell(2,pts);

              pts[1] = pid;
              x[0] = bounds[i]; x[1] = bounds[j]; x[2] = inner_bounds[k];
              newPts->InsertPoint(pid++, x);
              newLines->InsertNextCell(2,pts);
          }
      }
  }

  // Update selves and release memory

  output->SetPoints(newPts);
  newPts->Delete();

  output->SetLines(newLines);
  newLines->Delete();
}


//----------------------------------------------------------------------------
void vtkOutlineCornerSource::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkOutlineSource::PrintSelf(os,indent);
  os << indent << "CornerFactor: " << this->CornerFactor << "\n";
}
