// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkLabelHierarchy
 * @brief   contains an octree of labels
 *
 *
 * This class represents labels in a hierarchy used to denote rendering priority.
 * A binary tree of labels is maintained that subdivides the bounds of the
 * of the label anchors spatially. Which level of the tree a label occupies
 * determines its priority; those at higher levels of the tree will be
 * more likely to render than those at lower levels of the tree.
 *
 * Pass vtkLabelHierarchy objects to a vtkLabelPlacementMapper filter for dynamic,
 * non-overlapping, per-frame placement of labels.
 *
 * Note that if we have a d-dimensional binary tree and we want a fixed
 * number \f$n\f$ of labels in each node (all nodes, not just leaves),
 * we can compute the depth of tree required assuming a uniform distribution
 * of points. Given a total of \f$N\f$ points we know that
 * \f$\frac{N}{|T|} = n\f$, where \f$|T|\f$ is the cardinality of the tree (i.e.,
 * the number of nodes it contains).
 * Because we have a uniform distribution, the tree will be uniformly subdivided
 * and thus \f$|T| = 1 + 2^d + \left(2^d\right)^2 + \cdots + \left(2^d\right)^k\f$,
 * where \f$d\f$ is the dimensionality of the input points (fixed at 3 for now).
 * As \f$k\f$ becomes large, \f$|T|\approx 2 \left(2^d\right)^k\f$.
 * Using this approximation, we can solve for \f$k\f$:
 * \f[ k = \frac{\log{\frac{N}{2n}}}{\log{2^d}} \f]
 * Given a set of \f$N\f$ input label anchors, we'll compute \f$k\f$ and then
 * bin the anchors into tree nodes at level \f$k\f$ of the tree. After this,
 * all the nodes will be in the leaves of the tree and those leaves will be at
 * the \f$k\f$-th level; no anchors will be in levels \f$1, 2, \ldots, k-1\f$.
 * To fix that, we'll choose to move some anchors upwards.
 * The exact number to move upwards depends on \a TargetLabelCount. We'll
 * move as many up as required to have \a TargetLabelCount at each node.
 *
 * You should avoid situations where \a MaximumDepth does not allow for
 * \a TargetLabelCount or fewer entries at each node. The \a MaximumDepth
 * is a hard limit while \a TargetLabelCount is a suggested optimum. You will
 * end up with many more than \a TargetLabelCount entries per node and things
 * will be sloooow.
 */

#ifndef vtkLabelHierarchy_h
#define vtkLabelHierarchy_h

#include "vtkPointSet.h"
#include "vtkRenderingLabelModule.h" // For export macro
#include "vtkWrappingHints.h"        // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractArray;
class vtkCamera;
class vtkCoincidentPoints;
class vtkCoordinate;
class vtkDataArray;
class vtkIntArray;
class vtkLabelHierarchyIterator;
class vtkPoints;
class vtkPolyData;
class vtkRenderer;
class vtkTextProperty;

class VTKRENDERINGLABEL_EXPORT VTK_MARSHALAUTO vtkLabelHierarchy : public vtkPointSet
{
public:
  static vtkLabelHierarchy* New();
  vtkTypeMacro(vtkLabelHierarchy, vtkPointSet);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Override SetPoints so we can reset the hierarchy when the points change.
   */
  void SetPoints(vtkPoints*) override;

  /**
   * Fill the hierarchy with the input labels.
   */
  virtual void ComputeHierarchy();

  ///@{
  /**
   * The number of labels that is ideally present at any octree node.
   * It is best if this is a multiple of \f$2^d\f$.
   */
  vtkSetMacro(TargetLabelCount, int);
  vtkGetMacro(TargetLabelCount, int);
  ///@}

  ///@{
  /**
   * The maximum depth of the octree.
   */
  virtual void SetMaximumDepth(int depth);
  vtkGetMacro(MaximumDepth, int);
  ///@}

  /**
   * Enumeration of iterator types.
   */
  enum IteratorType
  {
    FULL_SORT,
    QUEUE,
    DEPTH_FIRST,
    FRUSTUM
  };

  ///@{
  /**
   * The default text property assigned to labels in this hierarchy.
   */
  virtual void SetTextProperty(vtkTextProperty* tprop);
  vtkGetObjectMacro(TextProperty, vtkTextProperty);
  ///@}

  ///@{
  /**
   * Set/get the array specifying the importance (priority) of each label.
   */
  virtual void SetPriorities(vtkDataArray* arr);
  vtkGetObjectMacro(Priorities, vtkDataArray);
  ///@}

  ///@{
  /**
   * Set/get the array specifying the text of each label.
   */
  virtual void SetLabels(vtkAbstractArray* arr);
  vtkGetObjectMacro(Labels, vtkAbstractArray);
  ///@}

  ///@{
  /**
   * Set/get the array specifying the orientation of each label.
   */
  virtual void SetOrientations(vtkDataArray* arr);
  vtkGetObjectMacro(Orientations, vtkDataArray);
  ///@}

  ///@{
  /**
   * Set/get the array specifying the icon index of each label.
   */
  virtual void SetIconIndices(vtkIntArray* arr);
  vtkGetObjectMacro(IconIndices, vtkIntArray);
  ///@}

