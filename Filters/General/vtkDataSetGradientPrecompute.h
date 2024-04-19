// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDataSetGradientPrecompute
 *
 *
 * Computes a geometry based vector field that the DataSetGradient filter uses to accelerate
 * gradient computation. This vector field is added to FieldData since it has a different
 * value for each vertex of each cell (a vertex shared by two cell has two different values).
 *
 * @par Thanks:
 * This file is part of the generalized Youngs material interface reconstruction algorithm
 * contributed by CEA/DIF - Commissariat a l'Energie Atomique, Centre DAM Ile-De-France <br> BP12,
 * F-91297 Arpajon, France. <br> Implementation by Thierry Carrard (CEA)
 */

#ifndef vtkDataSetGradientPrecompute_h
#define vtkDataSetGradientPrecompute_h

#include "vtkDataSetAlgorithm.h"
#include "vtkFiltersGeneralModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSGENERAL_EXPORT vtkDataSetGradientPrecompute : public vtkDataSetAlgorithm
{
public:
  static vtkDataSetGradientPrecompute* New();
  vtkTypeMacro(vtkDataSetGradientPrecompute, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static int GradientPrecompute(vtkDataSet* ds, vtkDataSetAlgorithm* self = nullptr);

protected:
  vtkDataSetGradientPrecompute();
  ~vtkDataSetGradientPrecompute() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkDataSetGradientPrecompute(const vtkDataSetGradientPrecompute&) = delete;
  void operator=(const vtkDataSetGradientPrecompute&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif /* VTK_DATA_SET_GRADIENT_PRECOMPUTE_H */
