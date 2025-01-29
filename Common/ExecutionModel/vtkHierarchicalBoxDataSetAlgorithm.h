// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHierarchicalBoxDataSetAlgorithm
 * @brief   superclass for algorithms that
 * produce vtkHierarchicalBoxDataSet as output.
 *
 * Algorithms that take any type of data object (including composite dataset)
 * and produce a vtkHierarchicalBoxDataSet in the output can subclass from this
 * class.
 */

#ifndef vtkHierarchicalBoxDataSetAlgorithm_h
#define vtkHierarchicalBoxDataSetAlgorithm_h

#include "vtkAlgorithm.h"
#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkDeprecation.h"                // For VTK_DEPRECATED_IN_9_5_0

VTK_ABI_NAMESPACE_BEGIN
class vtkHierarchicalBoxDataSet;
class VTK_DEPRECATED_IN_9_5_0("Please use `vtkOverlappingAMRAlgorithm` instead.")
  VTKCOMMONEXECUTIONMODEL_EXPORT vtkHierarchicalBoxDataSetAlgorithm : public vtkAlgorithm
{
public:
  static vtkHierarchicalBoxDataSetAlgorithm* New();
  vtkTypeMacro(vtkHierarchicalBoxDataSetAlgorithm, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the output data object for a port on this algorithm.
   */
  vtkHierarchicalBoxDataSet* GetOutput();
  vtkHierarchicalBoxDataSet* GetOutput(int);
  ///@}

  ///@{
  /**
   * Assign a data object as input. Note that this method does not
   * establish a pipeline connection. Use SetInputConnection() to
   * setup a pipeline connection.
   */
  void SetInputData(vtkDataObject*);
  void SetInputData(int, vtkDataObject*);
  ///@}

  /**
   * see vtkAlgorithm for details
   */
  vtkTypeBool ProcessRequest(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

protected:
  vtkHierarchicalBoxDataSetAlgorithm();
  ~vtkHierarchicalBoxDataSetAlgorithm() override;

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*)
  {
    return 1;
  }

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*)
  {
    return 1;
  }

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*)
  {
    return 1;
  }

  ///@{
  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*)
  {
    return 1;
  }
  ///@}

  virtual int RequestUpdateTime(vtkInformation*, vtkInformationVector**, vtkInformationVector*)
  {
    return 1;
  }

  // Create a default executive.
  vtkExecutive* CreateDefaultExecutive() override;

  // see algorithm for more info
  int FillOutputPortInformation(int port, vtkInformation* info) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  vtkDataObject* GetInput(int port);

private:
  vtkHierarchicalBoxDataSetAlgorithm(const vtkHierarchicalBoxDataSetAlgorithm&) = delete;
  void operator=(const vtkHierarchicalBoxDataSetAlgorithm&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
