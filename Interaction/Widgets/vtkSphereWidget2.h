/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSphereWidget2.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSphereWidget2 - 3D widget for manipulating a point on a sphere
// .SECTION Description
// This 3D widget interacts with a vtkSphereRepresentation class (i.e., it
// handles the events that drive its corresponding representation). It can be
// used to position a point on a sphere (for example, to place a light or
// camera), or to position a sphere in a scene, including translating and
// scaling the sphere.
//
// A nice feature of vtkSphereWidget2, like any 3D widget, is that it will
// work in combination with the current interactor style (or any other
// interactor observer). That is, if vtkSphereWidget2 does not handle an
// event, then all other registered observers (including the interactor
// style) have an opportunity to process the event. Otherwise, the
// vtkSphereWidget2 will terminate the processing of the event that it
// handles.
//
// To use this widget, you generally pair it with a vtkSphereRepresentation
// (or a subclass). Variuos options are available in the representation for
// controlling how the widget appears, and how the widget functions.
//
// .SECTION Event Bindings
// By default, the widget responds to the following VTK events (i.e., it
// watches the vtkRenderWindowInteractor for these events):
// <pre>
// If the handle or sphere are selected:
//   LeftButtonPressEvent - select the handle or sphere
//   LeftButtonReleaseEvent - release the handle ot sphere
//   MouseMoveEvent - move the handle or translate the sphere
// In all the cases, independent of what is picked, the widget responds to the
// following VTK events:
//   MiddleButtonPressEvent - translate the representation
//   MiddleButtonReleaseEvent - stop translating the representation
//   RightButtonPressEvent - scale the widget's representation
//   RightButtonReleaseEvent - stop scaling the representation
//   MouseMoveEvent - scale (if right button) or move (if middle button) the widget
// </pre>
//
// Note that the event bindings described above can be changed using this
// class's vtkWidgetEventTranslator. This class translates VTK events
// into the vtkSphereWidget2's widget events:
// <pre>
//   vtkWidgetEvent::Select -- some part of the widget has been selected
//   vtkWidgetEvent::EndSelect -- the selection process has completed
//   vtkWidgetEvent::Scale -- some part of the widget has been selected
//   vtkWidgetEvent::EndScale -- the selection process has completed
//   vtkWidgetEvent::Translate -- some part of the widget has been selected
//   vtkWidgetEvent::EndTranslate -- the selection process has completed
//   vtkWidgetEvent::Move -- a request for motion has been invoked
// </pre>
//
// In turn, when these widget events are processed, the vtkSphereWidget2
// invokes the following VTK events on itself (which observers can listen for):
// <pre>
//   vtkCommand::StartInteractionEvent (on vtkWidgetEvent::Select)
//   vtkCommand::EndInteractionEvent (on vtkWidgetEvent::EndSelect)
//   vtkCommand::InteractionEvent (on vtkWidgetEvent::Move)
// </pre>

//
// This class, and the affiliated vtkSphereRepresentation, are second generation
// VTK widgets. An earlier version of this functionality was defined in the
// class vtkSphereWidget.

// .SECTION See Also
// vtkSphereRepresentation vtkSphereWidget

#ifndef __vtkSphereWidget2_h
#define __vtkSphereWidget2_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkAbstractWidget.h"

class vtkSphereRepresentation;
class vtkHandleWidget;


class VTKINTERACTIONWIDGETS_EXPORT vtkSphereWidget2 : public vtkAbstractWidget
{
public:
  // Description:
  // Instantiate the object.
  static vtkSphereWidget2 *New();

  // Description:
  // Standard class methods for type information and printing.
  vtkTypeMacro(vtkSphereWidget2,vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify an instance of vtkWidgetRepresentation used to represent this
  // widget in the scene. Note that the representation is a subclass of
  // vtkProp so it can be added to the renderer independent of the widget.
  void SetRepresentation(vtkSphereRepresentation *r)
    {this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(r));}

  // Description:
  // Control the behavior of the widget (i.e., how it processes
  // events). Translation, and scaling can all be enabled and disabled.
  vtkSetMacro(TranslationEnabled,int);
  vtkGetMacro(TranslationEnabled,int);
  vtkBooleanMacro(TranslationEnabled,int);
  vtkSetMacro(ScalingEnabled,int);
  vtkGetMacro(ScalingEnabled,int);
  vtkBooleanMacro(ScalingEnabled,int);

  // Description:
  // Create the default widget representation if one is not set. By default,
  // this is an instance of the vtkSphereRepresentation class.
  void CreateDefaultRepresentation();

protected:
  vtkSphereWidget2();
  ~vtkSphereWidget2();

//BTX - manage the state of the widget
  int WidgetState;
  enum _WidgetState {Start=0,Active};
//ETX

  // These methods handle events
  static void SelectAction(vtkAbstractWidget*);
  static void EndSelectAction(vtkAbstractWidget*);
  static void TranslateAction(vtkAbstractWidget*);
  static void ScaleAction(vtkAbstractWidget*);
  static void MoveAction(vtkAbstractWidget*);

  // Control whether scaling and translation are supported
  int TranslationEnabled;
  int ScalingEnabled;

private:
  vtkSphereWidget2(const vtkSphereWidget2&);  //Not implemented
  void operator=(const vtkSphereWidget2&);  //Not implemented
};

#endif
