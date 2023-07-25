// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkZSpaceRenderWindowInteractor
 * @brief   Handle zSpace specific interactions.
 *
 * This class handle the zSpace specific interactions, done with the stylus.
 * It will internally update and retrieve the state of the zSpace devices
 * (through the zSpace manager instance, in the ProcessEvents method) and
 * emit events accordingly.
 */

#ifndef vtkZSpaceRenderWindowInteractor_h
#define vtkZSpaceRenderWindowInteractor_h

#include "vtkEventData.h" // For vtkEventDataDevice
#include "vtkRenderWindowInteractor3D.h"
#include "vtkRenderingZSpaceModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN

class VTKRENDERINGZSPACE_EXPORT vtkZSpaceRenderWindowInteractor : public vtkRenderWindowInteractor3D
{
public:
  /**
   * Construct object so that light follows camera motion.
   */
  static vtkZSpaceRenderWindowInteractor* New();
  vtkTypeMacro(vtkZSpaceRenderWindowInteractor, vtkRenderWindowInteractor3D);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * These methods correspond to the Exit, User and Pick
   * callbacks. They allow for the Style to invoke them.
   */
  virtual void ExitCallback();

  /**
   * Update WorldEventPosition and WorldEventOrientation, then
   * call event functions depending on the zSpace buttons states.
   */
  void ProcessEvents() override;

  /*
   * Return the pointer index as a device
   */
  vtkEventDataDevice GetPointerDevice();

  ///@{
  /**
   * LeftButton event function (invoke Button3DEvent)
   * Initiate a clip : choose a clipping plane origin
   * and normal with the stylus.
   */
  void OnLeftButtonDown(vtkEventDataDevice3D*);
  void OnLeftButtonUp(vtkEventDataDevice3D*);
  ///@}

  ///@{
  /**
   * MiddleButton event function (invoke Button3DEvent)
   * Allows to position a prop with the stylus.
   */
  void OnMiddleButtonDown(vtkEventDataDevice3D*);
  void OnMiddleButtonUp(vtkEventDataDevice3D*);
  ///@}

  ///@{
  /**
   * LeftButton event function (invoke Button3DEvent)
   * Perform an hardware picking with the stylus
   * and show picked data if ShowPickedData is true.
   */
  void OnRightButtonDown(vtkEventDataDevice3D*);
  void OnRightButtonUp(vtkEventDataDevice3D*);
  ///@}

protected:
  vtkZSpaceRenderWindowInteractor();
  ~vtkZSpaceRenderWindowInteractor() override = default;

  /**
   * This will start up the event loop and never return. If you call this
   * method it will loop processing events until the application is exited.
   */
  void StartEventLoop() override;

private:
  vtkZSpaceRenderWindowInteractor(const vtkZSpaceRenderWindowInteractor&) = delete;
  void operator=(const vtkZSpaceRenderWindowInteractor&) = delete;
};

VTK_ABI_NAMESPACE_END

#endif
