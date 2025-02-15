// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLinearTransformCellLocator
 * @brief   Cell locator adaptor to perform cell Location on datasets that are a linear
 * transformation of the original dataset.
 *
 * vtkLinearTransformCellLocator is a cell locator adaptor that can accept any cell locator, e.g.
 * vtkStaticCellLocator, vtkCellLocator, calculate the transformation matrix from the cell
 * locator adaptor's dataset to the given dataset inside BuildLocator, and then use the cell locator
 * and transformation to perform cell locator operations. The transformation matrix is computed
 * using the https://en.wikipedia.org/wiki/Kabsch_algorithm. UseAllPoints allows you to compute the
 * transformation using all the points of the dataset (use that when you are not sure if it's a
 * linear transformation) or 100 sample points (or less if the dataset is smaller) that are chosen
 * every-nth. IsLinearTransformation validates if the dataset is a linear transformation of the cell
 * locator's dataset based on the used points.
 *
 * @warning The cell locator adaptor MUST be built before using it.
 *
 * vtkLinearTransformCellLocator does NOT utilize ANY vtkLocator/vtkAbstractCellLocator parameter:
 *
 * @sa
 * vtkAbstractCellLocator vtkCellLocator vtkStaticCellLocator vtkCellTreeLocator vtkModifiedBSPTree
 * vtkOBBTree
 */
#ifndef vtkLinearTransformCellLocator_h
#define vtkLinearTransformCellLocator_h

#include "vtkAbstractCellLocator.h"
#include "vtkFiltersFlowPathsModule.h" // For export macro
#include "vtkSmartPointer.h"           // For vtkSmartPointer

VTK_ABI_NAMESPACE_BEGIN

class vtkTransform;

class VTKFILTERSFLOWPATHS_EXPORT vtkLinearTransformCellLocator : public vtkAbstractCellLocator
{
public:
  ///@{
  /**
   * Standard methods to instantiate, print and obtain type-related information.
   */
  static vtkLinearTransformCellLocator* New();
  vtkTypeMacro(vtkLinearTransformCellLocator, vtkAbstractCellLocator);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Set/Get the cell locator to be used internally.
   *
   * The cell locator MUST be built before using it.
   */
  virtual void SetCellLocator(vtkAbstractCellLocator* locator);
  vtkGetObjectMacro(CellLocator, vtkAbstractCellLocator);
  ///@}

  ///@{
  /**
   * UseAllPoints allows to compute the transformation using all the points of the dataset
   * (use that when you are not if it's a linear transformation) or 100 sample points
   * (or less if the dataset is smaller) that are chosen every-nth.
   *
   * Default is off.
   */
  vtkSetMacro(UseAllPoints, bool);
  vtkBooleanMacro(UseAllPoints, bool);
  vtkGetMacro(UseAllPoints, bool);
  ///@}

  /**
   * Get if the set dataset is a linear transformation of cell locator's dataset. The value is
   * inside BuildLocator().
   */
  vtkGetMacro(IsLinearTransformation, bool);

  // Reuse any superclass signatures that we don't override.
  using vtkAbstractCellLocator::FindCell;
  using vtkAbstractCellLocator::FindClosestPoint;
  using vtkAbstractCellLocator::FindClosestPointWithinRadius;
  using vtkAbstractCellLocator::IntersectWithLine;

