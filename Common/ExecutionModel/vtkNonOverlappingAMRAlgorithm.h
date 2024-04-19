// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkNonOverlappingAMRAlgorithm
 *  produce vtkNonOverlappingAMR as output.
 *
 *
 *
 */

#ifndef vtkNonOverlappingAMRAlgorithm_h
#define vtkNonOverlappingAMRAlgorithm_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkUniformGridAMRAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkNonOverlappingAMR;
class vtkInformation;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkNonOverlappingAMRAlgorithm
  : public vtkUniformGridAMRAlgorithm
{
public:
  static vtkNonOverlappingAMRAlgorithm* New();
  vtkTypeMacro(vtkNonOverlappingAMRAlgorithm, vtkUniformGridAMRAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the output data object for a port on this algorithm
   */
  vtkNonOverlappingAMR* GetOutput();
  vtkNonOverlappingAMR* GetOutput(int);
  ///@}

protected:
  vtkNonOverlappingAMRAlgorithm();
  ~vtkNonOverlappingAMRAlgorithm() override;

  ///@{
  /**
   * See algorithm for more info.
   */
  int FillOutputPortInformation(int port, vtkInformation* info) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;
  ///@}

private:
  vtkNonOverlappingAMRAlgorithm(const vtkNonOverlappingAMRAlgorithm&) = delete;
  void operator=(const vtkNonOverlappingAMRAlgorithm&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif /* VTKNONOVERLAPPINGAMRALGORITHM_H_ */
