/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkImplicitPlaneRepresentation.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenVRMenuRepresentation
 * @brief   Widget representation for vtkOpenVRPanelWidget
 * Implementation of the popup panel representation for the
 * vtkOpenVRPanelWidget.
 * This representation is rebuilt every time the selected/hovered prop changes.
 * Its position is set according to the camera orientation and is placed at a
 * distance defined in meters in the BuildRepresentation() method.
 *
 * WARNING: The panel might be occluded by other props.
 *   TODO: Improve placement method.
 **/

#ifndef vtkOpenVRMenuRepresentation_h
#define vtkOpenVRMenuRepresentation_h

#include "vtkRenderingOpenVRModule.h" // For export macro
#include "vtkWidgetRepresentation.h"
#include <deque> // for ivar

class vtkActor;
class vtkProperty;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkCellArray;
class vtkPoints;
class vtkTextActor3D;

class VTKRENDERINGOPENVR_EXPORT vtkOpenVRMenuRepresentation : public vtkWidgetRepresentation
{
public:
  /**
   * Instantiate the class.
   */
  static vtkOpenVRMenuRepresentation* New();

  //@{
  /**
   * Standard methods for the class.
   */
  vtkTypeMacro(vtkOpenVRMenuRepresentation, vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  //@{
  /**
   * Methods to interface with the vtkOpenVRPanelWidget.
   */
  void BuildRepresentation() override;

  void StartComplexInteraction(vtkRenderWindowInteractor* iren, vtkAbstractWidget* widget,
    unsigned long event, void* calldata) override;
  void ComplexInteraction(vtkRenderWindowInteractor* iren, vtkAbstractWidget* widget,
    unsigned long event, void* calldata) override;
  void EndComplexInteraction(vtkRenderWindowInteractor* iren, vtkAbstractWidget* widget,
    unsigned long event, void* calldata) override;
  //@}

  //@{
  /**
   * Methods supporting the rendering process.
   */
  void ReleaseGraphicsResources(vtkWindow*) override;
  vtkTypeBool HasTranslucentPolygonalGeometry() override;
  int RenderOverlay(vtkViewport*) override;
  //@}

  //@{
  /**
   * Methods to add/remove items to the menu, called by the menu widget
   */
  void PushFrontMenuItem(const char* name, const char* text, vtkCommand* cmd);
  void RenameMenuItem(const char* name, const char* text);
  void RemoveMenuItem(const char* name);
  void RemoveAllMenuItems();
  //@}

  vtkGetMacro(CurrentOption, double);

protected:
  vtkOpenVRMenuRepresentation();
  ~vtkOpenVRMenuRepresentation() override;

  class InternalElement;
  std::deque<InternalElement*> Menus;

  double CurrentOption; // count from start of the list
  double PlacedPos[3];
  double PlacedDOP[3];
  double PlacedVUP[3];
  double PlacedVRight[3];
  double PlacedOrientation[3];

private:
  vtkOpenVRMenuRepresentation(const vtkOpenVRMenuRepresentation&) = delete;
  void operator=(const vtkOpenVRMenuRepresentation&) = delete;
};

#endif
