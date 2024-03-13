// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkBorderRepresentation
 * @brief   represent a vtkBorderWidget
 *
 * This class is used to represent and render a vtBorderWidget. To
 * use this class, you need to specify the two corners of a rectangular
 * region.
 *
 * The class is typically subclassed so that specialized representations can
 * be created.  The class defines an API and a default implementation that
 * the vtkBorderRepresentation interacts with to render itself in the scene.
 *
 * @warning
 * The separation of the widget event handling (e.g., vtkBorderWidget) from
 * the representation (vtkBorderRepresentation) enables users and developers
 * to create new appearances for the widget. It also facilitates parallel
 * processing, where the client application handles events, and remote
 * representations of the widget are slaves to the client (and do not handle
 * events).
 *
 * @sa
 * vtkBorderWidget vtkTextWidget
 */

#ifndef vtkBorderRepresentation_h
#define vtkBorderRepresentation_h

#include "vtkCoordinate.h"               //Because of the viewport coordinate macro
#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWidgetRepresentation.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

#include "vtkNew.h" // for ivars

VTK_ABI_NAMESPACE_BEGIN
class vtkPoints;
class vtkPolyData;
class vtkTransform;
class vtkTransformPolyDataFilter;
class vtkPolyDataMapper2D;
class vtkActor2D;
class vtkProperty2D;
class vtkCellArray;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkBorderRepresentation
  : public vtkWidgetRepresentation
{
public:
  /**
   * Instantiate this class.
   */
  static vtkBorderRepresentation* New();

  ///@{
  /**
   * Define standard methods.
   */
  vtkTypeMacro(vtkBorderRepresentation, vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Specify opposite corners of the box defining the boundary of the
   * widget. By default, these coordinates are in the normalized viewport
   * coordinate system, with Position the lower left of the outline, and
   * Position2 relative to Position. Note that using these methods are
   * affected by the ProportionalResize flag. That is, if the aspect ratio of
   * the representation is to be preserved (e.g., ProportionalResize is on),
   * then the rectangle (Position,Position2) is a bounding rectangle.
   */
  vtkViewportCoordinateMacro(Position);
  vtkViewportCoordinateMacro(Position2);
  ///@}

  enum
  {
    BORDER_OFF = 0,
    BORDER_ON,
    BORDER_ACTIVE
  };

  ///@{
  /**
   * Specify when and if the border should appear. If ShowBorder is "on",
   * then the border will always appear. If ShowBorder is "off" then the
   * border will never appear.  If ShowBorder is "active" then the border
   * will appear when the mouse pointer enters the region bounded by the
   * border widget.
   * This method is provided as conveniency to set both horizontal and
   * vertical borders, and the polygon background.
   * BORDER_ON by default.
   * See Also: SetShowHorizontalBorder(), SetShowVerticalBorder(), SetShowPolygon()
   */
  virtual void SetShowBorder(int border);
  virtual int GetShowBorderMinValue();
  virtual int GetShowBorderMaxValue();
  virtual int GetShowBorder();
  void SetShowBorderToOff() { this->SetShowBorder(BORDER_OFF); }
  void SetShowBorderToOn() { this->SetShowBorder(BORDER_ON); }
  void SetShowBorderToActive() { this->SetShowBorder(BORDER_ACTIVE); }
  ///@}

  ///@{
  /**
   * Specify when and if the vertical border should appear.
   * BORDER_ON by default.
   * See Also: SetShowBorder(), SetShowHorizontalBorder()
   */
  vtkSetClampMacro(ShowVerticalBorder, int, BORDER_OFF, BORDER_ACTIVE);
  vtkGetMacro(ShowVerticalBorder, int);
  ///@}

  ///@{
  /**
   * Specify when and if the horizontal border should appear.
   * BORDER_ON by default.
   * See Also: SetShowBorder(), SetShowVerticalBorder()
   */
  vtkSetClampMacro(ShowHorizontalBorder, int, BORDER_OFF, BORDER_ACTIVE);
  vtkGetMacro(ShowHorizontalBorder, int);
  ///@}

  ///@{
  /**
   * Specify the properties of the border.
   */
  vtkGetObjectMacro(BorderProperty, vtkProperty2D);
  ///@}

  ///@{
  /**
   * Specify when and if the border's polygon background should appear.
   * BORDER_ON by default.
   * See Also: SetShowBorder()
   */
  virtual void SetShowPolygon(int border);
  virtual int GetShowPolygon();
  void SetShowPolygonToOff() { this->SetShowPolygon(BORDER_OFF); }
  void SetShowPolygonToOn() { this->SetShowPolygon(BORDER_ON); }
  void SetShowPolygonToActive() { this->SetShowPolygon(BORDER_ACTIVE); }
  ///@}

  ///@{
  /**
   * Specify when and if the border polygon background should appear.
   * BORDER_ON by default.
   * See Also: SetShowBorder(), SetShowPolygon()
   */
  vtkSetClampMacro(ShowPolygonBackground, int, BORDER_OFF, BORDER_ACTIVE);
  vtkGetMacro(ShowPolygonBackground, int);
  ///@}

  ///@{
  /**
   * Whether to enforce the minimum normalized viewport size and limit
   * the normalized viewport coordinates to [0.0 -> 1.0]. This keeps
   * widgets from being moved offscreen or being scaled down past their
   * minimum viewport size.
   *
   * Off by Default.
   */
  vtkSetMacro(EnforceNormalizedViewportBounds, vtkTypeBool);
  vtkGetMacro(EnforceNormalizedViewportBounds, vtkTypeBool);
  vtkBooleanMacro(EnforceNormalizedViewportBounds, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Indicate whether resizing operations should keep the x-y directions
   * proportional to one another. Also, if ProportionalResize is on, then
   * the rectangle (Position,Position2) is a bounding rectangle, and the
   * representation will be placed in the rectangle in such a way as to
   * preserve the aspect ratio of the representation.
   *
   * Off by Default.
   */
  vtkSetMacro(ProportionalResize, vtkTypeBool);
  vtkGetMacro(ProportionalResize, vtkTypeBool);
  vtkBooleanMacro(ProportionalResize, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Specify a minimum and/or maximum size [0.0 -> 1.0] that this representation
   * can take. These methods require two values: size values in the x and y
   * directions, respectively.
   *
   * Default is { 0.0, 0.0 }.
   */
  vtkSetVector2Macro(MinimumNormalizedViewportSize, double);
  vtkGetVector2Macro(MinimumNormalizedViewportSize, double);
  ///@}

  ///@{
  /**
   * Specify a minimum and/or maximum size (in pixels) that this representation
   * can take. These methods require two values: size values in the x and y
   * directions, respectively.
   *
   * Default is { 1, 1 }.
   */
  vtkSetVector2Macro(MinimumSize, int);
  vtkGetVector2Macro(MinimumSize, int);
  vtkSetVector2Macro(MaximumSize, int);
  vtkGetVector2Macro(MaximumSize, int);
  ///@}

  ///@{
  /**
   * The tolerance representing the distance to the widget (in pixels)
   * in which the cursor is considered to be on the widget, or on a
   * widget feature (e.g., a corner point or edge).
   *
   * Default is 3.
   */
  vtkSetClampMacro(Tolerance, int, 1, 10);
  vtkGetMacro(Tolerance, int);
  ///@}

  ///@{
  /**
   * After a selection event within the region interior to the border; the
   * normalized selection coordinates may be obtained.
   */
  vtkGetVectorMacro(SelectionPoint, double, 2);
  ///@}

  ///@{
  /**
   * This is a modifier of the interaction state. When set, widget interaction
   * allows the border (and stuff inside of it) to be translated with mouse
   * motion.
   */
  vtkSetMacro(Moving, vtkTypeBool);
  vtkGetMacro(Moving, vtkTypeBool);
  vtkBooleanMacro(Moving, vtkTypeBool);
  ///@}

  enum
  {
    AnyLocation = 0,
    LowerLeftCorner,
    LowerRightCorner,
    LowerCenter,
    UpperLeftCorner,
    UpperRightCorner,
    UpperCenter
  };

  ///@{
  /**
   * Set the representation position, by enumeration (
   * AnyLocation = 0,
   * LowerLeftCorner,
   * LowerRightCorner,
   * LowerCenter,
   * UpperLeftCorner,
   * UpperRightCorner,
   * UpperCenter)
   * related to the render window
   */
  virtual void SetWindowLocation(int enumLocation);
  vtkGetMacro(WindowLocation, int);
  ///@}

  /**
   * Update window location if a window location is set.
   *
   * This function was made public for VTK issue #18987.
   * Positioning and scaling needs a better API.
   */
  virtual void UpdateWindowLocation();

  /**
   * Define the various states that the representation can be in.
   */
  enum InteractionStateType
  {
    Outside = 0,
    Inside,
    AdjustingP0,
    AdjustingP1,
    AdjustingP2,
    AdjustingP3,
    AdjustingE0,
    AdjustingE1,
    AdjustingE2,
    AdjustingE3
  };

  vtkSetClampMacro(InteractionState, int, 0, AdjustingE3);

  /**
   * Return the MTime of this object. It takes into account MTimes
   * of position coordinates and border's property.
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Subclasses should implement these methods. See the superclasses'
   * documentation for more information.
   */
  void BuildRepresentation() override;
  void StartWidgetInteraction(double eventPos[2]) override;
  void WidgetInteraction(double eventPos[2]) override;
  virtual void GetSize(double size[2])
  {
    size[0] = 1.0;
    size[1] = 1.0;
  }
  int ComputeInteractionState(int X, int Y, int modify = 0) override;
  ///@}

  ///@{
  /**
   * These methods are necessary to make this representation behave as
   * a vtkProp.
   */
  void GetActors2D(vtkPropCollection*) override;
  void ReleaseGraphicsResources(vtkWindow*) override;
  int RenderOverlay(vtkViewport*) override;
  int RenderOpaqueGeometry(vtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport*) override;
  vtkTypeBool HasTranslucentPolygonalGeometry() override;
  ///@}

  void SetBWActorDisplayOverlayEdges(bool);
  void SetBWActorDisplayOverlayPolygon(bool);

  ///@{
  /**
   * Set/Get the RGB color of the border.
   * Default is white (1.0, 1.0, 1.0).
   */
  vtkSetVector3Macro(BorderColor, double);
  vtkGetVector3Macro(BorderColor, double);
  ///@}

  ///@{
  /**
   * Set/Get the thickness of the border in screen units.
   * Default is 1.0.
   */
  vtkSetClampMacro(BorderThickness, float, 0, VTK_FLOAT_MAX);
  vtkGetMacro(BorderThickness, float);
  ///@}

  ///@{
  /**
   * Set/Get the ratio between no radius and maximum radius.
   * In order to compute round corners, we create 2 points
   * in each side of the corner. The maximum radius is then
   * the minimum length of the two sides of each corners.
   * This maximum radius is scaled by the CornerRadiusStrength.
   * Default is 0.0 (no radius).
   */
  vtkSetClampMacro(CornerRadiusStrength, double, 0.0, 1.0);
  vtkGetMacro(CornerRadiusStrength, double);
  ///@}

  ///@{
  /**
   * Set/Get the number of points that define each round corners.
   * A high value increase the resolution of the corners.
   * Default is 20.
   */
  vtkSetClampMacro(CornerResolution, int, 0, 1000);
  vtkGetMacro(CornerResolution, int);
  ///@}

  ///@{
  /**
   * Set/Get the RGB color of the background polygon.
   * Default is white (1.0, 1.0, 1.0).
   */
  vtkSetVector3Macro(PolygonColor, double);
  vtkGetVector3Macro(PolygonColor, double);
  ///@}

  ///@{
  /**
   * Set/Get the opacity of the background color.
   * Default is 0.0.
   */
  vtkSetClampMacro(PolygonOpacity, double, 0.0, 1.0);
  vtkGetMacro(PolygonOpacity, double);
  ///@}

  ///@{
  /**
   * Convenience method to set the background color and the opacity at once
   */
  void SetPolygonRGBA(double rgba[4]);
  void SetPolygonRGBA(double r, double g, double b, double a);

  /**
   * Convenience method to get the background color and the opacity at once
   */
  void GetPolygonRGBA(double rgba[4]);
  void GetPolygonRGBA(double& r, double& g, double& b, double& a);
  ///@}

protected:
  vtkBorderRepresentation();
  ~vtkBorderRepresentation() override;

  // Ivars
  int ShowVerticalBorder = BORDER_ON;
  int ShowHorizontalBorder = BORDER_ON;
  int ShowPolygonBackground = BORDER_ON;
  vtkNew<vtkProperty2D> BorderProperty;
  vtkNew<vtkProperty2D> PolygonProperty;
  vtkTypeBool EnforceNormalizedViewportBounds = 0;
  vtkTypeBool ProportionalResize = 0;
  int Tolerance = 3;
  vtkTypeBool Moving = 0;
  double SelectionPoint[2] = { 0.0, 0.0 };

  // Layout (position of lower left and upper right corners of border)
  vtkNew<vtkCoordinate> PositionCoordinate;
  vtkNew<vtkCoordinate> Position2Coordinate;

  // Window location by enumeration
  int WindowLocation = AnyLocation;

  // Sometimes subclasses must negotiate with their superclasses
  // to achieve the correct layout.
  int Negotiated;
  virtual void NegotiateLayout();

  // Update the border visibility based on InteractionState.
  // See Also: SetShowHorizontalBorder(), SetShowHorizontalBorder(),
  // ComputeInteractionState()
  virtual void UpdateShowBorder();

  // Keep track of start position when moving border
  double StartPosition[2];

  // Border representation. Subclasses may use the BWTransform class
  // to transform their geometry into the region surrounded by the border.
  vtkNew<vtkPoints> BWPoints;
  vtkNew<vtkPolyData> BWPolyData;
  vtkNew<vtkPolyData> PolyDataEdges;
  vtkNew<vtkPolyData> PolyDataPolygon;
  vtkNew<vtkTransform> BWTransform;
  vtkNew<vtkTransformPolyDataFilter> BWTransformFilter;
  vtkNew<vtkPolyDataMapper2D> BWMapperEdges;
  vtkNew<vtkPolyDataMapper2D> BWMapperPolygon;
  vtkNew<vtkActor2D> BWActorEdges;
  vtkNew<vtkActor2D> BWActorPolygon;

  // Constraints on size
  double MinimumNormalizedViewportSize[2] = { 0.0, 0.0 };
  int MinimumSize[2] = { 1, 1 };
  int MaximumSize[2] = { VTK_INT_MAX, VTK_INT_MAX };

  // Properties of the border
  double BorderColor[3] = { 1.0, 1.0, 1.0 };
  float BorderThickness = 1.0;
  double CornerRadiusStrength = 0.0;
  int CornerResolution = 20;

  // Properties of the inner polygon (ie. the background)
  double PolygonColor[3] = { 1.0, 1.0, 1.0 };
  double PolygonOpacity = 0.0;

  /**
   * Create all 4 round corners with the specified radius and resolution.
   */
  void ComputeRoundCorners();

  /**
   * Create a quarter circle centered in point[idCenterX].x, point[idCenterY].y),
   * of radius 'radius' with a starting angle 'startAngle' ending in
   * 'startAngle + PI/2' with CornerResolution number of points.
   * Computed points are stored in the vtkPoints 'points' and
   * inserted in the vtkCellArray 'polys'
   */
  void ComputeOneRoundCorner(vtkCellArray* polys, vtkPoints* points, double radius, vtkIdType xPt,
    vtkIdType yPt, double startAngle);

private:
  vtkBorderRepresentation(const vtkBorderRepresentation&) = delete;
  void operator=(const vtkBorderRepresentation&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
