/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRectilinearGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkRectilinearGrid - a dataset that is topologically regular with variable spacing in the three coordinate directions
// .SECTION Description
// vtkRectilinearGrid is a data object that is a concrete implementation of
// vtkDataSet. vtkRectilinearGrid represents a geometric structure that is
// topologically regular with variable spacing in the three coordinate
// directions x-y-z.
//
// To define a vtkRectilinearGrid, you must specify the dimensions of the
// data and provide three arrays of values specifying the coordinates
// along the x-y-z axes. The coordinate arrays are specified using three
// vtkDataArray objects (one for x, one for y, one for z).

// .SECTION Caveats
// Make sure that the dimensions of the grid match the number of coordinates
// in the x-y-z directions. If not, unpredictable results (including
// program failure) may result. Also, you must supply coordinates in all
// three directions, even if the dataset topology is 2D, 1D, or 0D.

#ifndef __vtkRectilinearGrid_h
#define __vtkRectilinearGrid_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDataSet.h"
#include "vtkStructuredData.h" // For inline methods

class vtkVertex;
class vtkLine;
class vtkPixel;
class vtkVoxel;
class vtkDataArray;
class vtkPoints;

class VTKCOMMONDATAMODEL_EXPORT vtkRectilinearGrid : public vtkDataSet
{
public:
  static vtkRectilinearGrid *New();

  vtkTypeMacro(vtkRectilinearGrid,vtkDataSet);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return what type of dataset this is.
  int GetDataObjectType() {return VTK_RECTILINEAR_GRID;};

  // Description:
  // Copy the geometric and topological structure of an input rectilinear grid
  // object.
  void CopyStructure(vtkDataSet *ds);

  // Description:
  // Restore object to initial state. Release memory back to system.
  void Initialize();

  // Description:
  // Standard vtkDataSet API methods. See vtkDataSet for more information.
  vtkIdType GetNumberOfCells();
  vtkIdType GetNumberOfPoints();
  double *GetPoint(vtkIdType ptId);
  void GetPoint(vtkIdType id, double x[3]);
  vtkCell *GetCell(vtkIdType cellId);
  void GetCell(vtkIdType cellId, vtkGenericCell *cell);
  void GetCellBounds(vtkIdType cellId, double bounds[6]);
  vtkIdType FindPoint(double x, double y, double z) { return this->vtkDataSet::FindPoint(x, y, z);};
  vtkIdType FindPoint(double x[3]);
  vtkIdType FindCell(double x[3], vtkCell *cell, vtkIdType cellId, double tol2,
                     int& subId, double pcoords[3], double *weights);
  vtkIdType FindCell(double x[3], vtkCell *cell, vtkGenericCell *gencell,
                     vtkIdType cellId, double tol2, int& subId,
                     double pcoords[3], double *weights);
  vtkCell *FindAndGetCell(double x[3], vtkCell *cell, vtkIdType cellId,
                          double tol2, int& subId, double pcoords[3],
                          double *weights);
  int GetCellType(vtkIdType cellId);
  void GetCellPoints(vtkIdType cellId, vtkIdList *ptIds)
    {vtkStructuredData::GetCellPoints(cellId,ptIds,this->DataDescription,
                                      this->Dimensions);}
  void GetPointCells(vtkIdType ptId, vtkIdList *cellIds)
    {vtkStructuredData::GetPointCells(ptId,cellIds,this->Dimensions);}
  void ComputeBounds();
  int GetMaxCellSize() {return 8;}; //voxel is the largest
  void GetCellNeighbors(vtkIdType cellId, vtkIdList *ptIds,
                        vtkIdList *cellIds);

  // Description:
  // Given a user-supplied vtkPoints container object, this method fills in all
  // the points of the RectilinearGrid.
  void GetPoints(vtkPoints* pnts);

  // Description:
  // Set dimensions of rectilinear grid dataset.
  // This also sets the extent.
  void SetDimensions(int i, int j, int k);
  void SetDimensions(int dim[3]);

  // Description:
  // Get dimensions of this rectilinear grid dataset.
  vtkGetVectorMacro(Dimensions,int,3);

  // Description:
  // Return the dimensionality of the data.
  int GetDataDimension();

  // Description:
  // Convenience function computes the structured coordinates for a point x[3].
  // The cell is specified by the array ijk[3], and the parametric coordinates
  // in the cell are specified with pcoords[3]. The function returns a 0 if the
  // point x is outside of the grid, and a 1 if inside the grid.
  int ComputeStructuredCoordinates(double x[3], int ijk[3], double pcoords[3]);

