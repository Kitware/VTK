/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyLineWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPolyLineWidget
 * @brief   widget for vtkPolyLineRepresentation.
 *
 * vtkPolyLineWidget is the vtkAbstractWidget subclass for
 * vtkPolyLineRepresentation which manages the interactions with
 * vtkPolyLineRepresentation. This is based on vtkPolyLineWidget.
 * @sa
 * vtkPolyLineRepresentation, vtkPolyLineWidget
*/

#ifndef vtkPolyLineWidget_h
#define vtkPolyLineWidget_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkAbstractWidget.h"

class vtkPolyLineRepresentation;

class VTKINTERACTIONWIDGETS_EXPORT vtkPolyLineWidget : public vtkAbstractWidget
{
public:
  static vtkPolyLineWidget* New();
  vtkTypeMacro(vtkPolyLineWidget, vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Specify an instance of vtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of
   * vtkProp so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(vtkPolyLineRepresentation *r)
  {
    this->Superclass::SetWidgetRepresentation(
      reinterpret_cast<vtkWidgetRepresentation*>(r));
  }

  /**
   * Create the default widget representation if one is not set. By default,
   * this is an instance of the vtkPolyLineRepresentation class.
   */
  void CreateDefaultRepresentation();

protected:
  vtkPolyLineWidget();
  ~vtkPolyLineWidget();

  int WidgetState;
  enum _WidgetState {Start=0,Active};

  // These methods handle events
  static void SelectAction(vtkAbstractWidget*);
  static void EndSelectAction(vtkAbstractWidget*);
  static void TranslateAction(vtkAbstractWidget*);
  static void ScaleAction(vtkAbstractWidget*);
  static void MoveAction(vtkAbstractWidget*);

private:
  vtkPolyLineWidget(const vtkPolyLineWidget&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPolyLineWidget&) VTK_DELETE_FUNCTION;

};

#endif
