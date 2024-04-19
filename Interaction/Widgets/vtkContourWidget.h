// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkContourWidget
 * @brief   create a contour with a set of points
 *
 * The vtkContourWidget is used to select a set of points, and draw lines
 * between these points. The contour may be opened or closed, depending on
 * how the last point is added. The widget handles all processing of widget
 * events (that are triggered by VTK events). The vtkContourRepresentation is
 * responsible for all placement of the points, calculation of the lines, and
 * contour manipulation. This is done through two main helper classes:
 * vtkPointPlacer and vtkContourLineInterpolator. The representation is also
 * responsible for drawing the points and lines.
 *
 * @par Event Bindings:
 * By default, the widget responds to the following VTK events (i.e., it
 * watches the vtkRenderWindowInteractor for these events):
 * <pre>
 *   LeftButtonPressEvent - triggers a Select event
 *   RightButtonPressEvent - triggers a AddFinalPoint event
 *   MouseMoveEvent - triggers a Move event
 *   LeftButtonReleaseEvent - triggers an EndSelect event
 *   Delete key event - triggers a Delete event
 *   Shift + Delete key event - triggers a Reset event
 * </pre>
 *
 * @par Event Bindings:
 * Note that the event bindings described above can be changed using this
 * class's vtkWidgetEventTranslator. This class translates VTK events
 * into the vtkContourWidget's widget events:
 * <pre>
 *   vtkWidgetEvent::Select
 *        widget state is:
 *            Start or
 *            Define: If we already have at least 2 nodes, test
 *                 whether the current (X,Y) location is near an existing
 *                 node. If so, close the contour and change to Manipulate
 *                 state. Otherwise, attempt to add a node at this (X,Y)
 *                 location.
 *            Manipulate: If this (X,Y) location activates a node, then
 *                 set the current operation to Translate. Otherwise, if
 *                 this location is near the contour, attempt to add a
 *                 new node on the contour at this (X,Y) location.
 *
 * @par Event Bindings:
 *   vtkWidgetEvent::AddFinalPoint
 *        widget state is:
 *            Start: Do nothing.
 *            Define: If we already have at least 2 nodes, test
 *                 whether the current (X,Y) location is near an existing
 *                 node. If so, close the contour and change to Manipulate
 *                 state. Otherwise, attempt to add a node at this (X,Y)
 *                 location. If we do, then leave the contour open and
 *                 change to Manipulate state.
 *            Manipulate: Do nothing.
 *
 * @par Event Bindings:
 *   vtkWidgetEvent::Move
 *        widget state is:
 *            Start or
 *            Define: Do nothing.
 *            Manipulate: If our operation is Translate, then invoke
 *                  WidgetInteraction() on the representation. If our
 *                  operation is Inactive, then just attempt to activate
 *                  a node at this (X,Y) location.
 *
 * @par Event Bindings:
 *   vtkWidgetEvent::EndSelect
 *        widget state is:
 *            Start or
 *            Define: Do nothing.
 *            Manipulate: If our operation is not Inactive, set it to
 *                  Inactive.
 *
 * @par Event Bindings:
 *   vtkWidgetEvent::Delete
 *        widget state is:
 *            Start: Do nothing.
 *            Define: Remove the last point on the contour.
 *            Manipulate: Attempt to activate a node at (X,Y). If
 *                   we do activate a node, delete it. If we now
 *                   have less than 3 nodes, go back to Define state.
 *
 * @par Event Bindings:
 *   vtkWidgetEvent::Reset
 *        widget state is:
 *            Start: Do nothing.
 *            Define: Remove all points and line segments of the contour.
 *                 Essentially calls Initialize(nullptr)
 *            Manipulate: Do nothing.
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
 *   vtkCommand::WidgetValueChangedEvent (Invoked when the contour is closed
 *                                        for the first time. )
 * </pre>
 *
 * @sa
 * vtkHandleWidget
 */

#ifndef vtkContourWidget_h
#define vtkContourWidget_h

#include "vtkAbstractWidget.h"
#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWrappingHints.h"            // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkContourRepresentation;
class vtkPolyData;
class vtkIdList;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkContourWidget : public vtkAbstractWidget
{
public:
  /**
   * Instantiate this class.
   */
  static vtkContourWidget* New();

