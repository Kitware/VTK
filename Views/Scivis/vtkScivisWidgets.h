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
 * Adding a widget connects it to the view's interactor and renderer, enables
 * it, and keeps it alive until it is removed or the view goes away.  The
 * adders for the ones that occupy a region -- a plane, a box, a line, a spline
 * -- also place them over the scene.  What comes back is the VTK widget itself, configured through
 * its own representation.
 *
 * To hear what the user did, observe the widget: it fires
 * vtkCommand::InteractionEvent while being dragged and
 * vtkCommand::EndInteractionEvent when the drag finishes.  In Python, the
 * adders take callbacks that are handed the widget's value instead.
 *
 * @sa vtkScivisView vtkAbstractWidget
 */

#ifndef vtkScivisWidgets_h
#define vtkScivisWidgets_h

#include "vtkObject.h"
#include "vtkViewsScivisModule.h" // For export macro
#include "vtkWeakPointer.h"       // For ivar
#include "vtkWrappingHints.h"     // For VTK_MARSHALAUTO

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

class VTKVIEWSSCIVIS_EXPORT VTK_MARSHALAUTO vtkScivisWidgets : public vtkObject
{
public:
  static vtkScivisWidgets* New();
  vtkTypeMacro(vtkScivisWidgets, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Measure the scene.  The user clicks the ends of a distance or the three
   * points of an angle; read the result with
   * GetDistanceRepresentation()->GetDistance() or
   * GetAngleRepresentation()->GetAngle().
   */
  vtkDistanceWidget* AddDistance();
  vtkAngleWidget* AddAngle();
  ///@}

  /**
   * A label with a leader pointing at (@a x, @a y, @a z), in world coordinates.
   */
  vtkCaptionWidget* AddCaption(const char* text, double x, double y, double z);

  ///@{
  /**
   * A line between two points, a plane to cut with, a box to crop to, or a
   * spline through @a handles points.  Each is placed over the scene as it
   * stands when it is added.
   */
  vtkLineWidget2* AddLine();
  vtkImplicitPlaneWidget2* AddPlane();
  vtkBoxWidget2* AddBox();
  vtkSplineWidget2* AddSpline(int handles = 5);
  ///@}

  ///@{
  /**
   * Add a widget of any kind, or take one away.  Add() connects it to the view
   * and enables it; Remove() disables it and lets it go.  PlaceOverScene()
   * sizes a widget to the scene bounds, which Add() does not do on its own.
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
