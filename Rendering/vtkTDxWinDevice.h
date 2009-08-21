/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTDxWinDevice.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTDxWinDevice - Implementation of vtkTDxDevice on Windows
// .SECTION Description
// vtkTDxWinDevice is a concrete implementation of vtkTDxDevice on Windows
// It uses the COM API.
// .SECTION See Also
// vtkTDxDevice, vtkTDxWinDevice

#ifndef __vtkTDxWinDevice_h
#define __vtkTDxWinDevice_h

#include "vtkTDxDevice.h"

class vtkRenderWindowInteractor;

#include <BaseTsd.h> // for UINT_PTR

class VTK_RENDERING_EXPORT vtkTDxWinDevice : public vtkTDxDevice
{
public:
  static vtkTDxWinDevice *New();
  vtkTypeRevisionMacro(vtkTDxWinDevice,vtkTDxDevice);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Initialize the device with the current display and window ids.
  // It updates the value of GetInitialized().
  // Initialization can fail (if the device is not present or the driver is
  // not running). You must look for the value of
  // GetInitialized() before processing further.
  // \pre not_yet_initialized: !GetInitialized()
  void Initialize();
  
  // Description:
  // See description in the superclass. Implementation for Windows.
  virtual void Close();
  
protected:
  // Description:
  // Default constructor.
  vtkTDxWinDevice();
  
  // Description:
  // Destructor. If the device is not initialized, do nothing. If the device
  // is initialized, close the device.
  virtual ~vtkTDxWinDevice();
  
  UINT_PTR TimerId;
  
private:
  vtkTDxWinDevice(const vtkTDxWinDevice&);  // Not implemented.
  void operator=(const vtkTDxWinDevice&);  // Not implemented.
};

#endif
