/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOpenVRInteractorStyle.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenVRInteractorStyle
 * @brief   extended from vtkInteractorStyle3D to override command methods
 */

#ifndef vtkOpenVRInteractorStyle_h
#define vtkOpenVRInteractorStyle_h

#include "vtkRenderingOpenVRModule.h" // For export macro

#include "vtkEventData.h" // for enums
#include "vtkInteractorStyle3D.h"
#include "vtkNew.h"                // for ivars
#include "vtkOpenVRRenderWindow.h" // for enums

class vtkCell;
class vtkPlane;
class vtkOpenVRControlsHelper;
class vtkOpenVRHardwarePicker;
class vtkOpenVRMenuRepresentation;
class vtkOpenVRMenuWidget;
class vtkTextActor3D;
class vtkSelection;
class vtkSphereSource;

class VTKRENDERINGOPENVR_EXPORT vtkOpenVRInteractorStyle : public vtkInteractorStyle3D
{
public:
  static vtkOpenVRInteractorStyle* New();
  vtkTypeMacro(vtkOpenVRInteractorStyle, vtkInteractorStyle3D);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Override generic event bindings to call the corresponding action.
   */
  void OnButton3D(vtkEventData* edata) override;
  void OnMove3D(vtkEventData* edata) override;
  //@}

  //@{
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
  virtual void StartDolly3D(vtkEventDataDevice3D*);
  virtual void EndDolly3D(vtkEventDataDevice3D*);
  //@}

  //@{
  /**
   * Multitouch events binding.
   */
  void OnPan() override;
  void OnPinch() override;
  void OnRotate() override;
  //@}

  //@{
  /**
   * Methods for intertaction.
   */
  void ProbeData(vtkEventDataDevice controller);
  void LoadNextCameraPose();
  virtual void PositionProp(vtkEventData*);
  virtual void Clip(vtkEventDataDevice3D*);
  //@}

  //@{
  /**
   * Map controller inputs to actions.
   * Actions are defined by a VTKIS_*STATE*, interaction entry points,
   * and the corresponding method for interaction.
   */
  void MapInputToAction(vtkEventDataDevice device, vtkEventDataDeviceInput input, int state);
  //@}

  //@{
  /**
   * Define the helper text that goes with an input
   */
  void AddTooltipForInput(
    vtkEventDataDevice device, vtkEventDataDeviceInput input, const std::string& text);
  //@}

  //@{
  /**
   * Indicates if picking should be updated every frame. If so, the interaction
   * picker will try to pick a prop and rays will be updated accordingly.
   * Default is set to off.
   */
  vtkSetMacro(HoverPick, bool);
  vtkGetMacro(HoverPick, bool);
  vtkBooleanMacro(HoverPick, bool);
  //@}

  //@{
  /**
   * Specify if the grab mode use the ray to grab distant objects
   */
  vtkSetMacro(GrabWithRay, bool);
  vtkGetMacro(GrabWithRay, bool);
  vtkBooleanMacro(GrabWithRay, bool);
  //@}

  int GetInteractionState(vtkEventDataDevice device)
  {
    return this->InteractionState[static_cast<int>(device)];
  }

  void ShowRay(vtkEventDataDevice controller);
  void HideRay(vtkEventDataDevice controller);

  void ShowBillboard(const std::string& text);
  void HideBillboard();

  void ShowPickSphere(double* pos, double radius, vtkProp3D*);
  void ShowPickCell(vtkCell* cell, vtkProp3D*);
  void HidePickActor();

  void ToggleDrawControls();
  void SetDrawControls(bool);

  void SetInteractor(vtkRenderWindowInteractor* iren) override;

  // allow the user to add options to the menu
  vtkOpenVRMenuWidget* GetMenu() { return this->Menu.Get(); }

protected:
  vtkOpenVRInteractorStyle();
  ~vtkOpenVRInteractorStyle() override;

  void EndPickCallback(vtkSelection* sel);

  // Ray drawing
  void UpdateRay(vtkEventDataDevice controller);

  vtkNew<vtkOpenVRMenuWidget> Menu;
  vtkNew<vtkOpenVRMenuRepresentation> MenuRepresentation;
  vtkCallbackCommand* MenuCommand;
  static void MenuCallback(
    vtkObject* object, unsigned long event, void* clientdata, void* calldata);

  vtkNew<vtkTextActor3D> TextActor3D;
  vtkNew<vtkActor> PickActor;
  vtkNew<vtkSphereSource> Sphere;

  // device input to interaction state mapping
  int InputMap[vtkEventDataNumberOfDevices][vtkEventDataNumberOfInputs];
  vtkOpenVRControlsHelper* ControlsHelpers[vtkEventDataNumberOfDevices][vtkEventDataNumberOfInputs];

  // Utility routines
  void StartAction(int VTKIS_STATE, vtkEventDataDevice3D* edata);
  void EndAction(int VTKIS_STATE, vtkEventDataDevice3D* edata);

  // Pick using hardware selector
  bool HardwareSelect(vtkEventDataDevice controller, bool actorPassOnly);

  bool HoverPick;
  bool GrabWithRay;

  /**
   * Store required controllers information when performing action
   */
  int InteractionState[vtkEventDataNumberOfDevices];
  vtkProp3D* InteractionProps[vtkEventDataNumberOfDevices];
  vtkPlane* ClippingPlanes[vtkEventDataNumberOfDevices];

  vtkNew<vtkOpenVRHardwarePicker> HardwarePicker;

  /**
   * Controls helpers drawing
   */
  void AddTooltipForInput(vtkEventDataDevice device, vtkEventDataDeviceInput input);

private:
  vtkOpenVRInteractorStyle(const vtkOpenVRInteractorStyle&) = delete; // Not implemented.
  void operator=(const vtkOpenVRInteractorStyle&) = delete;           // Not implemented.
};

#endif
