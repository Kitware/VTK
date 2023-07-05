// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkIncrementalPointLocator
 * @brief   Abstract class in support of both
 *  point location and point insertion.
 *
 *
 *  Compared to a static point locator for pure location functionalities
 *  through some search structure established from a fixed set of points,
 *  an incremental point locator allows for, in addition, point insertion
 *  capabilities, with the search structure maintaining a dynamically
 *  increasing number of points. There are two incremental point locators,
 *  i.e., vtkPointLocator and vtkIncrementalOctreePointLocator. As opposed
 *  to the uniform bin-based search structure (adopted in vtkPointLocator)
 *  with a fixed spatial resolution, an octree mechanism (employed in
 *  vtkIncrementalOctreePointlocator) resorts to a hierarchy of tree-like
 *  sub-division of the 3D data domain. Thus it enables data-aware multi-
 *  resolution and accordingly accelerated point location as well as point
 *  insertion, particularly when handling a radically imbalanced layout of
 *  points as not uncommon in datasets defined on adaptive meshes. In other
 *  words, vtkIncrementalOctreePointLocator is an octree-based accelerated
 *  implementation of all functionalities of vtkPointLocator.
 *
 * @sa
 *  vtkLocator, vtkIncrementalOctreePointLocator, vtkPointLocator,
 *  vtkMergePoints vtkStaticPointLocator
 */

#ifndef vtkIncrementalPointLocator_h
#define vtkIncrementalPointLocator_h

#include "vtkAbstractPointLocator.h"
#include "vtkCommonDataModelModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkPoints;
class vtkIdList;

class VTKCOMMONDATAMODEL_EXPORT vtkIncrementalPointLocator : public vtkAbstractPointLocator
{
public:
  vtkTypeMacro(vtkIncrementalPointLocator, vtkAbstractPointLocator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Given a point x assumed to be covered by the search structure, return the
   * index of the closest point (already inserted to the search structure)
   * regardless of the associated minimum squared distance relative to the
   * squared insertion-tolerance distance. This method is used when performing
   * incremental point insertion. Note -1 indicates that no point is found.
   * InitPointInsertion() should have been called in advance.
   */
  virtual vtkIdType FindClosestInsertedPoint(const double x[3]) = 0;

  // -------------------------------------------------------------------------
  // ---------------------------- Point  Location ----------------------------
  // ---- All virtual functions related to point location are declared by ----
  // --------------- the parent class  vtkAbstractPointLocator ---------------
  // -------------------------------------------------------------------------

  // -------------------------------------------------------------------------
  // ---------------------------- Point Insertion ----------------------------
  // -------------------------------------------------------------------------

  /**
   * Initialize the point insertion process. newPts is an object, storing 3D
   * point coordinates, to which incremental point insertion puts coordinates.
   * It is created and provided by an external VTK class. Argument bounds
   * represents the spatial bounding box, into which the points fall.
   */
  virtual int InitPointInsertion(vtkPoints* newPts, const double bounds[6]) = 0;

  /**
   * Initialize the point insertion process. newPts is an object, storing 3D
   * point coordinates, to which incremental point insertion puts coordinates.
   * It is created and provided by an external VTK class. Argument bounds
   * represents the spatial bounding box, into which the points fall.
   */
  virtual int InitPointInsertion(vtkPoints* newPts, const double bounds[6], vtkIdType estSize) = 0;

  /**
   * Determine whether or not a given point has been inserted. Return the id of
   * the already inserted point if true, else return -1. InitPointInsertion()
   * should have been called in advance.
   */
  virtual vtkIdType IsInsertedPoint(double x, double y, double z) = 0;

  /**
   * Determine whether or not a given point has been inserted. Return the id of
   * the already inserted point if true, else return -1. InitPointInsertion()
   * should have been called in advance.
   */
  virtual vtkIdType IsInsertedPoint(const double x[3]) = 0;

  /**
   * Insert a point unless there has been a duplicate in the search structure.
   * Return 0 if the point was already in the list, otherwise return 1. If the
   * point was not in the list, it will be ADDED.  In either case, the id of
   * the point (newly inserted or not) is returned in the ptId argument.
   * This method is not thread safe.
   */
  virtual int InsertUniquePoint(const double x[3], vtkIdType& ptId) = 0;

  /**
   * Insert a given point with a specified point index ptId. InitPointInsertion()
   * should have been called prior to this function. Also, IsInsertedPoint()
   * should have been called in advance to ensure that the given point has not
   * been inserted unless point duplication is allowed.
   */
  virtual void InsertPoint(vtkIdType ptId, const double x[3]) = 0;

  /**
   * Insert a given point and return the point index. InitPointInsertion()
   * should have been called prior to this function. Also, IsInsertedPoint()
   * should have been called in advance to ensure that the given point has not
   * been inserted unless point duplication is allowed.
   */
  virtual vtkIdType InsertNextPoint(const double x[3]) = 0;

protected:
  vtkIncrementalPointLocator();
  ~vtkIncrementalPointLocator() override;

private:
  vtkIncrementalPointLocator(const vtkIncrementalPointLocator&) = delete;
  void operator=(const vtkIncrementalPointLocator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
