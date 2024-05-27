// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkChartXYZ
 * @brief   Factory class for drawing 3D XYZ charts.
 *
 *
 */

#ifndef vtkChartXYZ_h
#define vtkChartXYZ_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkColor.h"            // For vtkColor4ub
#include "vtkContextItem.h"
#include "vtkNew.h"           // For ivars
#include "vtkRect.h"          // For vtkRectf ivars
#include "vtkSmartPointer.h"  // For ivars
#include "vtkStdString.h"     // For vtkStdString
#include "vtkTextProperty.h"  // For axes text properties
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO
#include <vector>             // For ivars

VTK_ABI_NAMESPACE_BEGIN
class vtkAnnotationLink;
class vtkAxis;
class vtkContext3D;
class vtkContextMouseEvent;
class vtkPen;
class vtkPlaneCollection;
class vtkPlot3D;
class vtkTable;
class vtkTransform;
class vtkUnsignedCharArray;

class VTKCHARTSCORE_EXPORT VTK_MARSHALAUTO vtkChartXYZ : public vtkContextItem
{
public:
  vtkTypeMacro(vtkChartXYZ, vtkContextItem);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkChartXYZ* New();

  /**
   * Set the geometry in pixel coordinates (origin and width/height).
   * This method also sets up the end points of the axes of the chart.
   * For this reason, if you call SetAroundX(), you should call SetGeometry()
   * afterwards.
   *
   * This method will result in a plot with a fixed size. If you want it to scale
   * with the scene then use SetMargins.
   */
  void SetGeometry(const vtkRectf& bounds);

  /**
   * Set the margins in pixels ordered top right bottom left
   * The box will be drawn inside those margins, but the labels and textdecorations will still
   * escape. Note that the width and height automatically adapt to those of the scene.
   *
   * If you want a fixed size instead then use SetGeometry.
   */
  void SetMargins(const vtkVector4i& margins);

  /**
   * Set the rotation angle for the chart (AutoRotate mode only).
   */
  vtkSetMacro(Angle, double);

  /**
   * Set whether or not we're rotating about the X axis.
   */
  void SetAroundX(bool isX);

  /**
   * Set the vtkAnnotationLink for the chart.
   */
  virtual void SetAnnotationLink(vtkAnnotationLink* link);

  /**
   * Get the x (0), y (1) or z (2) axis.
   */
  vtkAxis* GetAxis(int axis);

  /**
   * Set the x (0), y (1) or z (2) axis.
   */
  virtual void SetAxis(int axisIndex, vtkAxis* axis);

  ///@{
  /**
   * Set the color for the axes.
   */
  void SetAxisColor(const vtkColor4ub& color);
  vtkColor4ub GetAxisColor();
  ///@}

  /**
   * Get the text property for axes. Useful for changing font size, font family, font file.
   *
   * For example to use a larger font which is capable of displaying unicode values change
   * the property like this:
   *
   *   chart->GetAxesTextProperty()->SetFontFamily(VTK_FONT_FILE);
   *   chart->GetAxesTextProperty()->SetFontFile("fonts/DejaVuSans.ttf");
   *   chart->GetAxesTextProperty()->SetFontSize(32);
   *
   * You'll need a unicode capable font in a suitable location.
   *
   * Now to get the X axis to display Theta subscript (0) set the label like this using
   * the hex unicode representation:
   *
   *   chart->SetXAxisLabel("\xcf\xb4\xe2\x82\x8d\xe2\x82\x80\xe2\x82\x8e");
   *
   * @return
   */
  vtkTextProperty* GetAxesTextProperty();

  /**
   * Set the X axis label
   */
  vtkSetMacro(XAxisLabel, vtkStdString);

  /**
   * Set the Y axis label
   */
  vtkSetMacro(YAxisLabel, vtkStdString);

  /**
   * Set the Z axis label
   */
  vtkSetMacro(ZAxisLabel, vtkStdString);

  /**
   * Set to true to ensure that axis labels are always on the outer edges of the chart.
   * Default is false, the legacy behaviour, for backwards compatibility, where axis
   * labelling may occur on inner or back edges.
   */
  vtkSetMacro(EnsureOuterEdgeAxisLabelling, bool);

  /**
   * Set whether or not we're using this chart to rotate on a timer.
   * Default value is false.
   */
  vtkSetMacro(AutoRotate, bool);

  /**
   * Set whether or not axes labels & tick marks should be drawn.
   * Default value is true.
   */
  void SetDecorateAxes(bool b);

  /**
   * Set whether or not the chart should automatically resize itself to fill
   * the scene.  Default value is true.
   */
  void SetFitToScene(bool b);

