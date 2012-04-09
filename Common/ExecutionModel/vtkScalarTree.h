/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScalarTree.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkScalarTree - organize data according to scalar values (used to accelerate contouring operations)

// .SECTION Description
// vtkScalarTree is an abstract class that defines the API to concrete
// scalar tree subclasses. A scalar tree is a data structure that organizes
// data according to its scalar value. This allows rapid access to data for
// those algorithms that access the data based on scalar value. For example,
// isocontouring operates on cells based on the scalar (isocontour) value.
//
// To use subclasses of this class, you must specify a dataset to operate on,
// and then specify a scalar value in the InitTraversal() method. Then
// calls to GetNextCell() return cells whose scalar data contains the
// scalar value specified.

// .SECTION See Also
// vtkSimpleScalarTree 

#ifndef __vtkScalarTree_h
#define __vtkScalarTree_h

#include "vtkObject.h"

class vtkCell;
class vtkDataArray;
class vtkDataSet;
class vtkIdList;
class vtkTimeStamp;

class VTK_FILTERING_EXPORT vtkScalarTree : public vtkObject
{
public:
  vtkTypeMacro(vtkScalarTree,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Build the tree from the points/cells defining this dataset.
  virtual void SetDataSet(vtkDataSet*);
  vtkGetObjectMacro(DataSet,vtkDataSet);

  // Description:
  // Construct the scalar tree from the dataset provided. Checks build times
  // and modified time from input and reconstructs the tree if necessary.
  virtual void BuildTree() = 0;
  
  // Description:
  // Initialize locator. Frees memory and resets object as appropriate.
  virtual void Initialize() = 0;

  // Description:
  // Begin to traverse the cells based on a scalar value. Returned cells
  // will have scalar values that span the scalar value specified.
  virtual void InitTraversal(double scalarValue) = 0;

  // Description:
  // Return the next cell that may contain scalar value specified to
  // initialize traversal. The value NULL is returned if the list is
  // exhausted. Make sure that InitTraversal() has been invoked first or
  // you'll get erratic behavior.
  virtual vtkCell *GetNextCell(vtkIdType &cellId, vtkIdList* &ptIds,
                               vtkDataArray *cellScalars) = 0;

protected:
  vtkScalarTree();
  ~vtkScalarTree();

  vtkDataSet   *DataSet;    //the dataset over which the scalar tree is built
  vtkDataArray *Scalars;    //the scalars of the DataSet

  vtkTimeStamp BuildTime; //time at which tree was built
  double       ScalarValue; //current scalar value for traversal

  virtual void ReportReferences(vtkGarbageCollector*);

private:
  vtkScalarTree(const vtkScalarTree&);  // Not implemented.
  void operator=(const vtkScalarTree&);  // Not implemented.
};

#endif


