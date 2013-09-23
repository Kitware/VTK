/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStructuredData.h"

#include "vtkIdList.h"
#include "vtkObjectFactory.h"
#include "vtkStructuredExtent.h"
#include <cassert>


// Return the topological dimension of the data (e.g., 0, 1, 2, or 3D).
int vtkStructuredData::GetDataDimension(int dataDescription)
{
  switch (dataDescription)
    {
    case VTK_EMPTY:
      return 0;  // Should I put -1?
    case VTK_SINGLE_POINT:
      return 0;
    case VTK_X_LINE:
    case VTK_Y_LINE:
    case VTK_Z_LINE:
      return 1;
    case VTK_XY_PLANE:
    case VTK_YZ_PLANE:
    case VTK_XZ_PLANE:
      return 2;
    case VTK_XYZ_GRID:
      return 3;
    default:
      return -1;
    }
}

//------------------------------------------------------------------------------
int vtkStructuredData::GetNumberOfNodes( int ext[6], int dataDescription )
{
  if( dataDescription == VTK_EMPTY )
    {
    dataDescription = vtkStructuredData::GetDataDescriptionFromExtent( ext );
    }

  int N = 0;
  int nodeDims[3];
  vtkStructuredData::GetDimensionsFromExtent( ext,nodeDims,dataDescription );

  switch( dataDescription )
    {
    case VTK_SINGLE_POINT:
      N = 1;
      break;
    case VTK_X_LINE:
      N = nodeDims[0];
      break;
    case VTK_Y_LINE:
      N = nodeDims[1];
      break;
    case VTK_Z_LINE:
      N = nodeDims[2];
      break;
    case VTK_XY_PLANE:
      N = nodeDims[0]*nodeDims[1];
      break;
    case VTK_YZ_PLANE:
      N = nodeDims[1]*nodeDims[2];
      break;
    case VTK_XZ_PLANE:
      N = nodeDims[0]*nodeDims[2];
      break;
    case VTK_XYZ_GRID:
      N = nodeDims[0]*nodeDims[1]*nodeDims[2];
      break;
    default:
      vtkGenericWarningMacro("Undefined data description!");
    }

  return( N );
}

//------------------------------------------------------------------------------
int vtkStructuredData::GetNumberOfCells( int ext[6], int dataDescription )
{
  if( dataDescription == VTK_EMPTY )
    {
    dataDescription = vtkStructuredData::GetDataDescriptionFromExtent( ext );
    }

  int N = 0;
  int cellDims[3];
  vtkStructuredData::GetCellDimensionsFromExtent(ext,cellDims,dataDescription);

  switch( dataDescription )
    {
    case VTK_SINGLE_POINT:
      N = 0;
      break;
    case VTK_X_LINE:
      N = cellDims[0];
      break;
    case VTK_Y_LINE:
      N = cellDims[1];
      break;
    case VTK_Z_LINE:
      N = cellDims[2];
      break;
    case VTK_XY_PLANE:
      N = cellDims[0]*cellDims[1];
      break;
    case VTK_YZ_PLANE:
      N = cellDims[1]*cellDims[2];
      break;
    case VTK_XZ_PLANE:
      N = cellDims[0]*cellDims[2];
      break;
    case VTK_XYZ_GRID:
      N = cellDims[0]*cellDims[1]*cellDims[2];
      break;
    default:
      vtkGenericWarningMacro("Undefined data description!");
    }
  return( N );
}

//------------------------------------------------------------------------------
int vtkStructuredData::GetDataDimension( int ext[6] )
{
  int dataDescription = vtkStructuredData::GetDataDescriptionFromExtent( ext );
  return( vtkStructuredData::GetDataDimension( dataDescription ) );
}

//------------------------------------------------------------------------------
// Returns the data description given the dimensions (eg. VTK_SINGLE_POINT,
// VTK_X_LINE, VTK_XY_PLANE etc.)
int vtkStructuredData::GetDataDescription(int dims[3])
{
  int tempDims[3];
  // It is essential that dims != tempDims, then alone will SetDimensions()
  // return the correct data description.
  tempDims[0] = dims[0] + 1;
  tempDims[1] = dims[1] + 1;
  tempDims[2] = dims[2] + 1;

  return vtkStructuredData::SetDimensions(dims, tempDims);
}

