/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredData.cxx
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
#include "vtkStructuredData.hh"

// Description:
// Return the topological dimension of the data (e.g., 0, 1, 2, or 3D).
int vtkStructuredData::GetDataDimension(int dataDescription)
{
  switch (dataDescription)
    {
    case VTK_SINGLE_POINT: return 0;

    case VTK_X_LINE: case VTK_Y_LINE: case VTK_Z_LINE: return 1;

    case VTK_XY_PLANE: case VTK_YZ_PLANE: case VTK_XZ_PLANE: return 2;

    case VTK_XYZ_GRID: return 3;

    default:
      return -1;                       
    }
}

// Description:
// Specify the dimensions of a regular, rectangular dataset. The input is
// the new dimensions (inDim) and the current dimensions (dim). The function 
// returns the dimension of the dataset (0-3D). If the dimensions are 
// improperly specified or are unchanged, a -1 is returned.
int vtkStructuredData::SetDimensions(int inDim[3], int dim[3])
{
  int dataDim, i;
  int dataDescription=(-1);

  vtkDebugMacro(<< " setting Dimensions to (" << inDim[0] << "," << inDim[1] << "," << inDim[2] << ")");

  if ( inDim[0] != dim[0] || inDim[1] != dim[1] || inDim[2] != dim[2] )
    {
    if ( inDim[0]<1 || inDim[1]<1 || inDim[2]<1 )
      {
      vtkErrorMacro (<< "Bad Dimensions, retaining previous values");
      return -1;
      }

    for (dataDim=0, i=0; i<3 ; i++)
      {
      dim[i] = inDim[i];
      if (inDim[i] > 1) dataDim++;
      }

    if ( dataDim == 3 )
      {
      dataDescription = VTK_XYZ_GRID;
      }
    else if ( dataDim == 2)
      {
      if ( inDim[0] == 1 ) dataDescription = VTK_YZ_PLANE;
      else if ( inDim[1] == 1 ) dataDescription = VTK_XZ_PLANE;
      else dataDescription = VTK_XY_PLANE;
      }
    else if ( dataDim == 1 )
      {
      if ( inDim[0] != 1 ) dataDescription = VTK_X_LINE;
      else if ( inDim[1] != 1 ) dataDescription = VTK_Y_LINE;
      else dataDescription = VTK_Z_LINE;
      }
    else
      {
      dataDescription = VTK_SINGLE_POINT;
      }
    }

  return dataDescription;
}

// Description:
// Get the points defining a cell. (See vtkDataSet for more info.)
void vtkStructuredData::GetCellPoints(int cellId, vtkIdList& ptIds,
                                      int dataDescription, int dim[3])
{
  int idx, loc[3], npts;
  int iMin, iMax, jMin, jMax, kMin, kMax;
  int d01 = dim[0]*dim[1];
 
  ptIds.Reset();
  iMin = iMax = jMin = jMax = kMin = kMax = 0;

  switch (dataDescription)
    {
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
      kMin = cellId / ((dim[0] - 1) * (dim[1] - 1));
      kMax = kMin + 1;
      break;
    }

  // Extract point ids
  for (npts=0,loc[2]=kMin; loc[2]<=kMax; loc[2]++)
    {
    for (loc[1]=jMin; loc[1]<=jMax; loc[1]++)
      {
      for (loc[0]=iMin; loc[0]<=iMax; loc[0]++)
        {
        idx = loc[0] + loc[1]*dim[0] + loc[2]*d01;
        ptIds.InsertId(npts++,idx);
        }
      }
    }
}

// Description:
// Get the cells using a point. (See vtkDataSet for more info.)
void vtkStructuredData::GetPointCells(int ptId, vtkIdList& cellIds, int dim[3])
{
  int ptDim[3], cellDim[3];
  int ptLoc[3], cellLoc[3];
  int i, j, cellId;
  static int offset[8][3] = {{-1,0,0}, {-1,-1,0}, {-1,-1,-1}, {-1,0,-1},
                             {0,0,0},  {0,-1,0},  {0,-1,-1},  {0,0,-1}};

  for (i=0; i<3; i++) 
    {
    ptDim[i] = dim[i];
    cellDim[i] = ptDim[i] - 1;
    }
//
//  Get the location of the point
//
  ptLoc[0] = ptId % ptDim[0];
  ptLoc[1] = (ptId / ptDim[0]) % ptDim[1];
  ptLoc[2] = ptId / (ptDim[0]*ptDim[1]);
//
//  From the point location, compute the cell locations.  There are at
//  most eight possible.
//
  cellIds.Reset();

  for (j=0; j<8; j++) 
    {
    for (i=0; i<3; i++) 
      {
      cellLoc[i] = ptLoc[i] + offset[j][i];
      if ( cellLoc[i] < 0 || cellLoc[i] >= cellDim[i] ) 
        break;
      }
    if ( i >= 3 ) //add cell
      {
      cellId = cellLoc[0] + cellLoc[1]*cellDim[0] + 
                            cellLoc[2]*cellDim[0]*cellDim[1];
      cellIds.InsertNextId(cellId);
      }
    }

  return;
}

