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
// .NAME vtkTreeBasedAMRGrid - a dataset for tree-based AMR grids
//
// .SECTION Description
// vtkTreeBasedAMRGrid is a data object that is a concrete implementation of
// vtkDataSet. vtkTreeBasedAMRGrid represents a geometric structure that is 
// topologically regular with variable spacing in the three coordinate
// directions x-y-z, and where each
// base cell can be further refined into hierarchical trees with subdivision
// factor of either 2 or 3.
//
// To define a vtkTreeBasedAMRGrid, you must specify the dimensions of the
// data and provide three arrays of values specifying the coordinates 
// along the x-y-z axes. The coordinate arrays are specified using three 
// vtkDataArray objects (one for x, one for y, one for z).
// You must also specify the subdivision factor and the number of levels for
// each cell.
//
// .SECTION Caveats
// Make sure that the dimensions of the grid match the number of coordinates
// in the x-y-z directions. If not, unpredictable results (including
// program failure) may result. Also, you must supply coordinates in all
// three directions, even if the dataset topology is 2D, 1D, or 0D.
//
// .SECTION Thanks
// This class was written by Philippe Pebay, Kitware SAS 2012.
//
// .SECTION See also
// vtkHyperTree

#ifndef __vtkTreeBasedAMRGrid_h
#define __vtkTreeBasedAMRGrid_h

#include "vtkDataSet.h"
#include "vtkStructuredData.h" // For inline methods

class vtkDataArray;
class vtkHyperTree;
class vtkLine;
class vtkPixel;
class vtkPoints;
class vtkVertex;
class vtkVoxel;

class VTK_FILTERING_EXPORT vtkTreeBasedAMRGrid : public vtkDataSet
{
public:
  static vtkTreeBasedAMRGrid *New();

  vtkTypeMacro(vtkTreeBasedAMRGrid,vtkDataSet);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return what type of dataset this is.
  int GetDataObjectType() {return VTK_TREE_BASED_AMR;};

  // Description:
  // Copy the geometric and topological structure of an input tree based AMR
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
  // Returns the points for this instance of rectilinear grid.
  vtkPoints* GetPoints();

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
  // Shallow and Deep copy.
  void ShallowCopy(vtkDataObject *src);  
  void DeepCopy(vtkDataObject *src);

  // Description:
  // Structured extent. The extent type is a 3D extent
  int GetExtentType() { return VTK_3D_EXTENT; };

protected:
  vtkTreeBasedAMRGrid();
  ~vtkTreeBasedAMRGrid();

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
  vtkTreeBasedAMRGrid(const vtkTreeBasedAMRGrid&);  // Not implemented.
  void operator=(const vtkTreeBasedAMRGrid&);  // Not implemented.
};

//----------------------------------------------------------------------------
inline vtkIdType vtkTreeBasedAMRGrid::GetNumberOfCells() 
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
inline vtkIdType vtkTreeBasedAMRGrid::GetNumberOfPoints()
{
  return this->Dimensions[0]*this->Dimensions[1]*this->Dimensions[2];
}

//----------------------------------------------------------------------------
inline int vtkTreeBasedAMRGrid::GetDataDimension()
{
  return vtkStructuredData::GetDataDimension(this->DataDescription);
}

#endif
