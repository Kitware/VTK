/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSet.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkDataSet - abstract class to specify dataset behavior
// .SECTION Description
// vtkDataSet is an abstract class that specifies an interface for dataset
// objects. vtkDataSet also provides methods to provide informations about
// the data, such as center, bounding box, and representative length.
//
// In vtk a dataset consists of a structure (geometry and topology) and 
// attribute data. The structure is defined implicitly or explicitly as
// a collection of cells. The geometry of the structure is contained in
// point coordinates plus the cell interpolation functions. The topology
// of the dataset structure is defined by cell types and how the cells
// share their defining points. 
//
// Attribute data in vtk is either point data (data at points) or cell data
// (data at cells). Typically filters operate on point data, but some may
// operate on cell data, both cell and point data, eithoer one, or none.

// .SECTION See Also
// vtkPointSet vtkStructuredPoints vtkStructuredGrid vtkUnstructuredGrid
// vtkRectilinearGrid vtkPolyData vtkPointData vtkCellData
// vtkDataObject vtkField

#ifndef __vtkDataSet_h
#define __vtkDataSet_h

#include "vtkDataObject.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkGenericCell.h"
#include "vtkCellTypes.h"

class VTK_EXPORT vtkDataSet : public vtkDataObject
{
public:
  const char *GetClassName() {return "vtkDataSet";};
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Copy the geometric and topological structure of an object. Note that
  // the invoking object and the object pointed to by the parameter ds must
  // be of the same type.
  // THIS METHOD IS NOT THREAD SAFE.
  virtual void CopyStructure(vtkDataSet *ds) = 0;

  // Description:
  // Determine the number of points composing the dataset.
  // THIS METHOD IS THREAD SAFE
  virtual int GetNumberOfPoints() = 0;

  // Description:
  // Determine the number of cells composing the dataset.
  // THIS METHOD IS THREAD SAFE
  virtual int GetNumberOfCells() = 0;

  // Description:
  // Get point coordinates with ptId such that: 0 <= ptId < NumberOfPoints.
  // THIS METHOD IS NOT THREAD SAFE.
  virtual float *GetPoint(int ptId) = 0;

  // Description:
  // Copy point coordinates into user provided array x[3] for specified
  // point id.
  // THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
  // THE DATASET IS NOT MODIFIED
  virtual void GetPoint(int id, float x[3]);

  // Description:
  // Get cell with cellId such that: 0 <= cellId < NumberOfCells.
  // THIS METHOD IS NOT THREAD SAFE.
  virtual vtkCell *GetCell(int cellId) = 0;

  // Description:
  // Get cell with cellId such that: 0 <= cellId < NumberOfCells. 
  // This is a thread-safe alternative to the previous GetCell()
  // method.
  // THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
  // THE DATASET IS NOT MODIFIED
  virtual void GetCell(int cellId, vtkGenericCell *cell) = 0;

  // Description:
  // Get the bounds of the cell with cellId such that:
  //     0 <= cellId < NumberOfCells.
  // A subclass may be able to determine the bounds of cell without using
  // an expensive GetCell() method. A default implementation is provided
  // that actually uses a GetCell() call.  This is to ensure the method
  // is available to all datasets.  Subclasses should override this method
  // to provide an efficient implementation.
  // THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
  // THE DATASET IS NOT MODIFIED
  virtual void GetCellBounds(int cellId, float bounds[6]);
  
  // Description:
  // Get type of cell with cellId such that: 0 <= cellId < NumberOfCells.
  // THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
  // THE DATASET IS NOT MODIFIED
  virtual int GetCellType(int cellId) = 0;

  // Description:
  // Get a list of types of cells in a dataset. The list consists of an array
  // of types (not necessarily in any order), with a single entry per type.
  // For example a dataset 5 triangles, 3 lines, and 100 hexahedra would
  // result a list of three entries, corresponding to the types VTK_TRIANGLE,
  // VTK_LINE, and VTK_HEXAHEDRON.
  // THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
  // THE DATASET IS NOT MODIFIED
  virtual void GetCellTypes(vtkCellTypes *types);

  // Description:
  // Topological inquiry to get points defining cell.
  // THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
  // THE DATASET IS NOT MODIFIED
  virtual void GetCellPoints(int cellId, vtkIdList *ptIds) = 0;

  // Description:
  // Topological inquiry to get cells using point.
  // THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
  // THE DATASET IS NOT MODIFIED
  virtual void GetPointCells(int ptId, vtkIdList *cellIds) = 0;

  // Description:
  // Topological inquiry to get all cells using list of points exclusive of
  // cell specified (e.g., cellId).
  // THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
  // THE DATASET IS NOT MODIFIED
  virtual void GetCellNeighbors(int cellId, vtkIdList *ptIds, 
				vtkIdList *cellIds);

  // Description:
  // Locate the closest point to the global coordinate x. Return the
  // point id. If point id < 0; then no point found. (This may arise
  // when point is outside of dataset.)
  // THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
  // THE DATASET IS NOT MODIFIED
  int FindPoint(float x, float y, float z)
    {
    float xyz[3];
    xyz[0] = x; xyz[1] = y; xyz[2] = z;
    return this->FindPoint (xyz);
    }
  virtual int FindPoint(float x[3]) = 0;

