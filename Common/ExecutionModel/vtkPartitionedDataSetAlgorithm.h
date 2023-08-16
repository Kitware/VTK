// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkPartitionedDataSetAlgorithm
 * @brief Superclass for algorithms that produce vtkPartitionedDataSetAlgorithm
 *
 * vtkPartitionedDataSetAlgorithm is intended as a superclass for algorithms that
 * produce a vtkPartitionedDataSet.
 */

#ifndef vtkPartitionedDataSetAlgorithm_h
#define vtkPartitionedDataSetAlgorithm_h

#include "vtkAlgorithm.h"
#include "vtkCommonExecutionModelModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkPartitionedDataSet;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkPartitionedDataSetAlgorithm : public vtkAlgorithm
{
public:
  vtkTypeMacro(vtkPartitionedDataSetAlgorithm, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the output data object for the specified output port.
   */
  vtkPartitionedDataSet* GetOutput();
  vtkPartitionedDataSet* GetOutput(int);
  ///@}

  vtkTypeBool ProcessRequest(
    vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* outInfo) override;

protected:
  vtkPartitionedDataSetAlgorithm();
  ~vtkPartitionedDataSetAlgorithm() override;

  ///@{
  /**
   * Methods for subclasses to override to handle different pipeline requests.
   */
  virtual int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*)
  {
    return 1;
  }
  virtual int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*)
  {
    return 1;
  }
  virtual int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*)
  {
    return 1;
  }
  virtual int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*)
  {
    return 1;
  }
  virtual int RequestUpdateTime(vtkInformation*, vtkInformationVector**, vtkInformationVector*)
  {
    return 1;
  }
  ///@}

  int FillOutputPortInformation(int port, vtkInformation* info) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkPartitionedDataSetAlgorithm(const vtkPartitionedDataSetAlgorithm&) = delete;
  void operator=(const vtkPartitionedDataSetAlgorithm&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
