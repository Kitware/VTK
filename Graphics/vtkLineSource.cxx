/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLineSource.cxx
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
#include <math.h>
#include "vtkLineSource.h"
#include "vtkPoints.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkLineSource* vtkLineSource::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkLineSource");
  if(ret)
    {
    return (vtkLineSource*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkLineSource;
}




vtkLineSource::vtkLineSource(int res)
{
  this->Point1[0] = -0.5;
  this->Point1[1] =  0.0;
  this->Point1[2] =  0.0;

  this->Point2[0] =  0.5;
  this->Point2[1] =  0.0;
  this->Point2[2] =  0.0;

  this->Resolution = (res < 1 ? 1 : res);
}

void vtkLineSource::Execute()
{
  int numLines=this->Resolution;
  int numPts=this->Resolution+1;
  float x[3], tc[3], v[3];
  int i, j;
  vtkPoints *newPoints; 
  vtkFloatArray *newTCoords; 
  vtkCellArray *newLines;
  vtkPolyData *output = this->GetOutput();
  
  vtkDebugMacro(<<"Creating line");

  newPoints = vtkPoints::New();
  newPoints->Allocate(numPts);
  newTCoords = vtkFloatArray::New();
  newTCoords->SetNumberOfComponents(2);
  newTCoords->Allocate(2*numPts);

  newLines = vtkCellArray::New();
  newLines->Allocate(newLines->EstimateSize(numLines,2));
//
// Generate points and texture coordinates
//
  for (i=0; i<3; i++)
    {
    v[i] = this->Point2[i] - this->Point1[i];
    }

  tc[1] = 0.0;
  tc[2] = 0.0;
  for (i=0; i<numPts; i++) 
    {
    tc[0] = ((float)i/this->Resolution);
    for (j=0; j<3; j++)
      {
      x[j] = this->Point1[j] + tc[0]*v[j];
      }
    newPoints->InsertPoint(i,x);
    newTCoords->InsertTuple(i,tc);
    }
//
//  Generate lines
//
  newLines->InsertNextCell(numPts);
  for (i=0; i < numPts; i++) 
    {
    newLines->InsertCellPoint (i);
    }
//
// Update ourselves and release memory
//
  output->SetPoints(newPoints);
  newPoints->Delete();

  output->GetPointData()->SetTCoords(newTCoords);
  newTCoords->Delete();

  output->SetLines(newLines);
  newLines->Delete();
}

void vtkLineSource::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataSource::PrintSelf(os,indent);

  os << indent << "Resolution: " << this->Resolution << "\n";

  os << indent << "Point 1: (" << this->Point1[0] << ", "
                               << this->Point1[1] << ", "
                               << this->Point1[2] << ")\n";

  os << indent << "Point 2: (" << this->Point2[0] << ", "
                               << this->Point2[1] << ", "
                               << this->Point2[2] << ")\n";


}
