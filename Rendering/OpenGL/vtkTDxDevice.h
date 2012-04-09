/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTDxDevice.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTDxDevice - API to access a 3DConnexion input device
// .SECTION Description
// vtkTDxDevice is an abstract class providing access to a 3DConnexion
// input device, such as the SpaceNavigator.
//
// Concrete classes are platform-dependent
// .SECTION See Also
// vtkTDxUnixDevice, vtkTDxWinDevice
// .SECTION Caveats
// THIS IS EXPERIMENTAL CODE. THE API MIGHT CHANGE.

#ifndef __vtkTDxDevice_h
#define __vtkTDxDevice_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkObject.h"

class vtkRenderWindowInteractor;

class VTKRENDERINGOPENGL_EXPORT vtkTDxDevice : public vtkObject
{
public:
  vtkTypeMacro(vtkTDxDevice,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Tell if the device is initialized. Initial value is false.
  bool GetInitialized() const;

  // Description:
  // Close the device. This is called by the destructor.
  // You don't have to close the device explicitly, as the destructor do it
  // automatically, but you can.
  // \pre initialized: GetInitialized().
  // \post restored: !GetInitialized()
  virtual void Close()=0;

  // Description:
  // Get the interactor on which events will be invoked.
  // Initial value is 0.
  // Called by the Interactor itself ONLY.
  vtkRenderWindowInteractor *GetInteractor() const;

  // Description:
  // Set the interactor on which events will be invoked.
  // Initial value is 0.
  // Called by the Interactor itself ONLY.
  // It can be called if the device is initialized or not.
  void SetInteractor(vtkRenderWindowInteractor *i);

protected:
  // Description:
  // Default constructor. Just set initial values for Initialized (false).
  vtkTDxDevice();

  // Description:
  // Destructor. If the device is not initialized, do nothing. If the device
  // is initialized, close the device. This behavior has to be implemented
  // in subclasses.
  virtual ~vtkTDxDevice();

  bool Initialized;
  vtkRenderWindowInteractor *Interactor;

private:
  vtkTDxDevice(const vtkTDxDevice&);  // Not implemented.
  void operator=(const vtkTDxDevice&);  // Not implemented.
};

#endif
