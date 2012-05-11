/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSliderWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSliderWidget - set a value by manipulating a slider
// .SECTION Description
// The vtkSliderWidget is used to set a scalar value in an application.  This
// class assumes that a slider is moved along a 1D parameter space (e.g., a
// spherical bead that can be moved along a tube).  Moving the slider
// modifies the value of the widget, which can be used to set parameters on
// other objects. Note that the actual appearance of the widget depends on
// the specific representation for the widget.
//
// To use this widget, set the widget representation. The representation is
// assumed to consist of a tube, two end caps, and a slider (the details may
// vary depending on the particulars of the representation). Then in the
// representation you will typically set minimum and maximum value, as well
// as the current value. The position of the slider must also be set, as well
// as various properties.
//
// .SECTION Event Bindings
// By default, the widget responds to the following VTK events (i.e., it
// watches the vtkRenderWindowInteractor for these events):
// <pre>
// If the slider bead is selected:
//   LeftButtonPressEvent - select slider (if on slider)
//   LeftButtonReleaseEvent - release slider (if selected)
//   MouseMoveEvent - move slider
// If the end caps or slider tube are selected:
//   LeftButtonPressEvent - move (or animate) to cap or point on tube;
// </pre>
//
// Note that the event bindings described above can be changed using this
// class's vtkWidgetEventTranslator. This class translates VTK events
// into the vtkSliderWidget's widget events:
// <pre>
//   vtkWidgetEvent::Select -- some part of the widget has been selected
//   vtkWidgetEvent::EndSelect -- the selection process has completed
//   vtkWidgetEvent::Move -- a request for slider motion has been invoked
// </pre>
//
// In turn, when these widget events are processed, the vtkSliderWidget
// invokes the following VTK events on itself (which observers can listen for):
// <pre>
//   vtkCommand::StartInteractionEvent (on vtkWidgetEvent::Select)
//   vtkCommand::EndInteractionEvent (on vtkWidgetEvent::EndSelect)
//   vtkCommand::InteractionEvent (on vtkWidgetEvent::Move)
// </pre>
//

#ifndef __vtkSliderWidget_h
#define __vtkSliderWidget_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkAbstractWidget.h"

class vtkSliderRepresentation;


class VTKINTERACTIONWIDGETS_EXPORT vtkSliderWidget : public vtkAbstractWidget
{
public:
  // Description:
  // Instantiate the class.
  static vtkSliderWidget *New();

  // Description:
  // Standard macros.
  vtkTypeMacro(vtkSliderWidget,vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify an instance of vtkWidgetRepresentation used to represent this
  // widget in the scene. Note that the representation is a subclass of vtkProp
  // so it can be added to the renderer independent of the widget.
  void SetRepresentation(vtkSliderRepresentation *r)
    {this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(r));}

  // Description:
  // Return the representation as a vtkSliderRepresentation.
  vtkSliderRepresentation *GetSliderRepresentation()
    {return reinterpret_cast<vtkSliderRepresentation*>(this->WidgetRep);}

  // Description:
  // Control the behavior of the slider when selecting the tube or caps. If
  // Jump, then selecting the tube, left cap, or right cap causes the slider to
  // jump to the selection point. If the mode is Animate, the slider moves
  // towards the selection point in NumberOfAnimationSteps number of steps.
  // If Off, then the slider does not move.
  vtkSetClampMacro(AnimationMode, int, AnimateOff, Animate);
  vtkGetMacro(AnimationMode, int);
  void SetAnimationModeToOff() { this->SetAnimationMode(AnimateOff); }
  void SetAnimationModeToJump() { this->SetAnimationMode(Jump); }
  void SetAnimationModeToAnimate() { this->SetAnimationMode(Animate); }

  // Description:
  // Specify the number of animation steps to take if the animation mode
  // is set to animate.
  vtkSetClampMacro(NumberOfAnimationSteps,int,1,VTK_LARGE_INTEGER);
  vtkGetMacro(NumberOfAnimationSteps,int);

  // Description:
  // Create the default widget representation if one is not set.
  void CreateDefaultRepresentation();

protected:
  vtkSliderWidget();
  ~vtkSliderWidget() {}

  // These are the events that are handled
  static void SelectAction(vtkAbstractWidget*);
  static void EndSelectAction(vtkAbstractWidget*);
  static void MoveAction(vtkAbstractWidget*);
  void AnimateSlider(int selectionState);

//BTX - manage the state of the widget
  int WidgetState;
  enum _WidgetState
  {
    Start=0,
    Sliding,
    Animating
  };

  int NumberOfAnimationSteps;
  int AnimationMode;
  enum AnimationState {
    AnimateOff,
    Jump,
    Animate
  };

//ETX


private:
  vtkSliderWidget(const vtkSliderWidget&);  //Not implemented
  void operator=(const vtkSliderWidget&);  //Not implemented
};

#endif