//------------------------------------------------------------------------------
void vtkStructuredData::GetCellExtentFromNodeExtent(
    int nodeExtent[6], int cellExtent[6], int dataDescription )
{
  if( dataDescription == VTK_EMPTY )
    {
    dataDescription =
        vtkStructuredData::GetDataDescriptionFromExtent( nodeExtent );
    }

  // Initialize the cell extent to be the same as the node extent
  for( int i=0; i < 6; ++i )
    {
    cellExtent[ i ] = nodeExtent[ i ];
    }

  switch( dataDescription )
    {
    case VTK_SINGLE_POINT:
      // Do nothing ?
      break;
    case VTK_X_LINE:
      cellExtent[1]--;
      assert( "post: Cell extents must be >= 1" && (cellExtent[1] >= 1) );
      break;
    case VTK_Y_LINE:
      cellExtent[3]--;
      assert( "post: Cell extents must be >= 1" && (cellExtent[3] >= 1) );
      break;
    case VTK_Z_LINE:
      cellExtent[5]--;
      assert( "post: Cell extents must be >= 1" && (cellExtent[5] >= 1) );
      break;
    case VTK_XY_PLANE:
      cellExtent[1]--;
      cellExtent[3]--;
      assert( "post: Cell extents must be >= 1" && (cellExtent[1] >= 1) );
      assert( "post: Cell extents must be >= 1" && (cellExtent[3] >= 1) );
      break;
    case VTK_YZ_PLANE:
      cellExtent[3]--;
      cellExtent[5]--;
      assert( "post: Cell extents must be >= 1" && (cellExtent[3] >= 1) );
      assert( "post: Cell extents must be >= 1" && (cellExtent[5] >= 1) );
      break;
    case VTK_XZ_PLANE:
      cellExtent[1]--;
      cellExtent[5]--;
      assert( "post: Cell extents must be >= 1" && (cellExtent[1] >= 1) );
      assert( "post: Cell extents must be >= 1" && (cellExtent[5] >= 1) );
      break;
    case VTK_XYZ_GRID:
      cellExtent[1]--;
      cellExtent[3]--;
      cellExtent[5]--;
      assert( "post: Cell extents must be >= 1" && (cellExtent[1] >= 1) );
      assert( "post: Cell extents must be >= 1" && (cellExtent[3] >= 1) );
      assert( "post: Cell extents must be >= 1" && (cellExtent[5] >= 1) );
      break;
    default:
      vtkGenericWarningMacro("Could not get dimensions for extent!");
    }

}

//------------------------------------------------------------------------------
void vtkStructuredData::GetCellDimensionsFromExtent(
    int ext[6], int celldims[3], int dataDescription )
{
  if( dataDescription == VTK_EMPTY )
    {
    dataDescription = vtkStructuredData::GetDataDescriptionFromExtent( ext );
    }

  celldims[0] = celldims[1] = celldims[2] = 0;
  switch( dataDescription )
    {
    case VTK_SINGLE_POINT:
      // Do nothing?
      break;
    case VTK_X_LINE:
      celldims[0] = ext[1]-ext[0];
      celldims[0] = (celldims[0] < 0)? 0 : celldims[0];
      break;
    case VTK_Y_LINE:
      celldims[1] = ext[3]-ext[2];
      celldims[1] = (celldims[1] < 0)? 0 : celldims[1];
      break;
    case VTK_Z_LINE:
      celldims[2] = ext[5]-ext[4];
      celldims[2] = (celldims[2] < 0)? 0 : celldims[2];
      break;
    case VTK_XY_PLANE:
      celldims[0] = ext[1]-ext[0];
      celldims[1] = ext[3]-ext[2];
      celldims[0] = (celldims[0] < 0)? 0 : celldims[0];
      celldims[1] = (celldims[1] < 0)? 0 : celldims[1];
      break;
    case VTK_YZ_PLANE:
      celldims[1] = ext[3]-ext[2];
      celldims[2] = ext[5]-ext[4];
      celldims[1] = (celldims[1] < 0)? 0 : celldims[1];
      celldims[2] = (celldims[2] < 0)? 0 : celldims[2];
      break;
    case VTK_XZ_PLANE:
      celldims[0] = ext[1]-ext[0];
      celldims[2] = ext[5]-ext[4];
      celldims[0] = (celldims[0] < 0)? 0 : celldims[0];
      celldims[2] = (celldims[2] < 0)? 0 : celldims[2];
      break;
    case VTK_XYZ_GRID:
      celldims[0] = ext[1]-ext[0];
      celldims[1] = ext[3]-ext[2];
      celldims[2] = ext[5]-ext[4];
      celldims[0] = (celldims[0] < 0)? 0 : celldims[0];
      celldims[1] = (celldims[1] < 0)? 0 : celldims[1];
      celldims[2] = (celldims[2] < 0)? 0 : celldims[2];
      break;
    default:
      vtkGenericWarningMacro("Could not get dimensions for extent!");
    }
}

//------------------------------------------------------------------------------
void vtkStructuredData::GetCellDimensionsFromNodeDimensions(
    int nodeDims[3], int cellDims[3] )
{
  assert( "pre: node dims must be at least 1" && nodeDims[0] >= 1);
  assert( "pre: node dims must be at least 1" && nodeDims[1] >= 1);
  assert( "pre: node dims must be at least 1" && nodeDims[2] >= 1);

  for( int i=0; i < 3; ++i )
    {
    cellDims[ i ] = nodeDims[ i ]-1;
    cellDims[ i ] = (cellDims[i] < 0 )? 0 : cellDims[ i ];
    }
}

