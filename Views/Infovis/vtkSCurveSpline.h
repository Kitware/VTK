// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2009 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkSCurveSpline
 * @brief   computes an interpolating spline using a
 * a SCurve basis.
 *
 *
 * vtkSCurveSpline is a concrete implementation of vtkSpline using a
 * SCurve basis.
 *
 * @sa
 * vtkSpline vtkKochanekSpline
 */

#ifndef vtkSCurveSpline_h
#define vtkSCurveSpline_h

#include "vtkSpline.h"
#include "vtkViewsInfovisModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKVIEWSINFOVIS_EXPORT vtkSCurveSpline : public vtkSpline
{
public:
  static vtkSCurveSpline* New();

  vtkTypeMacro(vtkSCurveSpline, vtkSpline);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Compute SCurve Splines for each dependent variable
   */
  void Compute() override;

  /**
   * Evaluate a 1D SCurve spline.
   */
  double Evaluate(double t) override;

  /**
   * Deep copy of SCurve spline data.
   */
  void DeepCopy(vtkSpline* s) override;

  vtkSetMacro(NodeWeight, double);
  vtkGetMacro(NodeWeight, double);

protected:
  vtkSCurveSpline();
  ~vtkSCurveSpline() override = default;

  double NodeWeight;

private:
  vtkSCurveSpline(const vtkSCurveSpline&) = delete;
  void operator=(const vtkSCurveSpline&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
