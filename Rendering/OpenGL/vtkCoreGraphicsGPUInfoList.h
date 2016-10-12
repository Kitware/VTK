/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCoreGraphicsGPUInfoList.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCoreGraphicsGPUInfoList
 * @brief   Get GPUs VRAM information using CoreGraphics.
 *
 * vtkCoreGraphicsGPUInfoList implements the Probe() method of vtkGPUInfoList
 * using Mac OS X APIs.
 * @sa
 * vtkGPUInfo vtkGPUInfoList
*/

#ifndef vtkCoreGraphicsGPUInfoList_h
#define vtkCoreGraphicsGPUInfoList_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkGPUInfoList.h"

class VTKRENDERINGOPENGL_EXPORT vtkCoreGraphicsGPUInfoList : public vtkGPUInfoList
{
public:
  static vtkCoreGraphicsGPUInfoList* New();
  vtkTypeMacro(vtkCoreGraphicsGPUInfoList, vtkGPUInfoList);
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
  vtkCoreGraphicsGPUInfoList();
  virtual ~vtkCoreGraphicsGPUInfoList();
  //@}

private:
  vtkCoreGraphicsGPUInfoList(const vtkCoreGraphicsGPUInfoList&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCoreGraphicsGPUInfoList&) VTK_DELETE_FUNCTION;
};

#endif
