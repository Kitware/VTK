/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredGridGeometryFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStructuredGridGeometryFilter.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkExtentTranslator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"

vtkStandardNewMacro(vtkStructuredGridGeometryFilter);

// Construct with initial extent of all the data
vtkStructuredGridGeometryFilter::vtkStructuredGridGeometryFilter()
{
  this->Extent[0] = 0;
  this->Extent[1] = VTK_LARGE_INTEGER;
  this->Extent[2] = 0;
  this->Extent[3] = VTK_LARGE_INTEGER;
  this->Extent[4] = 0;
  this->Extent[5] = VTK_LARGE_INTEGER;
}

int vtkStructuredGridGeometryFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkStructuredGrid *input = vtkStructuredGrid::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int *dims, dimension, dir[3], diff[3];
  int i, j, k, extent[6], *inExt;
  vtkIdType ptIds[4], idx, startIdx, startCellIdx, cellId;
  vtkPoints *newPts=0;
  vtkCellArray *newVerts=0;
  vtkCellArray *newLines=0;
  vtkCellArray *newPolys=0;
  vtkIdType totPoints, pos, cellPos;
  int offset[3], cellOffset[3], numPolys;
  double x[3];
  vtkPointData *pd, *outPD;
  vtkCellData *cd, *outCD;

  vtkDebugMacro(<< "Extracting structured points geometry");

  if ( input->GetPoints() == NULL)
    {
    vtkDebugMacro(<<"No data to extract");
    return 1;
    }

  pd = input->GetPointData();
  outPD = output->GetPointData();
  outPD->CopyNormalsOff();
  cd = input->GetCellData();
  outCD = output->GetCellData();
  dims = input->GetDimensions();
  inExt = input->GetExtent();
  
  // Based on the dimensions of the structured data, and the extent of 
  // the geometry, compute the combined extent plus the dimensionality 
  // of the data.
  //
  dimension = 3;
  for (i=0; i<3; i++)
    {
    extent[2*i] = this->Extent[2*i];
    if (extent[2*i] < inExt[2*i])
      {
      extent[2*i] = inExt[2*i];
      }
    extent[2*i+1] = this->Extent[2*i+1];
    if (extent[2*i+1] > inExt[2*i+1])
      {
      extent[2*i+1] = inExt[2*i+1];
      }
    
    // Handle empty extent.
    if (extent[2*i] > extent[2*i+1])
      {
      return 1;
      }
    
    // Compute dimensions.
    if ( (extent[2*i+1] - extent[2*i]) == 0 )
      {
      dimension--;
      }
    }
  
  // The easiest way to handle the rest of this is to use the "electric slide".
  // Translate the input extent so that it has minimums 0, 0, 0.  
  // It is only internal to this method, so it is OK.
  extent[0] -= inExt[0];
  extent[1] -= inExt[0];
  extent[2] -= inExt[2];
  extent[3] -= inExt[2];
  extent[4] -= inExt[4];
  extent[5] -= inExt[4];
  
  // Now create polygonal data based on dimension of data
  //
  // Compute starting index of the point and cell. First the starting
  // point index.
  startIdx = (extent[0]) + (extent[2])*dims[0] 
              + (extent[4])*dims[0]*dims[1];

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

      if ( input->IsPointVisible(startIdx) )
        {
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
        }
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

      //  Load data
      //
      if ( dir[0] == 0 ) 
        {
        offset[0] = 1;
        cellOffset[0] = 1;
        }
      else if (dir[0] == 1)
        {
        offset[0] = dims[0];
        cellOffset[0] = dims[0] - 1;
        }
      else
        {
        offset[0] = dims[0]*dims[1];
        cellOffset[0] = (dims[0] - 1) * (dims[1] - 1);
        }

      for (i=0; i<totPoints; i++) 
        {
        idx = startIdx + i*offset[0];
        input->GetPoint(idx, x);
        ptIds[0] = newPts->InsertNextPoint(x);
        outPD->CopyData(pd,idx,ptIds[0]);
        }

      for (i=0; i<(totPoints-1); i++) 
        {
        if ( input->IsPointVisible(startIdx + i*offset[0]) &&
             input->IsPointVisible(startIdx + (i+1)*offset[0]) )
          {
          idx = startCellIdx + i*cellOffset[0];
          ptIds[0] = i;
          ptIds[1] = i + 1;
          cellId = newLines->InsertNextCell(2,ptIds);
          outCD->CopyData(cd,idx,cellId);
          }
        }
      break;

    case 2: // --------------------- build plane -----------------------

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

      //  Create polygons
      //
      for (i=0; i<2; i++) 
        {
        if ( dir[i] == 0 )
          {
          offset[i] = 1;
          cellOffset[i] = 1;
          }
        else if ( dir[i] == 1 )
          {
          offset[i] = dims[0];
          cellOffset[i] = (dims[0] - 1);
          }
        else if ( dir[i] == 2 )
          {
          offset[i] = dims[0]*dims[1];
          cellOffset[i] = (dims[0] - 1) * (dims[1] - 1);
          }
        }

      // Create points whether visible or not.  Makes coding easier 
      // but generates extra data.
      for (pos=startIdx, j=0; j < (diff[dir[1]]+1); j++) 
        {
        for (i=0; i < (diff[dir[0]]+1); i++) 
          {
          idx = pos + i*offset[0];
          input->GetPoint(idx, x);
          ptIds[0] = newPts->InsertNextPoint(x);
          outPD->CopyData(pd,idx,ptIds[0]);
          }
        pos += offset[1];
        }

      for (pos=startIdx, cellPos=startCellIdx, j=0; j < diff[dir[1]]; j++) 
        {
        for (i=0; i < diff[dir[0]]; i++) 
          {
          if (input->IsPointVisible(pos+i*offset[0])
          && input->IsPointVisible(pos+(i+1)*offset[0])
          && input->IsPointVisible(pos+i*offset[0]+offset[1]) 
          && input->IsPointVisible(pos+(i+1)*offset[0]+offset[1]) ) 
            {
            idx = cellPos + i*cellOffset[0];
            ptIds[0] = i + j*(diff[dir[0]]+1);
            ptIds[1] = ptIds[0] + 1;
            ptIds[2] = ptIds[1] + diff[dir[0]] + 1;
            ptIds[3] = ptIds[2] - 1;
            cellId = newPolys->InsertNextCell(4,ptIds);
            outCD->CopyData(cd,idx,cellId);
            }
          }
        cellPos += cellOffset[1];
        pos += offset[1];
        }
      break;

    case 3: // ------------------- grab points in volume  --------------

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

      // Create vertices
      //
      offset[0] = dims[0];
      offset[1] = dims[0]*dims[1];

      for (k=0; k < (diff[2]+1); k++) 
        {
        for (j=0; j < (diff[1]+1); j++) 
          {
          pos = startIdx + j*offset[0] + k*offset[1];
          for (i=0; i < (diff[0]+1); i++) 
            {
            if ( input->IsPointVisible(pos+i) ) 
              {
              input->GetPoint(pos+i, x);
              ptIds[0] = newPts->InsertNextPoint(x);
              outPD->CopyData(pd,pos+i,ptIds[0]);
              cellId = newVerts->InsertNextCell(1,ptIds);
              outCD->CopyData(cd,pos+i,cellId);
              }
            }
          }
        }
        break; /* end this case */

    } // switch

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

  return 1;
}

