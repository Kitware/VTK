/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredGridOutlineFilter.cxx
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
#include "vtkStructuredGridOutlineFilter.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStructuredGridOutlineFilter* vtkStructuredGridOutlineFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkStructuredGridOutlineFilter");
  if(ret)
    {
    return (vtkStructuredGridOutlineFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkStructuredGridOutlineFilter;
}

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

