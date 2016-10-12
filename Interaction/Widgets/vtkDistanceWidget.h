/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDistanceWidget.h,v

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkDistanceWidget
 * @brief   measure the distance between two points
 *
 * The vtkDistanceWidget is used to measure the distance between two points.
 * The two end points can be positioned independently, and when they are
 * released, a special PlacePointEvent is invoked so that special operations
 * may be take to reposition the point (snap to grid, etc.) The widget has
 * two different modes of interaction: when initially defined (i.e., placing
 * the two points) and then a manipulate mode (adjusting the position of
 * the two points).
 *
 * To use this widget, specify an instance of vtkDistanceWidget and a
 * representation (a subclass of vtkDistanceRepresentation). The widget is
 * implemented using two instances of vtkHandleWidget which are used to
 * position the end points of the line. The representations for these two
 * handle widgets are provided by the vtkDistanceRepresentation.
 *
 * @par Event Bindings:
 * By default, the widget responds to the following VTK events (i.e., it
 * watches the vtkRenderWindowInteractor for these events):
 * <pre>
 *   LeftButtonPressEvent - add a point or select a handle
 *   MouseMoveEvent - position the second point or move a handle
 *   LeftButtonReleaseEvent - release the handle
 * </pre>
 *
 * @par Event Bindings:
 * Note that the event bindings described above can be changed using this
 * class's vtkWidgetEventTranslator. This class translates VTK events
 * into the vtkDistanceWidget's widget events:
 * <pre>
 *   vtkWidgetEvent::AddPoint -- add one point; depending on the state
 *                               it may the first or second point added. Or,
 *                               if near a handle, select the handle.
 *   vtkWidgetEvent::Move -- move the second point or handle depending on the state.
 *   vtkWidgetEvent::EndSelect -- the handle manipulation process has completed.
 * </pre>
 *
 * @par Event Bindings:
 * This widget invokes the following VTK events on itself (which observers
 * can listen for):
 * <pre>
 *   vtkCommand::StartInteractionEvent (beginning to interact)
 *   vtkCommand::EndInteractionEvent (completing interaction)
 *   vtkCommand::InteractionEvent (moving after selecting something)
 *   vtkCommand::PlacePointEvent (after point is positioned;
 *                                call data includes handle id (0,1))
 * </pre>
 *
 * @sa
 * vtkHandleWidget
*/

#ifndef vtkDistanceWidget_h
#define vtkDistanceWidget_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkAbstractWidget.h"

class vtkDistanceRepresentation;
class vtkHandleWidget;
class vtkDistanceWidgetCallback;


class VTKINTERACTIONWIDGETS_EXPORT vtkDistanceWidget : public vtkAbstractWidget
{
public:
  /**
   * Instantiate this class.
   */
  static vtkDistanceWidget *New();

  //@{
  /**
   * Standard methods for a VTK class.
   */
  vtkTypeMacro(vtkDistanceWidget,vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
  //@}

  /**
   * The method for activating and deactivating this widget. This method
   * must be overridden because it is a composite widget and does more than
   * its superclasses' vtkAbstractWidget::SetEnabled() method.
   */
  virtual void SetEnabled(int);

  /**
   * Specify an instance of vtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of vtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(vtkDistanceRepresentation *r)
    {this->Superclass::SetWidgetRepresentation(
      reinterpret_cast<vtkWidgetRepresentation*>(r));}

  /**
   * Return the representation as a vtkDistanceRepresentation.
   */
  vtkDistanceRepresentation *GetDistanceRepresentation()
    {return reinterpret_cast<vtkDistanceRepresentation*>(this->WidgetRep);}

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation();

  /**
   * Methods to change the whether the widget responds to interaction.
   * Overridden to pass the state to component widgets.
   */
  virtual void SetProcessEvents(int);

  /**
   * Description:
   * Enum defining the state of the widget. By default the widget is in Start mode,
   * and expects to be interactively placed. While placing the points the widget
   * transitions to Define state. Once placed, the widget enters the Manipulate state.
   */

  enum {Start=0,Define,Manipulate};

  //@{
  /**
   * Set the state of the widget. If the state is set to "Manipulate" then it
   * is assumed that the widget and its representation will be initialized
   * programmatically and is not interactively placed. Initially the widget
   * state is set to "Start" which means nothing will appear and the user
   * must interactively place the widget with repeated mouse selections. Set
   * the state to "Start" if you want interactive placement. Generally state
   * changes must be followed by a Render() for things to visually take
   * effect.
   */
  virtual void SetWidgetStateToStart();
  virtual void SetWidgetStateToManipulate();
  //@}

  /**
   * Return the current widget state.
   */
  virtual int GetWidgetState()
    {return this->WidgetState;}

protected:
  vtkDistanceWidget();
  ~vtkDistanceWidget();

  // The state of the widget
  int WidgetState;
  int CurrentHandle;

  // Callback interface to capture events when
  // placing the widget.
  static void AddPointAction(vtkAbstractWidget*);
  static void MoveAction(vtkAbstractWidget*);
  static void EndSelectAction(vtkAbstractWidget*);

  // The positioning handle widgets
  vtkHandleWidget *Point1Widget;
  vtkHandleWidget *Point2Widget;
  vtkDistanceWidgetCallback *DistanceWidgetCallback1;
  vtkDistanceWidgetCallback *DistanceWidgetCallback2;

  // Methods invoked when the handles at the
  // end points of the widget are manipulated
  void StartDistanceInteraction(int handleNum);
  void DistanceInteraction(int handleNum);
  void EndDistanceInteraction(int handleNum);

  friend class vtkDistanceWidgetCallback;

private:
  vtkDistanceWidget(const vtkDistanceWidget&) VTK_DELETE_FUNCTION;
  void operator=(const vtkDistanceWidget&) VTK_DELETE_FUNCTION;
};

#endif
