/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShrinkPolyData.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkShrinkPolyData.h"
#include "vtkObjectFactory.h"

//--------------------------------------------------------------------------
vtkShrinkPolyData* vtkShrinkPolyData::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkShrinkPolyData");
  if(ret)
    {
    return (vtkShrinkPolyData*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkShrinkPolyData;
}

vtkShrinkPolyData::vtkShrinkPolyData(float sf)
{
  sf = ( sf < 0.0 ? 0.0 : (sf > 1.0 ? 1.0 : sf));
  this->ShrinkFactor = sf;
}

void vtkShrinkPolyData::Execute()
{
  int j, k;
  float center[3];
  vtkPoints *inPts;
  vtkPointData *pd;
  vtkCellArray *inVerts,*inLines,*inPolys,*inStrips;
  int numNewPts, numNewLines, numNewPolys, polyAllocSize;
  int npts, *pts, newId, newIds[3];
  vtkPoints *newPoints;
  vtkCellArray *newVerts, *newLines, *newPolys;
  float *p1, *p2, *p3, pt[3];
  vtkPolyData *input = this->GetInput();
  vtkPolyData *output= this->GetOutput();
  vtkPointData *pointData = output->GetPointData(); 
  int   abortExecute=0;

  // Initialize
  //
  vtkDebugMacro(<<"Shrinking polygonal data");

  inPts = input->GetPoints();
  pd = input->GetPointData();

  inVerts = input->GetVerts();
  inLines = input->GetLines();
  inPolys = input->GetPolys();
  inStrips = input->GetStrips();

  // Count the number of new points and other primitives that 
  // need to be created.
  //
  numNewPts = input->GetNumberOfVerts();
  numNewLines = 0;
  numNewPolys = 0;
  polyAllocSize = 0;

  for (inLines->InitTraversal(); inLines->GetNextCell(npts,pts); )
    {
    numNewPts += (npts-1) * 2;
    numNewLines += npts - 1;
    }
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

  // Allocate
  //
  newPoints = vtkPoints::New();
  newPoints->Allocate(numNewPts);

  newVerts = vtkCellArray::New();
  newVerts->Allocate(input->GetNumberOfVerts());

  newLines = vtkCellArray::New();
  newLines->Allocate(numNewLines*3);
 
  newPolys = vtkCellArray::New();
  newPolys->Allocate(polyAllocSize);

  pointData->CopyAllocate(pd);

  // Copy vertices (no shrinking necessary)
  //
  for (inVerts->InitTraversal(); inVerts->GetNextCell(npts,pts) && !abortExecute; )
    {
    newVerts->InsertNextCell(npts);
    for (j=0; j<npts; j++)
      {
      newId = newPoints->InsertNextPoint(inPts->GetPoint(pts[j]));
      newVerts->InsertCellPoint(newId);
      pointData->CopyData(pd,pts[j],newId);
      }    
    abortExecute = GetAbortExecute();
    }
  this->UpdateProgress (0.10);

  // Lines need to be shrunk, and if polyline, split into separate pieces
  //
  for (inLines->InitTraversal(); inLines->GetNextCell(npts,pts) && !abortExecute; )
    {
    for (j=0; j<(npts-1); j++)
      {
      p1 = inPts->GetPoint(pts[j]);
      p2 = inPts->GetPoint(pts[j+1]);
      for (k=0; k<3; k++)
	{
	center[k] = (p1[k] + p2[k]) / 2.0;
	}

      for (k=0; k<3; k++)
	{
        pt[k] = center[k] + this->ShrinkFactor*(p1[k] - center[k]);
	}
      newIds[0] = newPoints->InsertNextPoint(pt);
      pointData->CopyData(pd,pts[j],newIds[0]);

      for (k=0; k<3; k++)
	{
        pt[k] = center[k] + this->ShrinkFactor*(p2[k] - center[k]);
	}
      newIds[1] = newPoints->InsertNextPoint(pt);
      pointData->CopyData(pd,pts[j+1],newIds[1]);

      newLines->InsertNextCell(2,newIds);
      }
    abortExecute = GetAbortExecute();
    }
  this->UpdateProgress (0.25);

  // Polygons need to be shrunk
  //
  for (inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts) && !abortExecute; )
    {
    for (center[0]=center[1]=center[2]=0.0, j=0; j<npts; j++)
      {
      p1 = inPts->GetPoint(pts[j]);
      for (k=0; k<3; k++)
	{
	center[k] += p1[k];
	}
      }

    for (k=0; k<3; k++)
      {
      center[k] /= npts;
      }

    newPolys->InsertNextCell(npts);
    for (j=0; j<npts; j++)
      {
      p1 = inPts->GetPoint(pts[j]);
      for (k=0; k<3; k++)
	{
        pt[k] = center[k] + this->ShrinkFactor*(p1[k] - center[k]);
	}
      newId = newPoints->InsertNextPoint(pt);
      newPolys->InsertCellPoint(newId);
      pointData->CopyData(pd,pts[j],newId);
      }
    abortExecute = GetAbortExecute();
    }
  this->UpdateProgress (0.75);

  // Triangle strips need to be shrunk and split into separate pieces.
  //
  int tmp;
  for (inStrips->InitTraversal(); inStrips->GetNextCell(npts,pts) && !abortExecute; )
    {
    for (j=0; j<(npts-2); j++)
      {
      p1 = inPts->GetPoint(pts[j]);
      p2 = inPts->GetPoint(pts[j+1]);
      p3 = inPts->GetPoint(pts[j+2]);
      for (k=0; k<3; k++)
	{
	center[k] = (p1[k] + p2[k] + p3[k]) / 3.0;
	}

      for (k=0; k<3; k++)
	{
        pt[k] = center[k] + this->ShrinkFactor*(p1[k] - center[k]);
	}
      newIds[0] = newPoints->InsertNextPoint(pt);
      pointData->CopyData(pd,pts[j],newIds[0]);

      for (k=0; k<3; k++)
	{
        pt[k] = center[k] + this->ShrinkFactor*(p2[k] - center[k]);
	}
      newIds[1] = newPoints->InsertNextPoint(pt);
      pointData->CopyData(pd,pts[j+1],newIds[1]);

      for (k=0; k<3; k++)
	{
        pt[k] = center[k] + this->ShrinkFactor*(p3[k] - center[k]);
	}
      newIds[2] = newPoints->InsertNextPoint(pt);
      pointData->CopyData(pd,pts[j+2],newIds[2]);

      // must reverse order for every other triangle
      if (j%2)
        {
        tmp = newIds[0];
        newIds[0] = newIds[2];
        newIds[2] = tmp;
        }
      newPolys->InsertNextCell(3,newIds);
      }
    abortExecute = GetAbortExecute();
    }

  // Update self and release memory
  //
  output->SetPoints(newPoints);
  newPoints->Delete();

  output->SetVerts(newVerts);
  newVerts->Delete();

  output->SetLines(newLines);
  newLines->Delete();
 
  output->SetPolys(newPolys);
  newPolys->Delete();

  output->GetCellData()->PassData(input->GetCellData());
}


void vtkShrinkPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataToPolyDataFilter::PrintSelf(os,indent);
  os << indent << "Shrink Factor: " << this->ShrinkFactor << "\n";
}
