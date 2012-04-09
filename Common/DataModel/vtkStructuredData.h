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
// .NAME vtkStructuredData - Singleton class for topologically regular data
//
// .SECTION Description
// vtkStructuredData is a singleton class that provides an interface for
// topologically regular data. Regular data is data that can be accessed
// in rectangular fashion using an i-j-k index. A finite difference grid,
// a volume, or a pixmap are all considered regular.
//
// .SECTION See Also
// vtkStructuredGrid vtkUniformGrid vtkRectilinearGrid vtkRectilinearGrid

#ifndef __vtkStructuredData_h
#define __vtkStructuredData_h

#include "vtkCommonDataModelModule.h" // For export macro
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

class VTKCOMMONDATAMODEL_EXPORT vtkStructuredData : public vtkObject
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
  static int GetDataDescriptionFromExtent( int ext[6] );

  // Description:
  // Return the topological dimension of the data (e.g., 0, 1, 2, or 3D).
  static int GetDataDimension(int dataDescription);
  static int GetDataDimension( int ext[6] );

  // Description:
  // Given the grid extent, this method returns the total number of nodes
  // within the extent.
  // The method accepts as an optional parameter the data description of the
  // grid. If a data description is not provided, it will be computed internally.
  // If the method is used within a tight loop it is advised to pre-acquire and
  // pass the data description to the method to avoid any latency associated
  // with computing the data description internally multiple times.
  static int GetNumberOfNodes( int ext[6], int dataDescription=VTK_EMPTY );

  // Description:
  // Given the grid extent, this method returns the total number of cells
  // within the extent.
  // The method accepts as an optional parameter the data description of the
  // grid. If a data description is not provided, it will be computed internally.
  // If the method is used within a tight loop it is advised to pre-acquire and
  // pass the data description to the method to avoid any latency associated
  // with computing the data description internally multiple times.
  static int GetNumberOfCells( int ext[6], int dataDescription=VTK_EMPTY );

  // Description:
  // Given the node extent of a grid, this method computes the corresponding
  // cell extent for the grid.
  // The method accepts as an optional parameter the data description of the
  // grid. If a data description is not provided, it will be computed internally.
  // If the method is used within a tight loop it is advised to pre-acquire and
  // pass the data description to the method to avoid any latency associated
  // with computing the data description internally multiple times.
  static void GetCellExtentFromNodeExtent(
      int nodeExtent[6], int cellExtent[6], int dataDescription=VTK_EMPTY );

  // Description:
  // Computes the structured grid dimensions based on the given extent.
  static void GetDimensionsFromExtent(
      int ext[6], int dims[3], int dataDescription=VTK_EMPTY );

  // Description:
  // Returns the cell dimensions, i.e., the number of cells along the i,j,k
  // for the grid with the given grid extent. Note, the grid extent is the
  // number of points.
  static void GetCellDimensionsFromExtent(
      int ext[6], int celldims[3], int dataDescription=VTK_EMPTY );

  // Description:
  // Given the node dimensions of the grid, in node dims, this method returns
  // the corresponding cell dimensions for the given grid.
  static void GetCellDimensionsFromNodeDimensions(
      int nodeDims[3],int cellDims[3] );

  // Description:
  // Given the global structured coordinates for a point or cell, ijk, w.r.t.
  // as well as, the global sub-grid cell or node extent, this method computes
  // the corresponding local structured coordinates, lijk, starting from 0.
  // The method accepts as an optional parameter the data description of the
  // grid. If a data description is not provided, it will be computed internally.
  // If the method is used within a tight loop it is advised to pre-acquire and
  // pass the data description to the method to avoid any latency associated
  // with computing the data description internally multiple times.
  static void GetLocalStructuredCoordinates(
      int ijk[3], int ext[6], int lijk[3], int dataDescription=VTK_EMPTY );

  // Description:
  // Given local structured coordinates, and the corresponding global sub-grid
  // extent, this method computes the global ijk coordinates.
  // The method accepts as an optional parameter the data description of the
  // grid. If a data description is not provided, it will be computed internally.
  // If the method is used within a tight loop it is advised to pre-acquire and
  // pass the data description to the method to avoid any latency associated
  // with computing the data description internally multiple times.
  static void GetGlobalStructuredCoordinates(
      int lijk[3], int ext[6], int ijk[3], int dataDescription=VTK_EMPTY );

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
  // The method accepts as an optional parameter the data description of the
  // grid. If a data description is not provided, it will be computed internally.
  // If the method is used within a tight loop it is advised to pre-acquire and
  // pass the data description to the method to avoid any latency associated
  // with computing the data description internally multiple times.
  static vtkIdType ComputePointIdForExtent(
      int extent[6], int ijk[3], int dataDescription=VTK_EMPTY );

  // Description:
  // Given a location in structured coordinates (i-j-k), and the extent
  // of the structured dataset, return the point id.
  static vtkIdType ComputeCellIdForExtent(
      int extent[6], int ijk[3], int dataDescription=VTK_EMPTY );

  // Description:
  // Given a location in structured coordinates (i-j-k), and the dimensions
  // of the structured dataset, return the point id.  This method does not
  // adjust for the beginning of the extent.
  // The method accepts as an optional parameter the data description of the
  // grid. If a data description is not provided, it will be computed internally.
  // If the method is used within a tight loop it is advised to pre-acquire and
  // pass the data description to the method to avoid any latency associated
  // with computing the data description internally multiple times.
  static vtkIdType ComputePointId(
      int dim[3], int ijk[3], int dataDescription=VTK_EMPTY );

  // Description:
  // Given a location in structured coordinates (i-j-k), and the dimensions
  // of the structured dataset, return the cell id.  This method does not
  // adjust for the beginning of the extent.
  // The method accepts as an optional parameter the data description of the
  // grid. If a data description is not provided, it will be computed internally.
  // If the method is used within a tight loop it is advised to pre-acquire and
  // pass the data description to the method to avoid any latency associated
  // with computing the data description internally multiple times.
  static vtkIdType ComputeCellId(
      int dim[3], int ijk[3], int dataDescription=VTK_EMPTY );

  // Description:
  // Given the global grid extent and the linear index of a cell within the
  // grid extent, this method computes the corresponding structured coordinates
  // of the given cell. This method adjusts for the beginning of the extent.
  // The method accepts as an optional parameter the data description of the
  // grid. If a data description is not provided, it will be computed internally.
  // If the method is used within a tight loop it is advised to pre-acquire and
  // pass the data description to the method to avoid any latency associated
  // with computing the data description internally multiple times.
  static void ComputeCellStructuredCoordsForExtent(
      const vtkIdType cellIdx, int ext[6], int ijk[3],
      int dataDescription=VTK_EMPTY );

  // Description:
  // Given a cellId and grid dimensions 'dim', get the structured coordinates
  // (i-j-k). This method does not adjust for the beginning of the extent.
  // The method accepts as an optional parameter the data description of the
  // grid. If a data description is not provided, it will be computed internally.
  // If the method is used within a tight loop it is advised to pre-acquire and
  // pass the data description to the method to avoid any latency associated
  // with computing the data description internally multiple times.
  static void ComputeCellStructuredCoords(
      const vtkIdType cellId, int dim[3], int ijk[3],
      int dataDescription=VTK_EMPTY );

  // Description:
  // Given a pointId and the grid extent ext, get the structured coordinates
  // (i-j-k). This method adjusts for the beginning of the extent.
  // The method accepts as an optional parameter the data description of the
  // grid. If a data description is not provided, it will be computed internally.
  // If the method is used within a tight loop it is advised to pre-acquire and
  // pass the data description to the method to avoid any latency associated
  // with computing the data description internally multiple times.
  static void ComputePointStructuredCoordsForExtent(
      const vtkIdType ptId, int ext[6], int ijk[3],
      int dataDescription=VTK_EMPTY );

  // Description:
  // Given a pointId and grid dimensions 'dim', get the structured coordinates
  // (i-j-k). This method does not adjust for the beginning of the extent.
  // The method accepts as an optional parameter the data description of the
  // grid. If a data description is not provided, it will be computed internally.
  // If the method is used within a tight loop it is advised to pre-acquire and
  // pass the data description to the method to avoid any latency associated
  // with computing the data description internally multiple times.
  static void ComputePointStructuredCoords(
      const vtkIdType ptId, int dim[3], int ijk[3],
      int dataDescription=VTK_EMPTY );

