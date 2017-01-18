/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkControlPointsItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkControlPointsItem
 * @brief   Abstract class for control points items.
 *
 * vtkControlPointsItem provides control point painting and management for
 * subclasses that provide points (typically control points of a transfer
 * function)
 * @sa
 * vtkScalarsToColorsItem
 * vtkPiecewiseControlPointsItem
*/

#ifndef vtkControlPointsItem_h
#define vtkControlPointsItem_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkCommand.h" // For vtkCommand enum
#include "vtkPlot.h"
#include "vtkVector.h" // For vtkVector2f

class vtkCallbackCommand;
class vtkContext2D;
class vtkPoints2D;
class vtkTransform2D;

class VTKCHARTSCORE_EXPORT vtkControlPointsItem: public vtkPlot
{
public:
  vtkTypeMacro(vtkControlPointsItem, vtkPlot);
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  // Events fires by this class (and subclasses).
  // \li CurrentPointChangedEvent is fired when the current point index is changed.
  // \li CurrentPointEditEvent is fired to request the application to show UI to
  // edit the current point.
  // \li vtkCommand::StartEvent and vtkCommand::EndEvent is fired
  // to mark groups of changes to control points.
  enum {
    CurrentPointChangedEvent = vtkCommand::UserEvent,
    CurrentPointEditEvent
  };

  /**
   * Bounds of the item, typically the bound of all the control points
   * except if custom bounds have been set \sa SetUserBounds.
   */
  void GetBounds(double bounds[4]) VTK_OVERRIDE;

  //@{
  /**
   * Set custom bounds, except if bounds are invalid, bounds will be
   * automatically computed based on the range of the control points
   * Invalid bounds by default.
   */
  vtkSetVector4Macro(UserBounds, double);
  vtkGetVector4Macro(UserBounds, double);
  //@}

  //@{
  /**
   * Controls the valid range for the values.
   * An invalid value (0, -1, 0., -1, 0, -1.) indicates that the valid
   * range is the current bounds. It is the default behavior.
   */
  vtkSetVector4Macro(ValidBounds, double);
  vtkGetVector4Macro(ValidBounds, double);
  //@}

  //@{
  /**
   * Get/set the radius for screen points.
   * Default is 6.f
   */
  vtkGetMacro(ScreenPointRadius, float);
  vtkSetMacro(ScreenPointRadius, float);
  //@}

  /**
   * Paint the points with a fixed size (cosmetic) which doesn't depend
   * on the scene zoom factor. Selected and unselected points are drawn
   * with a different color.
   */
  bool Paint(vtkContext2D *painter) VTK_OVERRIDE;

  /**
   * Select a point by its ID
   */
  void SelectPoint(vtkIdType pointId);

  /**
   * Utility function that selects a point providing its coordinates.
   * To be found, the position of the point must be no further away than its
   * painted point size
   */
  void SelectPoint(double* currentPoint);

  /**
   * Select all the points
   */
  void SelectAllPoints();

  /**
   * Unselect a point by its ID
   */
  void DeselectPoint(vtkIdType pointId);

  /**
   * Utility function that unselects a point providing its coordinates.
   * To be found, the position of the point must be no further away than its
   * painted point size
   */
  void DeselectPoint(double* currentPoint);

  /**
   * Unselect all the previously selected points
   */
  void DeselectAllPoints();

  /**
   * Toggle the selection of a point by its ID. If the point was selected then
   * unselect it, otherwise select it.
   */
  void ToggleSelectPoint(vtkIdType pointId);

  /**
   * Utility function that toggles the selection a point providing its
   * coordinates. To be found, the position of the point must be no further
   * away than its painted point size
   */
  void ToggleSelectPoint(double* currentPoint);

  /**
   * Select all points in the specified rectangle.
   */
  bool SelectPoints(const vtkVector2f& min, const vtkVector2f& max) VTK_OVERRIDE;

