// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class vtkRemoteInteractionAdapter
 * @brief Map vtk-js interaction events to native VTK events
 *
 * Apply an vtk-js events to a vtkRenderWindowInteractor.
 * For the expected format see
 * https://github.com/Kitware/vtk-js/blob/master/Sources/Interaction/Style/InteractorStyleRemoteMouse/index.js
 *
 * Events are processed in the `ProcessEvent` method which can be called
 * either as a static method providing all the relevant parameters as arguments
 * or  a class method with the parameters provided via member variables.
 *
 */

#ifndef vtkRemoteInteractionAdapter_h
#define vtkRemoteInteractionAdapter_h

#include "vtkObject.h"
#include "vtkWebCoreModule.h" // for exports

VTK_ABI_NAMESPACE_BEGIN

class vtkRenderWindowInteractor;

class VTKWEBCORE_EXPORT vtkRemoteInteractionAdapter : public vtkObject
{
public:
  static vtkRemoteInteractionAdapter* New();
  vtkTypeMacro(vtkRemoteInteractionAdapter, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * @brief Apply the vtk-js event to the internal RenderWindowInteractor
   * @param event stringified json representation of a vtk-js interaction event.
   * @return true if the event is processed , false otherwise
   */
  bool ProcessEvent(const std::string& event);

  /**
   * Static version of ProcessEvent(const std::string&)
   * @return true if the event is processed , false otherwise
   */
  static bool ProcessEvent(vtkRenderWindowInteractor* iren, const std::string& event,
    double devicePixelRatio = 1.0, double devicePixelRatioTolerance = 1e-5);

  ///@{
  // Get/Set the  ratio between physical (onscreen) pixel and logical (rendered image)
  vtkSetMacro(DevicePixelRatio, double);
  vtkGetMacro(DevicePixelRatio, double);
  ///@}

  ///@{
  /**
   * Tolerance used when truncating the event position from
   * physical to logical. i.e.  int event_position_x = int(event.at("x") * devicePixelRatio +
   * devicePixelRatioTolerance)
   */
  vtkSetMacro(DevicePixelRatioTolerance, double);
  vtkGetMacro(DevicePixelRatioTolerance, double);
  ///@}

  ///@{
  // Get/Set the Interactor to apply the event to.
  void SetInteractor(vtkRenderWindowInteractor* iren);
  vtkGetObjectMacro(Interactor, vtkRenderWindowInteractor);
  ///@}

protected:
  vtkRemoteInteractionAdapter();
  ~vtkRemoteInteractionAdapter() override;

private:
  vtkRemoteInteractionAdapter(const vtkRemoteInteractionAdapter&) = delete;
  void operator=(const vtkRemoteInteractionAdapter&) = delete;

  double DevicePixelRatio = 1.0;
  double DevicePixelRatioTolerance = 1e-5;
  vtkRenderWindowInteractor* Interactor = nullptr;
};

VTK_ABI_NAMESPACE_END
#endif
