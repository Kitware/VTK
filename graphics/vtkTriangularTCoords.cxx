/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTriangularTCoords.cxx
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
#include "vtkTriangularTCoords.h"
#include "vtkFloatTCoords.h"
#include <math.h>

void vtkTriangularTCoords::Execute()
{
  int tmp;
  int j, k;
  vtkPoints *inPts;
  vtkPointData *pd;
  vtkCellArray *inPolys,*inStrips;
  int numNewPts, numNewPolys, poly_alloc_size;
  vtkFloatTCoords *newTCoords;
  int npts, *pts, newId, newIds[3];
  int errorLogging = 1;
  vtkFloatPoints *newPoints;
  vtkCellArray *newPolys;
  float *p1, *p2, *p3, pt[3];
  float tCoords[6];
  vtkPolyData *input =(vtkPolyData *)this->Input;
  vtkPolyData *output=(vtkPolyData *)this->Output;
  vtkPointData *pointData = output->GetPointData(); 
//
// Initialize
//
  vtkDebugMacro(<<"Generating Triangular TCoords");

  inPts = input->GetPoints();
  pd = input->GetPointData();

  inPolys = input->GetPolys();
  inStrips = input->GetStrips();
//
// Count the number of new points and other primitives that 
// need to be created.
//
  numNewPts = input->GetNumberOfVerts();

  numNewPolys = 0;
  poly_alloc_size = 0;

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
//  Allocate texture data
//
  newTCoords = new vtkFloatTCoords(numNewPts,2);

//
// Allocate
//
  newPoints = new vtkFloatPoints(numNewPts);

  newPolys = new vtkCellArray;
  newPolys->Allocate(poly_alloc_size);

  pointData->CopyTCoordsOff();
  pointData->CopyAllocate(pd);

//
// Texture coordinates are the same for each triangle
//
  tCoords[0]= 0.0;
  tCoords[1]= 0.0;
  tCoords[2]= 1.0;
  tCoords[3]= 0.0;
  tCoords[4]= 0.5;
  tCoords[5]= sqrt(3.0)/2.0;

  for (inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts); )
    {
    if (npts != 3)
      {
      if (errorLogging) vtkWarningMacro(<< "No texture coordinates for this cell, it is not a triangle");
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
      newTCoords->SetTCoord (newId,&tCoords[2*j]);
      }
    }
//
// Triangle strips
//
  for (inStrips->InitTraversal(); inStrips->GetNextCell(npts,pts); )
    {

    for (j=0; j<(npts-2); j++)
      {
      p1 = inPts->GetPoint(pts[j]);
      p2 = inPts->GetPoint(pts[j+1]);
      p3 = inPts->GetPoint(pts[j+2]);

      newIds[0] = newPoints->InsertNextPoint(p1);
      pointData->CopyData(pd,pts[j],newIds[0]);
      newTCoords->SetTCoord (newIds[0],&tCoords[0]);

      newIds[1] = newPoints->InsertNextPoint(p2);
      pointData->CopyData(pd,pts[j+1],newIds[1]);
      newTCoords->SetTCoord (newIds[1],&tCoords[2]);

      newIds[2] = newPoints->InsertNextPoint(p3);
      pointData->CopyData(pd,pts[j+2],newIds[2]);
      newTCoords->SetTCoord (newIds[2],&tCoords[4]);

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
//
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
  vtkPolyToPolyFilter::PrintSelf(os,indent);
}
