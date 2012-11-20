/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkChartXYZ.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkChartXYZ - Factory class for drawing 3D XYZ charts.
//
// .SECTION Description

#ifndef __vtkChartXYZ_h
#define __vtkChartXYZ_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkContextItem.h"
#include "vtkColor.h"        // For vtkColor4ub
#include "vtkRect.h"         // For vtkRectf ivars
#include "vtkNew.h"          // For ivars
#include "vtkSmartPointer.h" // For ivars
#include <vector>            // For ivars

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

class VTKCHARTSCORE_EXPORT vtkChartXYZ : public vtkContextItem
{
public:
  vtkTypeMacro(vtkChartXYZ, vtkContextItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  static vtkChartXYZ * New();

  // Description:
  // Set the geometry in pixel coordinates (origin and width/height).
  // This method also sets up the end points of the axes of the chart.
  // For this reason, if you call SetAroundX(), you should call SetGeometry()
  // afterwards.
  void SetGeometry(const vtkRectf &bounds);

  // Description:
  // Set the rotation angle for the chart (AutoRotate mode only).
  void SetAngle(double angle);

  // Description:
  // Set whether or not we're rotating about the X axis.
  void SetAroundX(bool isX);

  // Description:
  // Set the vtkAnnotationLink for the chart.
  virtual void SetAnnotationLink(vtkAnnotationLink *link);

  // Description:
  // Get the x (0), y (1) or z (2) axis.
  vtkAxis * GetAxis(int axis);

  // Description:
  // Set the color for the axes.
  void SetAxisColor(const vtkColor4ub& color);
  vtkColor4ub GetAxisColor();

  // Description:
  // Set whether or not we're using this chart to rotate on a timer.
  // Default value is false.
  void SetAutoRotate(bool b);

  // Description:
  // Set whether or not axes labels & tick marks should be drawn.
  // Default value is true.
  void SetDecorateAxes(bool b);

  // Description:
  // Set whether or not the chart should automatically resize itself to fill
  // the scene.  Default value is true.
  void SetFitToScene(bool b);

  // Description:
  // Paint event for the chart, called whenever the chart needs to be drawn.
  virtual bool Paint(vtkContext2D *painter);

  // Description:
  // Adds a plot to the chart.
  virtual vtkIdType AddPlot(vtkPlot3D* plot);

  // Description:
  // Remove all the plots from this chart.
  void ClearPlots();

  // Description:
  // Determine the XYZ bounds of the plots within this chart.
  // This information is then used to set the range of the axes.
  void RecalculateBounds();

  // Description:
  // Use this chart's Geometry to set the endpoints of its axes.
  // This method also sets up a transformation that is used to
  // properly render the data within the chart.
  void RecalculateTransform();

  //BTX
  // Description:
  // Returns true if the transform is interactive, false otherwise.
  virtual bool Hit(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse press event. Keep track of zoom anchor position.
  virtual bool MouseButtonPressEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse move event. Perform pan or zoom as specified by the mouse bindings.
  virtual bool MouseMoveEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse wheel event.  Zooms in or out.
  virtual bool MouseWheelEvent(const vtkContextMouseEvent &mouse, int delta);

  // Description:
  // Key press event.  This allows the user to snap the chart to one of three
  // different 2D views.  "x" changes the view so we're looking down the X axis.
  // Similar behavior occurs for "y" or "z".
  virtual bool KeyPressEvent(const vtkContextKeyEvent &key);
  //ETX

protected:
  vtkChartXYZ();
  ~vtkChartXYZ();

  // Description:
  // Calculate the transformation matrices used to draw data points and axes
  // in the scene.  This function also sets up clipping planes that determine
  // whether or not a data point is within range.
  virtual void CalculateTransforms();

  // Description:
  // Given the x, y and z vtkAxis, and a transform, calculate the transform that
  // the points in a chart would need to be drawn within the axes. This assumes
  // that the axes have the correct start and end positions, and that they are
  // perpendicular.
  bool CalculatePlotTransform(vtkAxis *x, vtkAxis *y, vtkAxis *z,
                              vtkTransform *transform);

  // Description:
  // Rotate the chart in response to a mouse movement.
  bool Rotate(const vtkContextMouseEvent &mouse);

  // Description:
  // Pan the data within the chart in response to a mouse movement.
  bool Pan(const vtkContextMouseEvent &mouse);

  // Description:
  // Zoom in or out on the data in response to a mouse movement.
  bool Zoom(const vtkContextMouseEvent &mouse);

  // Description:
  // Spin the chart in response to a mouse movement.
  bool Spin(const vtkContextMouseEvent &mouse);

  // Description:
  // Adjust the rotation of the chart so that we are looking down the X axis.
  void LookDownX();

  // Description:
  // Adjust the rotation of the chart so that we are looking down the Y axis.
  void LookDownY();

  // Description:
  // Adjust the rotation of the chart so that we are looking down the Z axis.
  void LookDownZ();

  // Description:
  // Adjust the rotation of the chart so that we are looking up the X axis.
  void LookUpX();

  // Description:
  // Adjust the rotation of the chart so that we are looking up the Y axis.
  void LookUpY();

  // Description:
  // Adjust the rotation of the chart so that we are looking up the Z axis.
  void LookUpZ();

  // Description:
  // Check to see if the scene changed size since the last render.
  bool CheckForSceneResize();

  // Description:
  // Scale the axes up or down in response to a scene resize.
  void RescaleAxes();

  // Description:
  // Scale up the axes when the scene gets larger.
  void ScaleUpAxes();

  // Description:
  // Scale down the axes when the scene gets smaller.
  void ScaleDownAxes();

  // Description:
  // Change the scaling of the axes by a specified amount.
  void ZoomAxes(int delta);

  // Description:
  // Initialize a list of "test points".  These are used to determine whether
  // or not the chart fits completely within the bounds of the current scene.
  void InitializeAxesBoundaryPoints();

  // Description:
  // Initialize the "future box" transform.  This transform is a duplicate of
  // the Box transform, which dictates how the chart's axes should be drawn.
  // In ScaleUpAxes() and ScaleDownAxes(), we incrementally change the scaling
  // of the FutureBox transform to determine how much we need to zoom in or
  // zoom out to fit the chart within the newly resized scene.  Using a
  // separate transform for this process allows us to resize the Box in a
  // single step.
  void InitializeFutureBox();

  // Description:
  // Compute a bounding box for the data that is rendered within the axes.
  void ComputeDataBounds();

  // Description:
  // Draw the cube axes of this chart.
  void DrawAxes(vtkContext3D *context);

  // Description:
  // For each of the XYZ dimensions, find the axis line that is furthest
  // from the rendered data.
  void DetermineWhichAxesToLabel();

  // Description:
  // Draw tick marks and tick mark labels along the axes.
  void DrawTickMarks(vtkContext2D *painter);

  // Description:
  // Label the axes.
  void DrawAxesLabels(vtkContext2D *painter);

  // Description:
  // Compute how some text should be offset from an axis.  The parameter
  // bounds contains the bounding box of the text to be rendered.  The
  // result is stored in the parameter offset.
  void GetOffsetForAxisLabel(int axis, float *bounds, float *offset);

  // Description:
  // Calculate the next "nicest" numbers above and below the current minimum.
  // \return the "nice" spacing of the numbers.
  // This function was mostly copied from vtkAxis.
  double CalculateNiceMinMax(double &min, double &max, int axis);

  // Description:
  // Get the equation for the ith face of our bounding cube.
  void GetClippingPlaneEquation(int i, double *planeEquation);

  // Description:
  // The size and position of this chart.
  vtkRectf Geometry;

  // Description:
  // The 3 axes of this chart.
  std::vector< vtkSmartPointer<vtkAxis> > Axes;

  // Description:
  // This boolean indicates whether or not we're using this chart to rotate
  // on a timer.
  bool AutoRotate;

  // Description:
  // When we're in AutoRotate mode, this boolean tells us if we should rotate
  // about the X axis or the Y axis.
  bool IsX;

  // Description:
  // When we're in AutoRotate mode, this value tells the chart how much it
  // should be rotated.
  double Angle;

  // Description:
  // This boolean indicates whether or not we should draw tick marks
  // and axes labels.
  bool DrawAxesDecoration;

  // Description:
  // This boolean indicates whether or not we should automatically resize the
  // chart so that it snugly fills up the scene.
  bool FitToScene;

  // Description:
  // This is the transform that is applied when rendering data from the plots.
  vtkNew<vtkTransform> ContextTransform;

  // Description:
  // This transform translates and scales the plots' data points so that they
  // appear within the axes of this chart.  It is one of the factors that
  // makes up the ContextTransform.
  vtkNew<vtkTransform> PlotTransform;

  // Description:
  // This is the transform that is applied when rendering data from the plots.
  vtkNew<vtkTransform> Box;

  // Description:
  // This transform keeps track of how the chart has been rotated.
  vtkNew<vtkTransform> Rotation;

  // Description:
  // This transform keeps track of how the data points have been panned within
  // the chart.
  vtkNew<vtkTransform> Translation;

  // Description:
  // This transform keeps track of how the data points have been scaled
  // (zoomed in or zoomed out) within the chart.
  vtkNew<vtkTransform> Scale;

  // Description:
  // This transform keeps track of how the axes have been scaled
  // (zoomed in or zoomed out).
  vtkNew<vtkTransform> BoxScale;

  // Description:
  // This transform is initialized as a copy of Box.  It is used within
  // ScaleUpAxes() and ScaleDownAxes() to figure out how much we need to
  // zoom in or zoom out to fit our chart within the newly resized scene.
  vtkNew<vtkTransform> FutureBox;

  // Description:
  // This transform keeps track of the Scale of the FutureBox transform.
  vtkNew<vtkTransform> FutureBoxScale;

  // Description:
  // This is the pen that is used to draw data from the plots.
  vtkNew<vtkPen> Pen;

  // Description:
  // This is the pen that is used to draw the axes.
  vtkNew<vtkPen> AxisPen;

  // Description:
  // This link is used to share selected points with other classes.
  vtkSmartPointer<vtkAnnotationLink> Link;

  // Description:
  // The plots that are drawn within this chart.
  std::vector<vtkPlot3D *> Plots;

  // Description:
  // The label for the X Axis.
  std::string XAxisLabel;

  // Description:
  // The label for the Y Axis.
  std::string YAxisLabel;

  // Description:
  // The label for the Z Axis.
  std::string ZAxisLabel;

  // Description:
  // The six planes that define the bounding cube of our 3D axes.
  vtkNew<vtkPlaneCollection> BoundingCube;

  // Description:
  // Points used to determine whether the axes will fit within the scene as
  // currently sized, regardless of rotation.
  float AxesBoundaryPoints[14][3];

  // Description:
  // This member variable stores the size of the tick labels for each axis.
  // It is used to determine the position of the axis labels.
  float TickLabelOffset[3][2];

  // Description:
  // The height of the scene, as of the most recent call to Paint().
  int SceneHeight;

  // Description:
  // The weight of the scene, as of the most recent call to Paint().
  int SceneWidth;

  // Description:
  // Which line to label.
  int XAxisToLabel[3];
  int YAxisToLabel[3];
  int ZAxisToLabel[3];

  // Description:
  // What direction the data is from each labeled axis line.
  int DirectionToData[3];

  // Description:
  // A bounding box surrounding the currently rendered data points.
  double DataBounds[4];

private:
  vtkChartXYZ(const vtkChartXYZ &);    // Not implemented.
  void operator=(const vtkChartXYZ &); // Not implemented.
};

#endif