//------------------------------------------------------------------------------
void vtkStructuredData::GetDimensionsFromExtent(
    int ext[6], int dims[3], int dataDescription )
{
  dims[0] = dims[1] = dims[2] = 1;
  if( dataDescription == VTK_EMPTY )
    {
    dataDescription = vtkStructuredData::GetDataDescriptionFromExtent( ext );
    }

  switch( dataDescription )
    {
    case VTK_SINGLE_POINT:
      dims[0] = dims[1] = dims[2] = 1;
      break;
    case VTK_X_LINE:
      dims[0] = ext[1]-ext[0]+1;
      break;
    case VTK_Y_LINE:
      dims[1] = ext[3]-ext[2]+1;
      break;
    case VTK_Z_LINE:
      dims[2] = ext[5]-ext[4]+1;
      break;
    case VTK_XY_PLANE:
      dims[0] = ext[1]-ext[0]+1;
      dims[1] = ext[3]-ext[2]+1;
      break;
    case VTK_YZ_PLANE:
      dims[1] = ext[3]-ext[2]+1;
      dims[2] = ext[5]-ext[4]+1;
      break;
    case VTK_XZ_PLANE:
      dims[0] = ext[1]-ext[0]+1;
      dims[2] = ext[5]-ext[4]+1;
      break;
    case VTK_XYZ_GRID:
      dims[0] = ext[1]-ext[0]+1;
      dims[1] = ext[3]-ext[2]+1;
      dims[2] = ext[5]-ext[4]+1;
      break;
    default:
      vtkGenericWarningMacro("Could not get dimensions for extent!");
    }
}

//------------------------------------------------------------------------------
void vtkStructuredData::GetLocalStructuredCoordinates(
    int ijk[3], int ext[6], int lijk[3], int dataDescription )
{
  lijk[0] = lijk[1] = lijk[2] = 0;

  if( dataDescription == VTK_EMPTY )
    {
    dataDescription = vtkStructuredData::GetDataDescriptionFromExtent( ext );
    }

  switch( dataDescription )
    {
    case VTK_SINGLE_POINT:
      lijk[0] = lijk[1] = lijk[2] = 0;
      break;
    case VTK_X_LINE:
      lijk[0] = ijk[0]-ext[0];
      break;
    case VTK_Y_LINE:
      lijk[1] = ijk[1]-ext[2];
      break;
    case VTK_Z_LINE:
      lijk[2] = ijk[2]-ext[4];
      break;
    case VTK_XY_PLANE:
      lijk[0] = ijk[0]-ext[0];
      lijk[1] = ijk[1]-ext[2];
      break;
    case VTK_YZ_PLANE:
      lijk[1] = ijk[1]-ext[2];
      lijk[2] = ijk[2]-ext[4];
      break;
    case VTK_XZ_PLANE:
      lijk[0] = ijk[0]-ext[0];
      lijk[2] = ijk[2]-ext[4];
      break;
    case VTK_XYZ_GRID:
      lijk[0] = ijk[0]-ext[0];
      lijk[1] = ijk[1]-ext[2];
      lijk[2] = ijk[2]-ext[4];
      break;
    default:
      vtkGenericWarningMacro("Could not get local structured coordinates");
    }

  assert( "post: local ijk is out-of-bounds" && lijk[0] >= 0 );
  assert( "post: local ijk is out-of-bounds" && lijk[1] >= 0 );
  assert( "post: local ijk is out-of-bounds" && lijk[2] >= 0 );
}

//------------------------------------------------------------------------------
void vtkStructuredData::GetGlobalStructuredCoordinates(
    int lijk[3], int ext[6], int ijk[3], int dataDescription )
{
  if( dataDescription == VTK_EMPTY )
    {
    dataDescription = vtkStructuredData::GetDataDescriptionFromExtent( ext );
    }

  switch( dataDescription )
    {
    case VTK_SINGLE_POINT:
      ijk[0] = ext[0];
      ijk[1] = ext[2];
      ijk[2] = ext[4];
      break;
    case VTK_X_LINE:
      ijk[0] = ext[0] + lijk[0];
      ijk[1] = ext[2];
      ijk[2] = ext[4];
      break;
    case VTK_Y_LINE:
      ijk[0] = ext[0];
      ijk[1] = ext[2] + lijk[1];
      ijk[2] = ext[4];
      break;
    case VTK_Z_LINE:
      ijk[0] = ext[0];
      ijk[1] = ext[2];
      ijk[2] = ext[4] + lijk[2];
      break;
    case VTK_XY_PLANE:
      ijk[0] = ext[0] + lijk[0];
      ijk[1] = ext[2] + lijk[1];
      ijk[2] = ext[4];
      break;
    case VTK_YZ_PLANE:
      ijk[0] = ext[0];
      ijk[1] = ext[2] + lijk[1];
      ijk[2] = ext[4] + lijk[2];
      break;
    case VTK_XZ_PLANE:
      ijk[0] = ext[0] + lijk[0];
      ijk[1] = ext[2];
      ijk[2] = ext[4] + lijk[2];
      break;
    case VTK_XYZ_GRID:
      ijk[0] = ext[0] + lijk[0];
      ijk[1] = ext[2] + lijk[1];
      ijk[2] = ext[4] + lijk[2];
      break;
    default:
      vtkGenericWarningMacro("Could not get global structured coordinates");
    }
}