  /**
   * Return the number of selected points.
   */
  vtkIdType GetNumberOfSelectedPoints()const;

  /**
   * Returns the vtkIdType of the point given its coordinates and a tolerance
   * based on the screen point size.
   */
  vtkIdType FindPoint(double* pos);

  /**
   * Returns true if pos is above the pointId point, false otherwise.
   * It uses the size of the drawn point. To search what point is under the pos,
   * use the more efficient \sa FindPoint() instead.
   */
  bool IsOverPoint(double* pos, vtkIdType pointId);

  /**
   * Returns the id of the control point exactly matching pos, -1 if not found.
   */
  vtkIdType GetControlPointId(double* pos);

  /**
   * Utility function that returns an array of all the control points IDs
   * Typically: [0, 1, 2, ... n -1] where n is the point count
   * Can exclude the first and last point ids from the array.
   */
  void GetControlPointsIds(vtkIdTypeArray* ids,
                           bool excludeFirstAndLast = false)const;

  //@{
  /**
   * Controls whether or not control points are drawn (true) or clicked and
   * moved (false).
   * False by default.
   */
  vtkGetMacro(StrokeMode, bool);
  //@}

  //@{
  /**
   * If DrawPoints is false, SwitchPoints controls the behavior when a control
   * point is dragged past another point. The crossed point becomes current
   * (true) or the current point is blocked/stopped (false).
   * False by default.
   */
  vtkSetMacro(SwitchPointsMode, bool);
  vtkGetMacro(SwitchPointsMode, bool);
  //@}

  //@{
  /**
   * If EndPointsMovable is false, the two end points will not
   * be moved. True by default.
   */
  vtkSetMacro(EndPointsXMovable, bool);
  vtkGetMacro(EndPointsXMovable, bool);
  vtkSetMacro(EndPointsYMovable, bool);
  vtkGetMacro(EndPointsYMovable, bool);
  virtual bool GetEndPointsMovable();
  //@}

  //@{
  /**
   * If EndPointsRemovable is false, the two end points will not
   * be be removed. True by default.
   */
  vtkSetMacro(EndPointsRemovable, bool);
  vtkGetMacro(EndPointsRemovable, bool);
  //@}

  //@{
  /**
   * When set to true, labels are shown on the current control point and the end
   * points. Default is false.
   */
  vtkSetMacro(ShowLabels, bool);
  vtkGetMacro(ShowLabels, bool);
  //@}

  //@{
  /**
   * Get/Set the label format. Default is "%.4f, %.4f".
   */
  vtkSetStringMacro(LabelFormat);
  vtkGetStringMacro(LabelFormat);
  //@}

  /**
   * Add a point to the function. Returns the index of the point (0 based),
   * or -1 on error.
   * Subclasses should reimplement this function to do the actual work.
   */
  virtual vtkIdType AddPoint(double* newPos) = 0;

  /**
   * Remove a point of the function. Returns the index of the point (0 based),
   * or -1 on error.
   * Subclasses should reimplement this function to do the actual work.
   */
  virtual vtkIdType RemovePoint(double* pos) = 0;

  /**
   * Remove a point give its id. It is a utility function that internally call
   * the virtual method RemovePoint(double*) and return its result.
   */
  vtkIdType RemovePoint(vtkIdType pointId);

  /**
   * Remove the current point.
   */
  inline void RemoveCurrentPoint();

  /**
   * Returns the total number of points
   */
  virtual vtkIdType GetNumberOfPoints()const = 0;

  /**
   * Returns the x and y coordinates as well as the midpoint and sharpness
   * of the control point corresponding to the index.
   * point must be a double array of size 4.
   */
  virtual void GetControlPoint(vtkIdType index, double *point)const = 0;

  /**
   * Sets the x and y coordinates as well as the midpoint and sharpness
   * of the control point corresponding to the index.
   */
  virtual void SetControlPoint(vtkIdType index, double *point) = 0;

