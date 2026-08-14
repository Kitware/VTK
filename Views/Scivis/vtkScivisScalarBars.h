// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkScivisScalarBars
 * @brief   The scalar bars a view keeps for what it is drawing.
 *
 * vtkScivisScalarBars is the part of a view that labels the scene with what its
 * colors mean, reached as vtkScivisView::GetScalarBars().  Nothing has to be
 * switched on: color a representation by an array and a bar labelled with it
 * appears.
 *
 * A scalar bar describes the scene rather than any one representation, which is
 * why they are kept here rather than by the things being drawn.  Two
 * representations coloring by "Temperature" are one bar spanning both of their
 * ranges, not two bars disagreeing about the same quantity.  A bar goes away
 * when the last representation drawing that array is removed or hidden, and a
 * live bar's range only ever grows, so bars do not jump about as data comes and
 * goes.
 *
 * @par The set of bars is not yours to change:
 * There is no way to add or remove one, because which bars exist follows from
 * what the representations are drawing.  GetActor(), by index or by array name
 * and field association, reaches a bar to adjust what it looks like -- its
 * title, its label format, its size.  AutoVisibilityOff() removes the bars and
 * stops them being maintained, leaving the representations and their color maps
 * alone.
 *
 * @par Where the bars sit:
 * They are stacked down the right hand edge, sharing the height between them.
 * With Draggable on, each bar can be picked up and moved by the user, and one
 * that has been moved stays where it was put rather than being stacked again.
 *
 * @par Colors are the view's:
 * The map an array is drawn through comes from the view's
 * vtkLookupTableManager, which is shared between views that should agree on
 * color.  This component decides what is labelled, not what color it is.
 *
 * @sa vtkScivisView vtkLookupTableManager vtkScalarBarActor
 */

#ifndef vtkScivisScalarBars_h
#define vtkScivisScalarBars_h

#include "vtkObject.h"
#include "vtkViewsScivisModule.h" // For export macro
#include "vtkWeakPointer.h"       // For ivar

VTK_ABI_NAMESPACE_BEGIN
class vtkScalarBarActor;
class vtkScalarBarWidget;
class vtkScivisView;

class VTKVIEWSSCIVIS_EXPORT vtkScivisScalarBars : public vtkObject
{
public:
  static vtkScivisScalarBars* New();
  vtkTypeMacro(vtkScivisScalarBars, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Whether a bar is kept for each array being drawn.  Default is on.
   *
   * Turning this off removes the bars and stops them being maintained.  The
   * representations and the color maps they draw through are left alone.
   */
  void SetAutoVisibility(bool val);
  bool GetAutoVisibility();
  vtkBooleanMacro(AutoVisibility, bool);
  ///@}

  ///@{
  /**
   * Whether the user can pick a bar up and move it.  Default is off.
   *
   * A bar that has been moved keeps where it was put; the rest go on being
   * stacked down the right hand edge.  Turning this off takes the widgets away
   * but changes nothing about where the bars are, so one the user had moved
   * stays moved, and stays out of the stack.
   */
  void SetDraggable(bool val);
  bool GetDraggable();
  vtkBooleanMacro(Draggable, bool);
  ///@}

  /**
   * How many bars there are, which is how many arrays the visible
   * representations are drawing between them.
   */
  int GetNumberOfBars();

  ///@{
  /**
   * What the bar at @a index is labelled with, in array-name order.  The
   * association is one of the vtkDataObject::FIELD_ASSOCIATION_* values.
   */
  const char* GetArrayName(int index);
  int GetFieldAssociation(int index);
  ///@}

  ///@{
  /**
   * The bar at @a index, or the one labelled with @a arrayName drawn from
   * @a fieldAssoc.  Null when there is no such bar.
   */
  vtkScalarBarActor* GetActor(int index);
  vtkScalarBarActor* GetActor(const char* arrayName, int fieldAssoc);
  ///@}

  /**
   * The widget that makes the bar at @a index draggable, for its interaction
   * and its representation.  Null while Draggable is off, since there is no
   * widget until there is something to drag.
   */
  vtkScalarBarWidget* GetWidget(int index);

protected:
  vtkScivisScalarBars();
  ~vtkScivisScalarBars() override;

private:
  friend class vtkScivisView;

  // Held weakly, as the other components hold it: these are reached through the
  // view, but Python can outlive it.
  void SetView(vtkScivisView* view);

  /**
   * Bring the bars in line with what the representations are drawing: one per
   * array, spanning every visible representation of it, retiring the ones
   * nothing feeds any more.  Called by the view before it draws.
   */
  void Update();

  /**
   * Stack the bars that the user has not moved down the right hand edge.
   */
  void Position();

  /**
   * Give each bar a widget to be dragged by while Draggable is on.
   */
  void UpdateWidgets();

  /**
   * Note that the user has moved a bar, so that stacking leaves it alone.
   */
  void OnDragged(vtkObject* caller, unsigned long event, void* data);

  vtkScivisScalarBars(const vtkScivisScalarBars&) = delete;
  void operator=(const vtkScivisScalarBars&) = delete;

  vtkWeakPointer<vtkScivisView> View;
  bool AutoVisibility;
  bool Draggable;

  class Internals;
  Internals* Implementation;
};

VTK_ABI_NAMESPACE_END
#endif
