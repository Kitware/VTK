/*=========================================================================

  Program:   Visualization Library
  Module:    SGGeomF.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "StrGeom.hh"

vlStructuredGeometry::vlStructuredGeometry()
{
  this->Extent[0] = 0;
  this->Extent[1] = 100;
  this->Extent[2] = 0;
  this->Extent[3] = 100;
  this->Extent[4] = 0;
  this->Extent[5] = 0;
}


void vlStructuredGeometry::Execute()
{
  vlPointData *pd;
  int *dims, dimension, dir[3], diff[3];
  int i, j, k, extent[6];
  int ptIds[4], idx, startIdx;
  vlFloatPoints *newPts=0;
  vlCellArray *newVerts=0;
  vlCellArray *newLines=0;
  vlCellArray *newPolys=0;
  int totPoints, numPolys;
  int offset[3], pos;
  float *x;

  vlDebugMacro(<< "Creating structured geometry");
//
// Initialize
//
  this->Initialize();

  pd = this->Input->GetPointData();
  dims = this->Input->GetDimensions();
//
// Based on the dimensions of the structured data, and the extent of the geometry,
// compute the combined extent plus the dimensionality of the data
//
  for (dimension=3, i=0; i<3; i++)
    {
    extent[2*i] = this->Extent[2*i] < 0 ? 0 : this->Extent[2*i];
    extent[2*i] = this->Extent[2*i] >= dims[i] ? dims[i]-1 : this->Extent[2*i];
    extent[2*i+1] = this->Extent[2*i+1] >= dims[i] ? dims[i]-1 : this->Extent[2*i+1];
    if ( extent[2*i+1] < extent[2*i] ) extent[2*i+1] = extent[2*i];
    if ( (extent[2*i+1] - extent[2*i]) == 0 ) dimension--;
    }
//
// Now create polygonal data based on dimension of data
//
  startIdx = extent[0] + extent[2]*dims[0] + extent[4]*dims[0]*dims[1];

  switch (dimension) 
    {
    default:
      break;

    case 0: // --------------------- build point -----------------------

      if ( this->Input->IsPointVisible(startIdx) )
        {
        newPts = new vlFloatPoints(1);
        newVerts = new vlCellArray;
        newVerts->Allocate(newVerts->EstimateSize(1,1));
        this->PointData.CopyAllocate(pd,1);

        ptIds[0] = newPts->InsertNextPoint(this->Input->GetPoint(startIdx));
        this->PointData.CopyData(pd,startIdx,ptIds[0]);
        newVerts->InsertNextCell(1,ptIds);
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
      newPts = new vlFloatPoints(totPoints);
      newLines = new vlCellArray;
      newLines->Allocate(newLines->EstimateSize(totPoints-1,2));
      this->PointData.CopyAllocate(pd,totPoints);
//
//  Load data
//
      if ( dir[0] == 0 ) 
        offset[0] = 1;
      else if (dir[0] == 1)
        offset[0] = dims[0];
      else
        offset[0] = dims[0]*dims[1];

      for (i=0; i<totPoints; i++) 
        {
        idx = startIdx + i*offset[0];
        x = this->Input->GetPoint(idx);
        ptIds[0] = newPts->InsertNextPoint(x);
        this->PointData.CopyData(pd,idx,ptIds[0]);
        }

      for (idx=0,i=0; i<(totPoints-1); i++) 
        {
        if ( this->Input->IsPointVisible(idx) || this->Input->IsPointVisible(idx+offset[0]) )
          {
          ptIds[0] = i;
          ptIds[1] = i + 1;
          newLines->InsertNextCell(2,ptIds);
          }
        }
      break;

    case 2: // --------------------- build plane -----------------------
//
//  Create the data objects
//
      for (dir[0]=dir[1]=dir[2]=idx=0,i=0; i<3; i++)
        {
        if ( (diff[i] = extent[2*i+1] - extent[2*i]) != 0 )
          dir[idx++] = i;
        else
          dir[2] = i;
        }

      totPoints = (diff[dir[0]]+1) * (diff[dir[1]]+1);
      numPolys = diff[dir[0]]  * diff[dir[1]];

      newPts = new vlFloatPoints(totPoints);
      newPolys = new vlCellArray;
      newPolys->Allocate(newLines->EstimateSize(numPolys,4));
      this->PointData.CopyAllocate(pd,totPoints);
//
//  Create polygons
//
      for (i=0; i<2; i++) 
        {
        if ( dir[i] == 0 )
          offset[i] = 1;
        else if ( dir[i] == 1 )
          offset[i] = dims[0];
        else if ( dir[i] == 2 )
          offset[i] = dims[0]*dims[1];
        }

      // create points whether visible or not.  Makes coding easier but generates
      // extra data.
      for (pos=startIdx, j=0; j < (diff[dir[1]]+1); j++) 
        {
        for (i=0; i < (diff[dir[0]]+1); i++) 
          {
          idx = pos + i*offset[0];
          x = this->Input->GetPoint(idx);
          ptIds[0] = newPts->InsertNextPoint(x);
          this->PointData.CopyData(pd,idx,ptIds[0]);
          }
        pos += offset[1];
        }

      // create any polygon who has a visible vertex.  To turn off a polygon, all 
      // vertices have to be blanked.
      for (pos=startIdx, j=0; j < diff[dir[1]]; j++) 
        {
        for (i=0; i < diff[dir[0]]; i++) 
          {
          if (this->Input->IsPointVisible(pos+i*offset[0])
          || this->Input->IsPointVisible(pos+(i+1)*offset[0])
          || this->Input->IsPointVisible(pos+i*offset[0]+offset[1]) 
          || this->Input->IsPointVisible(pos+(i+1)*offset[0]+offset[1]) ) 
            {
            ptIds[0] = i + j*(diff[dir[0]]+1);
            ptIds[1] = ptIds[0] + 1;
            ptIds[2] = ptIds[1] + diff[dir[0]] + 1;
            ptIds[3] = ptIds[2] - 1;
            newPolys->InsertNextCell(4,ptIds);
            }
          }
        pos += offset[1];
        }
      break;

    case 3: // ------------------- grab points in volume  --------------

//
// Create data objects
//
      for (i=0; i<3; i++) diff[i] = extent[2*i+1] - extent[2*i];

      totPoints = (diff[0]+1) * (diff[1]+1) * (diff[2]+1);

      newPts = new vlFloatPoints(totPoints);
      newVerts = new vlCellArray;
      newVerts->Allocate(newVerts->EstimateSize(totPoints,1));
      this->PointData.CopyAllocate(pd,totPoints);
//
// Create vertices
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
            if ( this->Input->IsPointVisible(pos+i) ) 
              {
              x = this->Input->GetPoint(pos+i);
              ptIds[0] = newPts->InsertNextPoint(x);
              this->PointData.CopyData(pd,idx,ptIds[0]);
              newVerts->InsertNextCell(1,ptIds);
              }
            }
          }
        }
        break; /* end this case */

    } // switch
//
// Update self
//
  this->SetPoints(newPts);
  this->SetVerts(newVerts);
  this->SetLines(newLines);
  this->SetPolys(newPolys);
}

void vlStructuredGeometry::SetExtent(int iMin, int iMax, int jMin, int jMax, 
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

void vlStructuredGeometry::SetExtent(int *extent)
{
  int i;

  if ( extent[0] != this->Extent[0] || extent[1] != this->Extent[1] ||
  extent[2] != this->Extent[2] || extent[3] != this->Extent[3] ||
  extent[4] != this->Extent[4] || extent[5] != this->Extent[5] )
    {
    this->Modified();
    for (i=0; i<3; i++)
      {
      if ( extent[2*i] < 0 ) extent[2*i] = 0;
      if ( extent[2*i+1] < extent[2*i] ) extent[2*i+1] = extent[2*i];
      this->Extent[2*i] = extent[2*i];
      this->Extent[2*i+1] = extent[2*i+1];
      }
    }
}
