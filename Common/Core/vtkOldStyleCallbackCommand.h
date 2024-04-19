// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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

#include "vtkCommand.h"
#include "vtkCommonCoreModule.h" // For export macro

// the old style void fund(void *) callbacks
VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT vtkOldStyleCallbackCommand : public vtkCommand
{
public:
  vtkTypeMacro(vtkOldStyleCallbackCommand, vtkCommand);

  static vtkOldStyleCallbackCommand* New() { return new vtkOldStyleCallbackCommand; }

  /**
   * Satisfy the superclass API for callbacks.
   */
  void Execute(vtkObject* invoker, unsigned long eid, void* calldata) override;

  ///@{
  /**
   * Methods to set and get client and callback information.
   */
  void SetClientData(void* cd) { this->ClientData = cd; }
  void SetCallback(void (*f)(void* clientdata)) { this->Callback = f; }
  void SetClientDataDeleteCallback(void (*f)(void*)) { this->ClientDataDeleteCallback = f; }
  ///@}

  void* ClientData;
  void (*Callback)(void*);
  void (*ClientDataDeleteCallback)(void*);

protected:
  vtkOldStyleCallbackCommand();
  ~vtkOldStyleCallbackCommand() override;
};

VTK_ABI_NAMESPACE_END
#endif /* vtkOldStyleCallbackCommand_h */

// VTK-HeaderTest-Exclude: vtkOldStyleCallbackCommand.h
