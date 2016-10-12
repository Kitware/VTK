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
/**
 * @class   vtkTDxWinDevice
 * @brief   Implementation of vtkTDxDevice on Windows
 *
 * vtkTDxWinDevice is a concrete implementation of vtkTDxDevice on Windows
 * It uses the COM API.
 * @sa
 * vtkTDxDevice, vtkTDxWinDevice
*/

#ifndef vtkTDxWinDevice_h
#define vtkTDxWinDevice_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkTDxDevice.h"

class vtkRenderWindowInteractor;

// including <WinDef.h> directly leads to the following error:
// "C:\Program Files\Microsoft SDKs\Windows\v6.0A\include\winnt.h(81) :
// fatal error C1189: #error :  "No Target Architecture" "
// so we need to include <windows.h> instead.
#include <windows.h> // we need HWND from <WinDef.h>

class vtkTDxWinDevicePrivate;

class VTKRENDERINGOPENGL_EXPORT vtkTDxWinDevice : public vtkTDxDevice
{
public:
  static vtkTDxWinDevice *New();
  vtkTypeMacro(vtkTDxWinDevice,vtkTDxDevice);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Get the Handle of the Window. Initial value is 0.
   */
  HWND GetWindowHandle() const;

  /**
   * Set the handle of the Window.
   * \pre not_yet_initialized: !GetInitialized()
   */
  void SetWindowHandle(HWND hWnd);

  /**
   * Initialize the device with the current display and window ids.
   * It updates the value of GetInitialized().
   * Initialization can fail (if the device is not present or the driver is
   * not running). You must look for the value of
   * GetInitialized() before processing further.
   * If the case initialization is successful, GetIsListening() is false.
   * \pre not_yet_initialized: !GetInitialized()
   */
  void Initialize();

  /**
   * See description in the superclass. Implementation for Windows.
   */
  virtual void Close();

  /**
   * Tells if we are listening events on the device.
   */
  bool GetIsListening() const;

  /**
   * Call it when the window has or get the focus
   * \pre initialized: GetInitialized()
   * \pre not_yet: !GetIsListening()
   */
  void StartListening();

  /**
   * Call it when the window lose the focus.
   * \pre initialized: GetInitialized()
   * \pre is_listening: GetIsListening()
   */
  void StopListening();

  /**
   * Process the 3DConnexion event.
   * Called internally by the timer.
   */
  void ProcessEvent();

protected:
  /**
   * Default constructor.
   */
  vtkTDxWinDevice();

  /**
   * Destructor. If the device is not initialized, do nothing. If the device
   * is initialized, close the device.
   */
  virtual ~vtkTDxWinDevice();

  HWND WindowHandle;

  vtkTDxWinDevicePrivate *Private;
  bool IsListening;

private:
  vtkTDxWinDevice(const vtkTDxWinDevice&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTDxWinDevice&) VTK_DELETE_FUNCTION;
};

#endif