//------------------------------------------------------------------------------
// Given the extent, returns the data description given the dimensions
// (eg. VTK_SINGLE_POINT,VTK_X_LINE, VTK_XY_PLANE etc.)
int vtkStructuredData::GetDataDescriptionFromExtent(int ext[6] )
{
  int dims[3];
  vtkStructuredExtent::GetDimensions( ext, dims );
  return( vtkStructuredData::GetDataDescription( dims ) );
}

//------------------------------------------------------------------------------
// Specify the dimensions of a regular, rectangular dataset. The input is
// the new dimensions (inDim) and the current dimensions (dim). The function
// returns the dimension of the dataset (0-3D). If the dimensions are
// improperly specified a -1 is returned. If the dimensions are unchanged, a
// value of 100 is returned.
int vtkStructuredData::SetDimensions(int inDim[3], int dim[3])
{
  int dataDim, i;
  int dataDescription = VTK_UNCHANGED;

  if ( inDim[0] != dim[0] || inDim[1] != dim[1] || inDim[2] != dim[2] )
    {
    for (dataDim=0, i=0; i<3 ; i++)
      {
      dim[i] = inDim[i];
      if (inDim[i] > 1)
        {
        dataDim++;
        }
      }

    if ( inDim[0]<1 || inDim[1]<1 || inDim[2]<1 )
      {
      return VTK_EMPTY;
      }

    if ( dataDim == 3 )
      {
      dataDescription = VTK_XYZ_GRID;
      }
    else if ( dataDim == 2)
      {
      if ( inDim[0] == 1 )
        {
        dataDescription = VTK_YZ_PLANE;
        }
      else if ( inDim[1] == 1 )
        {
        dataDescription = VTK_XZ_PLANE;
        }
      else
        {
        dataDescription = VTK_XY_PLANE;
        }
      }
    else if ( dataDim == 1 )
      {
      if ( inDim[0] != 1 )
        {
        dataDescription = VTK_X_LINE;
        }
      else if ( inDim[1] != 1 )
        {
        dataDescription = VTK_Y_LINE;
        }
      else
        {
        dataDescription = VTK_Z_LINE;
        }
      }
    else
      {
      dataDescription = VTK_SINGLE_POINT;
      }
    }

  return dataDescription;
}

//------------------------------------------------------------------------------
// Specify the extent of a regular, rectangular dataset. The input is
// the new extent (inExt) and the current extent (ext). The function
// returns the dimension of the dataset (0-3D). If the extents are
// improperly specified a -1 is returned. If the dimensions are unchanged, a
// value of 100 is returned.
int vtkStructuredData::SetExtent(int inExt[6], int ext[6])
{
  int dataDim, i;
  int dataDescription;

  if ( inExt[0] == ext[0] && inExt[1] == ext[1] &&
       inExt[2] == ext[2] && inExt[3] == ext[3] &&
       inExt[4] == ext[4] && inExt[5] == ext[5])
    {
    return VTK_UNCHANGED;
    }

  dataDim = 0;
  for (i=0; i<3 ; ++i)
    {
    ext[i*2] = inExt[i*2];
    ext[i*2+1] = inExt[i*2+1];
    if (inExt[i*2] < inExt[i*2+1])
      {
      dataDim++;
      }
    }

  if ( inExt[0]>inExt[1] || inExt[2]>inExt[3] || inExt[4]>inExt[5] )
    {
    return VTK_EMPTY;
    }

  if ( dataDim == 3 )
    {
    dataDescription = VTK_XYZ_GRID;
    }
  else if ( dataDim == 2)
    {
    if ( inExt[0] == inExt[1] )
      {
      dataDescription = VTK_YZ_PLANE;
      }
    else if ( inExt[2] == inExt[3] )
      {
      dataDescription = VTK_XZ_PLANE;
      }
    else
      {
      dataDescription = VTK_XY_PLANE;
      }
    }
  else if ( dataDim == 1 )
    {
    if ( inExt[0] < inExt[1] )
      {
      dataDescription = VTK_X_LINE;
      }
    else if ( inExt[2] < inExt[3] )
      {
      dataDescription = VTK_Y_LINE;
      }
    else
      {
      dataDescription = VTK_Z_LINE;
      }
    }
  else
    {
    dataDescription = VTK_SINGLE_POINT;
    }

  return dataDescription;
}

