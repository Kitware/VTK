/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGPUInfo.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkGPUInfo - Stores GPU VRAM information.
// .SECTION Description
// vtkGPUInfo stores information about GPU Video RAM. An host can have
// several GPUs. The values are set by vtkGPUInfoList.
// .SECTION See Also
// vtkGPUInfoList vtkDirectXGPUInfoList vtkCoreGraphicsGPUInfoList

#ifndef vtkGPUInfo_h
#define vtkGPUInfo_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkObject.h"

class VTKRENDERINGCORE_EXPORT vtkGPUInfo : public vtkObject
{
public:
  static vtkGPUInfo* New();
  vtkTypeMacro(vtkGPUInfo, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get dedicated video memory in bytes. Initial value is 0.
  // Usually the fastest one. If it is not 0, it should be taken into
  // account first and DedicatedSystemMemory or SharedSystemMemory should be
  // ignored.
  vtkSetMacro(DedicatedVideoMemory, vtkTypeUInt64);
  vtkGetMacro(DedicatedVideoMemory, vtkTypeUInt64);

  // Description:
  // Set/Get dedicated system memory in bytes. Initial value is 0.
  // This is slow memory. If it is not 0, this value should be taken into
  // account only if there is no DedicatedVideoMemory and SharedSystemMemory
  // should be ignored.
  vtkSetMacro(DedicatedSystemMemory, vtkTypeUInt64);
  vtkGetMacro(DedicatedSystemMemory, vtkTypeUInt64);

  // Description:
  // Set/Get shared system memory in bytes. Initial value is 0.
  // Slowest memory. This value should be taken into account only if there is
  // neither DedicatedVideoMemory nor DedicatedSystemMemory.
  vtkSetMacro(SharedSystemMemory, vtkTypeUInt64);
  vtkGetMacro(SharedSystemMemory, vtkTypeUInt64);

protected:
  vtkGPUInfo();
  ~vtkGPUInfo();

  vtkTypeUInt64 DedicatedVideoMemory;
  vtkTypeUInt64 DedicatedSystemMemory;
  vtkTypeUInt64 SharedSystemMemory;

private:
  vtkGPUInfo(const vtkGPUInfo&); // Not implemented.
  void operator=(const vtkGPUInfo&); // Not implemented.
};

#endif
