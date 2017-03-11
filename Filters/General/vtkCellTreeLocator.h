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
 * algorithms for fast cell location presented in "Fast, Memory-Efficient Cell
 * location in Unstructured Grids for Visualization" by Christop Garth and Kenneth
 * I. Joy in VisWeek, 2011.
 *
 *
 * Cell Tree is a bounding interval hierarchy based data structure, where child boxes
 * do not form an exact split of the parent boxes along a dimension.  Therefore two axis-
 * aligned bounding planes (left max and right min) are stored for each node along a
 * dimension. This class implements the data structure (Cell Tree Node) and its build
 * and traversal algorithms described in the paper.
 * Some methods in building and traversing the cell tree in this class were derived
 * avtCellLocatorBIH class in the VisIT Visualization Tool
 *
 *
 *
 * @sa
 * vtkLocator vtkCellLocator vtkModifiedBSPTree
*/

#ifndef vtkCellTreeLocator_h
#define vtkCellTreeLocator_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkAbstractCellLocator.h"
#include <vector> // Needed for internal class

class vtkCellPointTraversal;
class vtkIdTypeArray;
class vtkCellArray;

class VTKFILTERSGENERAL_EXPORT vtkCellTreeLocator : public vtkAbstractCellLocator
{
  public:
    class vtkCellTree;
    class vtkCellTreeNode;

    vtkTypeMacro(vtkCellTreeLocator,vtkAbstractCellLocator);
    void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

    /**
     * Constructor sets the maximum number of cells in a leaf to 8
     * and number of buckets to 5.  Buckets are used in building the cell tree as described in the paper
     */
    static vtkCellTreeLocator *New();

     /**
      * Test a point to find if it is inside a cell. Returns the cellId if inside
      * or -1 if not.
      */
    vtkIdType FindCell(double pos[3], double vtkNotUsed, vtkGenericCell *cell,  double pcoords[3],
                       double* weights ) VTK_OVERRIDE;

    /**
     * Return intersection point (if any) AND the cell which was intersected by
     * the finite line. The cell is returned as a cell id and as a generic cell.
     */
    int IntersectWithLine(double a0[3], double a1[3], double tol,
                          double& t, double x[3], double pcoords[3],
                          int &subId, vtkIdType &cellId,
                          vtkGenericCell *cell) VTK_OVERRIDE;

    /**
     * Return a list of unique cell ids inside of a given bounding box. The
     * user must provide the vtkIdList to populate. This method returns data
     * only after the locator has been built.
     */
    void FindCellsWithinBounds(double *bbox, vtkIdList *cells) VTK_OVERRIDE;

    /*
      if the borland compiler is ever removed, we can use these declarations
      instead of reimplementaing the calls in this subclass
      using vtkAbstractCellLocator::IntersectWithLine;
      using vtkAbstractCellLocator::FindClosestPoint;
      using vtkAbstractCellLocator::FindClosestPointWithinRadius;
    */

    /**
     * reimplemented from vtkAbstractCellLocator to support bad compilers
     */
    int IntersectWithLine(
      double p1[3], double p2[3], double tol, double& t, double x[3],
      double pcoords[3], int &subId) VTK_OVERRIDE
    {
      return this->Superclass::IntersectWithLine(p1, p2, tol, t, x, pcoords, subId);
    }

    /**
     * Return intersection point (if any) AND the cell which was intersected by
     * the finite line. The cell is returned as a cell id and as a generic cell.
     * This function is a modification from the vtkModifiedBSPTree class using the
     * data structures in the paper to find intersections.
     */
    int IntersectWithLine(
      double p1[3], double p2[3], double tol, double &t, double x[3],
      double pcoords[3], int &subId, vtkIdType &cellId) VTK_OVERRIDE;