  /**
   * Perform any updates to the item that may be necessary before rendering.
   */
  void Update() override;

  /**
   * Paint event for the chart, called whenever the chart needs to be drawn.
   */
  bool Paint(vtkContext2D* painter) override;

  /**
   * Adds a plot to the chart.
   */
  virtual vtkIdType AddPlot(vtkPlot3D* plot);

  /**
   * Removes a plot from the chart.
   */
  virtual bool RemovePlot(vtkPlot3D* plot);

  /**
   * Remove all the plots from this chart.
   */
  void ClearPlots();

  /**
   * Determine the XYZ bounds of the plots within this chart.
   * This information is then used to set the range of the axes.
   */
  void RecalculateBounds();

  /**
   * Use this chart's Geometry to set the endpoints of its axes.
   * This method also sets up a transformation that is used to
   * properly render the data within the chart.
   */
  void RecalculateTransform();

  /**
   * Returns true if the transform is interactive, false otherwise.
   */
  bool Hit(const vtkContextMouseEvent& mouse) override;

  /**
   * Mouse press event. Keep track of zoom anchor position.
   */
  bool MouseButtonPressEvent(const vtkContextMouseEvent& mouse) override;

  /**
   * Mouse move event. Perform pan or zoom as specified by the mouse bindings.
   */
  bool MouseMoveEvent(const vtkContextMouseEvent& mouse) override;

  /**
   * Mouse wheel event.  Zooms in or out.
   */
  bool MouseWheelEvent(const vtkContextMouseEvent& mouse, int delta) override;

  /**
   * Key press event.  This allows the user to snap the chart to one of three
   * different 2D views.  "x" changes the view so we're looking down the X axis.
   * Similar behavior occurs for "y" or "z".
   */
  bool KeyPressEvent(const vtkContextKeyEvent& key) override;

  /**
   * Hide data outside the box.
   */
  void SetClippingPlanesEnabled(bool);

  /**
   * Check whether data outside the box will be hidden or not.
   */
  vtkGetMacro(ClippingPlanesEnabled, bool);

  /**
   * When rotating the mousewheel, scale not only the plot but also the box.
   */
  vtkSetMacro(ScaleBoxWithPlot, bool);

  /**
   * Check whether scaling the plot will also scale the box.
   */
  vtkGetMacro(ScaleBoxWithPlot, bool);

protected:
  vtkChartXYZ();
  ~vtkChartXYZ() override;

  /**
   * Rotation directions.
   */
  enum RotateDirection
  {
    LEFT,
    RIGHT,
    UP,
    DOWN
  };

  /**
   * The state of an axis.
   */
  enum AxisState
  {
    VERTICAL,
    VERTICAL_2,
    HORIZONTAL,
    HORIZONTAL_2,
    DO_NOT_LABEL,
    STANDARD
  };

  /**
   * The direction to data from an axis.
   */
  enum Direction
  {
    NORTH,
    NORTH_EAST,
    EAST,
    SOUTH_EAST,
    SOUTH,
    SOUTH_WEST,
    WEST,
    NORTH_WEST
  };

  /**
   * Calculate the transformation matrices used to draw data points and axes
   * in the scene.  This function also sets up clipping planes that determine
   * whether or not a data point is within range.
   */
  virtual void CalculateTransforms();

  /**
   * Given the x, y and z vtkAxis, and a transform, calculate the transform that
   * the points in a chart would need to be drawn within the axes. This assumes
   * that the axes have the correct start and end positions, and that they are
   * perpendicular.
   */
  bool CalculatePlotTransform(vtkAxis* x, vtkAxis* y, vtkAxis* z, vtkTransform* transform);

  /**
   * Rotate the chart in response to a mouse movement.
   */
  bool Rotate(const vtkContextMouseEvent& mouse);

  /**
   * Rotate the chart in a specific direction.
   */
  bool Rotate(RotateDirection rotateDirection);

  /**
   * Pan the data within the chart in response to a mouse movement.
   */
  bool Pan(const vtkContextMouseEvent& mouse);

  /**
   * Zoom in or out on the data in response to a mouse movement.
   */
  bool Zoom(const vtkContextMouseEvent& mouse);

  /**
   * Spin the chart in response to a mouse movement.
   */
  bool Spin(const vtkContextMouseEvent& mouse);

  /**
   * Adjust the rotation of the chart so that we are looking down the X axis.
   */
  void LookDownX();

  /**
   * Adjust the rotation of the chart so that we are looking down the Y axis.
   */
  void LookDownY();

  /**
   * Adjust the rotation of the chart so that we are looking down the Z axis.
   */
  void LookDownZ();

