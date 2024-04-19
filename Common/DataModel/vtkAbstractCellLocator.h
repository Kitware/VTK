// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAbstractCellLocator
 * @brief   an abstract base class for locators which find cells
 *
 * vtkAbstractCellLocator is a spatial search object to quickly locate cells in 3D.
 * vtkAbstractCellLocator supplies a basic interface which concrete subclasses
 * should implement.
 *
 * @warning
 * When deriving a class from vtkAbstractCellLocator, one should include the
 * 'hidden' member functions by the following construct in the derived class
 * \verbatim
 *  using vtkAbstractCellLocator::IntersectWithLine;
 *  using vtkAbstractCellLocator::FindClosestPoint;
 *  using vtkAbstractCellLocator::FindClosestPointWithinRadius;
 *  using vtkAbstractCellLocator::FindCell;
 * \endverbatim
 *
 * @sa
 * vtkLocator vtkCellLocator vtkStaticCellLocator vtkCellTreeLocator vtkModifiedBSPTree vtkOBBTree
 */

#ifndef vtkAbstractCellLocator_h
#define vtkAbstractCellLocator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkLocator.h"
#include "vtkNew.h" // For vtkNew

#include <memory> // For shared_ptr
#include <vector> // For Weights

VTK_ABI_NAMESPACE_BEGIN
class vtkCellArray;
class vtkGenericCell;
class vtkIdList;
class vtkPoints;

