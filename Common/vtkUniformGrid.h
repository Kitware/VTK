/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUniformGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkUniformGrid - topologically and geometrically regular array of data
// .SECTION Description
// vtkUniformGrid is a data object that is a concrete implementation of
// vtkDataSet. vtkUniformGrid represents a geometric structure that is 
// a topological and geometrical regular array of points. vtkUniformGrid 
// is essentially a simple vtkImageData that supports blanking.

#ifndef __vtkUniformGrid_h
#define __vtkUniformGrid_h

#include "vtkDataSet.h"

#include "vtkStructuredData.h" // Needed for inline methods

class vtkDataArray;
class vtkEmptyCell;
class vtkImageData;
class vtkLine;
class vtkPixel;
class vtkVertex;
class vtkVoxel;
class vtkStructuredVisibilityConstraint;
class vtkUnsignedCharArray;

class VTK_COMMON_EXPORT vtkUniformGrid : public vtkDataSet
{
public:
  static vtkUniformGrid *New();

  vtkTypeRevisionMacro(vtkUniformGrid,vtkDataSet);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Copy the geometric and topological structure of an input image data
  // object.
  void CopyStructure(vtkDataSet *ds);

  // Description:
  // Return what type of dataset this is.
  int GetDataObjectType() {return VTK_UNIFORM_GRID;};

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
                                      this->GetDimensions());}
  void GetPointCells(vtkIdType ptId, vtkIdList *cellIds)
    {vtkStructuredData::GetPointCells(ptId,cellIds,this->GetDimensions());}
  void ComputeBounds();
  void Initialize();
  int GetMaxCellSize() {return 8;}; //voxel is the largest
  virtual void GetScalarRange(double range[2]);
  double *GetScalarRange() {return this->Superclass::GetScalarRange();}

  // Description:
  // For streaming.  User/next filter specifies which piece the want updated.
  // The source of this data has to return exactly this piece.
  void SetUpdateExtent(int piece, int numPieces, int ghostLevel);
  void SetUpdateExtent(int piece, int numPieces)
    {this->SetUpdateExtent(piece, numPieces, 0);}

  // Description:
  // Call superclass method to avoid hiding
  // Since this data type does not use 3D extents, this set method
  // is useless but necessary since vtkDataSetToDataSetFilter does not
  // know what type of data it is working on.
  void SetUpdateExtent( int x1, int x2, int y1, int y2, int z1, int z2 )
    { this->Superclass::SetUpdateExtent( x1, x2, y1, y2, z1, z2 ); };
  void SetUpdateExtent( int ext[6] )
    { this->Superclass::SetUpdateExtent( ext ); };

  // Description:
  // Set dimensions of structured points dataset.
  void SetDimensions(int i, int j, int k);

  // Description:
  // Set dimensions of structured points dataset.
  void SetDimensions(int dims[3]);

  // Description:
  // Get dimensions of this structured points dataset.
  // Dimensions are computed from Extents during this call.
  int *GetDimensions();
  void GetDimensions(int dims[3]);

  // Description:
  // Convenience function computes the structured coordinates for a point x[3].
  // The voxel is specified by the array ijk[3], and the parametric coordinates
  // in the cell are specified with pcoords[3]. The function returns a 0 if the
  // point x is outside of the volume, and a 1 if inside the volume.
  int ComputeStructuredCoordinates(double x[3], int ijk[3], double pcoords[3]);
  
  // Description:
  // Return the dimensionality of the data.
  int GetDataDimension();

  // Description:
  // Description:
  // Different ways to set the extent of the data array.  The extent
  // should be set before the "Scalars" are set or allocated.
  // The Extent is stored  in the order (X, Y, Z).
  void SetExtent(int extent[6]);
  void SetExtent(int x1, int x2, int y1, int y2, int z1, int z2);
  vtkGetVector6Macro(Extent,int);

  // Description:
  // Return the actual size of the data in kilobytes. This number
  // is valid only after the pipeline has updated. The memory size
  // returned is guaranteed to be greater than or equal to the
  // memory required to represent the data (e.g., extra space in
  // arrays, etc. are not included in the return value). THIS METHOD
  // IS THREAD SAFE.
  unsigned long GetActualMemorySize();
  
  // Description:
  // Set the spacing (width,height,length) of the cubical cells that
  // compose the data set.
  vtkSetVector3Macro(Spacing,double);
  vtkGetVector3Macro(Spacing,double);
  
  // Description:
  // Set the origin of the data. The origin plus spacing determine the
  // position in space of the points.
  vtkSetVector3Macro(Origin,double);
  vtkGetVector3Macro(Origin,double);
  
  // Description:
  // Shallow and Deep copy.
  void ShallowCopy(vtkDataObject *src);  
  void DeepCopy(vtkDataObject *src);
  
  // Description:
  // The extent type is a 3D extent
  int GetExtentType() { return VTK_3D_EXTENT; };

  // Description:
  // Methods for supporting blanking of cells. Blanking turns on or off
  // points in the structured grid, and hence the cells connected to them.
  // These methods should be called only after the dimensions of the
  // grid are set.
  void BlankPoint(vtkIdType ptId);
  void UnBlankPoint(vtkIdType ptId);

  // Description:
  // Methods for supporting blanking of cells. Blanking turns on or off
  // cells in the structured grid.
  // These methods should be called only after the dimensions of the
  // grid are set.
  void BlankCell(vtkIdType ptId);
  void UnBlankCell(vtkIdType ptId);
  
  // Description:
  // Get the array that defines the blanking (visibility) of each point.
  vtkUnsignedCharArray *GetPointVisibilityArray(); 

  // Description:
  // Set an array that defines the (blanking) visibility of the points 
  // in the grid. Make sure that length of the visibility array matches 
  // the number of points in the grid.
  void SetPointVisibilityArray(vtkUnsignedCharArray *pointVisibility);

  // Description:
  // Get the array that defines the blanking (visibility) of each cell.
  vtkUnsignedCharArray *GetCellVisibilityArray(); 

  // Description:
  // Set an array that defines the (blanking) visibility of the cells 
  // in the grid. Make sure that length of the visibility array matches 
  // the number of points in the grid.
  void SetCellVisibilityArray(vtkUnsignedCharArray *pointVisibility);

  // Description:
  // Return non-zero value if specified point is visible.
  // These methods should be called only after the dimensions of the
  // grid are set.
  unsigned char IsPointVisible(vtkIdType ptId);
  
  // Description:
  // Return non-zero value if specified point is visible.
  // These methods should be called only after the dimensions of the
  // grid are set.
  unsigned char IsCellVisible(vtkIdType cellId);

  // Description:
  // Returns 1 if there is any visibility constraint on the points,
  // 0 otherwise.
  unsigned char GetPointBlanking();

  // Description:
  // Returns 1 if there is any visibility constraint on the cells,
  // 0 otherwise.
  unsigned char GetCellBlanking();

