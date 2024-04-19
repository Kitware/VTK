// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWarpVector
 * @brief   deform geometry with vector data
 *
 * vtkWarpVector is a filter that modifies point coordinates by moving
 * points along vector times the scale factor. Useful for showing flow
 * profiles or mechanical deformation.
 *
 * The filter passes both its point data and cell data to its output.
 */

#ifndef vtkWarpVector_h
#define vtkWarpVector_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPointSetAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSGENERAL_EXPORT vtkWarpVector : public vtkPointSetAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for instantiation, obtaining type information,
   * and printing.
   */
  static vtkWarpVector* New();
  vtkTypeMacro(vtkWarpVector, vtkPointSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Specify value to scale displacement.
   */
  vtkSetMacro(ScaleFactor, double);
  vtkGetMacro(ScaleFactor, double);
  ///@}

  ///@{
  /**
   * Set/get the desired precision for the output points type. By default
   * (DEFAULT_PRECISION) the output type is the same as the input points
   * type. Otherwise, specify the precision as SINGLE_PRECISION or
   * DOUBLE_PRECISION.
   */
  vtkSetMacro(OutputPointsPrecision, int);
  vtkGetMacro(OutputPointsPrecision, int);
  ///@}

  int FillInputPortInformation(int port, vtkInformation* info) override;

protected:
  vtkWarpVector();
  ~vtkWarpVector() override;

  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  double ScaleFactor;
  int OutputPointsPrecision;

private:
  vtkWarpVector(const vtkWarpVector&) = delete;
  void operator=(const vtkWarpVector&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