  /**
   * Return intersection point (if any) AND the cell which was intersected by
   * the finite line. The cell is returned as a cell id and as a generic cell.
   *
   * For other IntersectWithLine signatures, see vtkAbstractCellLocator.
   */
  int IntersectWithLine(const double p1[3], const double p2[3], double tol, double& t, double x[3],
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
  int IntersectWithLine(const double p1[3], const double p2[3], double tol, vtkPoints* points,
    vtkIdList* cellIds, vtkGenericCell* cell) override;

  /**
   * Return the closest point and the cell which is closest to the point x.
   * The closest point is somewhere on a cell, it need not be one of the
   * vertices of the cell.
   *
   * For other FindClosestPoint signatures, see vtkAbstractCellLocator.
   */
  void FindClosestPoint(const double x[3], double closestPoint[3], vtkGenericCell* cell,
    vtkIdType& cellId, int& subId, double& dist2) override
  {
    this->Superclass::FindClosestPoint(x, closestPoint, cell, cellId, subId, dist2);
  }

  /**
   * Return the closest point within a specified radius and the cell which is
   * closest to the point x. The closest point is somewhere on a cell, it
   * need not be one of the vertices of the cell. This method returns 1 if a
   * point is found within the specified radius. If there are no cells within
   * the specified radius, the method returns 0 and the values of
   * closestPoint, cellId, subId, and dist2 are undefined. If a closest point
   * is found, inside returns the return value of the EvaluatePosition call to
   * the closest cell; inside(=1) or outside(=0).
   */
  vtkIdType FindClosestPointWithinRadius(double x[3], double radius, double closestPoint[3],
    vtkGenericCell* cell, vtkIdType& cellId, int& subId, double& dist2, int& inside) override;

  /**
   * Return a list of unique cell ids inside of a given bounding box. The
   * user must provide the vtkIdList to populate.
   *
   * This function does NOT work when SupportLinearTransformation is on.
   */
  void FindCellsWithinBounds(double* bbox, vtkIdList* cells) override;

  /**
   * Take the passed line segment and intersect it with the data set.
   * For each intersection with the bounds of a cell, the cellIds
   * have the relevant information added. If cellIds is nullptr
   * pointer, then no information is generated for that list.
   */
  void FindCellsAlongLine(
    const double p1[3], const double p2[3], double tolerance, vtkIdList* cellsIds) override
  {
    this->Superclass::FindCellsAlongLine(p1, p2, tolerance, cellsIds);
  }

  /**
   * Take the passed line segment and intersect it with the data set.
   * For each intersection with the bounds of a cell, the cellIds
   * have the relevant information added sort by t. If cellIds is nullptr
   * pointer, then no information is generated for that list.
   */
  void FindCellsAlongPlane(
    const double o[3], const double n[3], double tolerance, vtkIdList* cells) override;

  /**
   * Find the cell containing a given point. returns -1 if no cell found
   * the cell parameters are copied into the supplied variables, a cell must
   * be provided to store the information.
   *
   * For other FindCell signatures, see vtkAbstractCellLocator.
   */
  vtkIdType FindCell(double x[3], double vtkNotUsed(tol2), vtkGenericCell* cell, int& subId,
    double pcoords[3], double* weights) override;

  /**
   * Quickly test if a point is inside the bounds of a particular cell.
   * This function should be used ONLY after the locator is built.
   */
  bool InsideCellBounds(double x[3], vtkIdType cellId) override;

  ///@{
  /**
   * Satisfy vtkLocator abstract interface.
   */
  void GenerateRepresentation(int level, vtkPolyData* pd) override;
  void FreeSearchStructure() override;
  void BuildLocator() override;
  void ForceBuildLocator() override;
  ///@}

  /**
   * Shallow copy of a vtkLinearTransformCellLocator.
   *
   * Before you shallow copy, make sure to call SetDataSet()
   */
  void ShallowCopy(vtkAbstractCellLocator* locator) override;

protected:
  vtkLinearTransformCellLocator();
  ~vtkLinearTransformCellLocator() override;

  void BuildLocatorInternal() override;

  bool ComputeTransformation();

  vtkSmartPointer<vtkTransform> InverseTransform;
  vtkSmartPointer<vtkTransform> Transform;
  bool IsLinearTransformation = false;
  bool UseAllPoints = false;

  vtkAbstractCellLocator* CellLocator;

private:
  vtkLinearTransformCellLocator(const vtkLinearTransformCellLocator&) = delete;
  void operator=(const vtkLinearTransformCellLocator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
