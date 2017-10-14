/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXGPUInfoList.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXGPUInfoList
 * @brief   Get GPUs VRAM information using X server
 * extensions.
 *
 * vtkXGPUInfoList implements Probe() method of vtkGPUInfoList
 * through some X server extensions API. NV-CONTROL for Nvidia.
 * ATIFGLEXTENSION for ATI is not supported yet.
 * There is no support for other vendors.
 * @sa
 * vtkGPUInfo vtkGPUInfoList
*/

#ifndef vtkXGPUInfoList_h
#define vtkXGPUInfoList_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkGPUInfoList.h"

class VTKRENDERINGOPENGL_EXPORT vtkXGPUInfoList : public vtkGPUInfoList
{
public:
  static vtkXGPUInfoList* New();
  vtkTypeMacro(vtkXGPUInfoList, vtkGPUInfoList);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Build the list of vtkInfoGPU if not done yet.
   * \post probed: IsProbed()
   */
  void Probe() override;

protected:
  //@{
  /**
   * Default constructor.
   */
  vtkXGPUInfoList();
  ~vtkXGPUInfoList() override;
  //@}

private:
  vtkXGPUInfoList(const vtkXGPUInfoList&) = delete;
  void operator=(const vtkXGPUInfoList&) = delete;
};

#endif
