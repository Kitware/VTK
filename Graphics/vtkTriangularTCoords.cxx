/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTriangularTCoords.cxx
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
#include "vtkTriangularTCoords.h"
#include "vtkFloatArray.h"
#include <math.h>
#include "vtkObjectFactory.h"

//--------------------------------------------------------------------------
vtkTriangularTCoords* vtkTriangularTCoords::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkTriangularTCoords");
  if(ret)
    {
    return (vtkTriangularTCoords*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkTriangularTCoords;
}

void vtkTriangularTCoords::Execute()
{
  vtkIdType tmp;
  int j;
  vtkPoints *inPts;
  vtkPointData *pd;
  vtkCellArray *inPolys,*inStrips;
  vtkIdType numNewPts, numNewPolys, polyAllocSize;
  vtkFloatArray *newTCoords;
  vtkIdType newId, numCells, cellId;
  vtkIdType *pts, newIds[3], npts;
  int errorLogging = 1;
  vtkPoints *newPoints;
  vtkCellArray *newPolys;
  float *p1, *p2, *p3;
  float tCoords[6];
  vtkPolyData *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();
  vtkPointData *pointData = output->GetPointData(); 

  // Initialize
  //
  vtkDebugMacro(<<"Generating triangular texture coordinates");

  inPts = input->GetPoints();
  pd = input->GetPointData();

  inPolys = input->GetPolys();
  inStrips = input->GetStrips();

  // Count the number of new points and other primitives that 
  // need to be created.
  //
  numNewPts = input->GetNumberOfVerts();

  numNewPolys = 0;
  polyAllocSize = 0;

  for (inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts); )
    {
    numNewPts += npts;
    numNewPolys++;
    polyAllocSize += npts + 1;
    }
  for (inStrips->InitTraversal(); inStrips->GetNextCell(npts,pts); )
    {
    numNewPts += (npts-2) * 3;
    polyAllocSize += (npts - 2) * 4;
    }
  numCells = inPolys->GetNumberOfCells() + inStrips->GetNumberOfCells();

  //  Allocate texture data
  //
  newTCoords = vtkFloatArray::New();
  newTCoords->SetNumberOfComponents(2);
  newTCoords->Allocate(2*numNewPts);

  // Allocate
  //
  newPoints = vtkPoints::New();
  newPoints->Allocate(numNewPts);

  newPolys = vtkCellArray::New();
  newPolys->Allocate(polyAllocSize);

  pointData->CopyTCoordsOff();
  pointData->CopyAllocate(pd);

  // Texture coordinates are the same for each triangle
  //
  tCoords[0]= 0.0;
  tCoords[1]= 0.0;
  tCoords[2]= 1.0;
  tCoords[3]= 0.0;
  tCoords[4]= 0.5;
  tCoords[5]= sqrt(3.0)/2.0;

  int abort=0;
  vtkIdType progressInterval=numCells/20 + 1;
  for (cellId=0, inPolys->InitTraversal(); 
       inPolys->GetNextCell(npts,pts) && !abort; cellId++)
    {
    if ( !(cellId % progressInterval) )
      {
      this->UpdateProgress((float)cellId/numCells);
      abort = this->GetAbortExecute();
      }
    
    if (npts != 3)
      {
      if (errorLogging) vtkWarningMacro(
        << "No texture coordinates for this cell, it is not a triangle");
      errorLogging = 0;
      continue;
      }
    newPolys->InsertNextCell(npts);
    for (j=0; j<npts; j++)
      {
      p1 = inPts->GetPoint(pts[j]);
      newId = newPoints->InsertNextPoint(p1);
      newPolys->InsertCellPoint(newId);
      pointData->CopyData(pd,pts[j],newId);
      newTCoords->SetTuple (newId,&tCoords[2*j]);
      }
    }

  // Triangle strips
  //
  for (inStrips->InitTraversal(); 
       inStrips->GetNextCell(npts,pts) && !abort; cellId++)
    {
    if ( !(cellId % progressInterval) )
      {
      this->UpdateProgress((float)cellId/numCells);
      abort = this->GetAbortExecute();
      }

    for (j=0; j<(npts-2); j++)
      {
      p1 = inPts->GetPoint(pts[j]);
      p2 = inPts->GetPoint(pts[j+1]);
      p3 = inPts->GetPoint(pts[j+2]);

      newIds[0] = newPoints->InsertNextPoint(p1);
      pointData->CopyData(pd,pts[j],newIds[0]);
      newTCoords->SetTuple (newIds[0],&tCoords[0]);

      newIds[1] = newPoints->InsertNextPoint(p2);
      pointData->CopyData(pd,pts[j+1],newIds[1]);
      newTCoords->SetTuple (newIds[1],&tCoords[2]);

      newIds[2] = newPoints->InsertNextPoint(p3);
      pointData->CopyData(pd,pts[j+2],newIds[2]);
      newTCoords->SetTuple (newIds[2],&tCoords[4]);

      // flip orientation for odd tris
      if (j%2) 
        {
        tmp = newIds[0];
        newIds[0] = newIds[2];
        newIds[2] = tmp;
        }
      newPolys->InsertNextCell(3,newIds);
      }
    }

  // Update self and release memory
  //
  output->SetPoints(newPoints);
  newPoints->Delete();

  output->SetPolys(newPolys);
  newPolys->Delete();

  output->GetPointData()->SetTCoords(newTCoords);
  newTCoords->Delete();
}

void vtkTriangularTCoords::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataToPolyDataFilter::PrintSelf(os,indent);
}
