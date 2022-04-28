/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellTreeLocator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCellTreeLocator
 * @brief   This class implements the data structures, construction
 * algorithms for fast cell location.
 *
 * Cell Tree is a bounding interval hierarchy based data structure, where child boxes
 * do not form an exact split of the parent boxes along a dimension.  Therefore two axis-
 * aligned bounding planes (left max and right min) are stored for each node along a
 * dimension. This class implements the data structure (Cell Tree Node) and its build
 * and traversal algorithms described in the paper.
 * Some methods in building and traversing the cell tree in this class were derived
 * from avtCellLocatorBIH class in the VisIT Visualization Tool.
 *
 * vtkCellTreeLocator utilizes the following parent class parameters:
 * - NumberOfCellsPerNode        (default 8)
 * - CacheCellBounds             (default false)
 * - UseExistingSearchStructure  (default false)
 * - LazyEvaluation              (default false)
 *
 * vtkCellTreeLocator does NOT utilize the following parameters:
 * - Automatic
 * - Level
 * - MaxLevel
 * - Tolerance
 * - RetainCellLists
 *
 * @cite "Fast, Memory-Efficient Cell location in Unstructured Grids for Visualization" by
 * Christoph Garth and Kenneth I. Joy in VisWeek, 2011.
 *
 * @sa
 * vtkAbstractCellLocator vtkCellLocator vtkStaticCellLocator vtkModifiedBSPTree vtkOBBTree
 */

#ifndef vtkCellTreeLocator_h
#define vtkCellTreeLocator_h

#include "vtkAbstractCellLocator.h"
#include "vtkCommonDataModelModule.h" // For export macro
#include <vector>                     // Needed for internal class

class vtkCellPointTraversal;
class vtkIdTypeArray;
class vtkCellArray;

class VTKCOMMONDATAMODEL_EXPORT vtkCellTreeLocator : public vtkAbstractCellLocator
{
public:
  ///@{
  /**
   * Standard methods to print and obtain type-related information.
   */
  vtkTypeMacro(vtkCellTreeLocator, vtkAbstractCellLocator);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Constructor sets the maximum number of cells in a leaf to 8
   * and number of buckets to 5.  Buckets are used in building the cell tree as described in the
   * paper
   */
  static vtkCellTreeLocator* New();

  ///@{
  /**
   * Set/Get the number of buckets.
   */
  vtkSetMacro(NumberOfBuckets, int);
  vtkGetMacro(NumberOfBuckets, int);
  ///@}

  // Re-use any superclass signatures that we don't override.
  using vtkAbstractCellLocator::FindCell;
  using vtkAbstractCellLocator::IntersectWithLine;

  /**
   * Return intersection point (if any) AND the cell which was intersected by
   * the finite line. The cell is returned as a cell id and as a generic cell.
   */
  int IntersectWithLine(const double a0[3], const double a1[3], double tol, double& t, double x[3],
    double pcoords[3], int& subId, vtkIdType& cellId, vtkGenericCell* cell) override;

  /**
   * Take the passed line segment and intersect it with the data set.
   * The return value of the function is 0 if no intersections were found.
   * For each intersection with the bounds of a cell or with a cell (if a cell is provided),
   * the points and cellIds have the relevant information added sorted by t.
   * If points or cellIds are nullptr pointers, then no information is generated for that list.
   *
   * For other IntersectWithLine signatures, see vtkAbstractCellLocator.
   */
  int IntersectWithLine(const double p1[3], const double p2[3], const double tol, vtkPoints* points,
    vtkIdList* cellIds, vtkGenericCell* cell) override;

  /**
   * Return a list of unique cell ids inside of a given bounding box. The
   * user must provide the vtkIdList to populate. This method returns data
   * only after the locator has been built.
   */
  void FindCellsWithinBounds(double* bbox, vtkIdList* cells) override;

