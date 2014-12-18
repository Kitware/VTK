/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAxesTransformWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAxesTransformWidget - 3D widget for performing 3D transformations around an axes
// .SECTION Description
// This 3D widget defines an axes which is used to guide transformation. The
// widget can translate, scale, and rotate around one of the three coordinate
// axes. The widget consists of a handle at the origin (used for
// translation), three axes (around which rotations occur), and three end
// arrows (or cones depending on the representation) that can be stretched to
// scale an object.  Optionally a text label can be used to indicate the
// amount of the transformation.
//
// To use this widget, you generally pair it with a
// vtkAxesTransformRepresentation (or a subclass). Various options are
// available in the representation for controlling how the widget appears,
// and how the widget functions.
//
// .SECTION Event Bindings
// By default, the widget responds to the following VTK events (i.e., it
// watches the vtkRenderWindowInteractor for these events):
// <pre>
// If the origin handle is selected:
//   LeftButtonPressEvent - activate the associated handle widget
//   LeftButtonReleaseEvent - release the handle widget associated with the point
//   MouseMoveEvent - move the handle and hence the origin and the widget
// If one of the lines is selected:
//   LeftButtonPressEvent - activate rotation by selecting one of the three axes.
//   LeftButtonReleaseEvent - end rotation
//   MouseMoveEvent - moving along the selected axis causes rotation to occur.
// If one of the arrows/cones is selected:
//   LeftButtonPressEvent - activate scaling by selecting the ends of one of the three axes.
//   LeftButtonReleaseEvent - end scaling
//   MouseMoveEvent - moving along the selected axis causes scaling to occur.
// </pre>
//
// Note that the event bindings described above can be changed using this
// class's vtkWidgetEventTranslator. This class translates VTK events
// into the vtkAxesTransformWidget's widget events:
// <pre>
//   vtkWidgetEvent::Select -- some part of the widget has been selected
//   vtkWidgetEvent::EndSelect -- the selection process has completed
//   vtkWidgetEvent::Move -- a request for slider motion has been invoked
// </pre>
//
// In turn, when these widget events are processed, the vtkAxesTransformWidget
// invokes the following VTK events on itself (which observers can listen for):
// <pre>
//   vtkCommand::StartInteractionEvent (on vtkWidgetEvent::Select)
//   vtkCommand::EndInteractionEvent (on vtkWidgetEvent::EndSelect)
//   vtkCommand::InteractionEvent (on vtkWidgetEvent::Move)
// </pre>
//

// .SECTION Caveats
// Note that the widget can be picked even when it is "behind"
// other actors.  This is an intended feature and not a bug.
//
// This class, and vtkAxesTransformRepresentation, are next generation VTK widgets.

// .SECTION See Also
// vtkAxesTransformRepresentation vtkAffineWidget vtkBoxWidget2

#ifndef vtkAxesTransformWidget_h
#define vtkAxesTransformWidget_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkAbstractWidget.h"

class vtkAxesTransformRepresentation;
class vtkHandleWidget;


class VTKINTERACTIONWIDGETS_EXPORT vtkAxesTransformWidget : public vtkAbstractWidget
{
public:
  // Description:
  // Instantiate the object.
  static vtkAxesTransformWidget *New();

  // Description:
  // Standard vtkObject methods
  vtkTypeMacro(vtkAxesTransformWidget,vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Override superclasses' SetEnabled() method because the line
  // widget must enable its internal handle widgets.
  virtual void SetEnabled(int enabling);

  // Description:
  // Specify an instance of vtkWidgetRepresentation used to represent this
  // widget in the scene. Note that the representation is a subclass of vtkProp
  // so it can be added to the renderer independent of the widget.
  void SetRepresentation(vtkAxesTransformRepresentation *r)
    {this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(r));}

  // Description:
  // Return the representation as a vtkAxesTransformRepresentation.
  vtkAxesTransformRepresentation *GetLineRepresentation()
    {return reinterpret_cast<vtkAxesTransformRepresentation*>(this->WidgetRep);}

  // Description:
  // Create the default widget representation if one is not set.
  void CreateDefaultRepresentation();

  // Description:
  // Methods to change the whether the widget responds to interaction.
  // Overridden to pass the state to component widgets.
  virtual void SetProcessEvents(int);

protected:
  vtkAxesTransformWidget();
  ~vtkAxesTransformWidget();

  int WidgetState;
  enum _WidgetState {Start=0,Active};
  int CurrentHandle;

  // These methods handle events
  static void SelectAction(vtkAbstractWidget*);
  static void EndSelectAction(vtkAbstractWidget*);
  static void MoveAction(vtkAbstractWidget*);

  // The positioning handle widgets
  vtkHandleWidget *OriginWidget; //first end point
  vtkHandleWidget *SelectionWidget; //used when selecting any one of the axes

private:
  vtkAxesTransformWidget(const vtkAxesTransformWidget&);  //Not implemented
  void operator=(const vtkAxesTransformWidget&);  //Not implemented
};

#endif
