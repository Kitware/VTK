/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTDxMacDevice.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkTDxMacDevice
 * @brief   Implementation of vtkTDxDevice on Mac
 *
 * vtkTDxMacDevice is a concrete implementation of vtkTDxDevice on Mac
 * It uses the 3DxMacWare SDK.
 * @sa
 * vtkTDxDevice, vtkTDxUnixDevice, vtkTDxWinDevice
*/

#ifndef vtkTDxMacDevice_h
#define vtkTDxMacDevice_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkTDxDevice.h"

#include <3dConnexionClient/ConnexionClientAPI.h> // 3DxMacWare SDK

class VTKRENDERINGOPENGL_EXPORT vtkTDxMacDevice : public vtkTDxDevice
{
public:
  static vtkTDxMacDevice *New();
  vtkTypeMacro(vtkTDxMacDevice,vtkTDxDevice);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Name of the client application to pass for registration with the
   * driver. Initial value is "3DxClientTest".
   */
  vtkGetStringMacro(ClientApplicationName);
  vtkSetStringMacro(ClientApplicationName);
  //@}

  /**
   * Initialize the device with the current ClientApplicationName.
   * It updates the value of GetInitialized().
   * Initialization can fail (if the device is not present or the driver is
   * not running). You must look for the value of
   * GetInitialized() before processing further.
   * \pre not_yet_initialized: !GetInitialized()
   * \pre valid_name: GetClientApplicationName()!=0
   */
  void Initialize();

  /**
   * See description in the superclass. Implementation for Mac.
   */
  virtual void Close();

  /**
   * Translate the X11 event by invoking a VTK event, if the event came from
   * the device.
   * Return true if the event passed in argument was effectively an event from
   * the device, return false otherwise.
   * \pre initialized: GetInitialized()
   * \pre s_exists: s!=0
   * \pre client_matches: s->client==this->ClientID
   */
  void ProcessEvent(const ConnexionDeviceState *s);

protected:
  /**
   * Default constructor. Just set initial values for
   * ClientApplicationName ("3DxClientTest").
   */
  vtkTDxMacDevice();

  /**
   * Destructor. If the device is not initialized, do nothing. If the device
   * is initialized, close the device.
   */
  virtual ~vtkTDxMacDevice();

  /**
   * Convert a C string to a Pascal String. Allocation happens with new[].
   * It is the responsibility of the user to call delete[].

   * Apple specific. String literal starting with \p are pascal strings: it is
   * an unsigned char array starting with the length and terminated by \0. The
   * length does not include the length value neither the \0.
   * Pascal strings literal are allowed with apple specific gcc option
   * -fpascal-strings. Pascal literal are writable is -fwritable-strings is
   * set. -Wwrite-strings is a related warning flag.
   * \pre s_exists: s!=0
   * \pre s_small_enough: strlen(s)<=255
   * \post result_exists: result!=0
   */
  unsigned char *CStringToPascalString(const char *s);

  char *ClientApplicationName;

  UInt16 ClientID;
  UInt16 LastButtonState;

private:
  vtkTDxMacDevice(const vtkTDxMacDevice&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTDxMacDevice&) VTK_DELETE_FUNCTION;
};

#endif