//------------------------------------------------------------------------------
// Get the points defining a cell. (See vtkDataSet for more info.)
void vtkStructuredData::GetCellPoints(vtkIdType cellId, vtkIdList *ptIds,
                                      int dataDescription, int dim[3])
{
  int loc[3];
  vtkIdType idx, npts;
  int iMin, iMax, jMin, jMax, kMin, kMax;
  vtkIdType d01 = static_cast<vtkIdType>(dim[0])*dim[1];

  ptIds->Reset();
  iMin = iMax = jMin = jMax = kMin = kMax = 0;

  switch (dataDescription)
    {
    case VTK_EMPTY:
      return;

    case VTK_SINGLE_POINT: // cellId can only be = 0
      break;

    case VTK_X_LINE:
      iMin = cellId;
      iMax = cellId + 1;
      break;

    case VTK_Y_LINE:
      jMin = cellId;
      jMax = cellId + 1;
      break;

    case VTK_Z_LINE:
      kMin = cellId;
      kMax = cellId + 1;
      break;

    case VTK_XY_PLANE:
      iMin = cellId % (dim[0]-1);
      iMax = iMin + 1;
      jMin = cellId / (dim[0]-1);
      jMax = jMin + 1;
      break;

    case VTK_YZ_PLANE:
      jMin = cellId % (dim[1]-1);
      jMax = jMin + 1;
      kMin = cellId / (dim[1]-1);
      kMax = kMin + 1;
      break;

    case VTK_XZ_PLANE:
      iMin = cellId % (dim[0]-1);
      iMax = iMin + 1;
      kMin = cellId / (dim[0]-1);
      kMax = kMin + 1;
      break;

    case VTK_XYZ_GRID:
      iMin = cellId % (dim[0] - 1);
      iMax = iMin + 1;
      jMin = (cellId / (dim[0] - 1)) % (dim[1] - 1);
      jMax = jMin + 1;
      kMin = cellId / (static_cast<vtkIdType>(dim[0] - 1) * (dim[1] - 1));
      kMax = kMin + 1;
      break;
    default:
      assert("check: impossible case." && 0); // reaching this line is a bug.
      break;
    }

  // Extract point ids
  for (npts=0,loc[2]=kMin; loc[2]<=kMax; loc[2]++)
    {
    for (loc[1]=jMin; loc[1]<=jMax; loc[1]++)
      {
      for (loc[0]=iMin; loc[0]<=iMax; loc[0]++)
        {
        idx = loc[0] + loc[1]*static_cast<vtkIdType>(dim[0]) + loc[2]*d01;
        ptIds->InsertId(npts++,idx);
        }
      }
    }
}

//------------------------------------------------------------------------------
// Get the cells using a point. (See vtkDataSet for more info.)
void vtkStructuredData::GetPointCells(vtkIdType ptId, vtkIdList *cellIds,
                                      int dim[3])
{
  vtkIdType cellDim[3];
  int ptLoc[3], cellLoc[3];
  int i, j;
  vtkIdType cellId;
  static int offset[8][3] = {{-1,0,0}, {-1,-1,0}, {-1,-1,-1}, {-1,0,-1},
                             {0,0,0},  {0,-1,0},  {0,-1,-1},  {0,0,-1}};

  for (i=0; i<3; i++)
    {
    cellDim[i] = dim[i] - 1;
    if (cellDim[i] == 0)
      {
      cellDim[i] = 1;
      }
    }

  //  Get the location of the point
  //
  ptLoc[0] = ptId % dim[0];
  ptLoc[1] = (ptId / dim[0]) % dim[1];
  ptLoc[2] = ptId / (static_cast<vtkIdType>(dim[0])*dim[1]);

  //  From the point location, compute the cell locations.  There are at
  //  most eight possible.
  //
  cellIds->Reset();

  for (j=0; j<8; j++)
    {
    for (i=0; i<3; i++)
      {
      cellLoc[i] = ptLoc[i] + offset[j][i];
      if ( cellLoc[i] < 0 || cellLoc[i] >= cellDim[i] )
        {
        break;
        }
      }
    if ( i >= 3 ) //add cell
      {
      cellId = cellLoc[0] + cellLoc[1]*cellDim[0] +
                            cellLoc[2]*cellDim[0]*cellDim[1];
      cellIds->InsertNextId(cellId);
      }
    }

  return;
}