  /**
   * Take the passed line segment and intersect it with the data set.
   * For each intersection with the bounds of a cell, the cellIds
   * have the relevant information added sort by t. If cellIds is nullptr
   * pointer, then no information is generated for that list.
   *
   * Reimplemented from vtkAbstractCellLocator to showcase that it's a supported function.
   */
  void FindCellsAlongLine(
    const double p1[3], const double p2[3], double tolerance, vtkIdList* cellsIds) override
  {
    this->Superclass::FindCellsAlongLine(p1, p2, tolerance, cellsIds);
  }

  /**
   * Find the cell containing a given point. returns -1 if no cell found
   * the cell parameters are copied into the supplied variables, a cell must
   * be provided to store the information.
   */
  vtkIdType FindCell(double pos[3], double vtkNotUsed(tol2), vtkGenericCell* cell, int& subId,
    double pcoords[3], double* weights) override;

  ///@{
  /**
   * Satisfy vtkLocator abstract interface.
   */
  void FreeSearchStructure() override;
  void GenerateRepresentation(int level, vtkPolyData* pd) override;
  virtual void BuildLocatorInternal();
  virtual void BuildLocatorIfNeeded();
  virtual void ForceBuildLocator();
  void BuildLocator() override;
  ///@}

  class vtkCellTree;
  class vtkCellTreeNode;
  /**
   * Internal classes made public to allow subclasses to create
   * customized some traversal algorithms
   */
  class VTKCOMMONDATAMODEL_EXPORT vtkCellTree
  {
  public:
    std::vector<vtkCellTreeNode> Nodes;
    std::vector<unsigned int> Leaves;
    friend class vtkCellPointTraversal;
    friend class vtkCellTreeNode;
    friend class vtkCellTreeBuilder;

  public:
    double DataBBox[6]; // This store the bounding values of the dataset
  };

  /**
   * This class is the basic building block of the cell tree.
   * Nodes consist of two split planes, LeftMax and RightMin,
   * one which holds all cells assigned to the left, one for the right.
   * The planes may overlap in the box, but cells are only assigned
   * to one side, so some searches must traverse both leaves until they have eliminated
   * candidates.
   * start is the location in the cell tree. e.g. for root node start is zero.
   * size is the number of the nodes under the (sub-)tree
   */
  class VTKCOMMONDATAMODEL_EXPORT vtkCellTreeNode
  {
  public:
  protected:
    unsigned int Index;
    double LeftMax;  // left max value
    double RightMin; // right min value

    unsigned int Sz; // size
    unsigned int St; // start

    friend class vtkCellPointTraversal;
    friend class vtkCellTreeBuilder;

  public:
    void MakeNode(unsigned int left, unsigned int d, double b[2]);
    void SetChildren(unsigned int left);
    bool IsNode() const;
    unsigned int GetLeftChildIndex() const;
    unsigned int GetRightChildIndex() const;
    unsigned int GetDimension() const;
    const double& GetLeftMaxValue() const;
    const double& GetRightMinValue() const;
    void MakeLeaf(unsigned int start, unsigned int size);
    bool IsLeaf() const;
    unsigned int Start() const;
    unsigned int Size() const;
  };

protected:
  vtkCellTreeLocator();
  ~vtkCellTreeLocator() override;

  int getDominantAxis(const double dir[3]);

  // Order nodes as near/far relative to ray
  void Classify(const double origin[3], const double dir[3], double& rDist, vtkCellTreeNode*& near,
    vtkCellTreeNode*& mid, vtkCellTreeNode*& far, int& mustCheck);

  int NumberOfBuckets;

  vtkCellTree* Tree;

  friend class vtkCellPointTraversal;
  friend class vtkCellTreeNode;
  friend class vtkCellTreeBuilder;

private:
  vtkCellTreeLocator(const vtkCellTreeLocator&) = delete;
  void operator=(const vtkCellTreeLocator&) = delete;
};

#endif
