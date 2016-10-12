/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContextArea.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkContextArea
 * @brief   Clipped, transformed area with axes for context items.
 *
 *
 * vtkContextArea provides an clipped drawing area surrounded by four axes.
 * The drawing area is transformed to map the 2D area described by
 * DrawAreaBounds into pixel coordinates. DrawAreaBounds is also used to
 * configure the axes. Item to be rendered in the draw area should be added
 * to the context item returned by GetDrawAreaItem().
 *
 * The size and shape of the draw area is configured by the following member
 * variables:
 * - Geometry: The rect (pixel coordinates) defining the location of the context
 *   area in the scene. This includes the draw area and axis ticks/labels.
 * - FillViewport: If true (default), Geometry is set to span the size returned
 *   by vtkContextDevice2D::GetViewportSize().
 * - DrawAreaResizeBehavior: Controls how the draw area should be shaped.
 *   Available options: Expand (default), FixedAspect, FixedRect, FixedMargins.
 * - FixedAspect: Aspect ratio to enforce for FixedAspect resize behavior.
 * - FixedRect: Rect used to enforce for FixedRect resize behavior.
 * - FixedMargins: Margins to enforce for FixedMargins resize behavior.
*/

#ifndef vtkContextArea_h
#define vtkContextArea_h

#include "vtkAbstractContextItem.h"

#include "vtkAxis.h" // For enums
#include "vtkChartsCoreModule.h" // For export macro
#include "vtkRect.h" // For vtkRect/vtkVector/vtkTuple
#include "vtkNew.h" // For vtkNew

class vtkContextClip;
class vtkContextTransform;
class vtkPlotGrid;

class VTKCHARTSCORE_EXPORT vtkContextArea: public vtkAbstractContextItem
{
public:
  typedef vtkTuple<int, 4> Margins;
  vtkTypeMacro(vtkContextArea, vtkAbstractContextItem)
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  static vtkContextArea *New();

  /**
   * Get the vtkAxis associated with the specified location.
   */
  vtkAxis* GetAxis(vtkAxis::Location location);

  /**
   * Returns the vtkAbstractContextItem that will draw in the clipped,
   * transformed space. This is the item to add children for.
   */
  vtkAbstractContextItem* GetDrawAreaItem();

  /**
   * Paint event for the item, called whenever the item needs to be drawn.
   */
  virtual bool Paint(vtkContext2D *painter);

  //@{
  /**
   * The rect defining the pixel location and size of the entire vtkContextArea,
   * including axis labels, title, etc. Note that this will be updated to the
   * window geometry if FillWindow is true.
   */
  vtkGetMacro(Geometry, vtkRecti)
  vtkSetMacro(Geometry, vtkRecti)
  //@}

  //@{
  /**
   * The data bounds of the clipped and transformed area inside of the axes.
   * This is used to configure the axes labels and setup the transform.
   */
  vtkGetMacro(DrawAreaBounds, vtkRectd)
  vtkSetMacro(DrawAreaBounds, vtkRectd)
  //@}

  enum DrawAreaResizeBehaviorType {
    DARB_Expand,
    DARB_FixedAspect,
    DARB_FixedRect,
    DARB_FixedMargins
  };

  //@{
  /**
   * Set the resize behavior for the draw area:
   * - @a Expand: The default behavior. The draw area will automatically resize
   * to take up as much of @a Geometry as possible. Margin sizes are
   * minimized based on the space required for axis labels/tick marks.
   * - FixedAspect: Same as Expand, but a fixed aspected ratio is enforced.
   * See SetFixedAspect.
   * - FixedRect: Draw area is always constrained to a fixed rectangle.
   * See SetFixedRect.
   * - FixMargins: The draw area expands to fill @a Geometry, but margins
   * (axis labels, etc) are fixed, rather than dynamically sized.
   * See SetFixedMargins.
   */
  vtkGetMacro(DrawAreaResizeBehavior, DrawAreaResizeBehaviorType)
  vtkSetMacro(DrawAreaResizeBehavior, DrawAreaResizeBehaviorType)
  //@}

  //@{
  /**
   * The fixed aspect ratio, if DrawAreaResizeBehavior is FixedAspect.
   * Defined as width/height. Default is 1.
   * Setting the aspect ratio will also set DrawAreaResizeBehavior to
   * FixedAspect.
   */
  vtkGetMacro(FixedAspect, float)
  virtual void SetFixedAspect(float aspect);
  //@}

  //@{
  /**
   * The fixed rect to use for the draw area, if DrawAreaResizeBehavior is
   * FixedRect. Units are in pixels, default is 300x300+0+0.
   * Setting the fixed rect will also set DrawAreaResizeBehavior to
   * FixedRect.
   */
  vtkGetMacro(FixedRect, vtkRecti)
  virtual void SetFixedRect(vtkRecti rect);
  virtual void SetFixedRect(int x, int y, int width, int height);
  //@}

