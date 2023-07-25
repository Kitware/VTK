// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkGPUInfoList
 * @brief   Stores the list of GPUs VRAM information.
 *
 * vtkGPUInfoList stores a list of vtkGPUInfo. An host can have
 * several GPUs. It creates and sets the list by probing the host with system
 * calls. This an abstract class. Concrete classes are OS specific.
 * @sa
 * vtkGPUInfo vtkDirectXGPUInfoList vtkCoreGraphicsGPUInfoList
 */

#ifndef vtkGPUInfoList_h
#define vtkGPUInfoList_h

#include "vtkObject.h"
#include "vtkRenderingCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkGPUInfoListArray; // STL Pimpl
class vtkGPUInfo;

class VTKRENDERINGCORE_EXPORT vtkGPUInfoList : public vtkObject
{
public:
  static vtkGPUInfoList* New();
  vtkTypeMacro(vtkGPUInfoList, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Build the list of vtkInfoGPU if not done yet.
   * Default implementation created an empty list. Useful if there is no
   * implementation available for a given architecture yet.
   * \post probed: IsProbed()
   */
  virtual void Probe() = 0;

  /**
   * Tells if the operating system has been probed. Initial value is false.
   */
  virtual bool IsProbed();

  /**
   * Return the number of GPUs.
   * \pre probed: IsProbed()
   */
  virtual int GetNumberOfGPUs();

  /**
   * Return information about GPU i.
   * \pre probed: IsProbed()
   * \pre valid_index: i>=0 && i<GetNumberOfGPUs()
   * \post result_exists: result!=0
   */
  virtual vtkGPUInfo* GetGPUInfo(int i);

protected:
  ///@{
  /**
   * Default constructor. Set Probed to false. Set Array to NULL.
   */
  vtkGPUInfoList();
  ~vtkGPUInfoList() override;
  ///@}

  bool Probed;
  vtkGPUInfoListArray* Array;

private:
  vtkGPUInfoList(const vtkGPUInfoList&) = delete;
  void operator=(const vtkGPUInfoList&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
