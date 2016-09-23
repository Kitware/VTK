/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDummyGPUInfoList.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

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

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkGPUInfoList.h"

class VTKRENDERINGOPENGL_EXPORT vtkDummyGPUInfoList : public vtkGPUInfoList
{
public:
  static vtkDummyGPUInfoList* New();
  vtkTypeMacro(vtkDummyGPUInfoList, vtkGPUInfoList);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Build the list of vtkInfoGPU if not done yet.
   * \post probed: IsProbed()
   */
  virtual void Probe();

protected:
  //@{
  /**
   * Default constructor.
   */
  vtkDummyGPUInfoList();
  virtual ~vtkDummyGPUInfoList();
  //@}

private:
  vtkDummyGPUInfoList(const vtkDummyGPUInfoList&) VTK_DELETE_FUNCTION;
  void operator=(const vtkDummyGPUInfoList&) VTK_DELETE_FUNCTION;
};

#endif