  // Description:
  // Given a location in structured coordinates (i-j-k), return the point id.
  vtkIdType ComputePointId(int ijk[3]);

  // Description:
  // Given a location in structured coordinates (i-j-k), return the cell id.
  vtkIdType ComputeCellId(int ijk[3]);

  // Description:
  // Given the IJK-coordinates of the point, it returns the corresponding
  // xyz-coordinates. The xyz coordinates are stored in the user-supplied
  // array p.
  void GetPoint(const int i,const int j,const int k,double p[3]);

  // Description:
  // Specify the grid coordinates in the x-direction.
  virtual void SetXCoordinates(vtkDataArray*);
  vtkGetObjectMacro(XCoordinates,vtkDataArray);

  // Description:
  // Specify the grid coordinates in the y-direction.
  virtual void SetYCoordinates(vtkDataArray*);
  vtkGetObjectMacro(YCoordinates,vtkDataArray);

  // Description:
  // Specify the grid coordinates in the z-direction.
  virtual void SetZCoordinates(vtkDataArray*);
  vtkGetObjectMacro(ZCoordinates,vtkDataArray);

  // Description:
  // Different ways to set the extent of the data array.  The extent
  // should be set before the "Scalars" are set or allocated.
  // The Extent is stored  in the order (X, Y, Z).
  void SetExtent(int extent[6]);
  void SetExtent(int x1, int x2, int y1, int y2, int z1, int z2);
  vtkGetVector6Macro(Extent, int);

  // Description:
  // Return the actual size of the data in kilobytes. This number
  // is valid only after the pipeline has updated. The memory size
  // returned is guaranteed to be greater than or equal to the
  // memory required to represent the data (e.g., extra space in
  // arrays, etc. are not included in the return value). THIS METHOD
  // IS THREAD SAFE.
  unsigned long GetActualMemorySize();

  // Description:
  // Shallow and Deep copy.
  void ShallowCopy(vtkDataObject *src);
  void DeepCopy(vtkDataObject *src);

  // Description:
  // Structured extent. The extent type is a 3D extent
  int GetExtentType() { return VTK_3D_EXTENT; };

  // Description:
  // Reallocates and copies to set the Extent to the UpdateExtent.
  // This is used internally when the exact extent is requested,
  // and the source generated more than the update extent.
  virtual void Crop(const int* updateExtent);

  //BTX
  // Description:
  // Retrieve an instance of this class from an information object.
  static vtkRectilinearGrid* GetData(vtkInformation* info);
  static vtkRectilinearGrid* GetData(vtkInformationVector* v, int i=0);
  //ETX

protected:
  vtkRectilinearGrid();
  ~vtkRectilinearGrid();

  // for the GetCell method
  vtkVertex *Vertex;
  vtkLine *Line;
  vtkPixel *Pixel;
  vtkVoxel *Voxel;

  int Dimensions[3];
  int DataDescription;

  int Extent[6];

  vtkDataArray *XCoordinates;
  vtkDataArray *YCoordinates;
  vtkDataArray *ZCoordinates;

  // Hang on to some space for returning points when GetPoint(id) is called.
  double PointReturn[3];

private:
  void Cleanup();

private:
  vtkRectilinearGrid(const vtkRectilinearGrid&);  // Not implemented.
  void operator=(const vtkRectilinearGrid&);  // Not implemented.
};

//----------------------------------------------------------------------------
inline vtkIdType vtkRectilinearGrid::GetNumberOfCells()
{
  vtkIdType nCells=1;
  int i;

  for (i=0; i<3; i++)
    {
    if (this->Dimensions[i] <= 0)
      {
      return 0;
      }
    if (this->Dimensions[i] > 1)
      {
      nCells *= (this->Dimensions[i]-1);
      }
    }

  return nCells;
}

//----------------------------------------------------------------------------
inline vtkIdType vtkRectilinearGrid::GetNumberOfPoints()
{
  return this->Dimensions[0]*this->Dimensions[1]*this->Dimensions[2];
}

//----------------------------------------------------------------------------
inline int vtkRectilinearGrid::GetDataDimension()
{
  return vtkStructuredData::GetDataDimension(this->DataDescription);
}

//----------------------------------------------------------------------------
inline vtkIdType vtkRectilinearGrid::ComputePointId(int ijk[3])
{
  return vtkStructuredData::ComputePointId(this->Dimensions,ijk);
}

//----------------------------------------------------------------------------
inline vtkIdType vtkRectilinearGrid::ComputeCellId(int ijk[3])
{
  return vtkStructuredData::ComputeCellId(this->Dimensions,ijk);
}

#endif
