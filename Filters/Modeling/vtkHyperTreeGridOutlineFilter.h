// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHyperTreeGridOutlineFilter
 * @brief   create wireframe outline for arbitrary data set
 *
 * vtkHyperTreeGridOutlineFilter is a filter that generates a wireframe outline of
 * HyperTreeGrid. The outline consists of the twelve edges of the dataset
 * bounding box.
 *
 * @sa
 * vtkOutlineFilter
 *
 * This class was optimized by Jacques-Bernard Lekien, 2019.
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkHyperTreeGridOutlineFilter_h
#define vtkHyperTreeGridOutlineFilter_h

#include "vtkFiltersModelingModule.h" // For export macro
#include "vtkHyperTreeGridAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkOutlineSource;

class VTKFILTERSMODELING_EXPORT vtkHyperTreeGridOutlineFilter : public vtkHyperTreeGridAlgorithm
{
public:
  static vtkHyperTreeGridOutlineFilter* New();
  vtkTypeMacro(vtkHyperTreeGridOutlineFilter, vtkHyperTreeGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Generate solid faces for the box. This is off by default.
   */
  vtkSetMacro(GenerateFaces, vtkTypeBool);
  vtkBooleanMacro(GenerateFaces, vtkTypeBool);
  vtkGetMacro(GenerateFaces, vtkTypeBool);
  ///@}

protected:
  vtkHyperTreeGridOutlineFilter();
  ~vtkHyperTreeGridOutlineFilter() override;

  vtkTypeBool GenerateFaces;
  vtkOutlineSource* OutlineSource;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;
  int FillOutputPortInformation(int port, vtkInformation* info) override;

  int ProcessTrees(vtkHyperTreeGrid* input, vtkDataObject* outputDO) override;

private:
  vtkHyperTreeGridOutlineFilter(const vtkHyperTreeGridOutlineFilter&) = delete;
  void operator=(const vtkHyperTreeGridOutlineFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