protected:
  vtkUniformGrid();
  ~vtkUniformGrid();

  // for the GetCell method
  vtkVertex *Vertex;
  vtkLine *Line;
  vtkPixel *Pixel;
  vtkVoxel *Voxel;
  vtkEmptyCell *EmptyCell;

  // The extent of what is currently in the structured grid.
  // Dimensions is just an array to return a value.
  // Its contents are out of data until GetDimensions is called.
  int Dimensions[3];
  int DataDescription;

  double Origin[3];
  double Spacing[3];

  vtkStructuredVisibilityConstraint* PointVisibility;

  void SetPointVisibility(vtkStructuredVisibilityConstraint *pointVisibility);
  vtkGetObjectMacro(PointVisibility, vtkStructuredVisibilityConstraint);

  vtkStructuredVisibilityConstraint* CellVisibility;

  void SetCellVisibility(vtkStructuredVisibilityConstraint *cellVisibility);
  vtkGetObjectMacro(CellVisibility, vtkStructuredVisibilityConstraint);

private:
  void InternalUniformGridCopy(vtkUniformGrid *src);
  void InternalUniformGridCopy(vtkImageData *src);
private:
  vtkUniformGrid(const vtkUniformGrid&);  // Not implemented.
  void operator=(const vtkUniformGrid&);  // Not implemented.
};


inline void vtkUniformGrid::GetPoint(vtkIdType id, double x[3])
{
  double *p=this->GetPoint(id);
  x[0] = p[0]; x[1] = p[1]; x[2] = p[2];
}



inline vtkIdType vtkUniformGrid::GetNumberOfPoints()
{
  int *dims = this->GetDimensions();
  return dims[0]*dims[1]*dims[2];
}

inline int vtkUniformGrid::GetDataDimension()
{
  return vtkStructuredData::GetDataDimension(this->DataDescription);
}

#endif