  /**
   * Move the points referred by pointIds by a given translation.
   * The new positions won't be outside the bounds.
   * MovePoints is typically called with GetControlPointsIds() or GetSelection().
   * Warning: if you pass this->GetSelection(), the array is deleted after
   * each individual point move. Increase the reference count of the array.
   * See also MoveAllPoints()
   */
  void MovePoints(const vtkVector2f& translation, vtkIdTypeArray* pointIds);

  /**
   * Utility function to move all the control points of the given translation
   * If dontMoveFirstAndLast is true, then the first and last points won't be
   * moved.
   */
  void MovePoints(const vtkVector2f& translation, bool dontMoveFirstAndLast = false);

  /**
   * Spread the points referred by pointIds
   * If factor > 0, points are moved away from each other.
   * If factor < 0, points are moved closer to each other
   * SpreadPoints is typically called with GetControlPointsIds() or GetSelection().
   * Warning: if you pass this->GetSelection(), the array is deleted after
   * each individual point move. Increase the reference count of the array.
   */
  void SpreadPoints(float factor, vtkIdTypeArray* pointIds);

  /**
   * Utility function to spread all the control points of a given factor
   * If dontSpreadFirstAndLast is true, then the first and last points won't be
   * spread.
   */
  void SpreadPoints(float factor, bool dontSpreadFirstAndLast = false);

  /**
   * Returns the current point ID selected or -1 if there is no point current.
   * No current point by default.
   */
  vtkIdType GetCurrentPoint()const;

  /**
   * Sets the current point selected.
   */
  void SetCurrentPoint(vtkIdType index);

  //@{
  /**
   * Gets the selected point pen and brush.
   */
  vtkGetObjectMacro(SelectedPointPen, vtkPen);
  //@}

  //@{
  /**
   * Depending on the control points item, the brush might not be taken into
   * account.
   */
  vtkGetObjectMacro(SelectedPointBrush, vtkBrush);
  //@}

  /**
   * Recompute the bounds next time they are requested.
   * You shouldn't have to call it but it is provided for rare cases.
   */
  void ResetBounds();

  //@{
  /**
   * Mouse button down event.
   */
  bool MouseButtonPressEvent(const vtkContextMouseEvent &mouse) VTK_OVERRIDE;
  bool MouseDoubleClickEvent(const vtkContextMouseEvent &mouse) VTK_OVERRIDE;
  //@}

  /**
   * Mouse move event.
   */
  bool MouseMoveEvent(const vtkContextMouseEvent &mouse) VTK_OVERRIDE;

  bool KeyPressEvent(const vtkContextKeyEvent &key) VTK_OVERRIDE;
  bool KeyReleaseEvent(const vtkContextKeyEvent &key) VTK_OVERRIDE;

protected:
  vtkControlPointsItem();
  ~vtkControlPointsItem() VTK_OVERRIDE;

  void StartChanges();
  void EndChanges();
  void StartInteraction();
  void StartInteractionIfNotStarted();
  void Interaction();
  void EndInteraction();
  int GetInteractionsCount()const;
  virtual void emitEvent(unsigned long event, void* params = 0) = 0;

  static void CallComputePoints(vtkObject* sender, unsigned long event, void* receiver, void* params);

  //@{
  /**
   * Must be reimplemented by subclasses to calculate the points to draw.
   * It's subclass responsibility to call ComputePoints() via the callback
   */
  virtual void ComputePoints();
  virtual vtkMTimeType GetControlPointsMTime() =0;
  //@}

  /**
   * Returns true if the supplied x, y coordinate is on a control point.
   */
  bool Hit(const vtkContextMouseEvent &mouse) VTK_OVERRIDE;

  //@{
  /**
   * Transform the mouse event in the control-points space. This is needed when
   * ColorTransferFunction is using log-scale.
   */
  virtual void TransformScreenToData(const vtkVector2f& in, vtkVector2f& out);
  virtual void TransformDataToScreen(const vtkVector2f& in, vtkVector2f& out);
  //@}

