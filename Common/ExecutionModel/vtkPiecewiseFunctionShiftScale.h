// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkPiecewiseFunctionShiftScale
 *
 *
 */

#ifndef vtkPiecewiseFunctionShiftScale_h
#define vtkPiecewiseFunctionShiftScale_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkPiecewiseFunctionAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkPiecewiseFunction;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkPiecewiseFunctionShiftScale
  : public vtkPiecewiseFunctionAlgorithm
{
public:
  static vtkPiecewiseFunctionShiftScale* New();
  vtkTypeMacro(vtkPiecewiseFunctionShiftScale, vtkPiecewiseFunctionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkSetMacro(PositionShift, double);
  vtkSetMacro(PositionScale, double);
  vtkSetMacro(ValueShift, double);
  vtkSetMacro(ValueScale, double);

  vtkGetMacro(PositionShift, double);
  vtkGetMacro(PositionScale, double);
  vtkGetMacro(ValueShift, double);
  vtkGetMacro(ValueScale, double);

protected:
  vtkPiecewiseFunctionShiftScale();
  ~vtkPiecewiseFunctionShiftScale() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  double PositionShift;
  double PositionScale;
  double ValueShift;
  double ValueScale;

private:
  vtkPiecewiseFunctionShiftScale(const vtkPiecewiseFunctionShiftScale&) = delete;
  void operator=(const vtkPiecewiseFunctionShiftScale&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
