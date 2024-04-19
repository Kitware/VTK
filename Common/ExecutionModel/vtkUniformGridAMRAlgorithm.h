// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkUniformGridAMRAlgorithm
 *  vtkUniformGridAMR as output.
 *
 *
 *  A base class for all algorithms that take as input any type of data object
 *  including composite datasets and produce vtkUniformGridAMR in the output.
 */

#ifndef vtkUniformGridAMRAlgorithm_h
#define vtkUniformGridAMRAlgorithm_h

#include "vtkAlgorithm.h"
#include "vtkCommonExecutionModelModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkUniformGridAMR;
class vtkInformation;
class vtkInformationVector;
class vtkExecutive;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkUniformGridAMRAlgorithm : public vtkAlgorithm
{
public:
  static vtkUniformGridAMRAlgorithm* New();
  vtkTypeMacro(vtkUniformGridAMRAlgorithm, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the output data object for a port on this algorithm
   */
  vtkUniformGridAMR* GetOutput();
  vtkUniformGridAMR* GetOutput(int);
  ///@}

  ///@{
  /**
   * Set an input of this algorithm.
   */
  void SetInputData(vtkDataObject*);
  void SetInputData(int, vtkDataObject*);
  ///@}

  /**
   * See vtkAlgorithm for details
   */
  vtkTypeBool ProcessRequest(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

protected:
  vtkUniformGridAMRAlgorithm();
  ~vtkUniformGridAMRAlgorithm() override;

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

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*)
  {
    return 1;
  }

  virtual int RequestUpdateTime(vtkInformation*, vtkInformationVector**, vtkInformationVector*)
  {
    return 1;
  }

  /**
   * Create a default executive
   */
  vtkExecutive* CreateDefaultExecutive() override;

  ///@{
  /**
   * See algorithm for more info.
   */
  int FillOutputPortInformation(int port, vtkInformation* info) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;
  ///@}

  vtkDataObject* GetInput(int port);

private:
  vtkUniformGridAMRAlgorithm(const vtkUniformGridAMRAlgorithm&) = delete;
  void operator=(const vtkUniformGridAMRAlgorithm&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif /* VTKUNIFORMGRIDAMRALGORITHM_H_ */