  //@{
  /**
   * Clamp the given 2D pos into the bounds of the function.
   * Return true if the pos has been clamped, false otherwise.
   */
  virtual bool ClampPos(double pos[2], double bounds[4]);
  bool ClampValidPos(double pos[2]);
  //@}

  //@{
  /**
   * Internal function that paints a collection of points and optionally
   * excludes some.
   */
  void DrawUnselectedPoints(vtkContext2D* painter);
  void DrawSelectedPoints(vtkContext2D* painter);
  virtual void DrawPoint(vtkContext2D* painter, vtkIdType index);
  //@}

  void SetCurrentPointPos(const vtkVector2f& newPos);
  vtkIdType SetPointPos(vtkIdType point, const vtkVector2f& newPos);
  void MoveCurrentPoint(const vtkVector2f& translation);
  vtkIdType MovePoint(vtkIdType point, const vtkVector2f& translation);

  inline vtkVector2f GetSelectionCenterOfMass()const;
  vtkVector2f GetCenterOfMass(vtkIdTypeArray* pointIDs)const;

  void Stroke(const vtkVector2f& newPos);
  virtual void EditPoint(float vtkNotUsed(tX), float vtkNotUsed(tY));
  /**
   * Mouse button release event.
   */
  bool MouseButtonReleaseEvent(const vtkContextMouseEvent &mouse) VTK_OVERRIDE;

  /**
   * Generate label for a control point.
   */
  virtual vtkStdString GetControlPointLabel(vtkIdType index);

  void AddPointId(vtkIdType addedPointId);

  /**
   * Return true if any of the end points is current point
   * or part of the selection
   */
  bool IsEndPointPicked();

  /**
   * Return true if the point is removable
   */
  bool IsPointRemovable(vtkIdType pointId);

  /**
   * Compute the bounds for this item. Typically, the bounds should be aligned
   * to the range of the vtkScalarsToColors or vtkPiecewiseFunction that is
   * being controlled by the subclasses.
   * Default implementation uses the range of the control points themselves.
   */
  virtual void ComputeBounds(double* bounds);

  /**
   * Returns true if control points are to be rendered in log-space. This is
   * true when vtkScalarsToColors is using log-scale, for example. Default
   * implementation always return false.
   */
  virtual bool UsingLogScale() { return false; }

  vtkCallbackCommand* Callback;
  vtkPen*             SelectedPointPen;
  vtkBrush*           SelectedPointBrush;
  int                 BlockUpdates;
  int                 StartedInteractions;
  int                 StartedChanges;
  vtkIdType           CurrentPoint;

  double              Bounds[4];
  double              UserBounds[4];
  double              ValidBounds[4];

  vtkTransform2D*     Transform;
  float               ScreenPointRadius;

  bool                StrokeMode;
  bool                SwitchPointsMode;
  bool                MouseMoved;
  bool                EnforceValidFunction;
  vtkIdType           PointToDelete;
  bool                PointAboutToBeDeleted;
  vtkIdType           PointToToggle;
  bool                PointAboutToBeToggled;
  bool                InvertShadow;
  bool                EndPointsXMovable;
  bool                EndPointsYMovable;
  bool                EndPointsRemovable;
  bool                ShowLabels;
  char*               LabelFormat;
private:
  vtkControlPointsItem(const vtkControlPointsItem &) VTK_DELETE_FUNCTION;
  void operator=(const vtkControlPointsItem &) VTK_DELETE_FUNCTION;

  void      ComputeBounds();

  vtkIdType RemovePointId(vtkIdType removedPointId);
};

//-----------------------------------------------------------------------------
void vtkControlPointsItem::RemoveCurrentPoint()
{
  this->RemovePoint(this->GetCurrentPoint());
}

//-----------------------------------------------------------------------------
vtkVector2f vtkControlPointsItem::GetSelectionCenterOfMass()const
{
  return this->GetCenterOfMass(this->Selection);
}

#endif