  // Description:
  // Locate cell based on global coordinate x and tolerance
  // squared. If cell and cellId is non-NULL, then search starts from
  // this cell and looks at immediate neighbors.  Returns cellId >= 0
  // if inside, < 0 otherwise.  The parametric coordinates are
  // provided in pcoords[3]. The interpolation weights are returned in
  // weights[]. (The number of weights is equal to the number of
  // points in the found cell). Tolerance is used to control how close
  // the point is to be considered "in" the cell.
  // THIS METHOD IS NOT THREAD SAFE.
  virtual int FindCell(float x[3], vtkCell *cell, int cellId, float tol2, 
                       int& subId, float pcoords[3], float *weights) = 0;

  // Description:
  // This is a version of the above method that can be used with 
  // multithreaded applications. A vtkGenericCell must be passes in
  // to be used in internal calls that might be made to GetCell()
  // THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
  // THE DATASET IS NOT MODIFIED
  virtual int FindCell(float x[3], vtkCell *cell, vtkGenericCell *gencell,
		       int cellId, float tol2, int& subId, float pcoords[3], 
		       float *weights) = 0;
  
  // Description:
  // Locate the cell that contains a point and return the cell. Also returns
  // the subcell id, parametric coordinates and weights for subsequent
  // interpolation. This method combines the derived class methods
  // int FindCell and vtkCell *GetCell. Derived classes may provide a more 
  // efficient implementation. See for example vtkStructuredPoints.
  // THIS METHOD IS NOT THREAD SAFE.
  virtual vtkCell *FindAndGetCell(float x[3], vtkCell *cell, int cellId, 
				  float tol2, int& subId, float pcoords[3], 
				  float *weights);

  // Description:
  // Datasets are composite objects and need to check each part for MTime
  // THIS METHOD IS THREAD SAFE
  unsigned long int GetMTime();

  // Description:
  // return pointer to this dataset's point data
  // THIS METHOD IS THREAD SAFE
  vtkCellData *GetCellData() {return this->CellData;};

  // Description:
  // return pointer to this dataset's point data
  // THIS METHOD IS THREAD SAFE
  vtkPointData *GetPointData() {return this->PointData;};

  // Description:
  // Reclaim any extra memory used to store data.
  // THIS METHOD IS NOT THREAD SAFE.
  virtual void Squeeze();

  // Description:
  // Compute the data bounding box from data points.
  // THIS METHOD IS NOT THREAD SAFE.
  virtual void ComputeBounds();

  // Description:
  // Return a pointer to the geometry bounding box in the form
  // (xmin,xmax, ymin,ymax, zmin,zmax).
  // THIS METHOD IS NOT THREAD SAFE.
  float *GetBounds();

  // Description:
  // Return a pointer to the geometry bounding box in the form
  // (xmin,xmax, ymin,ymax, zmin,zmax).
  // THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
  // THE DATASET IS NOT MODIFIED
  void GetBounds(float bounds[6]);

  // Description:
  // Get the center of the bounding box.
  // THIS METHOD IS NOT THREAD SAFE.
  float *GetCenter();

  // Description:
  // Get the center of the bounding box.
  // THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
  // THE DATASET IS NOT MODIFIED
  void GetCenter(float center[3]);
  
  // Description:
  // Return the length of the diagonal of the bounding box.
  // THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
  // THE DATASET IS NOT MODIFIED
  float GetLength();

  // Description:
  // Restore data object to initial state,
  // THIS METHOD IS NOT THREAD SAFE.
  void Initialize();

  // Description:
  // Convenience method to get the range of the scalar data (if there is any 
  // scalar data). Returns the (min/max) range of combined point and cell data.
  // If there are no point or cell scalars the method will return (0,1).
  // Note: Update needs to be called to create the scalars.
  // THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
  // THE DATASET IS NOT MODIFIED
  void GetScalarRange(float range[2]);

  // Description:
  // Convenience method to get the range of the scalar data (if there is any 
  // scalar data). 
  // THIS METHOD IS NOT THREAD SAFE.
  float *GetScalarRange();
  
  // Description:
  // Convenience method returns largest cell size in dataset. This is generally
  // used to allocate memory for supporting data structures.
  // THIS METHOD IS THREAD SAFE
  virtual int GetMaxCellSize() = 0;

  // Description:
  // For legacy compatibility. Do not use.
  void GetCellPoints(int cellId, vtkIdList &ptIds)
    {this->GetCellPoints(cellId, &ptIds);}
  void GetPointCells(int ptId, vtkIdList &cellIds)
    {this->GetPointCells(ptId, &cellIds);}
  void GetCellNeighbors(int cellId, vtkIdList& ptIds, vtkIdList& cellIds)
    {this->GetCellNeighbors(cellId, &ptIds, &cellIds);}
  
  int GetDataObjectType() {return VTK_DATA_SET;}
  
  // Description:
  // NOW OBSOLETE.  Use GetDataType instead.
  virtual int GetDataSetType() {return this->GetDataObjectType();}
  
protected:
  // Constructor with default bounds (0,1, 0,1, 0,1).
  vtkDataSet();
  ~vtkDataSet();  
  vtkDataSet(const vtkDataSet& ds);

  vtkCellData *CellData;   // Scalars, vectors, etc. associated w/ each cell
  vtkPointData *PointData;   // Scalars, vectors, etc. associated w/ each point
  vtkTimeStamp ComputeTime; // Time at which bounds, center, etc. computed
  float Bounds[6];  // (xmin,xmax, ymin,ymax, zmin,zmax) geometric bounds
  float ScalarRange[2];
  float Center[3];
};

inline void vtkDataSet::GetPoint(int id, float x[3])
{
  float *pt = this->GetPoint(id);
  x[0] = pt[0]; x[1] = pt[1]; x[2] = pt[2]; 
}

#endif
