/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOpenVRMenuWidget.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenVRMenuWidget
 * @brief   3D widget to display a menu in VR
 *
 * @sa
 * vtkOpenVRMenuRepresentation
 */

#ifndef vtkOpenVRMenuWidget_h
#define vtkOpenVRMenuWidget_h

#include "vtkAbstractWidget.h"
#include "vtkRenderingOpenVRModule.h" // For export macro
#include <deque>                      // for ivar

class vtkEventData;
class vtkOpenVRMenuRepresentation;
class vtkPropMap;
class vtkProp;

class VTKRENDERINGOPENVR_EXPORT vtkOpenVRMenuWidget : public vtkAbstractWidget
{
public:
  /**
   * Instantiate the object.
   */
  static vtkOpenVRMenuWidget* New();

  //@{
  /**
   * Standard vtkObject methods
   */
  vtkTypeMacro(vtkOpenVRMenuWidget, vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  /**
   * Specify an instance of vtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of vtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(vtkOpenVRMenuRepresentation* rep);

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

  //@{
  /**
   * Get the widget state.
   */
  vtkGetMacro(WidgetState, int);
  //@}

  // Manage the state of the widget
  enum _WidgetState
  {
    Start = 0,
    Active
  };

  //@{
  /**
   * Methods to add/remove items to the menu, called by the menu widget
   */
  void PushFrontMenuItem(const char* name, const char* text, vtkCommand* cmd);
  void RenameMenuItem(const char* name, const char* text);
  void RemoveMenuItem(const char* name);
  void RemoveAllMenuItems();
  //@}

  void Show(vtkEventData* ed);
  void ShowSubMenu(vtkOpenVRMenuWidget*);

protected:
  vtkOpenVRMenuWidget();
  ~vtkOpenVRMenuWidget() override;

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
  vtkOpenVRMenuWidget(const vtkOpenVRMenuWidget&) = delete;
  void operator=(const vtkOpenVRMenuWidget&) = delete;
};
#endif