    /**
     * reimplemented from vtkAbstractCellLocator to support bad compilers
     */
    int IntersectWithLine(
      const double p1[3], const double p2[3],
      vtkPoints *points, vtkIdList *cellIds) VTK_OVERRIDE
    {
      return this->Superclass::IntersectWithLine(p1, p2, points, cellIds);
    }

    /**
     * reimplemented from vtkAbstractCellLocator to support bad compilers
     */
    vtkIdType FindCell(double x[3]) VTK_OVERRIDE
    { return this->Superclass::FindCell(x); }

    //@{
    /**
     * Satisfy vtkLocator abstract interface.
     */
    void FreeSearchStructure() VTK_OVERRIDE;
    void GenerateRepresentation(int level, vtkPolyData *pd) VTK_OVERRIDE;
    virtual void BuildLocatorInternal();
    virtual void BuildLocatorIfNeeded();
    virtual void ForceBuildLocator();
    void BuildLocator() VTK_OVERRIDE;
    //@}

    //@{
    /**
     * Internal classes made public to allow subclasses to create
     * customized some traversal algorithms
     */
    class VTKFILTERSGENERAL_EXPORT vtkCellTree
    {
      public:
        std::vector<vtkCellTreeNode>  Nodes;
        std::vector<unsigned int> Leaves;
        friend class vtkCellPointTraversal;
        friend class vtkCellTreeNode;
        friend class vtkCellTreeBuilder;
    //@}

      public:
        float DataBBox[6]; // This store the bounding values of the dataset
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
    class VTKFILTERSGENERAL_EXPORT vtkCellTreeNode
    {
      public:

      protected:
        unsigned int Index;
        float LeftMax;  // left max value
        float RightMin;  // right min value

        unsigned int Sz; // size
        unsigned int St; // start

        friend class vtkCellTree;
        friend class vtkCellPointTraversal;
        friend class vtkCellTreeBuilder;

      public:
        void MakeNode( unsigned int left, unsigned int d, float b[2] );
        void SetChildren( unsigned int left );
        bool IsNode() const;
        unsigned int GetLeftChildIndex() const;
        unsigned int GetRightChildIndex() const;
        unsigned int GetDimension() const;
        const float& GetLeftMaxValue() const;
        const float& GetRightMinValue() const;
        void MakeLeaf( unsigned int start, unsigned int size );
        bool IsLeaf() const;
        unsigned int Start() const;
        unsigned int Size() const;
    };

protected:
     vtkCellTreeLocator();
    ~vtkCellTreeLocator() VTK_OVERRIDE;

   // Test ray against node BBox : clip t values to extremes
  bool RayMinMaxT(const double origin[3],
    const double dir[3],
    double &rTmin,
    double &rTmax);

  bool RayMinMaxT(const double bounds[6],
    const double origin[3],
    const double dir[3],
    double &rTmin,
    double &rTmax);

  int getDominantAxis(const double dir[3]);

  // Order nodes as near/far relative to ray
  void Classify(const double origin[3],
    const double dir[3],
    double &rDist,
    vtkCellTreeNode *&near, vtkCellTreeNode *&mid,
    vtkCellTreeNode *&far, int &mustCheck);

  // From vtkModifiedBSPTRee
  // We provide a function which does the cell/ray test so that
  // it can be overridden by subclasses to perform special treatment
  // (Example : Particles stored in tree, have no dimension, so we must
  // override the cell test to return a value based on some particle size
  virtual int IntersectCellInternal( vtkIdType cell_ID,  const double p1[3],
    const double p2[3],
    const double tol,
    double &t,
    double ipt[3],
    double pcoords[3],
    int &subId);


    int NumberOfBuckets;

    vtkCellTree* Tree;

    friend class vtkCellPointTraversal;
    friend class vtkCellTreeNode;
    friend class vtkCellTreeBuilder;

private:
  vtkCellTreeLocator(const vtkCellTreeLocator&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCellTreeLocator&) VTK_DELETE_FUNCTION;
};

#endif
