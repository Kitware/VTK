/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkVRMenuWidget.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkVRMenuWidget
 * @brief   3D widget to display a menu in VR
 *
 * @sa
 * vtkVRMenuRepresentation
 */

#ifndef vtkVRMenuWidget_h
#define vtkVRMenuWidget_h

#include "vtkAbstractWidget.h"
#include "vtkDeprecation.h"       // For VTK_DEPRECATED_IN_9_2_0
#include "vtkRenderingVRModule.h" // For export macro
#include <deque>                  // for ivar

class vtkEventData;
class vtkVRMenuRepresentation;

class VTKRENDERINGVR_EXPORT vtkVRMenuWidget : public vtkAbstractWidget
{
public:
  /**
   * Instantiate the object.
   */
  static vtkVRMenuWidget* New();

  ///@{
  /**
   * Standard vtkObject methods
   */
  vtkTypeMacro(vtkVRMenuWidget, vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Specify an instance of vtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of vtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(vtkVRMenuRepresentation* rep);

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

  ///@{
  /**
   * Get the widget state.
   */
  vtkGetMacro(WidgetState, int);
  ///@}

  // Manage the state of the widget
  enum WidgetStateType
  {
    Start = 0,
    Active
  };
#if !defined(VTK_LEGACY_REMOVE)
  VTK_DEPRECATED_IN_9_2_0("because leading underscore is reserved")
  typedef WidgetStateType _WidgetState;
#endif

  ///@{
  /**
   * Methods to add/remove items to the menu, called by the menu widget
   */
  void PushFrontMenuItem(const char* name, const char* text, vtkCommand* cmd);
  void RenameMenuItem(const char* name, const char* text);
  void RemoveMenuItem(const char* name);
  void RemoveAllMenuItems();
  ///@}

  void Show(vtkEventData* ed);
  void ShowSubMenu(vtkVRMenuWidget*);

protected:
  vtkVRMenuWidget();
  ~vtkVRMenuWidget() override;

  int WidgetState;

  class InternalElement;
  std::deque<InternalElement*> Menus;

  // These are the callbacks for this widget
  static void StartMenuAction(vtkAbstractWidget*);
  static void SelectMenuAction(vtkAbstractWidget*);
  static void MoveAction(vtkAbstractWidget*);

  vtkCallbackCommand* EventCommand;
  static void EventCallback(
    vtkObject* object, unsigned long event, void* clientdata, void* calldata);

  /**
   * Update callback to check for the hovered prop
   */
  static void Update(vtkAbstractWidget*);

private:
  vtkVRMenuWidget(const vtkVRMenuWidget&) = delete;
  void operator=(const vtkVRMenuWidget&) = delete;
};
#endif
