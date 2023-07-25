// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkInterpolateDataSetAttributes
 * @brief   interpolate scalars, vectors, etc. and other dataset attributes
 *
 * vtkInterpolateDataSetAttributes is a filter that interpolates data set
 * attribute values between input data sets. The input to the filter
 * must be datasets of the same type, same number of cells, and same
 * number of points. The output of the filter is a data set of the same
 * type as the input dataset and whose attribute values have been
 * interpolated at the parametric value specified.
 *
 * The filter is used by specifying two or more input data sets (total of N),
 * and a parametric value t (0 <= t <= N-1). The output will contain
 * interpolated data set attributes common to all input data sets. (For
 * example, if one input has scalars and vectors, and another has just
 * scalars, then only scalars will be interpolated and output.)
 */

#ifndef vtkInterpolateDataSetAttributes_h
#define vtkInterpolateDataSetAttributes_h

#include "vtkDataSetAlgorithm.h"
#include "vtkFiltersGeneralModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSetCollection;

class VTKFILTERSGENERAL_EXPORT vtkInterpolateDataSetAttributes : public vtkDataSetAlgorithm
{
public:
  static vtkInterpolateDataSetAttributes* New();
  vtkTypeMacro(vtkInterpolateDataSetAttributes, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return the list of inputs to this filter.
   */
  vtkDataSetCollection* GetInputList();

  ///@{
  /**
   * Specify interpolation parameter t.
   */
  vtkSetClampMacro(T, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(T, double);
  ///@}

protected:
  vtkInterpolateDataSetAttributes();
  ~vtkInterpolateDataSetAttributes() override;

  void ReportReferences(vtkGarbageCollector*) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  vtkDataSetCollection* InputList; // list of data sets to interpolate
  double T;                        // interpolation parameter

private:
  vtkInterpolateDataSetAttributes(const vtkInterpolateDataSetAttributes&) = delete;
  void operator=(const vtkInterpolateDataSetAttributes&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
