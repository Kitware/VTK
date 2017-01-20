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
/**
 * @class   vtkStructuredData
 * @brief   Singleton class for topologically regular data
 *
 *
 * vtkStructuredData is a singleton class that provides an interface for
 * topologically regular data. Regular data is data that can be accessed
 * in rectangular fashion using an i-j-k index. A finite difference grid,
 * a volume, or a pixmap are all considered regular.
 *
 * @sa
 * vtkStructuredGrid vtkUniformGrid vtkRectilinearGrid vtkRectilinearGrid
*/

#ifndef vtkStructuredData_h
#define vtkStructuredData_h

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

  //@{
  /**
   * Specify the dimensions of a regular, rectangular dataset. The input is
   * the new dimensions (inDim) and the current dimensions (dim). The function
   * returns the dimension of the dataset (0-3D). If the dimensions are
   * improperly specified a -1 is returned. If the dimensions are unchanged, a
   * value of 100 is returned.
   */
  static int SetDimensions(int inDim[3], int dim[3]);
  static int SetExtent(int inExt[6], int ext[6]);
  //@}

  //@{
  /**
   * Returns the data description given the dimensions (eg. VTK_SINGLE_POINT,
   * VTK_X_LINE, VTK_XY_PLANE etc.)
   */
  static int GetDataDescription(int dims[3]);
  static int GetDataDescriptionFromExtent( int ext[6] );
  //@}

  //@{
  /**
   * Return the topological dimension of the data (e.g., 0, 1, 2, or 3D).
   */
  static int GetDataDimension(int dataDescription);
  static int GetDataDimension( int ext[6] );
  //@}

  /**
   * Given the grid extent, this method returns the total number of points
   * within the extent.
   * The dataDescription field is not used.
   */
  static vtkIdType GetNumberOfPoints(int ext[6], int dataDescription=VTK_EMPTY);

  /**
   * Given the grid extent, this method returns the total number of cells
   * within the extent.
   * The dataDescription field is not used.
   */
  static vtkIdType GetNumberOfCells(int ext[6], int dataDescription=VTK_EMPTY);

  /**
   * Given the point extent of a grid, this method computes the corresponding
   * cell extent for the grid.
   * The dataDescription field is not used.
   */
  static void GetCellExtentFromPointExtent(
      int pntExtent[6], int cellExtent[6], int dataDescription=VTK_EMPTY );

  /**
   * Computes the structured grid dimensions based on the given extent.
   * The dataDescription field is not used.
   */
  static void GetDimensionsFromExtent(
      int ext[6], int dims[3], int dataDescription=VTK_EMPTY );

  /**
   * Returns the cell dimensions, i.e., the number of cells along the i,j,k
   * for the grid with the given grid extent. Note, the grid extent is the
   * number of points.
   * The dataDescription field is not used.
   */
  static void GetCellDimensionsFromExtent(
      int ext[6], int celldims[3], int dataDescription=VTK_EMPTY );

  /**
   * Given the dimensions of the grid, in pntdims, this method returns
   * the corresponding cell dimensions for the given grid.
   * The dataDescription field is not used.
   */
  static void GetCellDimensionsFromPointDimensions(
      int pntdims[3],int cellDims[3] );

  /**
   * Given the global structured coordinates for a point or cell, ijk, w.r.t.
   * as well as, the global sub-grid cell or point extent, this method computes
   * the corresponding local structured coordinates, lijk, starting from 0.
   * The dataDescription argument is not used.
   */
  static void GetLocalStructuredCoordinates(
      int ijk[3], int ext[6], int lijk[3], int dataDescription=VTK_EMPTY );

  /**
   * Given local structured coordinates, and the corresponding global sub-grid
   * extent, this method computes the global ijk coordinates.
   * The dataDescription parameter is not used.
   */
  static void GetGlobalStructuredCoordinates(
      int lijk[3], int ext[6], int ijk[3], int dataDescription=VTK_EMPTY );

  /**
   * Get the points defining a cell. (See vtkDataSet for more info.)
   */
  static void GetCellPoints(vtkIdType cellId, vtkIdList *ptIds,
                            int dataDescription, int dim[3]);

  /**
   * Get the cells using a point. (See vtkDataSet for more info.)
   */
  static void GetPointCells(vtkIdType ptId, vtkIdList *cellIds, int dim[3]);

  /**
   * Get the cells using the points ptIds, exclusive of the cell cellId.
   * (See vtkDataSet for more info.)
   */
  static void GetCellNeighbors(vtkIdType cellId, vtkIdList *ptIds,
                               vtkIdList *cellIds, int dim[3]);
  static void GetCellNeighbors(vtkIdType cellId, vtkIdList *ptIds,
                                 vtkIdList *cellIds, int dim[3], int seedLoc[3]);

  /**
   * Given a location in structured coordinates (i-j-k), and the extent
   * of the structured dataset, return the point id.
   * The dataDescription argument is not used.
   */
  static vtkIdType ComputePointIdForExtent(int extent[6], int ijk[3],
                                           int dataDescription=VTK_EMPTY );

  /**
   * Given a location in structured coordinates (i-j-k), and the extent
   * of the structured dataset, return the point id.
   * The dataDescription argument is not used.
   */
  static vtkIdType ComputeCellIdForExtent(
      int extent[6], int ijk[3], int dataDescription=VTK_EMPTY );

  /**
   * Given a location in structured coordinates (i-j-k), and the dimensions
   * of the structured dataset, return the point id.  This method does not
   * adjust for the beginning of the extent.
   * The dataDescription argument is not used.
   */
  static vtkIdType ComputePointId(
      int dim[3], int ijk[3], int dataDescription=VTK_EMPTY );

  /**
   * Given a location in structured coordinates (i-j-k), and the dimensions
   * of the structured dataset, return the cell id.  This method does not
   * adjust for the beginning of the extent.
   * The dataDescription argument is not used.
   */
  static vtkIdType ComputeCellId(
      int dim[3], int ijk[3], int dataDescription=VTK_EMPTY );

  /**
   * Given the global grid extent and the linear index of a cell within the
   * grid extent, this method computes the corresponding structured coordinates
   * of the given cell. This method adjusts for the beginning of the extent.
   * The dataDescription argument is not used.
   */
  static void ComputeCellStructuredCoordsForExtent(
      const vtkIdType cellIdx, int ext[6], int ijk[3],
      int dataDescription=VTK_EMPTY );

  /**
   * Given a cellId and grid dimensions 'dim', get the structured coordinates
   * (i-j-k). This method does not adjust for the beginning of the extent.
   * The dataDescription argument is not used.
   */
  static void ComputeCellStructuredCoords(
      const vtkIdType cellId, int dim[3], int ijk[3],
      int dataDescription=VTK_EMPTY );

  /**
   * Given a pointId and the grid extent ext, get the structured coordinates
   * (i-j-k). This method adjusts for the beginning of the extent.
   * The dataDescription argument is not used.
   */
  static void ComputePointStructuredCoordsForExtent(
      const vtkIdType ptId, int ext[6], int ijk[3],
      int dataDescription=VTK_EMPTY );

  /**
   * Given a pointId and grid dimensions 'dim', get the structured coordinates
   * (i-j-k). This method does not adjust for the beginning of the extent.
   * The dataDescription argument is not used.
   */
  static void ComputePointStructuredCoords(
      const vtkIdType ptId, int dim[3], int ijk[3],
      int dataDescription=VTK_EMPTY );