  /**
   * Adjust the rotation of the chart so that we are looking up the X axis.
   */
  void LookUpX();

  /**
   * Adjust the rotation of the chart so that we are looking up the Y axis.
   */
  void LookUpY();

  /**
   * Adjust the rotation of the chart so that we are looking up the Z axis.
   */
  void LookUpZ();

  /**
   * Check to see if the scene changed size since the last render.
   */
  bool CheckForSceneResize();

  /**
   * Scale the axes up or down in response to a scene resize.
   */
  void RescaleAxes();

  /**
   * Scale up the axes when the scene gets larger.
   */
  void ScaleUpAxes();

  /**
   * Scale down the axes when the scene gets smaller.
   */
  void ScaleDownAxes();

  /**
   * Change the scaling of the axes by a specified amount.
   */
  void ZoomAxes(int delta);

  /**
   * Initialize a list of "test points".  These are used to determine whether
   * or not the chart fits completely within the bounds of the current scene.
   */
  void InitializeAxesBoundaryPoints();

  /**
   * Initialize the "future box" transform.  This transform is a duplicate of
   * the Box transform, which dictates how the chart's axes should be drawn.
   * In ScaleUpAxes() and ScaleDownAxes(), we incrementally change the scaling
   * of the FutureBox transform to determine how much we need to zoom in or
   * zoom out to fit the chart within the newly resized scene.  Using a
   * separate transform for this process allows us to resize the Box in a
   * single step.
   */
  void InitializeFutureBox();

  /**
   * Compute a bounding box for the data that is rendered within the axes.
   */
  void ComputeDataBounds();

  /**
   * Draw the cube axes of this chart.
   */
  void DrawAxes(vtkContext3D* context);

  /**
   * For each of the XYZ dimensions, find the axis line that is furthest
   * from the rendered data.
   */
  void DetermineWhichAxesToLabel();

  /**
   * New style axis labelling, ensuring labelling is always at the edges of the
   * chart in the most sensible places.
   */
  void NewDetermineWhichAxesToLabel();

  /**
   * Old-style axis labelling, for compatibility; labelling may occur in less
   * optimal places e.g. on inner or back edges of the chart.
   */
  void LegacyDetermineWhichAxesToLabel();
  /**
   * Draw tick marks and tick mark labels along the axes.
   */
  void DrawTickMarks(vtkContext2D* painter);

  /**
   * Label the axes.
   */
  void DrawAxesLabels(vtkContext2D* painter);

  /**
   * Compute how some text should be offset from an axis.  The parameter
   * bounds contains the bounding box of the text to be rendered.  The
   * result is stored in the parameter offset.
   */
  void GetOffsetForAxisLabel(int axis, float* bounds, float* offset);

  /**
   * Calculate the next "nicest" numbers above and below the current minimum.
   * \return the "nice" spacing of the numbers.
   * This function was mostly copied from vtkAxis.
   */
  double CalculateNiceMinMax(double& min, double& max, int axis);

  /**
   * Get the equation for the ith face of our bounding cube.
   */
  void GetClippingPlaneEquation(int i, double* planeEquation);

  /**
   * Gets the current margin left in pixels irrespective of the size-strategy used.
   */
  std::size_t GetMarginLeft() const;

  /**
   * Gets the current margin top in pixels irrespective of the size-strategy used.
   */
  std::size_t GetMarginBottom() const;

  /**
   * Gets the current width of the plot in pixels irrespective of the size-strategy used.
   */
  std::size_t GetPlotWidth() const;

  /**
   * Gets the current height of the plot in pixels irrespective of the size-strategy used.
   */
  std::size_t GetPlotHeight() const;

  /**
   * Specifies how to calculate the size of the chart in function of the size of the scene.
   */
  enum
  {
    USE_MARGINS_AND_SCENE_SIZE,
    USE_GEOMETRY
  } SizeStrategy = USE_GEOMETRY;

  /**
   * The margins in pixels for the box ordered top right bottom left
   * Applicable only when SizeStrategy == USE_MARGINS_AND_SCENE_SIZE
   */
  vtkVector4i Margins = vtkVector4i(40, 40, 40, 40);

  /**
   * The size and position of this chart.
   * Applicable only when SizeStrategy == USE_GEOMETRY
   */
  vtkRectf Geometry = vtkRectf(40, 40, 120, 120);

  /**
   * The 3 axes of this chart.
   */
  std::vector<vtkSmartPointer<vtkAxis>> Axes;

  /**
   * This boolean indicates whether or not we're using this chart to rotate
   * on a timer.
   */
  bool AutoRotate = false;

