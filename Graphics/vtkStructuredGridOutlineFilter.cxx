/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredGridOutlineFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStructuredGridOutlineFilter.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkStructuredGridOutlineFilter, "1.40");
vtkStandardNewMacro(vtkStructuredGridOutlineFilter);

//----------------------------------------------------------------------------
// ComputeDivisionExtents has done most of the work for us.
// Now just connect the points.
void vtkStructuredGridOutlineFilter::Execute()
{
  vtkStructuredGrid *input=this->GetInput();
  int *ext, *wExt, cExt[6];
  int xInc, yInc, zInc;
  vtkPoints *inPts;
  vtkPoints *newPts;
  vtkCellArray *newLines;
  vtkPolyData *output=this->GetOutput();
  int idx;
  vtkIdType ids[2], numPts, offset;
  // for marching through the points along an edge.
  vtkIdType start = 0, id;
  int num = 0, inc = 0;
  int edgeFlag;
  int i;

  newLines = vtkCellArray::New();
  newPts = vtkPoints::New();
  inPts = input->GetPoints();

  ext = input->GetExtent();
  wExt = input->GetWholeExtent();

  // Since it is possible that the extent is larger than the whole extent,
  // and we want the outline to be the whole extent, 
  // compute the clipped extent.
  input->GetExtent(cExt);
  for (i = 0; i < 3; ++i)
    {
    if (cExt[2*i] < wExt[2*i])
      {
      cExt[2*i] = wExt[2*i];
      }
    if (cExt[2*i+1] > wExt[2*i+1])
      {
      cExt[2*i+1] = wExt[2*i+1];
      }
    }

  for (i = 0; i < 12; i++ )
    {  
    // Find the start of this edge, the length of this edge, and increment.
    ext = input->GetExtent();
    xInc = 1;
    yInc = ext[1]-ext[0]+1;
    zInc = yInc * (ext[3]-ext[2]+1);
    edgeFlag = 0;
    switch (i)
      {
      case 0:
        // start (0, 0, 0) increment z axis.
        if (ext[0] <= wExt[0] && ext[2] <= wExt[2])
          {
          edgeFlag = 1;
          }
        num = cExt[5]-cExt[4]+1;
        start = (wExt[0]-ext[0])*xInc + (wExt[2]-ext[2])*yInc + (cExt[4]-ext[4])*zInc;
        inc = zInc;
      break;
      case 1:
            // start (xMax, 0, 0) increment z axis.
        if (ext[1] >= wExt[1] && ext[2] <= wExt[2])
          {
          edgeFlag = 1;
          }
        num = cExt[5]-cExt[4]+1;
        start = (wExt[1]-ext[0])*xInc + (wExt[2]-ext[2])*yInc + (cExt[4]-ext[4])*zInc;
        inc = zInc;
        break;
      case 2:
        // start (0, yMax, 0) increment z axis.
        if (ext[0] <= wExt[0] && ext[3] >= wExt[3])
          {
          edgeFlag = 1;
          }
        num = cExt[5]-cExt[4]+1;
        start = (wExt[0]-ext[0])*xInc + (wExt[3]-ext[2])*yInc + (cExt[4]-ext[4])*zInc;
        inc = zInc;
        break;
      case 3:
        // start (xMax, yMax, 0) increment z axis.
        if (ext[1] >= wExt[1] && ext[3] >= wExt[3])
          {
          edgeFlag = 1;
          }
        num = cExt[5]-cExt[4]+1;
        start = (wExt[1]-ext[0])*xInc + (wExt[3]-ext[2])*yInc + (cExt[4]-ext[4])*zInc;
        inc = zInc;
        break;
      case 4:
        // start (0, 0, 0) increment y axis.
        if (ext[0] <= wExt[0] && ext[4] <= wExt[4])
          {
          edgeFlag = 1;
          }
        num = cExt[3]-cExt[2]+1;
        start = (wExt[0]-ext[0])*xInc + (cExt[2]-ext[2])*yInc + (wExt[4]-ext[4])*zInc;
        inc = yInc;
        break;
      case 5:
        // start (xMax, 0, 0) increment y axis.
        if (ext[1] >= wExt[1] && ext[4] <= wExt[4])
          {
          edgeFlag = 1;
          }
        num = cExt[3]-cExt[2]+1;
        start = (wExt[1]-ext[0])*xInc + (cExt[2]-ext[2])*yInc + (wExt[4]-ext[4])*zInc;
        inc = yInc;
        break;
      case 6:
        // start (0, 0, zMax) increment y axis.
        if (ext[0] <= wExt[0] && ext[5] >= wExt[5])
          {
          edgeFlag = 1;
          }
        num = cExt[3]-cExt[2]+1;
        start = (wExt[0]-ext[0])*xInc + (cExt[2]-ext[2])*yInc + (wExt[5]-ext[4])*zInc;
        inc = yInc;
        break;
      case 7:
        // start (xMax, 0, zMax) increment y axis.
        if (ext[1] >= wExt[1] && ext[5] >= wExt[5])
          {
          edgeFlag = 1;
          }
        num = cExt[3]-cExt[2]+1;
        start = (wExt[1]-ext[0])*xInc + (cExt[2]-ext[2])*yInc + (wExt[5]-ext[4])*zInc;
        inc = yInc;
        break;
      case 8:
        // start (0, 0, 0) increment x axis.
        if (ext[2] <= wExt[2] && ext[4] <= wExt[4])
          {
          edgeFlag = 1;
          }
        num = cExt[1]-cExt[0]+1;
        start = (cExt[0]-ext[0])*xInc + (wExt[2]-ext[2])*yInc + (wExt[4]-ext[4])*zInc;
        inc = xInc;
        break;
      case 9:
        // start (0, yMax, 0) increment x axis.
        if (ext[3] >= wExt[3] && ext[4] <= wExt[4])
          {
          edgeFlag = 1;
          }
        num = cExt[1]-cExt[0]+1;
        start = (cExt[0]-ext[0])*xInc + (wExt[3]-ext[2])*yInc + (wExt[4]-ext[4])*zInc;
        inc = xInc;
        break;
      case 10:
        // start (0, 0, zMax) increment x axis.
        if (ext[2] <= wExt[2] && ext[5] >= wExt[5])
          {
          edgeFlag = 1;
          }
        num = cExt[1]-cExt[0]+1;
        start = (cExt[0]-ext[0])*xInc + (wExt[2]-ext[2])*yInc + (wExt[5]-ext[4])*zInc;
        inc = xInc;
        break;
      case 11:
        // start (0, yMax, zMax) increment x axis.
        if (ext[3] >= wExt[3] && ext[5] >= wExt[5])
          {
          edgeFlag = 1;
          }
        num = cExt[1]-cExt[0]+1;
        start = (cExt[0]-ext[0])*xInc + (wExt[3]-ext[2])*yInc + (wExt[5]-ext[4])*zInc;
        inc = xInc;
        break;
      }
    
    if (edgeFlag && num > 1)
      {
      offset = newPts->GetNumberOfPoints();
      numPts = inPts->GetNumberOfPoints();

      // add points
      for (idx = 0; idx < num; ++idx)
        {
        id = start + idx * inc;
        // sanity check
        if (id < 0 || id >= numPts)
          {
          vtkErrorMacro("Error stepping through points.");
          return;
          }
        newPts->InsertNextPoint(inPts->GetPoint(id));
        }

      // add lines
      for (idx = 1; idx < num; ++idx)
        {
        ids[0] = idx+offset-1;
        ids[1] = idx+offset;
        newLines->InsertNextCell(2, ids);
        }
      }
    }

  output->SetPoints(newPts);
  newPts->Delete();
  output->SetLines(newLines);
  newLines->Delete();
}