protected:
  vtkStructuredData() {}
  ~vtkStructuredData() VTK_OVERRIDE {}

  /**
   * Computes the linear index for the given i-j-k structured of a grid with
   * of N1 and N2 dimensions along its principal directions. For example, the
   * principal directions of a 3-D grid are Ni and Nj and likewise for a 2-D
   * grid along the XY plane. For a grid in the XZ plane however, the principal
   * directions are Ni and Nk.
   */
  static vtkIdType GetLinearIndex(
      const int i, const int j, const int k, const int N1, const int N2 )
  {
      return( (static_cast<vtkIdType>(k)*N2+j)*N1+i );
  }

  //@{
  /**
   * Returns the structured coordinates (i,j,k) for the given linear index of
   * a grid with N1 and N2 dimensions along its principal directions.
   * NOTE: i,j,k are relative to the frame of reference of the grid. For example,
   * if the grid is on the XZ-Plane, then i=>i, j=>k, k=>j.
   */
  static void GetStructuredCoordinates(
      const vtkIdType idx, const int N1, const int N2,int &i, int &j, int &k )
  {
      int N12 = N1*N2;
      k = idx/N12;
      j = (idx-k*N12)/N1;
      i = idx-k*N12-j*N1;
  }
  //@}

  // Want to avoid importing <algorithm> in the header...
  template <typename T>
  static T Max(const T &a, const T &b)
  {
    return (a > b) ? a : b;
  }

