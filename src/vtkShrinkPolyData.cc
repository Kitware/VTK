/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShrinkPolyData.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkShrinkPolyData.hh"


vtkShrinkPolyData::vtkShrinkPolyData(float sf)
{
  this->ShrinkFactor = sf;
}

void vtkShrinkPolyData::Execute()
{
  int j, k;
  float center[3];
  vtkPoints *inPts;
  vtkPointData *pd;
  vtkCellArray *inVerts,*inLines,*inPolys,*inStrips;
  int numNewPts, numNewLines, numNewPolys, poly_alloc_size;
  int npts, *pts, newId, newIds[3];
  vtkFloatPoints *newPoints;
  vtkCellArray *newVerts, *newLines, *newPolys;
  float *p1, *p2, *p3, pt[3];
  vtkPolyData *input =(vtkPolyData *)this->Input;
  vtkPolyData *output=(vtkPolyData *)this->Output;
  vtkPointData *pointData = output->GetPointData(); 
//
// Initialize
//
  vtkDebugMacro(<<"Shrinking polygonal data");

  inPts = input->GetPoints();
  pd = input->GetPointData();

  inVerts = input->GetVerts();
  inLines = input->GetLines();
  inPolys = input->GetPolys();
  inStrips = input->GetStrips();
//
// Count the number of new points and other primitives that 
// need to be created.
//
  numNewPts = input->GetNumberOfVerts();
  numNewLines = 0;
  numNewPolys = 0;
  poly_alloc_size = 0;

  for (inLines->InitTraversal(); inLines->GetNextCell(npts,pts); )
    {
    numNewPts += (npts-1) * 2;
    numNewLines += npts - 1;
    }
  for (inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts); )
    {
    numNewPts += npts;
    numNewPolys++;
    poly_alloc_size += npts + 1;
    }
  for (inStrips->InitTraversal(); inStrips->GetNextCell(npts,pts); )
    {
    numNewPts += (npts-2) * 3;
    poly_alloc_size += (npts - 2) * 4;
    }
//
// Allocate
//
  newPoints = new vtkFloatPoints(numNewPts);

  newVerts = new vtkCellArray(input->GetNumberOfVerts());

  newLines = new vtkCellArray;
  newLines->Allocate(numNewLines*3);
 
  newPolys = new vtkCellArray;
  newPolys->Allocate(poly_alloc_size);

  pointData->CopyAllocate(pd);
//
// Copy vertices (no shrinking necessary)
//
  for (inVerts->InitTraversal(); inVerts->GetNextCell(npts,pts); )
    {
    newVerts->InsertNextCell(npts);
    for (j=0; j<npts; j++)
      {
      newId = newPoints->InsertNextPoint(inPts->GetPoint(pts[j]));
      newVerts->InsertCellPoint(newId);
      pointData->CopyData(pd,pts[j],newId);
      }    
    }
//
// Lines need to be shrunk, and if polyline, split into separate pieces
//
  for (inLines->InitTraversal(); inLines->GetNextCell(npts,pts); )
    {
    for (j=0; j<(npts-1); j++)
      {
      p1 = inPts->GetPoint(pts[j]);
      p2 = inPts->GetPoint(pts[j+1]);
      for (k=0; k<3; k++) center[k] = (p1[k] + p2[k]) / 2.0;

      for (k=0; k<3; k++)
        pt[k] = center[k] + this->ShrinkFactor*(p1[k] - center[k]);
      newIds[0] = newPoints->InsertNextPoint(pt);
      pointData->CopyData(pd,pts[j],newIds[0]);

      for (k=0; k<3; k++)
        pt[k] = center[k] + this->ShrinkFactor*(p2[k] - center[k]);
      newIds[1] = newPoints->InsertNextPoint(pt);
      pointData->CopyData(pd,pts[j+1],newIds[1]);

      newLines->InsertNextCell(2,newIds);
      }
    }
//
// Polygons need to be shrunk
//
  for (inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts); )
    {
    for (center[0]=center[1]=center[2]=0.0, j=0; j<npts; j++)
      {
      p1 = inPts->GetPoint(pts[j]);
      for (k=0; k<3; k++) center[k] += p1[k];
      }

    for (k=0; k<3; k++) center[k] /= npts;

    
    newPolys->InsertNextCell(npts);
    for (j=0; j<npts; j++)
      {
      p1 = inPts->GetPoint(pts[j]);
      for (k=0; k<3; k++)
        pt[k] = center[k] + this->ShrinkFactor*(p1[k] - center[k]);
      newId = newPoints->InsertNextPoint(pt);
      newPolys->InsertCellPoint(newId);
      pointData->CopyData(pd,pts[j],newId);
      }
    }
//
// Triangle strips need to be shrunk and split into separate pieces.
//
  for (inStrips->InitTraversal(); inStrips->GetNextCell(npts,pts); )
    {
    for (j=0; j<(npts-2); j++)
      {
      p1 = inPts->GetPoint(pts[j]);
      p2 = inPts->GetPoint(pts[j+1]);
      p3 = inPts->GetPoint(pts[j+1]);
      for (k=0; k<3; k++) center[k] = (p1[k] + p2[k] + p3[k]) / 3.0;

      for (k=0; k<3; k++)
        pt[k] = center[k] + this->ShrinkFactor*(p1[k] - center[k]);
      newIds[0] = newPoints->InsertNextPoint(pt);
      pointData->CopyData(pd,pts[j],newIds[0]);

      for (k=0; k<3; k++)
        pt[k] = center[k] + this->ShrinkFactor*(p2[k] - center[k]);
      newIds[1] = newPoints->InsertNextPoint(pt);
      pointData->CopyData(pd,pts[j+1],newIds[1]);

      for (k=0; k<3; k++)
        pt[k] = center[k] + this->ShrinkFactor*(p3[k] - center[k]);
      newIds[2] = newPoints->InsertNextPoint(pt);
      pointData->CopyData(pd,pts[j+2],newIds[2]);

      newPolys->InsertNextCell(3,newIds);
      }
    }
//
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
}


void vtkShrinkPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyToPolyFilter::PrintSelf(os,indent);
  os << indent << "Shrink Factor: " << this->ShrinkFactor << "\n";
}