  ///@{
  /**
   * Standard methods for a VTK class.
   */
  vtkTypeMacro(vtkContourWidget, vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * The method for activating and deactivating this widget. This method
   * must be overridden because it is a composite widget and does more than
   * its superclasses' vtkAbstractWidget::SetEnabled() method.
   */
  void SetEnabled(int) override;

  /**
   * Specify an instance of vtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of vtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(vtkContourRepresentation* r)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(r));
  }

  /**
   * Return the representation as a vtkContourRepresentation.
   */
  vtkContourRepresentation* GetContourRepresentation()
  {
    return reinterpret_cast<vtkContourRepresentation*>(this->WidgetRep);
  }

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

  /**
   * Convenient method to close the contour loop.
   */
  void CloseLoop();

  ///@{
  /**
   * Convenient method to change what state the widget is in.
   */
  vtkSetMacro(WidgetState, int);
  ///@}

  ///@{
  /**
   * Convenient method to determine the state of the method
   */
  vtkGetMacro(WidgetState, int);
  ///@}

  ///@{
  /**
   * Set / Get the AllowNodePicking value. This ivar indicates whether the nodes
   * and points between nodes can be picked/un-picked by Ctrl+Click on the node.
   */
  void SetAllowNodePicking(vtkTypeBool);
  vtkGetMacro(AllowNodePicking, vtkTypeBool);
  vtkBooleanMacro(AllowNodePicking, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Follow the cursor ? If this is ON, during definition, the last node of the
   * contour will automatically follow the cursor, without waiting for the
   * point to be dropped. This may be useful for some interpolators, such as the
   * live-wire interpolator to see the shape of the contour that will be placed
   * as you move the mouse cursor.
   */
  vtkSetMacro(FollowCursor, vtkTypeBool);
  vtkGetMacro(FollowCursor, vtkTypeBool);
  vtkBooleanMacro(FollowCursor, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Define a contour by continuously drawing with the mouse cursor.
   * Press and hold the left mouse button down to continuously draw.
   * Releasing the left mouse button switches into a snap drawing mode.
   * Terminate the contour by pressing the right mouse button.  If you
   * do not want to see the nodes as they are added to the contour, set the
   * opacity to 0 of the representation's property.  If you do not want to
   * see the last active node as it is being added, set the opacity to 0
   * of the representation's active property.
   */
  vtkSetMacro(ContinuousDraw, vtkTypeBool);
  vtkGetMacro(ContinuousDraw, vtkTypeBool);
  vtkBooleanMacro(ContinuousDraw, vtkTypeBool);
  ///@}

  /**
   * Initialize the contour widget from a user supplied set of points. The
   * state of the widget decides if you are still defining the widget, or
   * if you've finished defining (added the last point) are manipulating
   * it. Note that if the polydata supplied is closed, the state will be
   * set to manipulate.
   * State: Define = 0, Manipulate = 1.
   */
  virtual void Initialize(vtkPolyData* poly, int state = 1, vtkIdList* idList = nullptr);
  virtual void Initialize() { this->Initialize(nullptr); }

  // The state of the widget

  enum
  {
    Start,
    Define,
    Manipulate
  };

protected:
  vtkContourWidget();
  ~vtkContourWidget() override;

  int WidgetState;
  int CurrentHandle;
  vtkTypeBool AllowNodePicking;
  vtkTypeBool FollowCursor;
  vtkTypeBool ContinuousDraw;
  int ContinuousActive;

  // Callback interface to capture events when
  // placing the widget.
  static void SelectAction(vtkAbstractWidget*);
  static void AddFinalPointAction(vtkAbstractWidget*);
  static void MoveAction(vtkAbstractWidget*);
  static void EndSelectAction(vtkAbstractWidget*);
  static void DeleteAction(vtkAbstractWidget*);
  static void TranslateContourAction(vtkAbstractWidget*);
  static void ScaleContourAction(vtkAbstractWidget*);
  static void ResetAction(vtkAbstractWidget*);

  // Internal helper methods
  void SelectNode();
  void AddNode();

private:
  vtkContourWidget(const vtkContourWidget&) = delete;
  void operator=(const vtkContourWidget&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
