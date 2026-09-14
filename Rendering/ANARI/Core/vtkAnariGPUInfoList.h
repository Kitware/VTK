// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkAnariGPUInfoList
 * @brief   Do thing during Probe()
 *
 * vtkAnariGPUInfoList implements Probe() by just setting the count of
 * GPUs to be zero. Useful when an OS specific implementation is not available.
 * @sa vtkGPUInfo vtkGPUInfoList
 */

#ifndef vtkAnariGPUInfoList_h
#define vtkAnariGPUInfoList_h

#include "vtkGPUInfoList.h"
#include "vtkRenderingAnariCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkOverrideAttribute;

class VTKRENDERINGANARICORE_EXPORT vtkAnariGPUInfoList : public vtkGPUInfoList
{
public:
  static vtkAnariGPUInfoList* New();
  static vtkOverrideAttribute* CreateOverrideAttributes();
  vtkTypeMacro(vtkAnariGPUInfoList, vtkGPUInfoList);
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
  vtkAnariGPUInfoList();
  ~vtkAnariGPUInfoList() override;
  ///@}

private:
  vtkAnariGPUInfoList(const vtkAnariGPUInfoList&) = delete;
  void operator=(const vtkAnariGPUInfoList&) = delete;
};

#define vtkAnariGPUInfoList_OVERRIDE_ATTRIBUTES vtkAnariGPUInfoList::CreateOverrideAttributes()

VTK_ABI_NAMESPACE_END
#endif