//------------------------------------------------------------------------------
void vtkStructuredData::GetCellNeighbors(vtkIdType cellId, vtkIdList *ptIds,
                                        vtkIdList *cellIds, int dim[3])
{
  int j, seedLoc[3], ptLoc[3], cellLoc[3];
  vtkIdType cellDim[3];
  int offset[8][3];
  vtkIdType numPts = ptIds->GetNumberOfIds(), id, i;

  cellIds->Reset();

  // Start by finding the "space" of the points in i-j-k space.
  // The points define a point, line, or plane in topological space,
  // which results in degrees of freedom in three, two or one direction.
  // The numbers of DOF determines which neighbors to select.

  // Start by finding a seed point
  id = ptIds->GetId(0);
  seedLoc[0] = id % dim[0];
  seedLoc[1] = (id / dim[0]) % dim[1];
  seedLoc[2] = id / (dim[0]*dim[1]);

  // This defines the space around the seed
  offset[0][0] = -1; offset[0][1] = -1; offset[0][2] = -1;
  offset[1][0] =  0; offset[1][1] = -1; offset[1][2] = -1;
  offset[2][0] = -1; offset[2][1] =  0; offset[2][2] = -1;
  offset[3][0] =  0; offset[3][1] =  0; offset[3][2] = -1;
  offset[4][0] = -1; offset[4][1] = -1; offset[4][2] =  0;
  offset[5][0] =  0; offset[5][1] = -1; offset[5][2] =  0;
  offset[6][0] = -1; offset[6][1] =  0; offset[6][2] =  0;
  offset[7][0] =  0; offset[7][1] =  0; offset[7][2] =  0;

  // For the rest of the points, trim the seed region
  // This is essentially an intersection of edge neighbors.
  for (i=1; i<numPts; i++)
    {
    //  Get the location of the point
    id = ptIds->GetId(i);
    ptLoc[0] = id % dim[0];
    ptLoc[1] = (id / dim[0]) % dim[1];
    ptLoc[2] = id / (static_cast<vtkIdType>(dim[0])*dim[1]);

    if ( (ptLoc[0]-1) == seedLoc[0] )
      {
      offset[0][0] = -10;
      offset[2][0] = -10;
      offset[4][0] = -10;
      offset[6][0] = -10;
      }
    else if ( (ptLoc[0]+1) == seedLoc[0] )
      {
      offset[1][0] = -10;
      offset[3][0] = -10;
      offset[5][0] = -10;
      offset[7][0] = -10;
      }
    else if ( (ptLoc[1]-1) == seedLoc[1] )
      {
      offset[0][1] = -10;
      offset[1][1] = -10;
      offset[4][1] = -10;
      offset[5][1] = -10;
      }
    else if ( (ptLoc[1]+1) == seedLoc[1] )
      {
      offset[2][1] = -10;
      offset[3][1] = -10;
      offset[6][1] = -10;
      offset[7][1] = -10;
      }
    else if ( (ptLoc[2]-1) == seedLoc[2] )
      {
      offset[0][2] = -10;
      offset[1][2] = -10;
      offset[2][2] = -10;
      offset[3][2] = -10;
      }
    else if ( (ptLoc[2]+1) == seedLoc[2] )
      {
      offset[4][2] = -10;
      offset[5][2] = -10;
      offset[6][2] = -10;
      offset[7][2] = -10;
      }
    }

  // Load the non-trimmed cells
  cellDim[0] = dim[0] - 1;
  cellDim[1] = dim[1] - 1;
  cellDim[2] = dim[2] - 1;

  for(i=0; i<3; i++)
    {
    if ( cellDim[i] < 1 )
      {
      cellDim[i] = 1;
      }
    }

  for (j=0; j<8; j++)
    {
    for (i=0; i<3; i++)
      {
      if ( offset[j][i] != -10 )
        {
        cellLoc[i] = seedLoc[i] + offset[j][i];
        if ( cellLoc[i] < 0 || cellLoc[i] >= cellDim[i] )
          {
          break;
          }
        }
      else
        {
        break;
        }
      }
    if ( i >= 3 ) //add cell
      {
      id = cellLoc[0] + cellLoc[1]*cellDim[0] +
                        cellLoc[2]*cellDim[0]*cellDim[1];
      if (id != cellId )
        {
        cellIds->InsertNextId(id);
        }
      }
    }
}

//------------------------------------------------------------------------------
vtkIdType vtkStructuredData::ComputePointIdForExtent(
    int extent[6], int ijk[3], int dataDescription )
{
  vtkIdType idx = -1;

  // Get the grid dimensions for the given extent
  int dims[3];
  vtkStructuredData::GetDimensionsFromExtent( extent, dims );

  // Get the local ijk
  int lijk[3];
  vtkStructuredData::GetLocalStructuredCoordinates( ijk, extent, lijk );

  idx = vtkStructuredData::ComputePointId( dims, lijk, dataDescription );
  return( idx );
}

//------------------------------------------------------------------------------
vtkIdType vtkStructuredData::ComputePointId(
    int dims[3], int ijk[3], int dataDescription)
{
  vtkIdType idx = -1;

  if( dataDescription == VTK_EMPTY )
    {
    dataDescription = vtkStructuredData::GetDataDescription( dims );
    }

  int i,j,k,N1,N2;
  switch( dataDescription )
    {
    case VTK_SINGLE_POINT:
      idx = 0;
      break;
    case VTK_X_LINE:
      i   = ijk[0];
      j   = 0;
      k   = 0;
      N1  = dims[0];
      N2  = 1;
      idx = vtkStructuredData::GetLinearIndex( i,j,k,N1,N2 );
      break;
    case VTK_Y_LINE:
      i   = ijk[1];
      j   = 0;
      k   = 0;
      N1  = dims[1];
      N2  = 1;
      idx = vtkStructuredData::GetLinearIndex( i,j,k,N1,N2 );
      break;
    case VTK_Z_LINE:
      i   = ijk[2];
      j   = 0;
      k   = 0;
      N1  = dims[2];
      N2  = 1;
      idx = vtkStructuredData::GetLinearIndex( i,j,k,N1,N2 );
      break;
    case VTK_XY_PLANE:
      i   = ijk[0];
      j   = ijk[1];
      k   = 0;
      N1  = dims[0];
      N2  = dims[1];
      idx = vtkStructuredData::GetLinearIndex( i,j,k,N1,N2 );
      break;
    case VTK_YZ_PLANE:
      i   = ijk[1];
      j   = ijk[2];
      k   = 0;
      N1  = dims[1];
      N2  = dims[2];
      idx = vtkStructuredData::GetLinearIndex( i,j,k,N1,N2 );
      break;
    case VTK_XZ_PLANE:
      i   = ijk[0];
      j   = ijk[2];
      k   = 0;
      N1  = dims[0];
      N2  = dims[2];
      idx = vtkStructuredData::GetLinearIndex( i,j,k,N1,N2 );
      break;
    case VTK_XYZ_GRID:
      i   = ijk[0];
      j   = ijk[1];
      k   = ijk[2];
      N1  = dims[0];
      N2  = dims[1];
      idx = vtkStructuredData::GetLinearIndex( i,j,k,N1,N2 );
      break;
    default:
      vtkGenericWarningMacro("Could not get dimensions for extent!");
    }

  return( idx );
}

