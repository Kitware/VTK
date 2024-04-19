// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkVRMenuRepresentation
 * @brief   Widget representation for vtkVRMenuWidget
 * Implementation of the popup panel representation for the
 * vtkVRMenuWidget.
 * This representation is rebuilt every time the selected/hovered prop changes.
 * Its position is set according to the camera orientation and is placed at a
 * distance defined in meters in the BuildRepresentation() method.
 *
 * WARNING: The panel might be occluded by other props.
 *   TODO: Improve placement method.
 **/

#ifndef vtkVRMenuRepresentation_h
#define vtkVRMenuRepresentation_h

#include "vtkRenderingVRModule.h" // For export macro
#include "vtkWidgetRepresentation.h"
#include <deque> // for ivar

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGVR_EXPORT vtkVRMenuRepresentation : public vtkWidgetRepresentation
{
public:
  /**
   * Instantiate the class.
   */
  static vtkVRMenuRepresentation* New();

  ///@{
  /**
   * Standard methods for the class.
   */
  vtkTypeMacro(vtkVRMenuRepresentation, vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Methods to interface with the vtkVRMenuWidget.
   */
  void BuildRepresentation() override;

  void StartComplexInteraction(vtkRenderWindowInteractor* iren, vtkAbstractWidget* widget,
    unsigned long event, void* calldata) override;
  void ComplexInteraction(vtkRenderWindowInteractor* iren, vtkAbstractWidget* widget,
    unsigned long event, void* calldata) override;
  void EndComplexInteraction(vtkRenderWindowInteractor* iren, vtkAbstractWidget* widget,
    unsigned long event, void* calldata) override;
  ///@}

  ///@{
  /**
   * Methods supporting the rendering process.
   */
  void ReleaseGraphicsResources(vtkWindow*) override;
  vtkTypeBool HasTranslucentPolygonalGeometry() override;
  int RenderOverlay(vtkViewport*) override;
  ///@}

  ///@{
  /**
   * Methods to add/remove items to the menu, called by the menu widget
   */
  void PushFrontMenuItem(const char* name, const char* text, vtkCommand* cmd);
  void RenameMenuItem(const char* name, const char* text);
  void RemoveMenuItem(const char* name);
  void RemoveAllMenuItems();
  ///@}

  vtkGetMacro(CurrentOption, double);

protected:
  vtkVRMenuRepresentation();
  ~vtkVRMenuRepresentation() override;

  class InternalElement;
  std::deque<InternalElement*> Menus;

  double CurrentOption; // count from start of the list
  double PlacedPos[3];
  double PlacedDOP[3];
  double PlacedVUP[3];
  double PlacedVRight[3];
  double PlacedOrientation[3];

private:
  vtkVRMenuRepresentation(const vtkVRMenuRepresentation&) = delete;
  void operator=(const vtkVRMenuRepresentation&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
