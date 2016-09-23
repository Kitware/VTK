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
/**
 * @class   vtkScalarTree
 * @brief   organize data according to scalar values (used to accelerate contouring operations)
 *
 *
 * vtkScalarTree is an abstract class that defines the API to concrete
 * scalar tree subclasses. A scalar tree is a data structure that organizes
 * data according to its scalar value. This allows rapid access to data for
 * those algorithms that access the data based on scalar value. For example,
 * isocontouring operates on cells based on the scalar (isocontour) value.
 *
 * To use subclasses of this class, you must specify a dataset to operate on,
 * and then specify a scalar value in the InitTraversal() method. Then
 * calls to GetNextCell() return cells whose scalar data contains the
 * scalar value specified. (This describes serial traversal.)
 *
 * Methods supporting parallel traversal (such as threading) are also
 * supported. Basically thread-safe batches of cells (which are a
 * portion of the whole dataset) are available for processing using a
 * parallel For() operation. First request the number of batches, and
 * then for each batch, retrieve the array of cell ids in that batch. These
 * batches contain cell ids that are likely to contain the isosurface.
 *
 * @sa
 * vtkSimpleScalarTree vtkSpanSpace
*/

#ifndef vtkScalarTree_h
#define vtkScalarTree_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkObject.h"

class vtkCell;
class vtkDataArray;
class vtkDataSet;
class vtkIdList;
class vtkTimeStamp;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkScalarTree : public vtkObject
{
public:
  vtkTypeMacro(vtkScalarTree,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Build the tree from the points/cells and scalars defining this
   * dataset.
   */
  virtual void SetDataSet(vtkDataSet*);
  vtkGetObjectMacro(DataSet,vtkDataSet);
  //@}

  //@{
  /**
   * Build the tree from the points/cells and scalars defining the
   * dataset and scalars provided. Typically the scalars come from
   * the vtkDataSet specified, but sometimes a separate vtkDataArray
   * is provided to specify the scalars. If the scalar array is
   * explicitly set, then it takes precedence over the scalars held
   * in the vtkDataSet.
   */
  virtual void SetScalars(vtkDataArray*);
  vtkGetObjectMacro(Scalars,vtkDataArray);
  //@}

  /**
   * Construct the scalar tree from the dataset provided. Checks build times
   * and modified time from input and reconstructs the tree if necessary.
   */
  virtual void BuildTree() = 0;

  /**
   * Initialize locator. Frees memory and resets object as appropriate.
   */
  virtual void Initialize() = 0;

  /**
   * Begin to traverse the cells based on a scalar value. Returned cells
   * will have scalar values that span the scalar value specified. Note
   * that changing the scalarValue does not cause the scalar tree to be
   * modified, and hence it does not rebuild.
   */
  virtual void InitTraversal(double scalarValue) = 0;

  /**
   * Return the next cell that may contain scalar value specified to
   * initialize traversal. The value NULL is returned if the list is
   * exhausted. Make sure that InitTraversal() has been invoked first or
   * you'll get erratic behavior.
   */
  virtual vtkCell *GetNextCell(vtkIdType &cellId, vtkIdList* &ptIds,
                               vtkDataArray *cellScalars) = 0;

  /**
   * Return the current scalar value over which tree traversal is proceeding.
   * This is the scalar value provided in InitTraversal().
   */
  double GetScalarValue()
    {return this->ScalarValue;}

  // The following methods supports parallel (threaded) applications. Basically
  // batches of cells (which are a portion of the whole dataset) are available for
  // processing in a parallel For() operation.

  /**
   * Get the number of cell batches available for processing. Note
   * that this methods should be called after InitTraversal(). This is
   * because the number of batches available is typically a function
   * of the isocontour value. Note that the cells found in
   * [0...(NumberOfCellBatches-1)] will contain all the cells
   * potentially containing the isocontour.
   */
  virtual vtkIdType GetNumberOfCellBatches() = 0;

  /**
   * Return the array of cell ids in the specified batch. The method
   * also returns the number of cell ids in the array. Make sure to
   * call InitTraversal() beforehand.
   */
  virtual const vtkIdType* GetCellBatch(vtkIdType batchNum,
                                        vtkIdType& numCells) = 0;


protected:
  vtkScalarTree();
  ~vtkScalarTree() VTK_OVERRIDE;

  vtkDataSet   *DataSet;    //the dataset over which the scalar tree is built
  vtkDataArray *Scalars;    //the scalars of the DataSet
  double        ScalarValue; //current scalar value for traversal

  vtkTimeStamp BuildTime; //time at which tree was built

private:
  vtkScalarTree(const vtkScalarTree&) VTK_DELETE_FUNCTION;
  void operator=(const vtkScalarTree&) VTK_DELETE_FUNCTION;
};

#endif