  ///@{
  /**
   * Set/get the array specifying the size of each label.
   */
  virtual void SetSizes(vtkDataArray* arr);
  vtkGetObjectMacro(Sizes, vtkDataArray);
  ///@}

  ///@{
  /**
   * Set/get the array specifying the maximum width and height in world coordinates of each label.
   */
  virtual void SetBoundedSizes(vtkDataArray* arr);
  vtkGetObjectMacro(BoundedSizes, vtkDataArray);
  ///@}

  /**
   * Returns an iterator for this data object.
   * positionsAsNormals should only be true when labels are on a sphere centered at the origin
   * (3D world).
   * @param type - the type should be one of FULL_SORT, QUEUE, DEPTH_FIRST, or FRUSTUM.
   * @param ren - the current renderer (used for viewport information)
   * @param cam - the current camera.
   * @param frustumPlanes - should be the output of the camera's frustum planes.
   * @param positionsAsNormals - throws out octree nodes on the opposite side of the origin.
   * @param bucketSize - an array of 2 integers describing the width and height of label placer
   * buckets.
   */
  VTK_NEWINSTANCE vtkLabelHierarchyIterator* NewIterator(int type, vtkRenderer* ren, vtkCamera* cam,
    double frustumPlanes[24], bool positionsAsNormals, float bucketSize[2]);

  /**
   * Given a depth in the hierarchy (\a level) and a point \a pt in world space, compute \a ijk.
   * This is used to find other octree nodes at the same \a level that are within the search radius
   * for candidate labels to be placed. It is called with \a pt set to the camera eye point and
   * pythagorean quadruples increasingly distant from the origin are added to \a ijk to identify
   * octree nodes whose labels should be placed.
   * @param[out] ijk - discrete coordinates of the octree node at \a level containing \a pt.
   * @param[in]  pt - input world point coordinates
   * @param[in]  level - input octree level to be considered
   */
  void GetDiscreteNodeCoordinatesFromWorldPoint(int ijk[3], double pt[3], int level);

  /**
   * Given a \a level of the tree and \a ijk coordinates in a lattice,
   * compute a \a path down the tree to reach the corresponding lattice node.
   * If the lattice coordinates are outside the tree, this returns
   * false. Otherwise it returns true. This does <b>not</b> guarantee that
   * the path exists in the hierarchy.
   * @param[out] path - a vector of \a level integers specifying which child to descend at each
   * level to reach \a ijk
   * @param[in] ijk - discrete coordinates of the octree node at \a level
   * @param[in] level - input octree level to be considered
   */
  static bool GetPathForNodalCoordinates(int* path, int ijk[3], int level);

  ///@{
  /**
   * Inherited members (from vtkDataSet)
   */
  vtkIdType GetNumberOfCells() override;
  using vtkDataSet::GetCell;
  vtkCell* GetCell(vtkIdType) override;
  void GetCell(vtkIdType, vtkGenericCell*) override;
  int GetCellType(vtkIdType) override;
  using vtkDataSet::GetCellPoints;
  void GetCellPoints(vtkIdType, vtkIdList*) override;
  void GetPointCells(vtkIdType, vtkIdList*) override;
  vtkIdType FindCell(double*, vtkCell*, vtkIdType, double, int&, double*, double*) override;
  vtkIdType FindCell(
    double*, vtkCell*, vtkGenericCell*, vtkIdType, double, int&, double*, double*) override;
  int GetMaxCellSize() override;
  ///@}

  class Implementation;
  Implementation* GetImplementation() { return this->Impl; }

  ///@{
  /**
   * Provide access to original coordinates of sets of coincident points
   */
  vtkGetObjectMacro(CenterPts, vtkPoints);
  ///@}

  ///@{
  /**
   * Provide access to the set of coincident points that have been
   * perturbed by the hierarchy in order to render labels for each
   * without overlap.
   */
  vtkGetObjectMacro(CoincidentPoints, vtkCoincidentPoints);
  ///@}

  ///@{
  /**
   * Compute frustum for quickly excluding labels from rendering
   * that are outside the visible region.
   * This is a shared utility function.
   */
  static void GetAnchorFrustumPlanes(
    double frustumPlanes[24], vtkRenderer* ren, vtkCoordinate* anchorTransform);
  ///@}

protected:
  vtkLabelHierarchy();
  ~vtkLabelHierarchy() override;

  int TargetLabelCount;
  int MaximumDepth;
  vtkDataArray* Priorities;
  vtkAbstractArray* Labels;
  vtkDataArray* Orientations;
  vtkIntArray* IconIndices;
  vtkDataArray* Sizes;
  vtkDataArray* BoundedSizes;
  vtkCoincidentPoints* CoincidentPoints;
  vtkPoints* CenterPts;
  vtkTextProperty* TextProperty;

  Implementation* Impl;

  friend class vtkLabelHierarchyFrustumIterator;
  friend class vtkLabelHierarchyFullSortIterator;
  friend class implementation;

private:
  vtkLabelHierarchy(const vtkLabelHierarchy&) = delete;
  void operator=(const vtkLabelHierarchy&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkLabelHierarchy_h
