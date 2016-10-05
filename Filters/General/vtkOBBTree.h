/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOBBTree.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOBBTree
 * @brief   generate oriented bounding box (OBB) tree
 *
 * vtkOBBTree is an object to generate oriented bounding box (OBB) trees.
 * An oriented bounding box is a bounding box that does not necessarily line
 * up along coordinate axes. The OBB tree is a hierarchical tree structure
 * of such boxes, where deeper levels of OBB confine smaller regions of space.
 *
 * To build the OBB, a recursive, top-down process is used. First, the root OBB
 * is constructed by finding the mean and covariance matrix of the cells (and
 * their points) that define the dataset. The eigenvectors of the covariance
 * matrix are extracted, giving a set of three orthogonal vectors that define
 * the tightest-fitting OBB. To create the two children OBB's, a split plane
 * is found that (approximately) divides the number cells in half. These are
 * then assigned to the children OBB's. This process then continues until
 * the MaxLevel ivar limits the recursion, or no split plane can be found.
 *
 * A good reference for OBB-trees is Gottschalk & Manocha in Proceedings of
 * Siggraph `96.
 *
 * @warning
 * Since this algorithms works from a list of cells, the OBB tree will only
 * bound the "geometry" attached to the cells if the convex hull of the
 * cells bounds the geometry.
 *
 * @warning
 * Long, skinny cells (i.e., cells with poor aspect ratio) may cause
 * unsatisfactory results. This is due to the fact that this is a top-down
 * implementation of the OBB tree, requiring that one or more complete cells
 * are contained in each OBB. This requirement makes it hard to find good
 * split planes during the recursion process. A bottom-up implementation would
 * go a long way to correcting this problem.
 *
 * @sa
 * vtkLocator vtkCellLocator vtkPointLocator
*/

#ifndef vtkOBBTree_h
#define vtkOBBTree_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkAbstractCellLocator.h"

class vtkMatrix4x4;

// Special class defines node for the OBB tree
//

//
class VTKFILTERSGENERAL_EXPORT vtkOBBNode { //;prevent man page generation
public:
  vtkOBBNode();
  ~vtkOBBNode();

  double Corner[3]; //center point of this node
  double Axes[3][3]; //the axes defining the OBB - ordered from long->short
  vtkOBBNode *Parent; //parent node; NULL if root
  vtkOBBNode **Kids; //two children of this node; NULL if leaf
  vtkIdList *Cells; //list of cells in node
  void DebugPrintTree( int level, double *leaf_vol, int *minCells,
                       int *maxCells );

private:
  vtkOBBNode(const vtkOBBNode& other) VTK_DELETE_FUNCTION;
  vtkOBBNode& operator=(const vtkOBBNode& rhs) VTK_DELETE_FUNCTION;
};

//

class VTKFILTERSGENERAL_EXPORT vtkOBBTree : public vtkAbstractCellLocator
{
public:
  vtkTypeMacro(vtkOBBTree,vtkAbstractCellLocator);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Construct with automatic computation of divisions, averaging
   * 25 cells per octant.
   */
  static vtkOBBTree *New();

  // Re-use any superclass signatures that we don't override.
  using vtkAbstractCellLocator::IntersectWithLine;

  /**
   * Take the passed line segment and intersect it with the data set.
   * This method assumes that the data set is a vtkPolyData that describes
   * a closed surface, and the intersection points that are returned in
   * 'points' alternate between entrance points and exit points.
   * The return value of the function is 0 if no intersections were found,
   * -1 if point 'a0' lies inside the closed surface, or +1 if point 'a0'
   * lies outside the closed surface.
   * Either 'points' or 'cellIds' can be set to NULL if you don't want
   * to receive that information.
   */
  int IntersectWithLine(const double a0[3], const double a1[3],
                        vtkPoints *points, vtkIdList *cellIds) VTK_OVERRIDE;

  /**
   * Return the first intersection of the specified line segment with
   * the OBB tree, as well as information about the cell which the
   * line segment intersected.
   */
  int IntersectWithLine(double a0[3], double a1[3], double tol,
                        double& t, double x[3], double pcoords[3],
                        int &subId, vtkIdType &cellId, vtkGenericCell *cell) VTK_OVERRIDE;

  /**
   * Compute an OBB from the list of points given. Return the corner point
   * and the three axes defining the orientation of the OBB. Also return
   * a sorted list of relative "sizes" of axes for comparison purposes.
   */
  static void ComputeOBB(vtkPoints *pts, double corner[3], double max[3],
                  double mid[3], double min[3], double size[3]);

  /**
   * Compute an OBB for the input dataset using the cells in the data.
   * Return the corner point and the three axes defining the orientation
   * of the OBB. Also return a sorted list of relative "sizes" of axes for
   * comparison purposes.
   */
  void ComputeOBB(vtkDataSet *input, double corner[3], double max[3],
                  double mid[3], double min[3], double size[3]);

  /**
   * Determine whether a point is inside or outside the data used to build
   * this OBB tree.  The data must be a closed surface vtkPolyData data set.
   * The return value is +1 if outside, -1 if inside, and 0 if undecided.
   */
  int InsideOrOutside(const double point[3]);

  /**
   * Returns true if nodeB and nodeA are disjoint after optional
   * transformation of nodeB with matrix XformBtoA
   */
  int DisjointOBBNodes( vtkOBBNode *nodeA, vtkOBBNode *nodeB,
                        vtkMatrix4x4 *XformBtoA );

  /**
   * Returns true if line intersects node.
   */
  int LineIntersectsNode( vtkOBBNode *pA, double b0[3], double b1[3] );

  /**
   * Returns true if triangle (optionally transformed) intersects node.
   */
  int TriangleIntersectsNode( vtkOBBNode *pA,
                              double p0[3], double p1[3],
                              double p2[3], vtkMatrix4x4 *XformBtoA );

  /**
   * For each intersecting leaf node pair, call function.
   * OBBTreeB is optionally transformed by XformBtoA before testing.
   */
  int IntersectWithOBBTree( vtkOBBTree *OBBTreeB, vtkMatrix4x4 *XformBtoA,
                            int(*function)( vtkOBBNode *nodeA,
                                            vtkOBBNode *nodeB,
                                            vtkMatrix4x4 *Xform,
                                            void *arg ),
                            void *data_arg );

  //@{
  /**
   * Satisfy locator's abstract interface, see vtkLocator.
   */
  void FreeSearchStructure() VTK_OVERRIDE;
  void BuildLocator() VTK_OVERRIDE;
  //@}

  /**
   * Create polygonal representation for OBB tree at specified level. If
   * level < 0, then the leaf OBB nodes will be gathered. The aspect ratio (ar)
   * and line diameter (d) are used to control the building of the
   * representation. If a OBB node edge ratio's are greater than ar, then the
   * dimension of the OBB is collapsed (OBB->plane->line). A "line" OBB will be
   * represented either as two crossed polygons, or as a line, depending on
   * the relative diameter of the OBB compared to the diameter (d).
   */
  void GenerateRepresentation(int level, vtkPolyData *pd) VTK_OVERRIDE;

protected:
  vtkOBBTree();
  ~vtkOBBTree() VTK_OVERRIDE;

  // Compute an OBB from the list of cells given.  This used to be
  // public but should not have been.  A public call has been added
  // so that the functionality can be accessed.
  void ComputeOBB(vtkIdList *cells, double corner[3], double max[3],
                       double mid[3], double min[3], double size[3]);

  vtkOBBNode *Tree;
  void BuildTree(vtkIdList *cells, vtkOBBNode *parent, int level);
  vtkPoints *PointsList;
  int *InsertedPoints;
  int OBBCount;

  void DeleteTree(vtkOBBNode *OBBptr);
  void GeneratePolygons(vtkOBBNode *OBBptr, int level, int repLevel,
                        vtkPoints* pts, vtkCellArray *polys);

private:
  vtkOBBTree(const vtkOBBTree&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOBBTree&) VTK_DELETE_FUNCTION;
};

#endif
