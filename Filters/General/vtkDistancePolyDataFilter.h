// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDistancePolyDataFilter
 *
 *
 * Computes the signed distance from one vtkPolyData to another. The
 * signed distance to the second input is computed at every point in
 * the first input using vtkImplicitPolyDataDistance. Optionally, the signed
 * distance to the first input at every point in the second input can
 * be computed. This may be enabled by calling
 * ComputeSecondDistanceOn().
 *
 * If the signed distance is not desired, the unsigned distance can be
 * computed by calling SignedDistanceOff(). The signed distance field
 * may be negated by calling NegateDistanceOn();
 *
 * Directions can be computed in conjunction with distances by calling
 * ComputeDirectionsOn().
 *
 * This code was contributed in the VTK Journal paper:
 * "Boolean Operations on Surfaces in VTK Without External Libraries"
 * by Cory Quammen, Chris Weigle C., Russ Taylor
 * http://hdl.handle.net/10380/3262
 * http://www.midasjournal.org/browse/publication/797
 */

#ifndef vtkDistancePolyDataFilter_h
#define vtkDistancePolyDataFilter_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSGENERAL_EXPORT vtkDistancePolyDataFilter : public vtkPolyDataAlgorithm
{
public:
  static vtkDistancePolyDataFilter* New();
  vtkTypeMacro(vtkDistancePolyDataFilter, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Enable/disable computation of the signed distance between
   * the first poly data and the second poly data. Defaults to on.
   */
  vtkSetMacro(SignedDistance, vtkTypeBool);
  vtkGetMacro(SignedDistance, vtkTypeBool);
  vtkBooleanMacro(SignedDistance, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Enable/disable negation of the distance values. Defaults to
   * off. Has no effect if SignedDistance is off.
   */
  vtkSetMacro(NegateDistance, vtkTypeBool);
  vtkGetMacro(NegateDistance, vtkTypeBool);
  vtkBooleanMacro(NegateDistance, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Enable/disable computation of a second output poly data with the
   * distance from the first poly data at each point. Defaults to on.
   */
  vtkSetMacro(ComputeSecondDistance, vtkTypeBool);
  vtkGetMacro(ComputeSecondDistance, vtkTypeBool);
  vtkBooleanMacro(ComputeSecondDistance, vtkTypeBool);
  ///@}

  /**
   * Get the second output, which is a copy of the second input with an
   * additional distance scalar field.
   * Note this will return a valid data object only after this->Update() is
   * called.
   */
  vtkPolyData* GetSecondDistanceOutput();

  ///@{
  /**
   * Enable/disable computation of cell-center distance to the
   * second poly data. Defaults to on for backwards compatibility.
   *
   * If the first poly data consists of just vertex cells,
   * computing point and cell-center distances is redundant.
   */
  vtkSetMacro(ComputeCellCenterDistance, vtkTypeBool);
  vtkGetMacro(ComputeCellCenterDistance, vtkTypeBool);
  vtkBooleanMacro(ComputeCellCenterDistance, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Enable/disable computation of unit directions for the distances.
   * Defaults to off for backwards compatibility.
   */
  vtkSetMacro(ComputeDirection, vtkTypeBool);
  vtkGetMacro(ComputeDirection, vtkTypeBool);
  vtkBooleanMacro(ComputeDirection, vtkTypeBool);
  ///@}

protected:
  vtkDistancePolyDataFilter();
  ~vtkDistancePolyDataFilter() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  void GetPolyDataDistance(vtkPolyData*, vtkPolyData*);

private:
  vtkDistancePolyDataFilter(const vtkDistancePolyDataFilter&) = delete;
  void operator=(const vtkDistancePolyDataFilter&) = delete;

  vtkTypeBool SignedDistance;
  vtkTypeBool NegateDistance;
  vtkTypeBool ComputeSecondDistance;
  vtkTypeBool ComputeCellCenterDistance;
  vtkTypeBool ComputeDirection;
};

VTK_ABI_NAMESPACE_END
#endif
