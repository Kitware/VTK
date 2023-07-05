// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkZSpaceInteractorStyle
 *
 * vtkZSpaceInteractorStyle extends vtkInteractorStyle3D to override command methods.
 *
 * This class maps EventDataDevice3D device and input to an interaction state :
 *
 * - LeftButton (LeftController + Trigger) maps to VTKIS_PICK. It asks vtkPVZSpaceView to use its
 *   own PVHardwareSelector to pick a cell or a point, depending on the value of
 *   vtkPVZSpaceView::PickingFieldAssociation.
 *   Then informations about picking is shown on the bottom left of the screen. A pick actor is
 *   also shown to visualize the picked cell or picked point.
 *
 * - MiddleButton (GenericTracker + Trigger) maps to VTKIS_POSITION_PROP. It allows the user to
 *   grab the picked actor and move it with the stylus.
 *
 * - RightButton (RightController + Trigger) allows to position the widgets that respond to
 *   this vtkEventDataDevice3D, such as vtkBoxWidget2, vtkHandleWidget, vtkImplicitPlaneWidget2 and
 *   vtkTensorWidget. It doesn't map to any VTKIS_XXX.
 *
 * The move event will then call the method to position the current picked prop
 * if the state is VTKIS_POSITION_PROP.
 */

#ifndef vtkZSpaceInteractorStyle_h
#define vtkZSpaceInteractorStyle_h

#include "vtkDataObject.h" // for vtkDataObject enums
#include "vtkEventData.h"  // for enums
#include "vtkInteractorStyle3D.h"
#include "vtkNew.h"                   // for ivar
#include "vtkRenderingZSpaceModule.h" // for export macro

VTK_ABI_NAMESPACE_BEGIN

class vtkCell;
class vtkPlane;
class vtkSelection;
class vtkDataSet;
class vtkTextActor;
class vtkZSpaceRayActor;
class vtkZSpaceHardwarePicker;

class VTKRENDERINGZSPACE_EXPORT vtkZSpaceInteractorStyle : public vtkInteractorStyle3D
{
public:
  static vtkZSpaceInteractorStyle* New();
  vtkTypeMacro(vtkZSpaceInteractorStyle, vtkInteractorStyle3D);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Override generic event bindings to call the corresponding action.
   */
  void OnPick3D(vtkEventData* edata) override;
  void OnPositionProp3D(vtkEventData* edata) override;
  void OnMove3D(vtkEventData* edata) override;
  void OnSelect3D(vtkEventData* edata) override;
  ///@}

  ///@{
  /**
   * Interaction mode entry points.
   */
  virtual void StartPick(vtkEventDataDevice3D*);
  virtual void EndPick(vtkEventDataDevice3D*);
  virtual void StartPositionProp(vtkEventDataDevice3D*);
  virtual void EndPositionProp(vtkEventDataDevice3D*);
  ///@}

  ///@{
  /**
   * Methods for interaction.
   */
  void ProbeData(vtkEventDataDevice3D*);
  virtual void PositionProp(vtkEventData*, double* lwpos = nullptr, double* lwori = nullptr);
  ///@}

  ///@{
  /**
   * Indicates if picking should be updated every frame. If so, the interaction
   * picker will try to pick a prop and ray will be updated accordingly.
   * Default is set to off.
   */
  vtkSetMacro(HoverPick, bool);
  vtkGetMacro(HoverPick, bool);
  vtkBooleanMacro(HoverPick, bool);
  ///@}

  /**
   * Use FindPickedActor to update the InteractionProp.
   * Then update the ray length to the pick length if something is picked,
   * else to its max length.
   */
  void UpdateRay(vtkEventDataDevice3D*);

  /**
   * Set the zSpaceRayActor that is used to draw the ray stylus.
   */
  vtkSetMacro(ZSpaceRayActor, vtkZSpaceRayActor*);

  ///@{
  /**
   * Select the field association used when picking.
   * Default is vtkDataObject::FIELD_ASSOCIATION_CELLS.
   */
  vtkSetClampMacro(PickingFieldAssociation, int, vtkDataObject::FIELD_ASSOCIATION_POINTS,
    vtkDataObject::FIELD_ASSOCIATION_CELLS);
  vtkGetMacro(PickingFieldAssociation, int);
  ///@}

protected:
  vtkZSpaceInteractorStyle();
  ~vtkZSpaceInteractorStyle() override = default;

  /**
   * Create the text to display information about the selection,
   * create the PickActor to draw the picked cell or point and add
   * it to the renderer.
   */
  void EndPickCallback(vtkSelection* sel);

  ///@{
  /**
   * Utility routines
   */
  void StartAction(int VTKIS_STATE, vtkEventDataDevice3D* edata);
  void EndAction(int VTKIS_STATE, vtkEventDataDevice3D* edata);
  ///@}

  /**
   * Do a selection using the vtkZSpaceHardwarePicker.
   * The selection can be then retrived for the HardwarePricker member
   * variable.
   */
  bool HardwareSelect(vtkEventDataDevice3D* edd, bool actorPassOnly);

  /**
   * From the selection 'sel', find the corresponding dataset 'ds' and the point/cell id 'aid'.
   */
  bool FindDataSet(vtkSelection* sel, vtkSmartPointer<vtkDataSet>& ds, vtkIdType& aid);

  /**
   * Create a string that contains informations about the point or cell defined by the
   * index 'aid' in the dataset 'ds'.
   */
  std::string GetPickedText(vtkDataSet* ds, const vtkIdType& aid);

  /**
   * Create the PickActor to show the picked cell.
   */
  void CreatePickCell(vtkCell* cell);

  /**
   * Create the PickActor to show the picked point.
   */
  void CreatePickPoint(double* point);

  /**
   * Update the PickActor and the TextActor depending on
   * the PickedInteractionProp position and visibility.
   */
  void UpdatePickActor();

  /**
   * Remove the PickActor and the TextActor from the renderer.
   */
  void RemovePickActor();

  /**
   * If false, the ray is updated only when the picking action
   * is done by the user.
   * If true, the ray is updated constantly.
   *
   * Currently, this uses a vtkCellPicker and this leads to bad performances
   * if the actor(s) have a lot of cells.
   */
  bool HoverPick = false;

  // Used to draw picked cells or points
  vtkNew<vtkActor> PickActor;
  // XXX Very good chance we can swap it with this->InteractionProp
  // The text actor is linked to this prop
  vtkProp3D* PickedInteractionProp = nullptr;
  vtkNew<vtkTextActor> TextActor;
  vtkZSpaceRayActor* ZSpaceRayActor;

  // Used to do the actual picking action (not the interactive picking).
  vtkNew<vtkZSpaceHardwarePicker> HardwarePicker;

  // The field association used when picking with the ray
  int PickingFieldAssociation = vtkDataObject::FIELD_ASSOCIATION_CELLS;

private:
  vtkZSpaceInteractorStyle(const vtkZSpaceInteractorStyle&) = delete;
  void operator=(const vtkZSpaceInteractorStyle&) = delete;
};

VTK_ABI_NAMESPACE_END

#endif