// Specify (imin,imax, jmin,jmax, kmin,kmax) indices.
void vtkStructuredGridGeometryFilter::SetExtent(int iMin, int iMax, int jMin,
                                                int jMax, int kMin, int kMax)
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

// Specify (imin,imax, jmin,jmax, kmin,kmax) indices in array form.
void vtkStructuredGridGeometryFilter::SetExtent(int extent[6])
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

int vtkStructuredGridGeometryFilter::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  int piece, numPieces;
  int *wholeExt;
  int ext[6];
  vtkExtentTranslator *translator;
  
  translator = vtkExtentTranslator::SafeDownCast(
    inInfo->Get(vtkStreamingDemandDrivenPipeline::EXTENT_TRANSLATOR()));
  wholeExt =
    inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  memcpy(ext, wholeExt, 6*sizeof(int));
  
  // Get request from output information
  piece =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numPieces =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  // get the extent associated with the piece.
  if (translator == NULL)
    {
    // Default behavior
    if (piece != 0)
      {
      ext[0] = ext[2] = ext[4] = 0;
      ext[1] = ext[3] = ext[5] = -1;
      }
    }
  else
    {    
    translator->PieceToExtentThreadSafe(piece, numPieces, 0, wholeExt, ext, 
                                        translator->GetSplitMode(),0);
    }
  
  if (ext[0] < this->Extent[0])
    {
    ext[0] = this->Extent[0];
    }
  if (ext[1] > this->Extent[1])
    {
    ext[1] = this->Extent[1];
    }
  if (ext[2] < this->Extent[2])
    {
    ext[2] = this->Extent[2];
    }
  if (ext[3] > this->Extent[3])
    {
    ext[3] = this->Extent[3];
    }
  if (ext[4] < this->Extent[4])
    {
    ext[4] = this->Extent[4];
    }
  if (ext[5] > this->Extent[5])
    {
    ext[5] = this->Extent[5];
    }

  // Should not be necessary, but will make things clearer.
  if (ext[0] > ext[1] || ext[2] > ext[3] || ext[4] > ext[5])
    {
    ext[0] = ext[2] = ext[4] = 0;
    ext[1] = ext[3] = ext[5] = -1;
    }
  
  // Set the update extent of the input.
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), ext, 6);
  return 1;
}

int vtkStructuredGridGeometryFilter::FillInputPortInformation(
  int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkStructuredGrid");
  return 1;
}

void vtkStructuredGridGeometryFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Extent: \n";
  os << indent << "  Imin,Imax: (" 
     << this->Extent[0] << ", " << this->Extent[1] << ")\n";
  os << indent << "  Jmin,Jmax: (" 
     << this->Extent[2] << ", " << this->Extent[3] << ")\n";
  os << indent << "  Kmin,Kmax: (" 
     << this->Extent[4] << ", " << this->Extent[5] << ")\n";
}