  /**
   * When we're in AutoRotate mode, this boolean tells us if we should rotate
   * about the X axis or the Y axis.
   */
  bool IsX = false;

  /**
   * When we're in AutoRotate mode, this value tells the chart how much it
   * should be rotated.
   */
  double Angle = 0;

  /**
   * This boolean indicates whether or not we should draw tick marks
   * and axes labels.
   */
  bool DrawAxesDecoration = true;

  /**
   * This boolean indicates whether or not we should automatically resize the
   * chart so that it snugly fills up the scene.
   */
  bool FitToScene = true;

  /**
   * This is the transform that is applied when rendering data from the plots.
   */
  vtkNew<vtkTransform> ContextTransform;

  /**
   * This transform translates and scales the plots' data points so that they
   * appear within the axes of this chart.  It is one of the factors that
   * makes up the ContextTransform.
   */
  vtkNew<vtkTransform> PlotTransform;

  /**
   * This is the transform that is applied when rendering data from the plots.
   */
  vtkNew<vtkTransform> Box;

  /**
   * This transform keeps track of how the chart has been rotated.
   */
  vtkNew<vtkTransform> Rotation;

  /**
   * This transform keeps track of how the data points have been panned within
   * the chart.
   */
  vtkNew<vtkTransform> Translation;

  /**
   * This transform keeps track of how the data points have been scaled
   * (zoomed in or zoomed out) within the chart.
   */
  vtkNew<vtkTransform> Scale;

  /**
   * This transform keeps track of how the axes have been scaled
   * (zoomed in or zoomed out).
   */
  vtkNew<vtkTransform> BoxScale;

  /**
   * This transform is initialized as a copy of Box.  It is used within
   * ScaleUpAxes() and ScaleDownAxes() to figure out how much we need to
   * zoom in or zoom out to fit our chart within the newly resized scene.
   */
  vtkNew<vtkTransform> FutureBox;

  /**
   * This transform keeps track of the Scale of the FutureBox transform.
   */
  vtkNew<vtkTransform> FutureBoxScale;

  /**
   * This is the pen that is used to draw data from the plots.
   */
  vtkNew<vtkPen> Pen;

  /**
   * This is the pen that is used to draw the axes.
   */
  vtkNew<vtkPen> AxisPen;

  /**
   * This link is used to share selected points with other classes.
   */
  vtkSmartPointer<vtkAnnotationLink> Link;

  /**
   * The plots that are drawn within this chart.
   */
  std::vector<vtkPlot3D*> Plots;

  /**
   * These plots got removed (from Plots), try to reuse the free spot.
   */
  std::vector<vtkIdType> FreePlaces;

  /**
   * The text properties of the axes.
   */
  vtkNew<vtkTextProperty> AxesTextProperty;

  /**
   * The label for the X Axis.
   */
  std::string XAxisLabel;

  /**
   * The label for the Y Axis.
   */
  std::string YAxisLabel;

  /**
   * The label for the Z Axis.
   */
  std::string ZAxisLabel;

  /**
   * If set to true, use the new behaviour of ensuring that axis labels are on the outer
   * edges of the chart. If false, the legacy behaviour will be used where axis labels
   * can occur on inner or back edges of the chart.
   */
  bool EnsureOuterEdgeAxisLabelling = false;
  /**
   * The six planes that define the bounding cube of our 3D axes.
   */
  vtkNew<vtkPlaneCollection> BoundingCube;

  /**
   * Points used to determine whether the axes will fit within the scene as
   * currently sized, regardless of rotation.
   */
  float AxesBoundaryPoints[8][3];

  /**
   * This member variable stores the size of the tick labels for each axis.
   * It is used to determine the position of the axis labels.
   */
  float TickLabelOffset[3][2];

  /**
   * The height of the scene, as of the most recent call to Paint().
   */
  int SceneHeight;

  /**
   * The weight of the scene, as of the most recent call to Paint().
   */
  int SceneWidth;

  ///@{
  /**
   * Which line to label.
   */
  int XAxisToLabel[3];
  int YAxisToLabel[3];
  int ZAxisToLabel[3];
  ///@}

  /**
   * What direction the data is from each labeled axis line.
   */
  int DirectionToData[3];

  /**
   * A bounding box surrounding the currently rendered data points.
   */
  double DataBounds[4];

  /**
   * Hide data outside the box.
   */
  bool ClippingPlanesEnabled = true;

  /**
   * When rotating the mousewheel, scale not only the plot but also the box.
   */
  bool ScaleBoxWithPlot = true;

private:
  vtkChartXYZ(const vtkChartXYZ&) = delete;
  void operator=(const vtkChartXYZ&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
