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
 *
 * This widget allows the creation of a polyline interactively by adding or removing points
 * based on mouse position and a modifier key.
 *
 * - ctrl+click inserts a new point on the selected line
 * - shift+click deletes the selected point
 * - alt+click adds a new point anywhere depending on last selected point.
 *   If the first point is selected, the new point is added at the beginning,
 *   else it is added at the end.
 *
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
  void PrintSelf(ostream& os, vtkIndent indent) override;

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
  void CreateDefaultRepresentation() override;

protected:
  vtkPolyLineWidget();
  ~vtkPolyLineWidget() override;

  int WidgetState;
  enum _WidgetState {Start=0,Active};

  // These methods handle events
  static void SelectAction(vtkAbstractWidget*);
  static void EndSelectAction(vtkAbstractWidget*);
  static void TranslateAction(vtkAbstractWidget*);
  static void ScaleAction(vtkAbstractWidget*);
  static void MoveAction(vtkAbstractWidget*);

private:
  vtkPolyLineWidget(const vtkPolyLineWidget&) = delete;
  void operator=(const vtkPolyLineWidget&) = delete;

};

#endif
