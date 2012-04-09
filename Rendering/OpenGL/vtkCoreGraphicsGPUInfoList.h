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
// .NAME vtkCoreGraphicsGPUInfoList - Get GPUs VRAM information using CoreGraphics.
// .SECTION Description
// vtkCoreGraphicsGPUInfoList implements Probe() method of vtkGPUInfoList
// through the CoreGraphics API (Apple Mac OS X) and the IOKit API.
// The CoreGraphics framework is part of the ApplicationServices umbrella
// framework. IOKit is a framework on its own.
// ref: http://developer.apple.com/qa/qa2004/qa1168.html
// .SECTION See Also
// vtkGPUInfo vtkGPUInfoList

#ifndef __vtkCoreGraphicsGPUInfoList_h
#define __vtkCoreGraphicsGPUInfoList_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkGPUInfoList.h"

class VTKRENDERINGOPENGL_EXPORT vtkCoreGraphicsGPUInfoList : public vtkGPUInfoList
{
public:
  static vtkCoreGraphicsGPUInfoList* New();
  vtkTypeMacro(vtkCoreGraphicsGPUInfoList, vtkGPUInfoList);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Build the list of vtkInfoGPU if not done yet.
  // \post probed: IsProbed()
  virtual void Probe();

protected:
  // Description:
  // Default constructor.
  vtkCoreGraphicsGPUInfoList();
  virtual ~vtkCoreGraphicsGPUInfoList();

private:
  vtkCoreGraphicsGPUInfoList(const vtkCoreGraphicsGPUInfoList&); // Not implemented.
  void operator=(const vtkCoreGraphicsGPUInfoList&); // Not implemented.
};

#endif