//------------------------------------------------------------------------------
vtkIdType vtkStructuredData::ComputeCellIdForExtent(
    int extent[6], int ijk[3], int dataDescription )
{
  vtkIdType idx = -1;

  int nodeDims[3];
  vtkStructuredData::GetDimensionsFromExtent( extent, nodeDims );

  int lijk[3];
  vtkStructuredData::GetLocalStructuredCoordinates( ijk, extent, lijk );

  idx = vtkStructuredData::ComputeCellId( nodeDims, lijk, dataDescription );
  return( idx );
}

//------------------------------------------------------------------------------
vtkIdType vtkStructuredData::ComputeCellId(
    int dims[3], int ijk[3], int dataDescription )
{
  vtkIdType idx = -1;

  if( dataDescription == VTK_EMPTY )
  {
  dataDescription = vtkStructuredData::GetDataDescription( dims );
  }

  int i,j,k,N1,N2;
  switch( dataDescription )
    {
    case VTK_SINGLE_POINT:
      idx = 0;
      break;
    case VTK_X_LINE:
      i   = ijk[0];
      j   = 0;
      k   = 0;
      N1  = dims[0]-1;
      N2  = 1;
      idx = vtkStructuredData::GetLinearIndex( i,j,k,N1,N2 );
      break;
    case VTK_Y_LINE:
      i   = ijk[1];
      j   = 0;
      k   = 0;
      N1  = dims[1]-1;
      N2  = 1;
      idx = vtkStructuredData::GetLinearIndex( i,j,k,N1,N2 );
      break;
    case VTK_Z_LINE:
      i   = ijk[2];
      j   = 0;
      k   = 0;
      N1  = dims[2]-1;
      N2  = 1;
      idx = vtkStructuredData::GetLinearIndex( i,j,k,N1,N2 );
      break;
    case VTK_XY_PLANE:
      i   = ijk[0];
      j   = ijk[1];
      k   = 0;
      N1  = dims[0]-1;
      N2  = dims[1]-1;
      idx = vtkStructuredData::GetLinearIndex( i,j,k,N1,N2 );
      break;
    case VTK_YZ_PLANE:
      i   = ijk[1];
      j   = ijk[2];
      k   = 0;
      N1  = dims[1]-1;
      N2  = dims[2]-1;
      idx = vtkStructuredData::GetLinearIndex( i,j,k,N1,N2 );
      break;
    case VTK_XZ_PLANE:
      i   = ijk[0];
      j   = ijk[2];
      k   = 0;
      N1  = dims[0]-1;
      N2  = dims[2]-1;
      idx = vtkStructuredData::GetLinearIndex( i,j,k,N1,N2 );
      break;
    case VTK_XYZ_GRID:
      i   = ijk[0];
      j   = ijk[1];
      k   = ijk[2];
      N1  = dims[0]-1;
      N2  = dims[1]-1;
      idx = vtkStructuredData::GetLinearIndex( i,j,k,N1,N2 );
      break;
    default:
      vtkGenericWarningMacro("Could not get dimensions for extent!");
    }
  return( idx );
}

//------------------------------------------------------------------------------
void vtkStructuredData::ComputeCellStructuredCoordsForExtent(
    const vtkIdType cellIdx, int ext[6], int ijk[3], int dataDescription )
{
  // STEP 0: Get the node dimensions
  int nodeDims[3];
  vtkStructuredData::GetDimensionsFromExtent( ext, nodeDims );

  // STEP 1: Compute the local ijk of the cell corresponding to the
  // given cellIdx
  int lijk[3];
  vtkStructuredData::ComputeCellStructuredCoords(
      cellIdx, nodeDims, lijk, dataDescription );

  // STEP 2: Convert the local ijk to global ijk w.r.t. the given extent
  vtkStructuredData::GetGlobalStructuredCoordinates( lijk, ext, ijk );
}

