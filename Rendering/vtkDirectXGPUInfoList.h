/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDirectXGPUInfoList.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkDirectXGPUInfoList - Get GPUs VRAM information using DirectX.
// .SECTION Description
// vtkDirectXGPUInfoList implements Probe() method of vtkGPUInfoList
// through the DirectX API. As recommended by Microsoft, the WMI interface is
// used for Windows XP and the DXGI interface is used for Windows Vista and
// later. (see documentation of VideoMemory sample of the DirectX SDK)
// ref: http://msdn.microsoft.com/en-us/library/cc308070(VS.85).aspx
// .SECTION See Also
// vtkGPUInfo vtkGPUInfoList

#ifndef __vtkDirectXGPUInfoList_h
#define __vtkDirectXGPUInfoList_h

#include "vtkGPUInfoList.h"

#include <d3d9.h> // DirectX, HMONITOR

class VTK_RENDERING_EXPORT vtkDirectXGPUInfoList : public vtkGPUInfoList
{
public:
  static vtkDirectXGPUInfoList* New();
  vtkTypeMacro(vtkDirectXGPUInfoList, vtkGPUInfoList);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Build the list of vtkInfoGPU if not done yet.
  // \post probed: IsProbed()
  virtual void Probe();
  
protected:
  // Description:
  // Default constructor.
  vtkDirectXGPUInfoList();
  virtual ~vtkDirectXGPUInfoList();
  
  // Description:
  // Probe the GPUs with the DXGI api (Windows Vista or later).
  // It returns true if it succeeded (DXGI API is supported and probing
  // succeeded)
  // \pre m_exists: m!=0
  // \pre info_exist: info!=0
  bool ProbeInfoWithDXGI(HMONITOR m,
                         vtkGPUInfo *info);

  // Description:
  // Probe the GPUs with the WMI api (Windows XP or later).
  // \pre m_exists: m!=0
  // \pre info_exist: info!=0
  void ProbeInfoWithWMI(HMONITOR m,
                        vtkGPUInfo *info);
  
  // Description:
  // Used by ProbeInfoWithWMI().
  // \pre pre hm_exists: hm!=0
  // \pre strDeviceID_exists: strDeviceID!=0
  // \pre cchDeviceID_is_positive: cchDeviceID>0
  bool GetDeviceIDFromHMonitor(HMONITOR hm,
                               WCHAR *strDeviceID,
                               int cchDeviceID);
  
private:
  vtkDirectXGPUInfoList(const vtkDirectXGPUInfoList&); // Not implemented.
  void operator=(const vtkDirectXGPUInfoList&); // Not implemented.
};

#endif
