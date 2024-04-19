// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOverlappingAMRAlgorithm
 * @brief   A base class for all algorithms that take as input vtkOverlappingAMR and
 *  produce vtkOverlappingAMR.
 */

#ifndef vtkOverlappingAMRAlgorithm_h
#define vtkOverlappingAMRAlgorithm_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkUniformGridAMRAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkOverlappingAMR;
class vtkInformation;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkOverlappingAMRAlgorithm : public vtkUniformGridAMRAlgorithm
{
public:
  static vtkOverlappingAMRAlgorithm* New();
  vtkTypeMacro(vtkOverlappingAMRAlgorithm, vtkUniformGridAMRAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the output data object for a port on this algorithm
   */
  vtkOverlappingAMR* GetOutput();
  vtkOverlappingAMR* GetOutput(int);
  ///@}

protected:
  vtkOverlappingAMRAlgorithm();
  ~vtkOverlappingAMRAlgorithm() override;

  ///@{
  /**
   * See algorithm for more info.
   */
  int FillOutputPortInformation(int port, vtkInformation* info) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;
  ///@}

private:
  vtkOverlappingAMRAlgorithm(const vtkOverlappingAMRAlgorithm&) = delete;
  void operator=(const vtkOverlappingAMRAlgorithm&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif /* VTKOVERLAPPINGAMRALGORITHM_H_ */
