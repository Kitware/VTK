/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSet.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
// operate on cell data, both cell and point data, either one, or none.

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

class VTK_COMMON_EXPORT vtkDataSet : public vtkDataObject
{
public:
  vtkTypeMacro(vtkDataSet,vtkDataObject);
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
  virtual vtkIdType GetNumberOfPoints() = 0;

  // Description:
  // Determine the number of cells composing the dataset.
  // THIS METHOD IS THREAD SAFE
  virtual vtkIdType GetNumberOfCells() = 0;

  // Description:
  // Get point coordinates with ptId such that: 0 <= ptId < NumberOfPoints.
  // THIS METHOD IS NOT THREAD SAFE.
  virtual float *GetPoint(vtkIdType ptId) = 0;

  // Description:
  // Copy point coordinates into user provided array x[3] for specified
  // point id.
  // THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
  // THE DATASET IS NOT MODIFIED
  virtual void GetPoint(vtkIdType id, float x[3]);

  // Description:
  // Get cell with cellId such that: 0 <= cellId < NumberOfCells.
  // THIS METHOD IS NOT THREAD SAFE.
  virtual vtkCell *GetCell(vtkIdType cellId) = 0;

  // Description:
  // Get cell with cellId such that: 0 <= cellId < NumberOfCells. 
  // This is a thread-safe alternative to the previous GetCell()
  // method.
  // THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
  // THE DATASET IS NOT MODIFIED
  virtual void GetCell(vtkIdType cellId, vtkGenericCell *cell) = 0;

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
  virtual void GetCellBounds(vtkIdType cellId, float bounds[6]);
  
  // Description:
  // Get type of cell with cellId such that: 0 <= cellId < NumberOfCells.
  // THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
  // THE DATASET IS NOT MODIFIED
  virtual int GetCellType(vtkIdType cellId) = 0;

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
  virtual void GetCellPoints(vtkIdType cellId, vtkIdList *ptIds) = 0;

  // Description:
  // Topological inquiry to get cells using point.
  // THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
  // THE DATASET IS NOT MODIFIED
  virtual void GetPointCells(vtkIdType ptId, vtkIdList *cellIds) = 0;

  // Description:
  // Topological inquiry to get all cells using list of points exclusive of
  // cell specified (e.g., cellId). Note that the list consists of only
  // cells that use ALL the points provided.
  // THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
  // THE DATASET IS NOT MODIFIED
  virtual void GetCellNeighbors(vtkIdType cellId, vtkIdList *ptIds, 
				vtkIdList *cellIds);

  // Description:
  // Locate the closest point to the global coordinate x. Return the
  // point id. If point id < 0; then no point found. (This may arise
  // when point is outside of dataset.)
  // THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
  // THE DATASET IS NOT MODIFIED
  vtkIdType FindPoint(float x, float y, float z)
    {
    float xyz[3];
    xyz[0] = x; xyz[1] = y; xyz[2] = z;
    return this->FindPoint (xyz);
    }
  virtual vtkIdType FindPoint(float x[3]) = 0;

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
  virtual vtkIdType FindCell(float x[3], vtkCell *cell, vtkIdType cellId,
                             float tol2, int& subId, float pcoords[3],
                             float *weights) = 0;

  // Description:
  // This is a version of the above method that can be used with 
  // multithreaded applications. A vtkGenericCell must be passed in
  // to be used in internal calls that might be made to GetCell()
  // THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
  // THE DATASET IS NOT MODIFIED
  virtual vtkIdType FindCell(float x[3], vtkCell *cell,
                             vtkGenericCell *gencell, vtkIdType cellId,
                             float tol2, int& subId, float pcoords[3],
                             float *weights) = 0;
  
  // Description:
  // Locate the cell that contains a point and return the cell. Also returns
  // the subcell id, parametric coordinates and weights for subsequent
  // interpolation. This method combines the derived class methods
  // int FindCell and vtkCell *GetCell. Derived classes may provide a more 
  // efficient implementation. See for example vtkStructuredPoints.
  // THIS METHOD IS NOT THREAD SAFE.
  virtual vtkCell *FindAndGetCell(float x[3], vtkCell *cell, vtkIdType cellId, 
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
  virtual void GetScalarRange(float range[2]);

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
  // Return the actual size of the data in kilobytes. This number
  // is valid only after the pipeline has updated. The memory size
  // returned is guaranteed to be greater than or equal to the
  // memory required to represent the data (e.g., extra space in
  // arrays, etc. are not included in the return value). THIS METHOD
  // IS THREAD SAFE.
  unsigned long GetActualMemorySize();
  
  // Description:
  // Return the type of data object.
  int GetDataObjectType() 
    {return VTK_DATA_SET;}
  
  // Description:
  // Shallow and Deep copy.
  void ShallowCopy(vtkDataObject *src);  
  void DeepCopy(vtkDataObject *src);

//BTX
  enum FieldDataType 
  {
    DATA_OBJECT_FIELD=0,
    POINT_DATA_FIELD=1,
    CELL_DATA_FIELD=2
  };
//ETX
  
protected:
  // Constructor with default bounds (0,1, 0,1, 0,1).
  vtkDataSet();
  ~vtkDataSet();  
  vtkDataSet(const vtkDataSet&);
  void operator=(const vtkDataSet&);  

  vtkCellData *CellData;   // Scalars, vectors, etc. associated w/ each cell
  vtkPointData *PointData;   // Scalars, vectors, etc. associated w/ each point
  vtkTimeStamp ComputeTime; // Time at which bounds, center, etc. computed
  float Bounds[6];  // (xmin,xmax, ymin,ymax, zmin,zmax) geometric bounds
  float ScalarRange[2];
  float Center[3];

private:
  void InternalDataSetCopy(vtkDataSet *src);  
};

inline void vtkDataSet::GetPoint(vtkIdType id, float x[3])
{
  float *pt = this->GetPoint(id);
  x[0] = pt[0]; x[1] = pt[1]; x[2] = pt[2]; 
}

#endif