class VTKCOMMONDATAMODEL_EXPORT vtkAbstractCellLocator : public vtkLocator
{
public:
  vtkTypeMacro(vtkAbstractCellLocator, vtkLocator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify the preferred/maximum number of cells in each node/bucket.
   * Default 32. Locators generally operate by subdividing space into
   * smaller regions until the number of cells in each region (or node)
   * reaches the desired level.
   */
  vtkSetClampMacro(NumberOfCellsPerNode, int, 1, VTK_INT_MAX);
  vtkGetMacro(NumberOfCellsPerNode, int);
  ///@}

  ///@{
  /**
   * Boolean controls whether the bounds of each cell are computed only
   * once and then saved.  Should be 10 to 20% faster if repeatedly
   * calling any of the Intersect/Find routines and the extra memory
   * won't cause disk caching (48 extra bytes per cell are required to
   * save the bounds).
   */
  vtkSetMacro(CacheCellBounds, vtkTypeBool);
  vtkGetMacro(CacheCellBounds, vtkTypeBool);
  vtkBooleanMacro(CacheCellBounds, vtkTypeBool);
  ///@}

  /**
   * This function can be used either internally or externally to compute only the cached
   * cell bounds if CacheCellBounds is on.
   */
  void ComputeCellBounds();

  ///@{
  /**
   * Boolean controls whether to maintain list of cells in each node.
   * not applicable to all implementations, but if the locator is being used
   * as a geometry simplification technique, there is no need to keep them.
   */
  vtkSetMacro(RetainCellLists, vtkTypeBool);
  vtkGetMacro(RetainCellLists, vtkTypeBool);
  vtkBooleanMacro(RetainCellLists, vtkTypeBool);
  ///@}

  /**
   * Return intersection point (if any) of finite line with cells contained
   * in cell locator. See vtkCell.h parameters documentation.
   *
   * THIS FUNCTION IS NOT THREAD SAFE.
   */
  virtual int IntersectWithLine(const double p1[3], const double p2[3], double tol, double& t,
    double x[3], double pcoords[3], int& subId);

  /**
   * Return intersection point (if any) AND the cell which was intersected by
   * the finite line.
   *
   * THIS FUNCTION IS NOT THREAD SAFE.
   */
  virtual int IntersectWithLine(const double p1[3], const double p2[3], double tol, double& t,
    double x[3], double pcoords[3], int& subId, vtkIdType& cellId);

  /**
   * Return intersection point (if any) AND the cell which was intersected by
   * the finite line. The cell is returned as a cell id and as a generic cell.
   *
   * This function takes in a vtkGenericCell to avoid using the internal vtkGenericCell.
   *
   * THIS FUNCTION IS THREAD SAFE.
   */
  virtual int IntersectWithLine(const double p1[3], const double p2[3], double tol, double& t,
    double x[3], double pcoords[3], int& subId, vtkIdType& cellId, vtkGenericCell* cell);

  /**
   * Take the passed line segment and intersect it with the data set.
   * The return value of the function is 0 if no intersections were found,
   * -1 if point 'a0' lies inside the closed surface, or +1 if point 'a0'
   * lies outside the closed surface. This method assumes that the data set
   * is a vtkPolyData that describes a closed surface, and the intersection
   * points that are returned in 'points' alternate between entrance points and exit points.
   *
   * Either 'points' or 'cellIds' can be set to nullptr if you don't want
   * to receive that information.
   *
   * This method is only implemented in vtkOBBTree.
   *
   * THIS FUNCTION IS THREAD SAFE.
   */
  virtual int IntersectWithLine(
    const double p1[3], const double p2[3], vtkPoints* points, vtkIdList* cellIds);

  /**
   * Take the passed line segment and intersect it with the data set.
   * The return value of the function is 0 if no intersections were found.
   * For each intersection with a cell, the points and cellIds have the relevant information
   * added sorted by t. If points or cellIds are nullptr pointers, then no information is
   * generated for that list.
   *
   * THIS FUNCTION IS NOT THREAD SAFE.
   */
  virtual int IntersectWithLine(
    const double p1[3], const double p2[3], double tol, vtkPoints* points, vtkIdList* cellIds);

  /**
   * Take the passed line segment and intersect it with the data set.
   * The return value of the function is 0 if no intersections were found.
   * For each intersection with the bounds of a cell or with a cell (if a cell is provided),
   * the points and cellIds have the relevant information added sorted by t.
   * If points or cellIds are nullptr pointers, then no information is generated for that list.
   *
   * This function takes in a vtkGenericCell to avoid using the internal vtkGenericCell.
   *
   * THIS FUNCTION IS THREAD SAFE.
   */
  virtual int IntersectWithLine(const double p1[3], const double p2[3], double tol,
    vtkPoints* points, vtkIdList* cellIds, vtkGenericCell* cell);

  /**
   * Return the closest point and the cell which is closest to the point x.
   * The closest point is somewhere on a cell, it need not be one of the
   * vertices of the cell.
   *
   * A vtkAbstractCellLocator subclass needs to implement FindClosestPointWithinRadius
   * which is used internally to implement FindClosestPoint.
   *
   * THIS FUNCTION IS NOT THREAD SAFE.
   */
  virtual void FindClosestPoint(
    const double x[3], double closestPoint[3], vtkIdType& cellId, int& subId, double& dist2);

  /**
   * Return the closest point and the cell which is closest to the point x.
   * The closest point is somewhere on a cell, it need not be one of the
   * vertices of the cell.
   *
   * A vtkAbstractCellLocator subclass needs to implement FindClosestPointWithinRadius
   * which is used internally to implement FindClosestPoint.
   *
   * This function takes in a vtkGenericCell to avoid using the internal vtkGenericCell.
   *
   * THIS FUNCTION IS THREAD SAFE.
   */
  virtual void FindClosestPoint(const double x[3], double closestPoint[3], vtkGenericCell* cell,
    vtkIdType& cellId, int& subId, double& dist2);

  /**
   * Return the closest point within a specified radius and the cell which is
   * closest to the point x. The closest point is somewhere on a cell, it
   * need not be one of the vertices of the cell. This method returns 1 if
   * a point is found within the specified radius. If there are no cells within
   * the specified radius, the method returns 0 and the values of closestPoint,
   * cellId, subId, and dist2 are undefined.
   *
   * THIS FUNCTION IS NOT THREAD SAFE.
   */
  virtual vtkIdType FindClosestPointWithinRadius(double x[3], double radius, double closestPoint[3],
    vtkIdType& cellId, int& subId, double& dist2);

  /**
   * Return the closest point within a specified radius and the cell which is
   * closest to the point x. The closest point is somewhere on a cell, it
   * need not be one of the vertices of the cell. This method returns 1 if
   * a point is found within the specified radius. If there are no cells within
   * the specified radius, the method returns 0 and the values of closestPoint,
   * cellId, subId, and dist2 are undefined.
   *
   * This function takes in a vtkGenericCell to avoid using the internal vtkGenericCell.
   *
   * THIS FUNCTION IS THREAD SAFE.
   */
  virtual vtkIdType FindClosestPointWithinRadius(double x[3], double radius, double closestPoint[3],
    vtkGenericCell* cell, vtkIdType& cellId, int& subId, double& dist2);

  /**
   * Return the closest point within a specified radius and the cell which is
   * closest to the point x. The closest point is somewhere on a cell, it
   * need not be one of the vertices of the cell. This method returns 1 if a
   * point is found within the specified radius. If there are no cells within
   * the specified radius, the method returns 0 and the values of
   * closestPoint, cellId, subId, and dist2 are undefined. If a closest point
   * is found, inside returns the return value of the EvaluatePosition call to
   * the closest cell; inside(=1) or outside(=0).
   *
   * This function takes in a vtkGenericCell to avoid using the internal vtkGenericCell.
   *
   * THIS FUNCTION IS THREAD SAFE.
   */
  virtual vtkIdType FindClosestPointWithinRadius(double x[3], double radius, double closestPoint[3],
    vtkGenericCell* cell, vtkIdType& cellId, int& subId, double& dist2, int& inside);

  /**
   * Return a list of unique cell ids inside of a given bounding box. The
   * user must provide the vtkIdList to populate.
   *
   * THIS FUNCTION IS THREAD SAFE.
   */
  virtual void FindCellsWithinBounds(double* bbox, vtkIdList* cells);

  /**
   * Take the passed line segment and intersect it with the data set.
   * For each intersection with the bounds of a cell, the cellIds
   * have the relevant information added sort by t. If cellIds is nullptr
   * pointer, then no information is generated for that list.
   *
   * A vtkAbstractCellLocator subclass needs to implement IntersectWithLine that
   * takes cells ids, which is used internally to implement FindCellsAlongLine.
   *
   * THIS FUNCTION IS THREAD SAFE.
   */
  virtual void FindCellsAlongLine(
    const double p1[3], const double p2[3], double tolerance, vtkIdList* cells);

  /**
   * Given an unbounded plane defined by an origin o[3] and unit normal n[3],
   * return the list of unique cell ids in the buckets containing the
   * plane. It is possible that an empty cell list is returned. The user must
   * provide the vtkIdList cell list to populate. This method returns data
   * only after the locator has been built.
   *
   * THIS FUNCTION IS THREAD SAFE.
   */
  virtual void FindCellsAlongPlane(
    const double o[3], const double n[3], double tolerance, vtkIdList* cells);

  /**
   * Returns the Id of the cell containing the point,
   * returns -1 if no cell found. This interface uses a tolerance of zero
   *
   * THIS FUNCTION IS NOT THREAD SAFE.
   */
  virtual vtkIdType FindCell(double x[3]);

  ///@{
  /**
   * Find the cell containing a given point. returns -1 if no cell found
   * the cell parameters are copied into the supplied variables, a cell must
   * be provided to store the information.
   *
   * THIS FUNCTION IS THREAD SAFE.
   */
  virtual vtkIdType FindCell(
    double x[3], double tol2, vtkGenericCell* GenCell, double pcoords[3], double* weights);
  virtual vtkIdType FindCell(double x[3], double tol2, vtkGenericCell* GenCell, int& subId,
    double pcoords[3], double* weights);
  ///@}

  /**
   * Quickly test if a point is inside the bounds of a particular cell.
   * Some locators cache cell bounds and this function can make use
   * of fast access to the data. This function should be used ONLY after the locator is built.
   */
  virtual bool InsideCellBounds(double x[3], vtkIdType cell_ID);

  /**
   * Shallow copy of a vtkAbstractCellLocator.
   *
   * Before you shallow copy, make sure to call SetDataSet()
   */
  virtual void ShallowCopy(vtkAbstractCellLocator*) {}

protected:
  vtkAbstractCellLocator();
  ~vtkAbstractCellLocator() override;

  ///@{
  /**
   * This command is used internally by the locator to copy
   * all cell Bounds into the internal CellBounds array. Subsequent
   * calls to InsideCellBounds(...) can make use of the data
   * A valid dataset must be present for this to work. Returns true
   * if bounds wre copied, false otherwise.
   */
  virtual bool StoreCellBounds();
  virtual void FreeCellBounds();
  ///@}

  /**
   * To be called in `FindCell(double[3])`. If need be, the internal `Weights` array size is
   * updated to be able to host all points of the largest cell of the input data set.
   */
  void UpdateInternalWeights();

  int NumberOfCellsPerNode;
  vtkTypeBool RetainCellLists;
  vtkTypeBool CacheCellBounds;
  vtkNew<vtkGenericCell> GenericCell;
  std::shared_ptr<std::vector<double>> CellBoundsSharedPtr;
  double* CellBounds; // The is just used for simplicity in the internal code

  /**
   * This time stamp helps us decide if we want to update internal `Weights` array size.
   */
  vtkTimeStamp WeightsTime;

  static bool IsInBounds(const double bounds[6], const double x[3], double tol = 0.0);

  /*
   *  This function should be used ONLY after the locator is built.
   *  cellBoundsPtr should be assigned to a double cellBounds[6] BEFORE calling this function.
   */
  void GetCellBounds(vtkIdType cellId, double*& cellBoundsPtr);

  /**
   * This array is resized so that it can fit points from the cell hosting the most in the input
   * data set. Resizing is done in `UpdateInternalWeights`.
   *
   * @note This array needs resized in `FindCell(double[3])`.
   */
  std::vector<double> Weights;

private:
  vtkAbstractCellLocator(const vtkAbstractCellLocator&) = delete;
  void operator=(const vtkAbstractCellLocator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
