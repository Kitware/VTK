// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHyperTreeGridPlaneCutter
 * @brief   cut an hyper tree grid volume with
 * a plane and generate a polygonal cut surface.
 *
 *
 * vtkHyperTreeGridPlaneCutter is a filter that takes as input an hyper tree
 * grid and a single plane and generates the polygonal data intersection surface.
 * This cut is computed at the leaf cells of the hyper tree.
 * It is left as an option to decide whether the cut should be computed over
 * the original AMR mesh or over its dual; in the latter case, perfect
 * connectivity (i.e., mesh conformity in the FE sense) is achieved at the
 * cost of interpolation to the dual of the input AMR mesh, and therefore
 * of missing intersection plane pieces near the primal boundary.
 *
 * @sa
 * vtkHyperTreeGrid vtkHyperTreeGridAlgorithm
 *
 * @par Thanks:
 * This class was written by Philippe Pebay on a idea of Guenole Harel and Jacques-Bernard Lekien,
 * 2016 This class was modified by Rogeli Grima Torres, 2016 This class was modified by
 * Jacques-Bernard Lekien, 2018 This work was supported by Commissariat a l'Energie Atomique CEA,
 * DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkHyperTreeGridPlaneCutter_h
#define vtkHyperTreeGridPlaneCutter_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkHyperTreeGridAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkCellArray;
class vtkCutter;
class vtkIdList;
class vtkPoints;
class vtkHyperTreeGridNonOrientedGeometryCursor;
class vtkHyperTreeGridNonOrientedMooreSuperCursor;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperTreeGridPlaneCutter : public vtkHyperTreeGridAlgorithm
{
public:
  static vtkHyperTreeGridPlaneCutter* New();
  vtkTypeMacro(vtkHyperTreeGridPlaneCutter, vtkHyperTreeGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify the plane with its [a,b,c,d] Cartesian coefficients:
   * a*x + b*y + c*z = d
   */
  void SetPlane(double a, double b, double c, double d);
  vtkGetVector4Macro(Plane, double);
  ///@}

  ///@{
  /**
   * Returns 0 if plane's normal is aligned with X axis, 1 if it is aligned with Y axis, 2 if it
   * is aligned with Z axis. Returns -1 if not aligned with any principal axis.
   */
  vtkGetMacro(AxisAlignment, int);
  ///@}

  ///@{
  /**
   * Returns true if plane's normal is aligned with the corresponding axis, false elsewise.
   */
  bool IsPlaneOrthogonalToXAxis() { return this->AxisAlignment == 0; }
  bool IsPlaneOrthogonalToYAxis() { return this->AxisAlignment == 1; }
  bool IsPlaneOrthogonalToZAxis() { return this->AxisAlignment == 2; }
  //}@

  ///@{
  /**
   * Set/Get whether output mesh should be computed on dual grid
   */
  vtkSetMacro(Dual, int);
  vtkGetMacro(Dual, int);
  vtkBooleanMacro(Dual, int);
  ///@}

protected:
  vtkHyperTreeGridPlaneCutter();
  ~vtkHyperTreeGridPlaneCutter() override;

  /**
   * Resets every attributes to a minimal state needed for the algorithm to execute
   */
  virtual void Reset();

  /**
   * For this algorithm the output is a vtkPolyData instance
   */
  int FillOutputPortInformation(int, vtkInformation*) override;

  /**
   * Top-level routine to generate plane cut
   */
  int ProcessTrees(vtkHyperTreeGrid*, vtkDataObject*) override;

  /**
   * Recursively descend into tree down to leaves, cutting primal cells
   */
  void RecursivelyProcessTreePrimal(vtkHyperTreeGridNonOrientedGeometryCursor*);

  /**
   * Recursively decide whether cell is intersected by plane
   */
  bool RecursivelyPreProcessTree(vtkHyperTreeGridNonOrientedGeometryCursor*);

  /**
   * Recursively descend into tree down to leaves, cutting dual cells
   */
  void RecursivelyProcessTreeDual(vtkHyperTreeGridNonOrientedMooreSuperCursor*);

  /**
   * Check if a cursor is intersected by a plane
   */
  bool CheckIntersection(double[8][3], double[8]);

  // Check if a cursor is intersected by a plane.
  // Don't return function evaluations
  bool CheckIntersection(double[8][3]);

  /**
   * Compute the intersection between an edge and a plane
   */
  void PlaneCut(int, int, double[8][3], int&, double[][3]);

  /**
   * Reorder cut points following the perimeter of the cut.
   */
  void ReorderCutPoints(int, double[][3]);

  /**
   * Storage for the plane cutter parameters
   */
  double Plane[4];

  /**
   * Decide whether output mesh should be a computed on dual grid
   */
  int Dual;

  /**
   * Storage for pre-selected cells to be processed in dual mode
   */
  vtkBitArray* SelectedCells;

  /**
   * Storage for points of output unstructured mesh
   */
  vtkPoints* Points;

  /**
   * Storage for cells of output unstructured mesh
   */
  vtkCellArray* Cells;

  /**
   * Storage for dual vertex indices
   */
  vtkIdList* Leaves;

  /**
   * Storage for dual vertices at center of primal cells
   */
  vtkPoints* Centers;

  /**
   * Cutter to be used on dual cells
   */
  vtkCutter* Cutter;

  /**
   * material Mask
   */
  vtkBitArray* InMask;

  /**
   * Flag computed at plane creation to know whether it is aligned with x, y or z axis
   */
  int AxisAlignment;

private:
  vtkHyperTreeGridPlaneCutter(const vtkHyperTreeGridPlaneCutter&) = delete;
  void operator=(const vtkHyperTreeGridPlaneCutter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif /* vtkHyperTreeGridPlaneCutter_h */
