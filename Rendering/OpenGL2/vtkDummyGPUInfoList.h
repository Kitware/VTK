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

// .NAME vtkDummyGPUInfoList - Do thing during Probe()
// .SECTION Description
// vtkDummyGPUInfoList implements Probe() by just setting the count of
// GPUs to be zero. Useful when an OS specific implementation is not available.
// .SECTION See Also
// vtkGPUInfo vtkGPUInfoList

#ifndef vtkDummyGPUInfoList_h
#define vtkDummyGPUInfoList_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkGPUInfoList.h"

class VTKRENDERINGOPENGL2_EXPORT vtkDummyGPUInfoList : public vtkGPUInfoList
{
public:
  static vtkDummyGPUInfoList* New();
  vtkTypeMacro(vtkDummyGPUInfoList, vtkGPUInfoList);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Build the list of vtkInfoGPU if not done yet.
  // \post probed: IsProbed()
  virtual void Probe();

protected:
  // Description:
  // Default constructor.
  vtkDummyGPUInfoList();
  virtual ~vtkDummyGPUInfoList();

private:
  vtkDummyGPUInfoList(const vtkDummyGPUInfoList&); // Not implemented.
  void operator=(const vtkDummyGPUInfoList&); // Not implemented.
};

#endif
