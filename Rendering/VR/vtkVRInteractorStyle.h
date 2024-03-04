// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkVRInteractorStyle
 * @brief   Extended from vtkInteractorStyle3D to override command methods.
 *
 * This abstract class defines an interactor style in a virtual reality context.
 */

#ifndef vtkVRInteractorStyle_h
#define vtkVRInteractorStyle_h

#include "vtkEventData.h" // for enums
#include "vtkInteractorStyle3D.h"
#include "vtkNew.h"               // for vtkNew
#include "vtkRenderingVRModule.h" // For export macro
#include "vtkSmartPointer.h"      // for vtkSmartPointer

#include <map>    // for std::map
#include <vector> // for std::vector

VTK_ABI_NAMESPACE_BEGIN
class vtkCallbackCommand;
class vtkCell;
class vtkPlane;
class vtkRenderWindowInteractor;
class vtkSelection;
class vtkSphereSource;
class vtkTextActor3D;
class vtkVRControlsHelper;
class vtkVRHardwarePicker;
class vtkVRMenuRepresentation;
class vtkVRMenuWidget;

class VTKRENDERINGVR_EXPORT vtkVRInteractorStyle : public vtkInteractorStyle3D
{
public:
  vtkTypeMacro(vtkVRInteractorStyle, vtkInteractorStyle3D);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Override generic event bindings to call the corresponding action.
   */
  void OnSelect3D(vtkEventData* edata) override;
  void OnNextPose3D(vtkEventData* edata) override;
  void OnViewerMovement3D(vtkEventData* edata) override;
  void OnMove3D(vtkEventData* edata) override;
  void OnMenu3D(vtkEventData* edata) override;
  void OnElevation3D(vtkEventData* edata) override;
  ///@}

  ///@{
  /**
   * Interaction mode entry points.
   */
  virtual void StartPick(vtkEventDataDevice3D*);
  virtual void EndPick(vtkEventDataDevice3D*);
  virtual void StartLoadCamPose(vtkEventDataDevice3D*);
  virtual void EndLoadCamPose(vtkEventDataDevice3D*);
  virtual void StartPositionProp(vtkEventDataDevice3D*);
  virtual void EndPositionProp(vtkEventDataDevice3D*);
  virtual void StartClip(vtkEventDataDevice3D*);
  virtual void EndClip(vtkEventDataDevice3D*);
  virtual void StartMovement3D(int interactionState, vtkEventDataDevice3D*);
  virtual void EndMovement3D(vtkEventDataDevice3D*);
  ///@}

  ///@{
  /**
   * Complex gesture events binding.
   */
  void OnPan() override;
  void OnPinch() override;
  void OnRotate() override;
  ///@}

  ///@{
  /**
   * Methods for interaction.
   */
  void ProbeData(vtkEventDataDevice controller);
  void PositionProp(vtkEventData*, double* lwpos = nullptr, double* lwori = nullptr) override;
  void Clip(vtkEventDataDevice3D*);
  virtual void LoadNextCameraPose() = 0;
  ///@}

  /**
   * Move the camera on the "XY" plan (ground) using the thumbstick/trackpad position
   * (up/down and left/right), according to the headset view direction.
   */
  void GroundMovement3D(vtkEventDataDevice3D*);

  /**
   * Move the camera following the "Z" axis (elevation) using the thumbstick/trackpad
   * position (up/down).
   */
  void Elevation3D(vtkEventDataDevice3D*);

  /**
   * Instant movement to the picked point
   */
  void Teleportation3D(vtkEventDataDevice3D* edd);

  ///@{
  /**
   * Map controller inputs to actions.
   * Actions are defined by a VTKIS_*STATE*, interaction entry points,
   * and the corresponding method for interaction.
   */
  void MapInputToAction(vtkCommand::EventIds eid, int state);
  void MapInputToAction(vtkCommand::EventIds eid, vtkEventDataAction action, int state);
  int GetMappedAction(
    vtkCommand::EventIds eid, vtkEventDataAction action = vtkEventDataAction::Press);
  ///@}

  /**
   * Define the helper text that goes with an input.
   */
  void AddTooltipForInput(
    vtkEventDataDevice device, vtkEventDataDeviceInput input, const std::string& text = {});

  /**
   * Creates a new ControlsHelper suitable for use with the child class.
   */
  virtual vtkVRControlsHelper* MakeControlsHelper() = 0;

  ///@{
  /**
   * Indicates if picking should be updated every frame. If so, the interaction
   * picker will try to pick a prop and rays will be updated accordingly.
   * Default is set to off.
   */
  vtkSetMacro(HoverPick, bool);
  vtkGetMacro(HoverPick, bool);
  vtkBooleanMacro(HoverPick, bool);
  ///@}

  ///@{
  /**
   * Specify if the grab mode uses the ray to grab distant objects.
   * Default is set to on.
   */
  vtkSetMacro(GrabWithRay, bool);
  vtkGetMacro(GrabWithRay, bool);
  vtkBooleanMacro(GrabWithRay, bool);
  ///@}

  enum MovementStyle
  {
    FLY_STYLE,
    GROUNDED_STYLE
  };

