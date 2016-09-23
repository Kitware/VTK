/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSpanSpace.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSpanSpace
 * @brief   organize data according to scalar span space
 *
 * This is a helper class used to accelerate contouring operations. Given an
 * dataset, it organizes the dataset cells into a 2D binned space, with axes
 * (scalar_min,scalar_max). This so-called span space can then be traversed
 * quickly to find the cells that intersect a particular contour value.
 *
 * This class has an API that supports both serial and parallel
 * operation.  The parallel API enables the using class to grab arrays
 * (or batches) of cells that lie along a particular row in the span
 * space. These arrays can then be processed separately in parallel.
 *
 * Learn more about span space in these two publications: 1) "A Near
 * Optimal Isosorface Extraction Algorithm Using the Spsn Space."
 * Yarden Livnat et al. and 2) Isosurfacing in SPan Space with Utmost
 * Efficiency." Han-Wei Shen et al.
 *
 * @sa
 * vtkSimpleScalarTree
*/

#ifndef vtkSpanSpace_h
#define vtkSpanSpace_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkScalarTree.h"

class vtkInternalSpanSpace;


class VTKCOMMONEXECUTIONMODEL_EXPORT vtkSpanSpace : public vtkScalarTree
{
public:
  /**
   * Instantiate a scalar tree with default number of rows of 100.
   */
  static vtkSpanSpace *New();

  //@{
  /**
   * Standard type related macros and PrintSelf() method.
   */
  vtkTypeMacro(vtkSpanSpace,vtkScalarTree);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  //----------------------------------------------------------------------
  // The following methods are specific to the creationg and configuration of
  // vtkSpanSpace.

  //@{
  /**
   * Set/Get the resolution N of the span space. The span space can be
   * envisioned as a rectangular lattice of NXN buckets (i.e., N rows
   * and N columns), where each bucket stores a list of cell ids. The
   * i-j coordinate of each cell (hence its location in the lattice)
   * is determined from the cell's 2-tuple (smin,smax) scalar range.
   * By default Resolution = 100.
   */
  vtkSetClampMacro(Resolution,vtkIdType,1,VTK_INT_MAX);
  vtkGetMacro(Resolution,vtkIdType);
  //@}

  //----------------------------------------------------------------------
  // The following methods satisfy the vtkScalarTree abstract API.

  /**
   * Initialize locator. Frees memory and resets object as appropriate.
   */
  void Initialize() VTK_OVERRIDE;

  /**
   * Construct the scalar tree from the dataset provided. Checks build times
   * and modified time from input and reconstructs the tree if necessary.
   */
  void BuildTree() VTK_OVERRIDE;

  /**
   * Begin to traverse the cells based on a scalar value. Returned cells
   * will have scalar values that span the scalar value specified. Note this
   * method must be called prior to parallel or serial traversal since it
   * specifies the scalar value to be extracted.
   */
  void InitTraversal(double scalarValue) VTK_OVERRIDE;

  /**
   * Return the next cell that may contain scalar value specified to
   * InitTraversal(). The value NULL is returned if the list is
   * exhausted. Make sure that InitTraversal() has been invoked first or
   * you'll get erratic behavior. This is inherently a serial operation.
   */
  vtkCell *GetNextCell(vtkIdType &cellId, vtkIdList* &ptIds,
                               vtkDataArray *cellScalars) VTK_OVERRIDE;

  // The following methods supports parallel (threaded)
  // applications. Basically batches of cells (which represent a
  // portion of the whole dataset) are available for processing in a
  // parallel For() operation. In the case of span space, a batch is
  // taken from a portion of the span rectangle.

  /**
   * Get the number of cell batches available for processing. Note
   * that this methods should be called after InitTraversal(). This is
   * because the number of batches available is typically a function
   * of the isocontour value. Note that the cells found in
   * [0...(NumberOfCellBatches-1)] will contain all the cells
   * potentially containing the isocontour.
   */
  vtkIdType GetNumberOfCellBatches() VTK_OVERRIDE;

  /**
   * Return the array of cell ids in the specified batch. The method
   * also returns the number of cell ids in the array. Make sure to
   * call InitTraversal() beforehand.
   */
  const vtkIdType* GetCellBatch(vtkIdType batchNum,
                                        vtkIdType& numCells) VTK_OVERRIDE;

protected:
  vtkSpanSpace();
  ~vtkSpanSpace() VTK_OVERRIDE;

  vtkIdType Resolution;
  vtkInternalSpanSpace *SpanSpace;
  vtkIdType BatchSize;

private:
  // Internal variables supporting span space traversal
  vtkIdType RMin[2]; //span space lower left corner
  vtkIdType RMax[2]; //span space upper right corner

  // This supports serial traversal via GetNextCell()
  vtkIdType CurrentRow; //the span space row currently being processed
  vtkIdType *CurrentSpan; //pointer to current span row
  vtkIdType CurrentIdx; //position into the current span row
  vtkIdType CurrentNumCells; //number of cells on the current span row

private:
  vtkSpanSpace(const vtkSpanSpace&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSpanSpace&) VTK_DELETE_FUNCTION;
};

#endif
