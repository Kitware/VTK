/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenVRRenderWindowInteractor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenVRRenderWindowInteractor
 * @brief   implements OpenVR specific functions
 * required by vtkRenderWindowInteractor.
 */

#ifndef vtkOpenVRRenderWindowInteractor_h
#define vtkOpenVRRenderWindowInteractor_h

#include "vtkEventData.h"             // for ivar
#include "vtkNew.h"                   // ivars
#include "vtkRenderingOpenVRModule.h" // For export macro
#include "vtkVRRenderWindowInteractor.h"

#include <functional> // for ivar
#include <map>        // for ivar
#include <openvr.h>   // for ivar
#include <string>     // for ivar
#include <tuple>      // for ivar

class vtkOpenVRRenderWindow;

class VTKRENDERINGOPENVR_EXPORT vtkOpenVRRenderWindowInteractor : public vtkVRRenderWindowInteractor
{
public:
  /**
   * Construct object so that light follows camera motion.
   */
  static vtkOpenVRRenderWindowInteractor* New();
  vtkTypeMacro(vtkOpenVRRenderWindowInteractor, vtkRenderWindowInteractor3D);

  /**
   * Initialize the event handler
   */
  virtual void Initialize() override;

  virtual void DoOneEvent(vtkVRRenderWindow* renWin, vtkRenderer* ren) override;

  ///@{
  /**
   * Assign an event or std::function to an event path
   */
  void AddAction(std::string path, vtkCommand::EventIds, bool isAnalog);
  void AddAction(std::string path, bool isAnalog, std::function<void(vtkEventData*)>);
  ///@}

protected:
  vtkOpenVRRenderWindowInteractor();
  ~vtkOpenVRRenderWindowInteractor() = default;

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
    LeftHand = 0,
    RightHand,
    Head,
    NumberOfTrackers
  };

  struct TrackerActions
  {
    vr::VRInputValueHandle_t Source = vr::k_ulInvalidInputValueHandle;
    vr::TrackedDevicePose_t LastPose;
  };
  TrackerActions Trackers[NumberOfTrackers];

private:
  vtkOpenVRRenderWindowInteractor(const vtkOpenVRRenderWindowInteractor&) = delete;
  void operator=(const vtkOpenVRRenderWindowInteractor&) = delete;
};

#endif
