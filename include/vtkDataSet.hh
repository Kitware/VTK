/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSet.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
// vtkDataSet is an abstract class that specifies an interface for 
// data objects. (Data objects are synonymous with datasets.) vtkDataSet
// also provides methods to provide informations about the data, such
// as center, bounding box, and representative length.

// .SECTION See Also
// vtkPointSet vtkStructuredPoints

#ifndef __vtkDataSet_h
#define __vtkDataSet_h

#include "vtkObject.hh"
#include "vtkIdList.hh"
#include "vtkFloatPoints.hh"
#include "vtkPointData.hh"
#include "vtkCell.hh"

class vtkSource;

class vtkDataSet : public vtkObject 
{
public:
  vtkDataSet();
  vtkDataSet(const vtkDataSet& ds);
  char *GetClassName() {return "vtkDataSet";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Provides opportunity for data to insure internal consistency before 
  // access. Also causes owning filter (if any) to update itself.
  virtual void Update();

  // Description:
  // Force the data to update itself no matter what.
  virtual void ForceUpdate();

  // Description:
  // Create concrete instance of this dataset.
  virtual vtkDataSet *MakeObject() = 0;

  // Description:
  // Copy the geometric and topological structure of an object. Note that
  // the invoking object and the object pointed to by the parameter ds must
  // be of the same type.
  virtual void CopyStructure(vtkDataSet *ds) = 0;

  // Description:
  // Return class name of data type. This is one of vtkStructuredGrid, 
  // vtkStructuredPoints, vtkUnstructuredGrid, vtkPolyData.
  virtual char *GetDataType() = 0;

  // Description:
  // Determine number of points composing dataset.
  virtual int GetNumberOfPoints() = 0;

  // Description:
  // Determine number of cells composing dataset.
  virtual int GetNumberOfCells() = 0;

  // Description:
  // Get point coordinates with ptId such that: 0 <= ptId < NumberOfPoints.
  virtual float *GetPoint(int ptId) = 0;

  // Description:
  // Copy point coordinates into user provided array x[3] for specified
  // point id.
  virtual void GetPoint(int id, float x[3]);

  // Description:
  // Get cell with cellId such that: 0 <= cellId < NumberOfCells.
  virtual vtkCell *GetCell(int cellId) = 0;

  // Description:
  // Get type of cell with cellId such that: 0 <= cellId < NumberOfCells.
  virtual int GetCellType(int cellId) = 0;

  // Description:
  // Topological inquiry to get points defining cell.
  virtual void GetCellPoints(int cellId, vtkIdList& ptIds) = 0;

  // Description:
  // Topological inquiry to get cells using point.
  virtual void GetPointCells(int ptId, vtkIdList& cellIds) = 0;

  // Description:
  // Topological inquiry to get all cells using list of points exclusive of
  // cell specified (e.g., cellId).
  virtual void GetCellNeighbors(int cellId, vtkIdList& ptIds, vtkIdList& cellIds);

  // Description:
  // Locate the closest point to the global coordinate x. Return the point
  // id and distance squared. If point id < 0; then no point found. (This
  // may arise when point is outside of dataset.)
  virtual int FindPoint(float x[3]) = 0;

  // Description:
  // Locate cell based on global coordinate x and tolerance squared. If
  // cell is non-NULL, then search starts from this cell and looks at 
  // immediate neighbors. Returns cellId >= 0 if inside, < 0 otherwise.
  // The parametric coordinates are provided in pcoords[3]. The interpolation
  // weights are returned in weights[]. (The number of weights is equal to the 
  // number of points in the found cell). Tolerance is used to control how
  // close the point is to be considered "in" the cell.
  virtual int FindCell(float x[3], vtkCell *cell, float tol2, int& subId, 
                       float pcoords[3], float *weights) = 0;

  // Description:
  // Locate the cell that contains a point and return the cell. Also returns
  // the subcell id, parametric coordinates and weights for subsequent
  // interpolation. This method combines the derived class methods
  // int FindCell and vtkCell *GetCell. Derived classes may provide a more efficient
  // implementation. See for example vtkStructuredPoints.
  virtual vtkCell *FindAndGetCell(float x[3], vtkCell *cell, float tol2, int& subId,
			    float pcoords[3], float *weights);

  // Datasets are composite objects and need to check each part for MTime
  unsigned long int GetMTime();

  // Description:
  // Release data back to system to conserve memory resource. Used during
  // visualization network execution.
  void ReleaseData();

  // Description:
  // Return flag indicating whether data should be released after use  
  // by a filter.
  int ShouldIReleaseData();

  // Description:
  // Set/Get the DataReleased ivar.
  vtkSetMacro(DataReleased,int);
  vtkGetMacro(DataReleased,int);

  // Description:
  // Turn on/off flag to control whether this object's data is released
  // after being used by a filter.
  vtkSetMacro(ReleaseDataFlag,int);
  vtkGetMacro(ReleaseDataFlag,int);
  vtkBooleanMacro(ReleaseDataFlag,int);

  // Description:
  // Turn on/off flag to control whether every object releases its data
  // after being used by a filter.
  vtkSetMacro(GlobalReleaseDataFlag,int);
  vtkGetMacro(GlobalReleaseDataFlag,int);
  vtkBooleanMacro(GlobalReleaseDataFlag,int);

  // return pointer to this dataset's point data
  vtkPointData *GetPointData() {return &this->PointData;};

  // Description:
  // Reclaim any extra memory used to store data.
  virtual void Squeeze();

  // Description:
  // Set the owner of this data object for Sources.
  vtkSetObjectMacro(Source,vtkSource);
  
  // compute geometric bounds, center, longest side
  virtual void ComputeBounds();
  float *GetBounds();
  void GetBounds(float bounds[6]);
  float *GetCenter();
  void GetCenter(float center[3]);
  float GetLength();

  // Restore data object to initial state,
  virtual void Initialize();

  // Description:
  // Convenience method to get the range of the scalar data (if there is any 
  // scalar data). Otherwise the method will return (0,1).
  float *GetScalarRange();
  
  // Description:
  // Convenience method returns largest cell size in dataset. This is generally
  // used to allocate memory for supporting data structures.
  virtual int GetMaxCellSize() = 0;

protected:
  vtkSource *Source; // if I am the output of a Source this is a pntr to it
  vtkPointData PointData;   // Scalars, vectors, etc. associated w/ each point
  vtkTimeStamp ComputeTime; // Time at which bounds, center, etc. computed
  float Bounds[6];  // (xmin,xmax, ymin,ymax, zmin,zmax) geometric bounds

  int DataReleased; //keep track of data release during network execution
  int ReleaseDataFlag; //data will release after use by a filter
  static int GlobalReleaseDataFlag; //all data will release after use by a filter
};

inline void vtkDataSet::GetPoint(int id, float x[3])
{
  float *pt = this->GetPoint(id);
  x[0] = pt[0]; x[1] = pt[1]; x[2] = pt[2]; 
}

#endif
