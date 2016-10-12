/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSimpleScalarTree.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSimpleScalarTree
 * @brief   organize data according to scalar values (used to accelerate contouring operations)
 *
 * vtkSimpleScalarTree creates a pointerless binary tree that helps search
 * for cells that lie within a particular scalar range. This object is used
 * to accelerate some contouring (and other scalar-based techniques).
 *
 * The tree consists of an array of (min,max) scalar range pairs per
 * node in the tree. The (min,max) range is determined from looking at
 * the range of the children of the tree node. If the node is a leaf,
 * then the range is determined by scanning the range of scalar data
 * in n cells in the dataset. The n cells are determined by arbitrary
 * selecting cell ids from id(i) to id(i+n), and where n is specified
 * using the BranchingFactor ivar. Note that leaf node i=0 contains
 * the scalar range computed from cell ids (0,n-1); leaf node i=1
 * contains the range from cell ids (n,2n-1); and so on. The
 * implication is that there are no direct lists of cell ids per leaf
 * node, instead the cell ids are implicitly known. Despite the
 * arbitrary grouping of cells, in practice this scalar tree actually
 * performs quite well due to spatial/data coherence.
 *
 * This class has an API that supports both serial and parallel
 * operation.  The parallel API enables the using class to grab arrays
 * (or batches) of cells that potentially intersect the
 * isocontour. These batches can then be processed in separate
 * threads.
 *
 * @sa
 * vtkSpanSpace
*/

#ifndef vtkSimpleScalarTree_h
#define vtkSimpleScalarTree_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkScalarTree.h"

class vtkScalarNode;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkSimpleScalarTree : public vtkScalarTree
{
public:
  /**
   * Instantiate scalar tree with maximum level of 20 and branching
   * factor of three.
   */
  static vtkSimpleScalarTree *New();

  //@{
  /**
   * Standard type related macros and PrintSelf() method.
   */
  vtkTypeMacro(vtkSimpleScalarTree,vtkScalarTree);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Set the branching factor for the tree. This is the number of
   * children per tree node. Smaller values (minimum is 2) mean deeper
   * trees and more memory overhead. Larger values mean shallower
   * trees, less memory usage, but worse performance.
   */
  vtkSetClampMacro(BranchingFactor,int,2,VTK_INT_MAX);
  vtkGetMacro(BranchingFactor,int);
  //@}

  //@{
  /**
   * Get the level of the scalar tree. This value may change each time the
   * scalar tree is built and the branching factor changes.
   */
  vtkGetMacro(Level,int);
  //@}

  //@{
  /**
   * Set the maximum allowable level for the tree.
   */
  vtkSetClampMacro(MaxLevel,int,1,VTK_INT_MAX);
  vtkGetMacro(MaxLevel,int);
  //@}

  /**
   * Construct the scalar tree from the dataset provided. Checks build times
   * and modified time from input and reconstructs the tree if necessary.
   */
  void BuildTree() VTK_OVERRIDE;

  /**
   * Initialize locator. Frees memory and resets object as appropriate.
   */
  void Initialize() VTK_OVERRIDE;

  /**
   * Begin to traverse the cells based on a scalar value. Returned cells
   * will likely have scalar values that span the scalar value specified.
   */
  void InitTraversal(double scalarValue) VTK_OVERRIDE;

  /**
   * Return the next cell that may contain scalar value specified to
   * initialize traversal. The value NULL is returned if the list is
   * exhausted. Make sure that InitTraversal() has been invoked first or
   * you'll get erratic behavior.
   */
  vtkCell *GetNextCell(vtkIdType &cellId, vtkIdList* &ptIds,
                               vtkDataArray *cellScalars) VTK_OVERRIDE;

  // The following methods supports parallel (threaded)
  // applications. Basically batches of cells (which represent a
  // portion of the whole dataset) are available for processing in a
  // parallel For() operation.

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
  vtkSimpleScalarTree();
  ~vtkSimpleScalarTree() VTK_OVERRIDE;

  int MaxLevel;
  int Level;
  int BranchingFactor; //number of children per node
  vtkScalarNode *Tree; //pointerless scalar range tree
  int TreeSize; //allocated size of tree
  vtkIdType LeafOffset; //offset to leaf nodes of tree

private:
  vtkIdType NumCells; //the number of cells in this dataset
  vtkIdType TreeIndex; //traversal location within tree
  int       ChildNumber; //current child in traversal
  vtkIdType CellId; //current cell id being examined
  int       FindStartLeaf(vtkIdType index, int level);
  int       FindNextLeaf(vtkIdType index,int level);

  vtkIdType *CandidateCells; //to support parallel computing
  vtkIdType  NumCandidates;

private:
  vtkSimpleScalarTree(const vtkSimpleScalarTree&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSimpleScalarTree&) VTK_DELETE_FUNCTION;
};

#endif