private:
  vtkStructuredData(const vtkStructuredData&) VTK_DELETE_FUNCTION;
  void operator=(const vtkStructuredData&) VTK_DELETE_FUNCTION;
};

//------------------------------------------------------------------------------
inline void vtkStructuredData::GetCellDimensionsFromExtent(
    int ext[6], int celldims[3], int)
{
  celldims[0] = vtkStructuredData::Max(ext[1] - ext[0], 0);
  celldims[1] = vtkStructuredData::Max(ext[3] - ext[2], 0);
  celldims[2] = vtkStructuredData::Max(ext[5] - ext[4], 0);
}

//------------------------------------------------------------------------------
inline vtkIdType vtkStructuredData::ComputePointId(int dims[3], int ijk[3], int)
{
  return vtkStructuredData::GetLinearIndex(ijk[0], ijk[1], ijk[2],
                                           dims[0], dims[1]);
}

//------------------------------------------------------------------------------
inline vtkIdType vtkStructuredData::ComputeCellId(int dims[3], int ijk[3], int)
{
  return vtkStructuredData::GetLinearIndex(
        ijk[0], ijk[1], ijk[2],
        vtkStructuredData::Max(dims[0] - 1, 1),
        vtkStructuredData::Max(dims[1] - 1, 1));
}

//------------------------------------------------------------------------------
inline vtkIdType vtkStructuredData::GetNumberOfPoints(int ext[6], int)
{
  return static_cast<vtkIdType>(ext[1] - ext[0] + 1) *
         static_cast<vtkIdType>(ext[3] - ext[2] + 1) *
         static_cast<vtkIdType>(ext[5] - ext[4] + 1);
}

//------------------------------------------------------------------------------
inline vtkIdType vtkStructuredData::GetNumberOfCells(int ext[6], int)
{
  int cellDims[3];
  vtkStructuredData::GetCellDimensionsFromExtent(ext,cellDims);

  // Replace 0's with 1's so we can just multiply them regardless of cell type.
  cellDims[0] = vtkStructuredData::Max(cellDims[0], 1);
  cellDims[1] = vtkStructuredData::Max(cellDims[1], 1);
  cellDims[2] = vtkStructuredData::Max(cellDims[2], 1);

  // Note, when we compute the result below, we statically cast to vtkIdType to
  // ensure the compiler will generate a 32x32=64 instruction.
  return static_cast<vtkIdType>(cellDims[0]) *
         static_cast<vtkIdType>(cellDims[1]) *
         static_cast<vtkIdType>(cellDims[2]);
}

//------------------------------------------------------------------------------
inline void vtkStructuredData::GetCellExtentFromPointExtent(
    int nodeExtent[6], int cellExtent[6], int)
{
  cellExtent[0] = nodeExtent[0];
  cellExtent[2] = nodeExtent[2];
  cellExtent[4] = nodeExtent[4];

  cellExtent[1] = vtkStructuredData::Max(nodeExtent[0], nodeExtent[1] - 1);
  cellExtent[3] = vtkStructuredData::Max(nodeExtent[2], nodeExtent[3] - 1);
  cellExtent[5] = vtkStructuredData::Max(nodeExtent[4], nodeExtent[5] - 1);
}

