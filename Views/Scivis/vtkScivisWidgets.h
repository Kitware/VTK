// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkScivisWidgets
 * @brief   The widgets a view is interacted with through.
 *
 * vtkScivisWidgets is the part of a view that holds the things the user can
 * take hold of and move: a ruler laid across the data, a plane to clip it
 * with, a box to crop it to.  It is reached as vtkScivisView::GetWidgets().
 *
 * @code
 * vtkDistanceWidget* ruler = view->GetWidgets()->AddDistance();
 * vtkImplicitPlaneWidget2* plane = view->GetWidgets()->AddPlane();
 * @endcode
 *
 * @par What adding one does:
 * A widget needs two things from the view and gets both here: the interactor
 * it listens to and the renderer it draws in.  It is then enabled
 * and kept alive until it is removed or the view goes away.  Add() takes any
 * vtkAbstractWidget, so a widget this class knows nothing about is wired up the
 * same way.
 *
 * The ones that occupy a region rather than being clicked into place -- a
 * plane, a box, a line, a spline -- are also sized to the scene, so they arrive
 * around the data rather than around a unit cube.
 *
 * @par These are VTK's widgets, not representations:
 * What comes back is the widget itself, configured through the objects it
 * already hands out -- vtkAbstractWidget::GetRepresentation() and the typed
 * accessors beside it.  They are deliberately not vtkScivisRepresentations: a
 * widget already has a representation in VTK's sense, and every type here has
 * one named for it in VTK::InteractionWidgets.
 *
 * @par Hearing what the user did:
 * In C++, observe the widget: it fires vtkCommand::InteractionEvent while it is
 * being dragged and vtkCommand::EndInteractionEvent when the drag finishes.
 * Python is given the value rather than the event -- see the callback arguments
 * on the same methods there.
 *
 * @sa vtkScivisView vtkAbstractWidget
 */

#ifndef vtkScivisWidgets_h
#define vtkScivisWidgets_h

#include "vtkObject.h"
#include "vtkViewsScivisModule.h" // For export macro
#include "vtkWeakPointer.h"       // For ivar

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractWidget;
class vtkAngleWidget;
class vtkBoxWidget2;
class vtkCaptionWidget;
class vtkDistanceWidget;
class vtkImplicitPlaneWidget2;
class vtkLineWidget2;
class vtkScivisView;
class vtkSplineWidget2;

class VTKVIEWSSCIVIS_EXPORT vtkScivisWidgets : public vtkObject
{
public:
  static vtkScivisWidgets* New();
  vtkTypeMacro(vtkScivisWidgets, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Measure the scene.  Both are placed by the user, who clicks the ends of a
   * distance or the three points of an angle.
   *
   * The measurement is read from the widget's representation --
   * GetDistanceRepresentation()->GetDistance(),
   * GetAngleRepresentation()->GetAngle() -- while it is being dragged or once
   * it is finished.
   */
  vtkDistanceWidget* AddDistance();
  vtkAngleWidget* AddAngle();
  ///@}

  /**
   * A label with a leader pointing at @a anchor, in world coordinates.
   */
  vtkCaptionWidget* AddCaption(const char* text, double x, double y, double z);

  ///@{
  /**
   * Take hold of a region of the scene: a line between two points, a plane to
   * cut with, a box to crop to, or a spline through @a handles points.
   *
   * These are placed over the scene as it stands when they are added, so they
   * arrive around the data rather than around a unit cube.
   */
  vtkLineWidget2* AddLine();
  vtkImplicitPlaneWidget2* AddPlane();
  vtkBoxWidget2* AddBox();
  vtkSplineWidget2* AddSpline(int handles = 5);
  ///@}

  ///@{
  /**
   * Add a widget of any kind, or take one away.
   *
   * Add() wires a widget to the view's interactor and renderer, enables it and
   * keeps it alive.  Remove() disables it and lets it go.
   *
   * Placing is asked for rather than assumed: PlaceOverScene() sizes a widget
   * that occupies a region -- a plane, a box -- to the data, which is what the
   * methods above do for the kinds that want it.  It sets the representation's
   * place factor to 1 so the widget spans the scene bounds exactly.  A widget the user clicks into
   * place, or that points at somewhere it was given, has nothing to place.
   */
  void Add(vtkAbstractWidget* widget);
  void PlaceOverScene(vtkAbstractWidget* widget);
  void Remove(vtkAbstractWidget* widget);
  void RemoveAll();
  ///@}

  ///@{
  /**
   * The widgets being held, in the order they were added.
   */
  int GetNumberOfWidgets();
  vtkAbstractWidget* GetWidget(int index);
  ///@}

protected:
  vtkScivisWidgets();
  ~vtkScivisWidgets() override;

private:
  friend class vtkScivisView;

  // Held weakly, as the other components hold it: these are reached through the
  // view, but Python can outlive it.
  void SetView(vtkScivisView* view);

  vtkScivisWidgets(const vtkScivisWidgets&) = delete;
  void operator=(const vtkScivisWidgets&) = delete;

  vtkWeakPointer<vtkScivisView> View;

  class Internals;
  Internals* Implementation;
};

VTK_ABI_NAMESPACE_END
#endif
