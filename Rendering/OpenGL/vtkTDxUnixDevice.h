/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTDxUnixDevice.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTDxUnixDevice - Implementation of vtkTDxDevice on Unix
// .SECTION Description
// vtkTDxUnixDevice is a concrete implementation of vtkTDxDevice on Unix
// It uses the Magellan API.
// .SECTION See Also
// vtkTDxDevice, vtkTDxWinDevice

#ifndef __vtkTDxUnixDevice_h
#define __vtkTDxUnixDevice_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkTDxDevice.h"
//#include <X11/Xlib.h> // Needed for X types used in the public interface
class vtkRenderWindowInteractor;

// We cannot include <X11/Xlib.h> (which defines "Display *",
// "Window" and "XEvent *") because it defines macro like None that would
// conflict with qt4/Qt/qcoreevent.h which defines None as a QEvent::Type
// value.
typedef void vtkTDxUnixDeviceDisplay;
typedef unsigned int vtkTDxUnixDeviceWindow;
typedef void vtkTDxUnixDeviceXEvent;

class VTKRENDERINGOPENGL_EXPORT vtkTDxUnixDevice : public vtkTDxDevice
{
public:
  static vtkTDxUnixDevice *New();
  vtkTypeMacro(vtkTDxUnixDevice,vtkTDxDevice);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the ID of the X Display. Initial value is 0.
  // The return value type is actually a "Display *"
  vtkTDxUnixDeviceDisplay *GetDisplayId() const;

  // Description:
  // Get the ID of the X Window. Initial value is 0.
  // The return value type is actually a "Window"
  vtkTDxUnixDeviceWindow GetWindowId() const;

  // Description:
  // Set the ID of the X Display.
  // The argument type is actually a "Display *".
  // \pre not_yet_initialized: !GetInitialized()
  void SetDisplayId(vtkTDxUnixDeviceDisplay *id);

  // Description:
  // Set the ID of the X Window.
  // \pre not_yet_initialized: !GetInitialized()
  void SetWindowId(vtkTDxUnixDeviceWindow id);

  // Description:
  // Initialize the device with the current display and window ids.
  // It updates the value of GetInitialized().
  // Initialization can fail (if the device is not present or the driver is
  // not running). You must look for the value of
  // GetInitialized() before processing further.
  // This interactor does not have to be set  before calling Initialize().
  // However, in order to handle the events the Interactor has to be set
  // otherwise ProcessEvent will be a no-op.
  // \pre not_yet_initialized: !GetInitialized()
  // \pre valid_display: GetDisplayId()!=0
  // \pre valid_window: GetWindowId()!=0
  // \pre valid_interactor: GetInteractor()!=0
  void Initialize();

  // Description:
  // See description in the superclass. Implementation for Unix.
  virtual void Close();

  // Description:
  // Translate the X11 event by invoking a VTK event, if the event came from
  // the device.
  // Return true if the event passed in argument was effectively an event from
  // the device, return false otherwise.
  // The interactor has to be set in order to get some events, otherwise they
  // will be ignored.
  // \pre initialized: GetInitialized()
  // \pre e_exists: e!=0
  // \pre e_is_client_message: e->type==ClientMessage
  bool ProcessEvent(const vtkTDxUnixDeviceXEvent *e);

  // Description:
  // PROBABLY TRANSFER IT TO THE SUPERCLASS
  // Get/Set Scale factor on the translation motion. Initial value is 1.0
  vtkGetMacro(TranslationScale,double);
  vtkSetMacro(TranslationScale,double);

  // Description:
  // PROBABLY TRANSFER IT TO THE SUPERCLASS
  // Get/Set Scale factor on the rotation motion. Initial value is 1.0
  vtkGetMacro(RotationScale,double);
  vtkSetMacro(RotationScale,double);

  // Description:
  // Set the sensitivity of the device for the current application.
  // A neutral value is 1.0.
  // \pre initialized: GetInitialized()
  void SetSensitivity(double sensitivity);

protected:
  // Description:
  // Default constructor. Just set initial values for
  // DisplayId (0), WindowId (0), TranslationScale (1.0),
  // RotationScale (1.0).
  vtkTDxUnixDevice();

  // Description:
  // Destructor. If the device is not initialized, do nothing. If the device
  // is initialized, close the device.
  virtual ~vtkTDxUnixDevice();

  vtkTDxUnixDeviceDisplay *DisplayId;
  vtkTDxUnixDeviceWindow WindowId;

  double TranslationScale;
  double RotationScale;

private:
  vtkTDxUnixDevice(const vtkTDxUnixDevice&);  // Not implemented.
  void operator=(const vtkTDxUnixDevice&);  // Not implemented.
};

#endif
