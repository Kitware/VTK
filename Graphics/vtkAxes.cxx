/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAxes.cxx
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
#include "vtkAxes.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"



//----------------------------------------------------------------------------
vtkAxes* vtkAxes::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkAxes");
  if(ret)
    {
    return (vtkAxes*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkAxes;
}




// Construct with origin=(0,0,0) and scale factor=1.
vtkAxes::vtkAxes()
{
  this->Origin[0] = 0.0;  
  this->Origin[1] = 0.0;  
  this->Origin[2] = 0.0;

  this->ScaleFactor = 1.0;
  
  this->Symmetric = 0;
  this->ComputeNormals = 1;
}

void vtkAxes::Execute()
{
  int numPts=6, numLines=3;
  vtkPoints *newPts;
  vtkCellArray *newLines;
  vtkFloatArray *newScalars;
  vtkFloatArray *newNormals;
  float x[3], n[3];
  vtkIdType ptIds[2];
  vtkPolyData *output = this->GetOutput();
  
  vtkDebugMacro(<<"Creating x-y-z axes");

  newPts = vtkPoints::New();
  newPts->Allocate(numPts);
  newLines = vtkCellArray::New();
  newLines->Allocate(newLines->EstimateSize(numLines,2));
  newScalars = vtkFloatArray::New();
  newScalars->Allocate(numPts);
  newScalars->SetName("Axes");
  newNormals = vtkFloatArray::New();
  newNormals->SetNumberOfComponents(3);
  newNormals->Allocate(numPts);
  newNormals->SetName("Normals");
  
//
// Create axes
//
  x[0] = this->Origin[0];
  x[1] = this->Origin[1];
  x[2] = this->Origin[2];
  if (this->Symmetric)
    {
    x[0] = this->Origin[0] - this->ScaleFactor;
    }
  n[0] = 0.0; n[1] = 1.0; n[2] = 0.0; 
  ptIds[0] = newPts->InsertNextPoint(x);
  newScalars->InsertNextValue(0.0);
  newNormals->InsertNextTuple(n);

  x[0] = this->Origin[0] + this->ScaleFactor;
  x[1] = this->Origin[1];
  x[2] = this->Origin[2];
  ptIds[1] = newPts->InsertNextPoint(x);
  newLines->InsertNextCell(2,ptIds);
  newScalars->InsertNextValue(0.0);
  newNormals->InsertNextTuple(n);

  x[0] = this->Origin[0];
  x[1] = this->Origin[1];
  x[2] = this->Origin[2];
  if (this->Symmetric)
    {
    x[1] = this->Origin[1] - this->ScaleFactor;
    }
  n[0] = 0.0; n[1] = 0.0; n[2] = 1.0; 
  ptIds[0] = newPts->InsertNextPoint(x);
  newScalars->InsertNextValue(0.25);
  newNormals->InsertNextTuple(n);

  x[0] = this->Origin[0];
  x[1] = this->Origin[1] + this->ScaleFactor;
  x[2] = this->Origin[2];
  ptIds[1] = newPts->InsertNextPoint(x);
  newScalars->InsertNextValue(0.25);
  newNormals->InsertNextTuple(n);
  newLines->InsertNextCell(2,ptIds);

  x[0] = this->Origin[0];
  x[1] = this->Origin[1];
  x[2] = this->Origin[2];
  if (this->Symmetric)
    {
    x[2] = this->Origin[2] - this->ScaleFactor;
    }
  n[0] = 1.0; n[1] = 0.0; n[2] = 0.0; 
  ptIds[0] = newPts->InsertNextPoint(x);
  newScalars->InsertNextValue(0.5);
  newNormals->InsertNextTuple(n);

  x[0] = this->Origin[0];
  x[1] = this->Origin[1];
  x[2] = this->Origin[2] + this->ScaleFactor;
  ptIds[1] = newPts->InsertNextPoint(x);
  newScalars->InsertNextValue(0.5);
  newNormals->InsertNextTuple(n);
  newLines->InsertNextCell(2,ptIds);

  //
  // Update our output and release memory
  // 
  output->SetPoints(newPts);
  newPts->Delete();

  output->GetPointData()->SetScalars(newScalars);
  newScalars->Delete();

  if (this->ComputeNormals)
    {
    output->GetPointData()->SetNormals(newNormals);
    }
  newNormals->Delete();

  output->SetLines(newLines);
  newLines->Delete();
}

//----------------------------------------------------------------------------
// This source does not know how to generate pieces yet.
int vtkAxes::ComputeDivisionExtents(vtkDataObject *vtkNotUsed(output),
				      int idx, int numDivisions)
{
  if (idx == 0 && numDivisions == 1)
    {
    // I will give you the whole thing
    return 1;
    }
  else
    {
    // I have nothing to give you for this piece.
    return 0;
    }
}

void vtkAxes::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataSource::PrintSelf(os,indent);
  os << indent << "Origin: (" << this->Origin[0] << ", "
               << this->Origin[1] << ", "
               << this->Origin[2] << ")\n";
  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
  os << indent << "Symmetric: " << this->Symmetric << "\n";
  os << indent << "ComputeNormals: " << this->ComputeNormals << "\n";
}
