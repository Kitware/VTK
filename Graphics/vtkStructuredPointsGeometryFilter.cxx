/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredPointsGeometryFilter.cxx
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
#include "vtkStructuredPointsGeometryFilter.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkStructuredPointsGeometryFilter* vtkStructuredPointsGeometryFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkStructuredPointsGeometryFilter");
  if(ret)
    {
    return (vtkStructuredPointsGeometryFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkStructuredPointsGeometryFilter;
}




// Construct with initial extent of all the data
vtkStructuredPointsGeometryFilter::vtkStructuredPointsGeometryFilter()
{
  this->Extent[0] = 0;
  this->Extent[1] = VTK_LARGE_INTEGER;
  this->Extent[2] = 0;
  this->Extent[3] = VTK_LARGE_INTEGER;
  this->Extent[4] = 0;
  this->Extent[5] = VTK_LARGE_INTEGER;
}

void vtkStructuredPointsGeometryFilter::Execute()
{
  int *dims, dimension, dir[3], diff[3];
  int i, j, k, extent[6];
  int idx, startIdx, startCellIdx;
  vtkIdType ptIds[4];
  int cellId;
  vtkPoints *newPts=0;
  vtkCellArray *newVerts=0;
  vtkCellArray *newLines=0;
  vtkCellArray *newPolys=0;
  int totPoints, numPolys;
  int offset[3], pos;
  float *x;
  vtkPointData *pd, *outPD;
  vtkCellData *cd, *outCD;
  vtkImageData *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();

  vtkDebugMacro(<< "Extracting structured points geometry");

  pd = input->GetPointData();
  outPD = output->GetPointData();
  cd = input->GetCellData();
  outCD = output->GetCellData();
//  this->PointData.CopyNormalsOff();
  dims = input->GetDimensions();
//
// Based on the dimensions of the structured data, and the extent of the geometry,
// compute the combined extent plus the dimensionality of the data
//
  for (dimension=3, i=0; i<3; i++)
    {
    extent[2*i] = this->Extent[2*i] < 0 ? 0 : this->Extent[2*i];
    extent[2*i] = this->Extent[2*i] >= dims[i] ? dims[i]-1 : this->Extent[2*i];
    extent[2*i+1] = this->Extent[2*i+1] >= dims[i] ? dims[i]-1 : this->Extent[2*i+1];
    if ( extent[2*i+1] < extent[2*i] )
      {
      extent[2*i+1] = extent[2*i];
      }
    if ( (extent[2*i+1] - extent[2*i]) == 0 )
      {
      dimension--;
      }
    }
//
// Now create polygonal data based on dimension of data
//
  startIdx = extent[0] + extent[2]*dims[0] + extent[4]*dims[0]*dims[1];
// The cell index is a bit more complicated at the boundaries
  if (dims[0] == 1)
    {
    startCellIdx = extent[0];
    }
  else
    {
    startCellIdx =  (extent[0] < dims[0] - 1) ? extent[0]
                                              : extent[0]-1;
    }
  if (dims[1] == 1)
    {
    startCellIdx += extent[2]*(dims[0]-1);
    }
  else
    {
    startCellIdx += (extent[2] < dims[1] - 1) ? extent[2]*(dims[0]-1)
                                              : (extent[2]-1)*(dims[0]-1);
    }
  if (dims[2] == 1)
    {
    startCellIdx += extent[4]*(dims[0]-1)*(dims[1]-1);
    }
  else
    {
    startCellIdx += (extent[4] < dims[2] - 1) ? extent[4]*(dims[0]-1)*(dims[1]-1)
                                              : (extent[4]-1)*(dims[0]-1)*(dims[1]-1);
    }

  switch (dimension) 
    {
    default:
      break;

    case 0: // --------------------- build point -----------------------

      newPts = vtkPoints::New();
      newPts->Allocate(1);
      newVerts = vtkCellArray::New();
      newVerts->Allocate(newVerts->EstimateSize(1,1));
      outPD->CopyAllocate(pd,1);
      outCD->CopyAllocate(cd,1);

      ptIds[0] = newPts->InsertNextPoint(input->GetPoint(startIdx));
      outPD->CopyData(pd,startIdx,ptIds[0]);

      cellId = newVerts->InsertNextCell(1,ptIds);
      outCD->CopyData(cd,startIdx,cellId);
      break;

    case 1: // --------------------- build line -----------------------

      for (dir[0]=dir[1]=dir[2]=totPoints=0, i=0; i<3; i++)
        {
        if ( (diff[i] = extent[2*i+1] - extent[2*i]) > 0 ) 
          {
          dir[0] = i;
          totPoints = diff[i] + 1;
          break;
          }
        }
      newPts = vtkPoints::New();
      newPts->Allocate(totPoints);
      newLines = vtkCellArray::New();
      newLines->Allocate(newLines->EstimateSize(totPoints-1,2));
      outPD->CopyAllocate(pd,totPoints);
      outCD->CopyAllocate(cd,totPoints - 1);
//
//  Load data
//
      if ( dir[0] == 0 ) 
	{
        offset[0] = 1;
	}
      else if (dir[0] == 1)
	{
        offset[0] = dims[0];
	}
      else
	{
        offset[0] = dims[0]*dims[1];
	}

      for (i=0; i<totPoints; i++) 
        {
        idx = startIdx + i*offset[0];
        x = input->GetPoint(idx);
        ptIds[0] = newPts->InsertNextPoint(x);
        outPD->CopyData(pd,idx,ptIds[0]);
        }

      if ( dir[0] == 0 ) 
	{
        offset[0] = 1;
	}
      else if (dir[0] == 1)
	{
        offset[0] = dims[0] - 1;
	}
      else
	{
        offset[0] = (dims[0] - 1) * (dims[1] - 1);
	}

      for (i=0; i<(totPoints-1); i++) 
        {
        idx = startCellIdx + i*offset[0];
        ptIds[0] = i;
        ptIds[1] = i + 1;
        cellId = newLines->InsertNextCell(2,ptIds);
        outCD->CopyData(cd,idx,cellId);
        }
      break;

    case 2: // --------------------- build plane -----------------------
//
//  Create the data objects
//
      for (dir[0]=dir[1]=dir[2]=idx=0,i=0; i<3; i++)
        {
        if ( (diff[i] = extent[2*i+1] - extent[2*i]) != 0 )
	  {
          dir[idx++] = i;
	  }
        else
	  {
          dir[2] = i;
	  }
        }

      totPoints = (diff[dir[0]]+1) * (diff[dir[1]]+1);
      numPolys = diff[dir[0]]  * diff[dir[1]];

      newPts = vtkPoints::New();
      newPts->Allocate(totPoints);
      newPolys = vtkCellArray::New();
      newPolys->Allocate(newLines->EstimateSize(numPolys,4));
      outPD->CopyAllocate(pd,totPoints);
      outCD->CopyAllocate(cd,numPolys);
//
//  Create vertices
//
      for (i=0; i<2; i++) 
        {
        if ( dir[i] == 0 )
	  {
          offset[i] = 1;
	  }
        else if ( dir[i] == 1 )
	  {
          offset[i] = dims[0];
	  }
        else if ( dir[i] == 2 )
	  {
          offset[i] = dims[0]*dims[1];
	  }
        }

      for (pos=startIdx, j=0; j < (diff[dir[1]]+1); j++) 
        {
        for (i=0; i < (diff[dir[0]]+1); i++) 
          {
          idx = pos + i*offset[0];
          x = input->GetPoint(idx);
          ptIds[0] = newPts->InsertNextPoint(x);
          outPD->CopyData(pd,idx,ptIds[0]);
          }
        pos += offset[1];
        }

//
//  Create cells
//
      for (i=0; i<2; i++) 
        {
        if ( dir[i] == 0 )
	  {
          offset[i] = 1;
	  }
        else if ( dir[i] == 1 )
	  {
          offset[i] = (dims[0] - 1);
	  }
        else if ( dir[i] == 2 )
	  {
          offset[i] = (dims[0] - 1) * (dims[1] - 1);
	  }
        }

      for (pos=startCellIdx, j=0; j < diff[dir[1]]; j++) 
        {
        for (i=0; i < diff[dir[0]]; i++) 
          {
          idx = pos + i*offset[0];
          ptIds[0] = i + j*(diff[dir[0]]+1);
          ptIds[1] = ptIds[0] + 1;
          ptIds[2] = ptIds[1] + diff[dir[0]] + 1;
          ptIds[3] = ptIds[2] - 1;
          cellId = newPolys->InsertNextCell(4,ptIds);
          outCD->CopyData(cd,idx,cellId);
          }
        pos += offset[1];
        }
      break;

    case 3: // ------------------- grab points in volume  --------------

//
// Create data objects
//
      for (i=0; i<3; i++)
	{
	  diff[i] = extent[2*i+1] - extent[2*i];
	}
      totPoints = (diff[0]+1) * (diff[1]+1) * (diff[2]+1);

      newPts = vtkPoints::New();
      newPts->Allocate(totPoints);
      newVerts = vtkCellArray::New();
      newVerts->Allocate(newVerts->EstimateSize(totPoints,1));
      outPD->CopyAllocate(pd,totPoints);
      outCD->CopyAllocate(cd,totPoints);
//
// Create vertices and cells
//
      offset[0] = dims[0];
      offset[1] = dims[0]*dims[1];

      for (pos=startIdx, k=0; k < (diff[2]+1); k++) 
        {
        for (j=0; j < (diff[1]+1); j++) 
          {
          pos = startIdx + j*offset[0] + k*offset[1];
          for (i=0; i < (diff[0]+1); i++) 
            {
            x = input->GetPoint(pos+i);
            ptIds[0] = newPts->InsertNextPoint(x);
            outPD->CopyData(pd,pos+i,ptIds[0]);
            cellId = newVerts->InsertNextCell(1,ptIds);
            outCD->CopyData(cd,pos+i,cellId);
            }
          }
        }
        break; /* end this case */

    } // switch
//
// Update self and release memory
//
  if (newPts)
    {
    output->SetPoints(newPts);
    newPts->Delete();
    }

  if (newVerts)
    {
    output->SetVerts(newVerts);
    newVerts->Delete();
    }

  if (newLines)
    {
    output->SetLines(newLines);
    newLines->Delete();
    }

  if (newPolys)
    {
    output->SetPolys(newPolys);
    newPolys->Delete();
    }
}

void vtkStructuredPointsGeometryFilter::SetExtent(int iMin, int iMax, 
                                                 int jMin, int jMax, 
                                                 int kMin, int kMax)
{
  int extent[6];

  extent[0] = iMin;
  extent[1] = iMax;
  extent[2] = jMin;
  extent[3] = jMax;
  extent[4] = kMin;
  extent[5] = kMax;

  this->SetExtent(extent);
}

// Specify (imin,imax, jmin,jmax, kmin,kmax) indices.
void vtkStructuredPointsGeometryFilter::SetExtent(int extent[6])
{
  int i;

  if ( extent[0] != this->Extent[0] || extent[1] != this->Extent[1] ||
  extent[2] != this->Extent[2] || extent[3] != this->Extent[3] ||
  extent[4] != this->Extent[4] || extent[5] != this->Extent[5] )
    {
    this->Modified();
    for (i=0; i<3; i++)
      {
      if ( extent[2*i] < 0 )
	{
	extent[2*i] = 0;
	}
      if ( extent[2*i+1] < extent[2*i] )
	{
	extent[2*i+1] = extent[2*i];
	}
      this->Extent[2*i] = extent[2*i];
      this->Extent[2*i+1] = extent[2*i+1];
      }
    }
}

void vtkStructuredPointsGeometryFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStructuredPointsToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Extent: \n";
  os << indent << "  Imin,Imax: (" << this->Extent[0] << ", " << this->Extent[1] << ")\n";
  os << indent << "  Jmin,Jmax: (" << this->Extent[2] << ", " << this->Extent[3] << ")\n";
  os << indent << "  Kmin,Kmax: (" << this->Extent[4] << ", " << this->Extent[5] << ")\n";
}
