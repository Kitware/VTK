/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCellArray - object to represent cell connectivity
// .SECTION Description
// vtkCellArray is a supporting object that explicitly represents cell
// connectivity. The cell array structure is a raw integer list
// of the form: (n,id1,id2,...,idn, n,id1,id2,...,idn, ...)
// where n is the number of points in the cell, and id is a zero-offset index
// into an associated point list.
//
// Advantages of this data structure are its compactness, simplicity, and
// easy interface to external data.  However, it is totally inadequate for
// random access.  This functionality (when necessary) is accomplished by
// using the vtkCellTypes and vtkCellLinks objects to extend the definition of
// the data structure.
//
// .SECTION See Also
// vtkCellTypes vtkCellLinks

#ifndef __vtkCellArray_h
#define __vtkCellArray_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

#include "vtkIdTypeArray.h" // Needed for inline methods
#include "vtkCell.h" // Needed for inline methods

class VTKCOMMONDATAMODEL_EXPORT vtkCellArray : public vtkObject
{
public:
  vtkTypeMacro(vtkCellArray,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Instantiate cell array (connectivity list).
  static vtkCellArray *New();

  // Description:
  // Allocate memory and set the size to extend by.
  int Allocate(const vtkIdType sz, const int ext=1000)
    {return this->Ia->Allocate(sz,ext);}

  // Description:
  // Free any memory and reset to an empty state.
  void Initialize();

  // Description:
  // Get the number of cells in the array.
  vtkGetMacro(NumberOfCells, vtkIdType);

  // Description:
  // Set the number of cells in the array.
  // DO NOT do any kind of allocation, advanced use only.
  vtkSetMacro(NumberOfCells, vtkIdType);

  // Description:
  // Utility routines help manage memory of cell array. EstimateSize()
  // returns a value used to initialize and allocate memory for array based
  // on number of cells and maximum number of points making up cell.  If
  // every cell is the same size (in terms of number of points), then the
  // memory estimate is guaranteed exact. (If not exact, use Squeeze() to
  // reclaim any extra memory.)
  vtkIdType EstimateSize(vtkIdType numCells, int maxPtsPerCell)
    {return numCells*(1+maxPtsPerCell);}

  // Description:
  // A cell traversal methods that is more efficient than vtkDataSet traversal
  // methods.  InitTraversal() initializes the traversal of the list of cells.
  void InitTraversal() {this->TraversalLocation=0;};

  // Description:
  // A cell traversal methods that is more efficient than vtkDataSet traversal
  // methods.  GetNextCell() gets the next cell in the list. If end of list
  // is encountered, 0 is returned.
  int GetNextCell(vtkIdType& npts, vtkIdType* &pts);

  // Description:
  // A cell traversal methods that is more efficient than vtkDataSet traversal
  // methods.  GetNextCell() gets the next cell in the list. If end of list
  // is encountered, 0 is returned.
  int GetNextCell(vtkIdList *pts);

  // Description:
  // Get the size of the allocated connectivity array.
  vtkIdType GetSize()
    {return this->Ia->GetSize();}

  // Description:
  // Get the total number of entries (i.e., data values) in the connectivity
  // array. This may be much less than the allocated size (i.e., return value
  // from GetSize().)
  vtkIdType GetNumberOfConnectivityEntries()
    {return this->Ia->GetMaxId()+1;}

  // Description:
  // Internal method used to retrieve a cell given an offset into
  // the internal array.
  void GetCell(vtkIdType loc, vtkIdType &npts, vtkIdType* &pts);

  // Description:
  // Internal method used to retrieve a cell given an offset into
  // the internal array.
  void GetCell(vtkIdType loc, vtkIdList* pts);

  // Description:
  // Insert a cell object. Return the cell id of the cell.
  vtkIdType InsertNextCell(vtkCell *cell);

  // Description:
  // Create a cell by specifying the number of points and an array of point
  // id's.  Return the cell id of the cell.
  vtkIdType InsertNextCell(vtkIdType npts, const vtkIdType* pts);

  // Description:
  // Create a cell by specifying a list of point ids. Return the cell id of
  // the cell.
  vtkIdType InsertNextCell(vtkIdList *pts);

  // Description:
  // Create cells by specifying count, and then adding points one at a time
  // using method InsertCellPoint(). If you don't know the count initially,
  // use the method UpdateCellCount() to complete the cell. Return the cell
  // id of the cell.
  vtkIdType InsertNextCell(int npts);

  // Description:
  // Used in conjunction with InsertNextCell(int npts) to add another point
  // to the list of cells.
  void InsertCellPoint(vtkIdType id);

  // Description:
  // Used in conjunction with InsertNextCell(int npts) and InsertCellPoint() to
  // update the number of points defining the cell.
  void UpdateCellCount(int npts);

  // Description:
  // Computes the current insertion location within the internal array.
  // Used in conjunction with GetCell(int loc,...).
  vtkIdType GetInsertLocation(int npts)
    {return (this->InsertLocation - npts - 1);};

  // Description:
  // Get/Set the current traversal location.
  vtkIdType GetTraversalLocation()
    {return this->TraversalLocation;}
  void SetTraversalLocation(vtkIdType loc)
    {this->TraversalLocation = loc;}

  // Description:
  // Computes the current traversal location within the internal array. Used
  // in conjunction with GetCell(int loc,...).
  vtkIdType GetTraversalLocation(vtkIdType npts)
    {return(this->TraversalLocation-npts-1);}

  // Description:
  // Special method inverts ordering of current cell. Must be called
  // carefully or the cell topology may be corrupted.
  void ReverseCell(vtkIdType loc);

  // Description:
  // Replace the point ids of the cell with a different list of point ids.
  void ReplaceCell(vtkIdType loc, int npts, const vtkIdType *pts);

  // Description:
  // Returns the size of the largest cell. The size is the number of points
  // defining the cell.
  int GetMaxCellSize();

  // Description:
  // Get pointer to array of cell data.
  vtkIdType *GetPointer()
    {return this->Ia->GetPointer(0);}

  // Description:
  // Get pointer to data array for purpose of direct writes of data. Size is the
  // total storage consumed by the cell array. ncells is the number of cells
  // represented in the array.
  vtkIdType *WritePointer(const vtkIdType ncells, const vtkIdType size);

  // Description:
  // Define multiple cells by providing a connectivity list. The list is in
  // the form (npts,p0,p1,...p(npts-1), repeated for each cell). Be careful
  // using this method because it discards the old cells, and anything
  // referring these cells becomes invalid (for example, if BuildCells() has
  // been called see vtkPolyData).  The traversal location is reset to the
  // beginning of the list; the insertion location is set to the end of the
  // list.
  void SetCells(vtkIdType ncells, vtkIdTypeArray *cells);

  // Description:
  // Perform a deep copy (no reference counting) of the given cell array.
  void DeepCopy(vtkCellArray *ca);

  // Description:
  // Return the underlying data as a data array.
  vtkIdTypeArray* GetData()
    {return this->Ia;}

  // Description:
  // Reuse list. Reset to initial condition.
  void Reset();

  // Description:
  // Reclaim any extra memory.
  void Squeeze()
    {this->Ia->Squeeze();}

  // Description:
  // Return the memory in kilobytes consumed by this cell array. Used to
  // support streaming and reading/writing data. The value returned is
  // guaranteed to be greater than or equal to the memory required to
  // actually represent the data represented by this object. The
  // information returned is valid only after the pipeline has
  // been updated.
  unsigned long GetActualMemorySize();

protected:
  vtkCellArray();
  ~vtkCellArray();

  vtkIdType NumberOfCells;
  vtkIdType InsertLocation;     //keep track of current insertion point
  vtkIdType TraversalLocation;   //keep track of traversal position
  vtkIdTypeArray *Ia;

private:
  vtkCellArray(const vtkCellArray&);  // Not implemented.
  void operator=(const vtkCellArray&);  // Not implemented.
};


//----------------------------------------------------------------------------
inline vtkIdType vtkCellArray::InsertNextCell(vtkIdType npts,
                                              const vtkIdType* pts)
{
  vtkIdType i = this->Ia->GetMaxId() + 1;
  vtkIdType *ptr = this->Ia->WritePointer(i, npts+1);

  for ( *ptr++ = npts, i = 0; i < npts; i++)
    {
    *ptr++ = *pts++;
    }

  this->NumberOfCells++;
  this->InsertLocation += npts + 1;

  return this->NumberOfCells - 1;
}

//----------------------------------------------------------------------------
inline vtkIdType vtkCellArray::InsertNextCell(int npts)
{
  this->InsertLocation = this->Ia->InsertNextValue(npts) + 1;
  this->NumberOfCells++;

  return this->NumberOfCells - 1;
}

//----------------------------------------------------------------------------
inline void vtkCellArray::InsertCellPoint(vtkIdType id)
{
  this->Ia->InsertValue(this->InsertLocation++, id);
}

//----------------------------------------------------------------------------
inline void vtkCellArray::UpdateCellCount(int npts)
{
  this->Ia->SetValue(this->InsertLocation-npts-1, npts);
}

//----------------------------------------------------------------------------
inline vtkIdType vtkCellArray::InsertNextCell(vtkIdList *pts)
{
  return this->InsertNextCell(pts->GetNumberOfIds(), pts->GetPointer(0));
}

//----------------------------------------------------------------------------
inline vtkIdType vtkCellArray::InsertNextCell(vtkCell *cell)
{
  return this->InsertNextCell(cell->GetNumberOfPoints(),
                              cell->PointIds->GetPointer(0));
}

//----------------------------------------------------------------------------
inline void vtkCellArray::Reset()
{
  this->NumberOfCells = 0;
  this->InsertLocation = 0;
  this->TraversalLocation = 0;
  this->Ia->Reset();
}

//----------------------------------------------------------------------------
inline int vtkCellArray::GetNextCell(vtkIdType& npts, vtkIdType* &pts)
{
  if ( this->Ia->GetMaxId() >= 0 &&
       this->TraversalLocation <= this->Ia->GetMaxId() )
    {
    npts = this->Ia->GetValue(this->TraversalLocation++);
    pts = this->Ia->GetPointer(this->TraversalLocation);
    this->TraversalLocation += npts;
    return 1;
    }
  npts=0;
  pts=0;
  return 0;
}

//----------------------------------------------------------------------------
inline void vtkCellArray::GetCell(vtkIdType loc, vtkIdType &npts,
                                  vtkIdType* &pts)
{
  npts = this->Ia->GetValue(loc++);
  pts  = this->Ia->GetPointer(loc);
}

//----------------------------------------------------------------------------
inline void vtkCellArray::ReverseCell(vtkIdType loc)
{
  int i;
  vtkIdType tmp;
  vtkIdType npts=this->Ia->GetValue(loc);
  vtkIdType *pts=this->Ia->GetPointer(loc+1);
  for (i=0; i < (npts/2); i++)
    {
    tmp = pts[i];
    pts[i] = pts[npts-i-1];
    pts[npts-i-1] = tmp;
    }
}

//----------------------------------------------------------------------------
inline void vtkCellArray::ReplaceCell(vtkIdType loc, int npts,
                                      const vtkIdType *pts)
{
  vtkIdType *oldPts=this->Ia->GetPointer(loc+1);
  for (int i=0; i < npts; i++)
    {
    oldPts[i] = pts[i];
    }
}

//----------------------------------------------------------------------------
inline vtkIdType *vtkCellArray::WritePointer(const vtkIdType ncells,
                                             const vtkIdType size)
{
  this->NumberOfCells = ncells;
  this->InsertLocation = 0;
  this->TraversalLocation = 0;
  return this->Ia->WritePointer(0,size);
}

#endif
