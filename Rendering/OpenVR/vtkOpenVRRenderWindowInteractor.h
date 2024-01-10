// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenVRRenderWindowInteractor
 * @brief   Implements OpenVR specific functions required by vtkVRRenderWindowInteractor.
 */

#ifndef vtkOpenVRRenderWindowInteractor_h
#define vtkOpenVRRenderWindowInteractor_h

#include "vtkEventData.h"             // for ivar
#include "vtkRenderingOpenVRModule.h" // For export macro
#include "vtkVRRenderWindowInteractor.h"

#include <functional> // for ivar
#include <map>        // for ivar
#include <openvr.h>   // for ivar
#include <string>     // for ivar

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGOPENVR_EXPORT vtkOpenVRRenderWindowInteractor : public vtkVRRenderWindowInteractor
{
public:
  static vtkOpenVRRenderWindowInteractor* New();
  vtkTypeMacro(vtkOpenVRRenderWindowInteractor, vtkVRRenderWindowInteractor);

  /**
   * Initialize the event handler.
   */
  void Initialize() override;

  /**
   * Implements the event loop.
   */
  void DoOneEvent(vtkVRRenderWindow* renWin, vtkRenderer* ren) override;

  ///@{
  /**
   * Assign an event or std::function to an event path.
   */
  void AddAction(const std::string& path, const vtkCommand::EventIds&, bool isAnalog) override;
  void AddAction(
    const std::string& path, bool isAnalog, const std::function<void(vtkEventData*)>&) override;
  ///@}

protected:
  /**
   * Create and set the openvr style on this
   * Set ActionManifestFileName to vtk_openvr_actions.json
   * Set ActionSetName to /actions/vtk
   */
  vtkOpenVRRenderWindowInteractor();
  ~vtkOpenVRRenderWindowInteractor() override = default;

  class ActionData
  {
  public:
    vr::VRActionHandle_t ActionHandle;
    vtkCommand::EventIds EventId;
    std::function<void(vtkEventData*)> Function;
    bool UseFunction = false;
    bool IsAnalog = false;
  };

  std::map<std::string, ActionData> ActionMap;
  vr::VRActionSetHandle_t ActionsetVTK = vr::k_ulInvalidActionSetHandle;

  enum TrackerEnum
  {
    LEFT_HAND = 0,
    RIGHT_HAND,
    HEAD,
    NUMBER_OF_TRACKERS
  };

  struct TrackerActions
  {
    vr::VRInputValueHandle_t Source = vr::k_ulInvalidInputValueHandle;
    vr::TrackedDevicePose_t LastPose;
  };

  TrackerActions Trackers[NUMBER_OF_TRACKERS];

private:
  vtkOpenVRRenderWindowInteractor(const vtkOpenVRRenderWindowInteractor&) = delete;
  void operator=(const vtkOpenVRRenderWindowInteractor&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