  ///@{
  /**
   * Specify the movement style between 'Flying" and "Grounded".
   * Default is Flying.
   */
  vtkSetMacro(Style, MovementStyle);
  vtkGetMacro(Style, MovementStyle);
  ///@}

  /**
   * Return interaction state for the specified device (dolly, pick, none, etc...).
   *
   * \sa SetInteractionState()
   */
  int GetInteractionState(vtkEventDataDevice device)
  {
    int deviceIndex = static_cast<int>(device);
    if (deviceIndex < 0 || deviceIndex >= vtkEventDataNumberOfDevices)
    {
      // Since VTKIS_*STATE* are expected to be >= VTKIS_NONE with VTKIS_NONE == 0,
      // return -1 if device is invalid.
      return -1;
    }
    return this->InteractionState[static_cast<int>(device)];
  }

  /**
   * Set interaction state for the specified device (dolly, pick, none, etc...).
   *
   * This method **does not** call `this->Modified()`.
   *
   * \sa GetInteractionState()
   */
  void SetInteractionState(vtkEventDataDevice device, int state);

  ///@{
  /**
   * Show/hide the ray for the specified controller.
   */
  void ShowRay(vtkEventDataDevice controller);
  void HideRay(vtkEventDataDevice controller);
  ///@}

  ///@{
  /**
   * Show/hide billboard with given text string.
   */
  void ShowBillboard(const std::string& text);
  void HideBillboard();
  ///@}

  /**
   * Make the pick actor a sphere of given radius centered at given position,
   * and show it.
   */
  void ShowPickSphere(double* pos, double radius, vtkProp3D*);

  /**
   * Make the pick actor a polydata built from the points and edges of the
   * given cell, and show it.
   */

  void ShowPickCell(vtkCell* cell, vtkProp3D*);

  /**
   * Hide the pick actor (sphere or polydata).
   */
  void HidePickActor();

  ///@{
  /**
   * Control visibility of descriptive tooltips for controller/HMD models.
   */
  void ToggleDrawControls();
  void SetDrawControls(bool);
  ///@}

  /**
   * Set the Interactor wrapper being controlled by this object.
   */
  void SetInteractor(vtkRenderWindowInteractor* iren) override;

  /**
   * Setup default actions defined with an action path and a corresponding command.
   */
  virtual void SetupActions(vtkRenderWindowInteractor* iren) = 0;

  /**
   * Return the menu to allow the user to add options to it.
   */
  vtkVRMenuWidget* GetMenu() { return this->Menu.Get(); }

protected:
  vtkVRInteractorStyle();
  ~vtkVRInteractorStyle() override;

  /**
   * Update and draw the ray.
   */
  virtual void UpdateRay(vtkEventDataDevice controller);

  void EndPickCallback(vtkSelection* sel);
  static void MenuCallback(
    vtkObject* object, unsigned long event, void* clientdata, void* calldata);

  /**
   * Utility routines.
   */
  void StartAction(int VTKIS_STATE, vtkEventDataDevice3D* edata);
  void EndAction(int VTKIS_STATE, vtkEventDataDevice3D* edata);

  /**
   * Pick using hardware selector.
   */
  bool HardwareSelect(vtkEventDataDevice controller, bool actorPassOnly);

  /**
   * Update the 3D movement according to the given interaction state.
   */
  void Movement3D(int interactionState, vtkEventData* edata);

  bool HoverPick = false;
  bool GrabWithRay = true;

  vtkNew<vtkVRMenuWidget> Menu;
  vtkNew<vtkVRMenuRepresentation> MenuRepresentation;
  vtkNew<vtkCallbackCommand> MenuCommand;

  vtkNew<vtkTextActor3D> TextActor3D;
  vtkNew<vtkActor> PickActor;
  vtkNew<vtkSphereSource> Sphere;
  vtkNew<vtkVRHardwarePicker> HardwarePicker;

  // Device input to interaction state mapping
  std::map<std::tuple<vtkCommand::EventIds, vtkEventDataAction>, int> InputMap;
  vtkVRControlsHelper* ControlsHelpers[vtkEventDataNumberOfDevices][vtkEventDataNumberOfInputs];

  // Store required controllers information when performing action
  int InteractionState[vtkEventDataNumberOfDevices];
  std::vector<vtkSmartPointer<vtkProp3D>> InteractionProps;
  std::vector<vtkSmartPointer<vtkPlane>> ClippingPlanes;

  // Store headset world orientation
  double HeadsetDir[3] = { 0, 0, 0 };

  // Store movement style
  MovementStyle Style = vtkVRInteractorStyle::FLY_STYLE;

  // Interaction timers
  vtkNew<vtkTimerLog> LastGroundMovement3DEventTime;
  vtkNew<vtkTimerLog> LastElevation3DEventTime;

  // Interaction trackpad position
  double LastGroundMovementTrackPadPosition[2] = { 0, 0 };
  double LastElevationTrackPadPosition[2] = { 0, 0 };

private:
  vtkVRInteractorStyle(const vtkVRInteractorStyle&) = delete;
  void operator=(const vtkVRInteractorStyle&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