//------------------------------------------------------------------------------
void vtkStructuredData::ComputeCellStructuredCoords(
    const vtkIdType cellIdx, int dim[3], int ijk[3], int dataDescription )
{
  if( dataDescription == VTK_EMPTY )
    {
    dataDescription = vtkStructuredData::GetDataDescription( dim );
    }

  int N1,N2;
  ijk[0] = ijk[1] = ijk[2] = 0;
  switch( dataDescription )
    {
    case VTK_SINGLE_POINT:
      // Do nothing
      break;
    case VTK_X_LINE:
      N1 = dim[0]-1;
      N2 = 1;
      vtkStructuredData::GetStructuredCoordinates(
          cellIdx,N1,N2,ijk[0],ijk[1],ijk[2] );
      break;
    case VTK_Y_LINE:
      N1 = dim[1]-1;
      N2 = 1;
      vtkStructuredData::GetStructuredCoordinates(
          cellIdx,N1,N2,ijk[1],ijk[0],ijk[2] );
      break;
    case VTK_Z_LINE:
      N1 = dim[2]-1;
      N2 = 1;
      vtkStructuredData::GetStructuredCoordinates(
          cellIdx,N1,N2,ijk[2],ijk[0],ijk[1] );
      break;
    case VTK_XY_PLANE:
      N1 = dim[0]-1;
      N2 = dim[1]-1;
      vtkStructuredData::GetStructuredCoordinates(
          cellIdx,N1,N2,ijk[0],ijk[1],ijk[2] );
      break;
    case VTK_YZ_PLANE:
      N1 = dim[1]-1;
      N2 = dim[2]-1;
      vtkStructuredData::GetStructuredCoordinates(
          cellIdx,N1,N2,ijk[1],ijk[2],ijk[0] );
      break;
    case VTK_XZ_PLANE:
      N1 = dim[0]-1;
      N2 = dim[2]-1;
      vtkStructuredData::GetStructuredCoordinates(
          cellIdx,N1,N2,ijk[0],ijk[2],ijk[1] );
      break;
    case VTK_XYZ_GRID:
      N1 = dim[0]-1;
      N2 = dim[1]-1;
      vtkStructuredData::GetStructuredCoordinates(
         cellIdx,N1,N2,ijk[0],ijk[1],ijk[2] );
      break;
    default:
      vtkGenericWarningMacro("Could not get dimensions for extent!");
    }
}

//------------------------------------------------------------------------------
void vtkStructuredData::ComputePointStructuredCoordsForExtent(
    const vtkIdType ptId, int ext[6], int ijk[3], int dataDescription )
{
  // STEP 0: Get the node dimensions
  int nodeDims[3];
  vtkStructuredData::GetDimensionsFromExtent( ext, nodeDims );

  // STEP 1: Compute the local ijk of the cell corresponding to the
  // given cellIdx
  int lijk[3];
  vtkStructuredData::ComputePointStructuredCoords(
      ptId, nodeDims, lijk, dataDescription );

  // STEP 2: Convert the local ijk to global ijk w.r.t. the given extent
  vtkStructuredData::GetGlobalStructuredCoordinates( lijk, ext, ijk );
}

//------------------------------------------------------------------------------
void vtkStructuredData::ComputePointStructuredCoords(
    const vtkIdType ptId, int dim[3], int ijk[3], int dataDescription )
{
  if( dataDescription == VTK_EMPTY )
    {
    dataDescription = vtkStructuredData::GetDataDescription( dim );
    }

  int N1,N2;
  ijk[0] = ijk[1] = ijk[2] = 0;
  switch( dataDescription )
    {
    case VTK_SINGLE_POINT:
      // Do nothing
      break;
    case VTK_X_LINE:
      N1 = dim[0];
      N2 = 1;
      vtkStructuredData::GetStructuredCoordinates(
          ptId,N1,N2,ijk[0],ijk[1],ijk[2] );
      break;
    case VTK_Y_LINE:
      N1 = dim[1];
      N2 = 1;
      vtkStructuredData::GetStructuredCoordinates(
          ptId,N1,N2,ijk[1],ijk[0],ijk[2] );
      break;
    case VTK_Z_LINE:
      N1 = dim[2];
      N2 = 1;
      vtkStructuredData::GetStructuredCoordinates(
          ptId,N1,N2,ijk[2],ijk[0],ijk[1] );
      break;
    case VTK_XY_PLANE:
      N1 = dim[0];
      N2 = dim[1];
      vtkStructuredData::GetStructuredCoordinates(
          ptId,N1,N2,ijk[0],ijk[1],ijk[2] );
      break;
    case VTK_YZ_PLANE:
      N1 = dim[1];
      N2 = dim[2];
      vtkStructuredData::GetStructuredCoordinates(
          ptId,N1,N2,ijk[1],ijk[2],ijk[0] );
      break;
    case VTK_XZ_PLANE:
      N1 = dim[0];
      N2 = dim[2];
      vtkStructuredData::GetStructuredCoordinates(
          ptId,N1,N2,ijk[0],ijk[2],ijk[1] );
      break;
    case VTK_XYZ_GRID:
      N1 = dim[0];
      N2 = dim[1];
      vtkStructuredData::GetStructuredCoordinates(
         ptId,N1,N2,ijk[0],ijk[1],ijk[2] );
      break;
    default:
      vtkGenericWarningMacro("Could not get dimensions for extent!");
    }
}
