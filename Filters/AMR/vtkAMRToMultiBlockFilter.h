// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAMRToMultiBlockFilter
 *
 *
 * A filter that accepts as input an AMR dataset and produces a corresponding
 * vtkMultiBlockDataset as output.
 *
 * @sa
 * vtkOverlappingAMR vtkMultiBlockDataSet
 */

#ifndef vtkAMRToMultiBlockFilter_h
#define vtkAMRToMultiBlockFilter_h

#include "vtkFiltersAMRModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkInformation;
class vtkInformationVector;
class vtkIndent;
class vtkMultiProcessController;
class vtkOverlappingAMR;
class vtkMultiBlockDataSet;

class VTKFILTERSAMR_EXPORT vtkAMRToMultiBlockFilter : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkAMRToMultiBlockFilter* New();
  vtkTypeMacro(vtkAMRToMultiBlockFilter, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& oss, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get a multiprocess controller for parallel processing.
   * By default this parameter is set to nullptr by the constructor.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

  // Standard pipeline routines

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int FillOutputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

protected:
  vtkAMRToMultiBlockFilter();
  ~vtkAMRToMultiBlockFilter() override;

  ///@{
  /**
   * Copies the AMR data to the output multi-block datastructure.
   */
  void CopyAMRToMultiBlock(vtkOverlappingAMR* amr, vtkMultiBlockDataSet* mbds);
  vtkMultiProcessController* Controller;
  ///@}

private:
  vtkAMRToMultiBlockFilter(const vtkAMRToMultiBlockFilter&) = delete;
  void operator=(const vtkAMRToMultiBlockFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif /* vtkAMRToMultiBlockFilter_h */
