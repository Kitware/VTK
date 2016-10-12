/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOldStyleCallbackCommand.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOldStyleCallbackCommand
 * @brief   supports legacy function callbacks for VTK
 *
 * vtkOldStyleCallbackCommand is a callback that supports the legacy callback
 * methods found in VTK. For example, the legacy method
 * vtkProcessObject::SetStartMethod() is actually invoked using the
 * command/observer design pattern of VTK, and the vtkOldStyleCallbackCommand
 * is used to provide the legacy functionality. The callback function should
 * have the form void func(void *clientdata), where clientdata is special data
 * that should is associated with this instance of vtkCallbackCommand.
 *
 * @warning
 * This is legacy glue. Please do not use; it will be eventually eliminated.
 *
 * @sa
 * vtkCommand vtkCallbackCommand
*/

#ifndef vtkOldStyleCallbackCommand_h
#define vtkOldStyleCallbackCommand_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkCommand.h"

// the old style void fund(void *) callbacks
class VTKCOMMONCORE_EXPORT vtkOldStyleCallbackCommand : public vtkCommand
{
public:
  vtkTypeMacro(vtkOldStyleCallbackCommand,vtkCommand);

  static vtkOldStyleCallbackCommand *New()
    {return new vtkOldStyleCallbackCommand;};

  /**
   * Satisfy the superclass API for callbacks.
   */
  void Execute(vtkObject *invoker,
               unsigned long eid,
               void *calldata) VTK_OVERRIDE;

  //@{
  /**
   * Methods to set and get client and callback information.
   */
  void SetClientData(void *cd)
    {this->ClientData = cd;};
  void SetCallback(void (*f)(void *clientdata))
    {this->Callback = f;};
  void SetClientDataDeleteCallback(void (*f)(void *))
    {this->ClientDataDeleteCallback = f;};
  //@}

  void *ClientData;
  void (*Callback)(void *);
  void (*ClientDataDeleteCallback)(void *);

protected:
  vtkOldStyleCallbackCommand();
  ~vtkOldStyleCallbackCommand() VTK_OVERRIDE;
};


#endif /* vtkOldStyleCallbackCommand_h */

// VTK-HeaderTest-Exclude: vtkOldStyleCallbackCommand.h
