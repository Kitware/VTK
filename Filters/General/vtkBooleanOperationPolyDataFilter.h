// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkBooleanOperationPolyDataFilter
 *
 *
 * Computes the boundary of the union, intersection, or difference
 * volume computed from the volumes defined by two input surfaces. The
 * two surfaces do not need to be manifold, but if they are not,
 * unexpected results may be obtained. The resulting surface is
 * available in the first output of the filter. The second output
 * contains a set of polylines that represent the intersection between
 * the two input surfaces.
 *
 * @warning This filter is not designed to perform 2D boolean operations,
 * and in fact relies on the inputs having no co-planar, overlapping cells.
 *
 * This code was contributed in the VTK Journal paper:
 * "Boolean Operations on Surfaces in VTK Without External Libraries"
 * by Cory Quammen, Chris Weigle C., Russ Taylor
 * http://hdl.handle.net/10380/3262
 * http://www.midasjournal.org/browse/publication/797
 */

#ifndef vtkBooleanOperationPolyDataFilter_h
#define vtkBooleanOperationPolyDataFilter_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#include "vtkDataSetAttributes.h" // Needed for CopyCells() method

VTK_ABI_NAMESPACE_BEGIN
class vtkIdList;

class VTKFILTERSGENERAL_EXPORT vtkBooleanOperationPolyDataFilter : public vtkPolyDataAlgorithm
{
public:
  /**
   * Construct object that computes the boolean surface.
   */
  static vtkBooleanOperationPolyDataFilter* New();

  vtkTypeMacro(vtkBooleanOperationPolyDataFilter, vtkPolyDataAlgorithm);

  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum OperationType
  {
    VTK_UNION = 0,
    VTK_INTERSECTION,
    VTK_DIFFERENCE
  };

  ///@{
  /**
   * Set the boolean operation to perform. Defaults to union.
   */
  vtkSetClampMacro(Operation, int, VTK_UNION, VTK_DIFFERENCE);
  vtkGetMacro(Operation, int);
  void SetOperationToUnion() { this->SetOperation(VTK_UNION); }
  void SetOperationToIntersection() { this->SetOperation(VTK_INTERSECTION); }
  void SetOperationToDifference() { this->SetOperation(VTK_DIFFERENCE); }
  ///@}

  ///@{
  /**
   * Turn on/off cell reorientation of the intersection portion of the
   * surface when the operation is set to DIFFERENCE. Defaults to on.
   */
  vtkSetMacro(ReorientDifferenceCells, vtkTypeBool);
  vtkGetMacro(ReorientDifferenceCells, vtkTypeBool);
  vtkBooleanMacro(ReorientDifferenceCells, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/get the tolerance used to determine when a point's absolute
   * distance is considered to be zero. Defaults to 1e-6.
   */
  vtkSetMacro(Tolerance, double);
  vtkGetMacro(Tolerance, double);
  ///@}

protected:
  vtkBooleanOperationPolyDataFilter();
  ~vtkBooleanOperationPolyDataFilter() override;

  /**
   * Labels triangles in mesh as part of the intersection or union surface.
   */
  void SortPolyData(vtkPolyData* input, vtkIdList* intersectionList, vtkIdList* unionList);

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int, vtkInformation*) override;

private:
  vtkBooleanOperationPolyDataFilter(const vtkBooleanOperationPolyDataFilter&) = delete;
  void operator=(const vtkBooleanOperationPolyDataFilter&) = delete;

  /**
   * Copies cells with indices given by from one vtkPolyData to
   * another. The point and cell field lists are used to determine
   * which fields should be copied.
   */
  void CopyCells(vtkPolyData* in, vtkPolyData* out, int idx,
    vtkDataSetAttributes::FieldList& pointFieldList, vtkDataSetAttributes::FieldList& cellFieldList,
    vtkIdList* cellIds, bool reverseCells);

  /**
   * Tolerance used to determine when a point's absolute
   * distance is considered to be zero.
   */
  double Tolerance;

  /**
   * Which operation to perform.
   * Can be VTK_UNION, VTK_INTERSECTION, or VTK_DIFFERENCE.
   */
  int Operation;

  ///@{
  /**
   * Determines if cells from the intersection surface should be
   * reversed in the difference surface.
   */
  vtkTypeBool ReorientDifferenceCells;
  ///@}
};

VTK_ABI_NAMESPACE_END
#endif