protected:
  vtkStructuredData() {};
  ~vtkStructuredData() {};

  // Description:
  // Computes the linear index for the given i-j-k structured of a grid with
  // of N1 and N2 dimensions along its principal directions. For example, the
  // principal directions of a 3-D grid are Ni and Nj and likewise for a 2-D
  // grid along the XY plane. For a grid in the XZ plane however, the principal
  // directions are Ni and Nk.
  static vtkIdType GetLinearIndex(
      const int i, const int j, const int k, const int N1, const int N2 )
    {
      return( (k*N2+j)*N1+i );
    }

  // Description:
  // Returns the structured coordinates (i,j,k) for the given linear index of
  // a grid with N1 and N2 dimensions along its principal directions.
  // NOTE: i,j,k are relative to the frame of reference of the grid. For example,
  // if the grid is on the XZ-Plane, then i=>i, j=>k, k=>j.
  static void GetStructuredCoordinates(
      const vtkIdType idx, const int N1, const int N2,int &i, int &j, int &k )
    {
      int N12 = N1*N2;
      k = idx/N12;
      j = (idx-k*N12)/N1;
      i = idx-k*N12-j*N1;
    }

private:
  vtkStructuredData(const vtkStructuredData&);  // Not implemented.
  void operator=(const vtkStructuredData&);  // Not implemented.
};


#endif

// VTK-HeaderTest-Exclude: vtkStructuredData.h
