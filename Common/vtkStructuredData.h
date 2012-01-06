/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkStructuredData - abstract class for topologically regular data
// .SECTION Description
// vtkStructuredData is an abstract class that specifies an interface for
// topologically regular data. Regular data is data that can be accessed
// in rectangular fashion using an i-j-k index. A finite difference grid,
// a volume, or a pixmap are all considered regular.
// .SECTION See Also
// vtkStructuredGrid vtkUniformGrid vtkRectilinearGrid vtkRectilinearGrid

#ifndef __vtkStructuredData_h
#define __vtkStructuredData_h

#include "vtkObject.h"

class vtkIdList;

#define VTK_UNCHANGED 0
#define VTK_SINGLE_POINT 1
#define VTK_X_LINE 2
#define VTK_Y_LINE 3
#define VTK_Z_LINE 4
#define VTK_XY_PLANE 5
#define VTK_YZ_PLANE 6
#define VTK_XZ_PLANE 7
#define VTK_XYZ_GRID 8
#define VTK_EMPTY 9

class VTK_COMMON_EXPORT vtkStructuredData : public vtkObject
{
public:
  vtkTypeMacro(vtkStructuredData,vtkObject);

  // Description:
  // Specify the dimensions of a regular, rectangular dataset. The input is
  // the new dimensions (inDim) and the current dimensions (dim). The function
  // returns the dimension of the dataset (0-3D). If the dimensions are
  // improperly specified a -1 is returned. If the dimensions are unchanged, a
  // value of 100 is returned.
  static int SetDimensions(int inDim[3], int dim[3]);
  static int SetExtent(int inExt[6], int ext[6]);

  // Description:
  // Returns the data description given the dimensions (eg. VTK_SINGLE_POINT,
  // VTK_X_LINE, VTK_XY_PLANE etc.)
  static int GetDataDescription(int dims[3]);

  // Description:
  // Return the topological dimension of the data (e.g., 0, 1, 2, or 3D).
  static int GetDataDimension(int dataDescription);

  // Description:
  // Get the points defining a cell. (See vtkDataSet for more info.)
  static void GetCellPoints(vtkIdType cellId, vtkIdList *ptIds,
                            int dataDescription, int dim[3]);

  // Description:
  // Get the cells using a point. (See vtkDataSet for more info.)
  static void GetPointCells(vtkIdType ptId, vtkIdList *cellIds, int dim[3]);

  // Description:
  // Get the cells using the points ptIds, exclusive of the cell cellId.
  // (See vtkDataSet for more info.)
  static void GetCellNeighbors(vtkIdType cellId, vtkIdList *ptIds,
                               vtkIdList *cellIds, int dim[3]);

  // Description:
  // Given a location in structured coordinates (i-j-k), and the extent
  // of the structured dataset, return the point id.
  static vtkIdType ComputePointIdForExtent(int extent[6], int ijk[3]) {
    vtkIdType ydim = static_cast<vtkIdType>(extent[3] - extent[2] + 1);
    vtkIdType xdim = static_cast<vtkIdType>(extent[1] - extent[0] + 1);
    return ((ijk[2] - extent[4])*ydim + (ijk[1] - extent[2]))*xdim 
            + (ijk[0] - extent[0]); }

  // Description:
  // Given a location in structured coordinates (i-j-k), and the extent
  // of the structured dataset, return the point id.
  static vtkIdType ComputeCellIdForExtent(int extent[6], int ijk[3]) {
    vtkIdType ydim = static_cast<vtkIdType>(extent[3] - extent[2]);
    if (ydim == 0) ydim = 1;
    vtkIdType xdim = static_cast<vtkIdType>(extent[1] - extent[0]);
    if (xdim == 0) xdim = 1;
    return ((ijk[2] - extent[4])*(ydim) + (ijk[1] - extent[2]))*(xdim)
            + (ijk[0] - extent[0]); }

 // Description:
  // Given a location in structured coordinates (i-j-k), and the dimensions
  // of the structured dataset, return the point id.  This method does not
  // adjust for the beginning of the extent.
  static vtkIdType ComputePointId(int dim[3], int ijk[3]) {
    return (ijk[2]*static_cast<vtkIdType>(dim[1]) + ijk[1])*dim[0] + ijk[0];}

  // Description:
  // Given a location in structured coordinates (i-j-k), and the dimensions
  // of the structured dataset, return the cell id.  This method does not
  // adjust for the beginning of the extent.
  static vtkIdType ComputeCellId(int dim[3], int ijk[3]) {
    return (ijk[2]*static_cast<vtkIdType>(dim[1]-1) + ijk[1])*(dim[0]-1) + ijk[0];}

  // Description:
  // Given a cellId and grid dimensions 'dim', get the structured coordinates
  // (i-j-k). This method does not adjust for the beginning of the extent.
  static void ComputeCellStructuredCoords(const vtkIdType cellId, int dim[3], int ijk[3])
  {
    int Ni  = dim[0]-1;
    int Nj  = dim[1]-1;
    
    int Nij = Ni*Nj;

    int k = cellId/Nij + 1;
    int j = (cellId - (k-1)*Nij)/Ni + 1;
    int i = cellId - (k-1)*Nij - (j-1)*Ni + 1;
    ijk[0] = i-1;
    ijk[1] = j-1;
    ijk[2] = k-1;
  }

  // Description:
  // Given a pointId and grid dimensions 'dim', get the structured coordinates
  // (i-j-k). This method does not adjust for the beginning of the extent.
  static void ComputePointStructuredCoords(const vtkIdType cellId, int dim[3], int ijk[3])
  {
    int Ni  = dim[0];
    int Nj  = dim[1];

    int Nij = Ni*Nj;

    int k = cellId/Nij + 1;
    int j = (cellId - (k-1)*Nij)/Ni + 1;
    int i = cellId - (k-1)*Nij - (j-1)*Ni + 1;
    ijk[0] = i-1;
    ijk[1] = j-1;
    ijk[2] = k-1;
  }

protected:
  vtkStructuredData() {};
  ~vtkStructuredData() {};

private:
  vtkStructuredData(const vtkStructuredData&);  // Not implemented.
  void operator=(const vtkStructuredData&);  // Not implemented.
};


#endif