//------------------------------------------------------------------------------
inline void vtkStructuredData::GetDimensionsFromExtent(int ext[6], int dims[3],
                                                       int)
{
  dims[0] = ext[1] - ext[0] + 1;
  dims[1] = ext[3] - ext[2] + 1;
  dims[2] = ext[5] - ext[4] + 1;
}

//------------------------------------------------------------------------------
inline void vtkStructuredData::GetCellDimensionsFromPointDimensions(
    int nodeDims[3], int cellDims[3])
{
  cellDims[0] = vtkStructuredData::Max(nodeDims[0] - 1, 0);
  cellDims[1] = vtkStructuredData::Max(nodeDims[1] - 1, 0);
  cellDims[2] = vtkStructuredData::Max(nodeDims[2] - 1, 0);
}

//------------------------------------------------------------------------------
inline void vtkStructuredData::GetLocalStructuredCoordinates(
    int ijk[3], int ext[6], int lijk[3], int)
{
  lijk[0] = ijk[0] - ext[0];
  lijk[1] = ijk[1] - ext[2];
  lijk[2] = ijk[2] - ext[4];
}

//------------------------------------------------------------------------------
inline void vtkStructuredData::GetGlobalStructuredCoordinates(
    int lijk[3], int ext[6], int ijk[3], int)
{
  ijk[0] = ext[0] + lijk[0];
  ijk[1] = ext[2] + lijk[1];
  ijk[2] = ext[4] + lijk[2];
}

//------------------------------------------------------------------------------
inline vtkIdType vtkStructuredData::ComputePointIdForExtent(
    int extent[6], int ijk[3], int)
{
  int dims[3];
  vtkStructuredData::GetDimensionsFromExtent(extent, dims);

  int lijk[3];
  vtkStructuredData::GetLocalStructuredCoordinates(ijk, extent, lijk);

  return vtkStructuredData::ComputePointId(dims, lijk);
}

//------------------------------------------------------------------------------
inline vtkIdType vtkStructuredData::ComputeCellIdForExtent(
    int extent[6], int ijk[3], int)
{
  int nodeDims[3];
  vtkStructuredData::GetDimensionsFromExtent(extent, nodeDims);

  int lijk[3];
  vtkStructuredData::GetLocalStructuredCoordinates(ijk, extent, lijk);

  return vtkStructuredData::ComputeCellId(nodeDims, lijk);
}

//------------------------------------------------------------------------------
inline void vtkStructuredData::ComputeCellStructuredCoords(
    const vtkIdType cellId, int dims[3], int ijk[3], int)
{
  vtkStructuredData::GetStructuredCoordinates(cellId,
                                              dims[0] - 1, dims[1] - 1,
                                              ijk[0], ijk[1], ijk[2]);
}

//------------------------------------------------------------------------------
inline void vtkStructuredData::ComputeCellStructuredCoordsForExtent(
    const vtkIdType cellIdx, int ext[6], int ijk[3], int)
{
  int nodeDims[3];
  vtkStructuredData::GetDimensionsFromExtent(ext, nodeDims);

  int lijk[3];
  vtkStructuredData::ComputeCellStructuredCoords(cellIdx, nodeDims, lijk);

  vtkStructuredData::GetGlobalStructuredCoordinates(lijk, ext, ijk);
}

//------------------------------------------------------------------------------
inline void vtkStructuredData::ComputePointStructuredCoords(
    const vtkIdType ptId, int dim[3], int ijk[3], int)
{
  vtkStructuredData::GetStructuredCoordinates(ptId, dim[0], dim[1],
                                              ijk[0], ijk[1], ijk[2]);
}

//------------------------------------------------------------------------------
inline void vtkStructuredData::ComputePointStructuredCoordsForExtent(
    const vtkIdType ptId, int ext[6], int ijk[3], int)
{
  int nodeDims[3];
  vtkStructuredData::GetDimensionsFromExtent(ext, nodeDims);

  int lijk[3];
  vtkStructuredData::ComputePointStructuredCoords(ptId, nodeDims, lijk);

  vtkStructuredData::GetGlobalStructuredCoordinates(lijk, ext, ijk);
}

#endif

// VTK-HeaderTest-Exclude: vtkStructuredData.h