  //@{
  /**
   * The left, right, bottom, and top margins for the draw area, if
   * DrawAreaResizeBehavior is FixedMargins. Units are in pixels, default is
   * { 0, 0, 0, 0 }.
   * Setting the fixed margins will also set DrawAreaResizeBehavior to
   * FixedMargins.
   */
  virtual const Margins& GetFixedMargins() { return this->FixedMargins; }
  virtual void GetFixedMarginsArray(int margins[4]);
  virtual const int* GetFixedMarginsArray();
  virtual void SetFixedMargins(Margins margins);
  virtual void SetFixedMargins(int margins[4]);
  virtual void SetFixedMargins(int left, int right, int bottom, int top);
  //@}

  //@{
  /**
   * If true, Geometry is set to (0, 0, vpSize[0], vpSize[1]) at the start
   * of each Paint call. vpSize is vtkContextDevice2D::GetViewportSize. Default
   * is true.
   */
  vtkGetMacro(FillViewport, bool)
  vtkSetMacro(FillViewport, bool)
  vtkBooleanMacro(FillViewport, bool)
  //@}

  //@{
  /**
   * Turn on/off grid visibility.
   */
  virtual void SetShowGrid(bool show);
  virtual bool GetShowGrid();
  virtual void ShowGridOn() { this->SetShowGrid(true); }
  virtual void ShowGridOff() { this->SetShowGrid(false); }
  //@}

protected:
  vtkContextArea();
  ~vtkContextArea();

  /**
   * Sync the Axes locations with Geometry, and update the DrawAreaGeometry
   * to account for Axes size (margins). Must be called while the painter
   * is active.
   */
  void LayoutAxes(vtkContext2D *painter);

  /**
   * Return the draw area's geometry.
   */
  vtkRecti ComputeDrawAreaGeometry(vtkContext2D *painter);

  //@{
  /**
   * Working implementations for ComputeDrawAreaGeometry.
   */
  vtkRecti ComputeExpandedDrawAreaGeometry(vtkContext2D *painter);
  vtkRecti ComputeFixedAspectDrawAreaGeometry(vtkContext2D *painter);
  vtkRecti ComputeFixedRectDrawAreaGeometry(vtkContext2D *painter);
  vtkRecti ComputeFixedMarginsDrawAreaGeometry(vtkContext2D *painter);
  //@}

  /**
   * Set the transform to map DrawAreaBounds to DrawAreaGeometry. Should be
   * called after LayoutAxes to ensure that DrawAreaGeometry is up to date.
   */
  void UpdateDrawArea();

  /**
   * vtkAxis objects that surround the draw area, indexed by vtkAxis::Location.
   */
  vtkTuple<vtkAxis*, 4> Axes;

  /**
   * The vtkPlotGrid that renders a grid atop the data in the draw area.
   */
  vtkNew<vtkPlotGrid> Grid;

  /**
   * The context item that clips rendered data.
   */
  vtkNew<vtkContextClip> Clip;

  /**
   * The context item that clips rendered data.
   */
  vtkNew<vtkContextTransform> Transform;

  /**
   * The rect defining the pixel location and size of the entire vtkContextArea,
   * including axis label, title, etc.
   */
  vtkRecti Geometry;

  /**
   * The data bounds of the clipped and transformed area inside of the axes.
   * This is used to configure the axes labels and setup the transform.
   */
  vtkRectd DrawAreaBounds;

  /**
   * The rect defining the pixel location and size of the clipped and
   * transformed area inside the axes. Relative to Geometry.
   */
  vtkRecti DrawAreaGeometry;

  /**
   * Controls how the draw area size is determined.
   */
  DrawAreaResizeBehaviorType DrawAreaResizeBehavior;

  /**
   * The fixed aspect ratio, if DrawAreaResizeBehavior is FixedAspect.
   * Defined as width/height. Default is 1.
   */
  float FixedAspect;

  /**
   * The fixed rect to use for the draw area, if DrawAreaResizeBehavior is
   * FixedRect. Units are in pixels, default is 300x300+0+0.
   */
  vtkRecti FixedRect;

  /**
   * The left, right, bottom, and top margins for the draw area, if
   * DrawAreaResizeBehavior is FixedMargins. Units are in pixels, default is
   * { 0, 0, 0, 0 }
   */
  Margins FixedMargins;

  /**
   * If true, Geometry is set to (0, 0, vpSize[0], vpSize[1]) at the start
   * of each Paint call. vpSize is vtkContextDevice2D::GetViewportSize. Default
   * is true.
   */
  bool FillViewport;

private:
  vtkContextArea(const vtkContextArea &) VTK_DELETE_FUNCTION;
  void operator=(const vtkContextArea &) VTK_DELETE_FUNCTION;

  // Smart pointers for axis lifetime management. See this->Axes.
  vtkNew<vtkAxis> TopAxis;
  vtkNew<vtkAxis> BottomAxis;
  vtkNew<vtkAxis> LeftAxis;
  vtkNew<vtkAxis> RightAxis;
};

#endif //vtkContextArea_h
