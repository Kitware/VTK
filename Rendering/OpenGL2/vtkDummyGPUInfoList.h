// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkDummyGPUInfoList
 * @brief   Do thing during Probe()
 *
 * vtkDummyGPUInfoList implements Probe() by just setting the count of
 * GPUs to be zero. Useful when an OS specific implementation is not available.
 * @sa
 * vtkGPUInfo vtkGPUInfoList
 */

#ifndef vtkDummyGPUInfoList_h
#define vtkDummyGPUInfoList_h

#include "vtkGPUInfoList.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGOPENGL2_EXPORT vtkDummyGPUInfoList : public vtkGPUInfoList
{
public:
  static vtkDummyGPUInfoList* New();
  vtkTypeMacro(vtkDummyGPUInfoList, vtkGPUInfoList);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Build the list of vtkInfoGPU if not done yet.
   * \post probed: IsProbed()
   */
  void Probe() override;

protected:
  ///@{
  /**
   * Default constructor.
   */
  vtkDummyGPUInfoList();
  ~vtkDummyGPUInfoList() override;
  ///@}

private:
  vtkDummyGPUInfoList(const vtkDummyGPUInfoList&) = delete;
  void operator=(const vtkDummyGPUInfoList&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
