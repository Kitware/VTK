/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractiveChartXYZ.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkInteractiveChartXYZ - Factory class for drawing 3D XYZ charts.
//
// .SECTION Description

#ifndef __vtkInteractiveChartXYZ_h
#define __vtkInteractiveChartXYZ_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkChartXYZ.h"
#include "vtkRect.h"         // For vtkRectf ivars
#include "vtkNew.h"          // For ivars
#include "vtkSmartPointer.h" // For ivars

class vtkContext3D;
class vtkContextMouseEvent;
class vtkPlane;
class vtkTable;
class vtkTransform;
class vtkUnsignedCharArray;

class VTKCHARTSCORE_EXPORT vtkInteractiveChartXYZ : public vtkChartXYZ
{
public:
  vtkTypeMacro(vtkInteractiveChartXYZ, vtkChartXYZ);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  static vtkInteractiveChartXYZ * New();

  // Description:
  // Update any data as necessary before drawing the chart.
  void Update();

  // Description:
  // Paint event for the chart, called whenever the chart needs to be drawn.
  virtual bool Paint(vtkContext2D *painter);

  // Description:
  // Set the input for the chart.
  virtual void SetInput(vtkTable *input, const vtkStdString &x,
                        const vtkStdString &y, const vtkStdString &z);

  // Description:
  // Set the input for the chart, including a dimension for color.
  virtual void SetInput(vtkTable *input, const vtkStdString &x,
                        const vtkStdString &y, const vtkStdString &z,
                        const vtkStdString &color);

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
  vtkInteractiveChartXYZ();
  ~vtkInteractiveChartXYZ();

  // Description:
  // Calculate the transformation matrices used to draw data points and axes
  // in the scene.  This function also sets up clipping planes that determine
  // whether or not a data point is within range.
  virtual void CalculateTransforms();

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
  // Determine what data points fall within the bounds of the chart axes.
  void UpdateClippedPoints();

  // Description:
  // Determine whether an individual data point falls within the bounds of the
  // chart axes.
  bool PointShouldBeClipped(vtkVector3f point);

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
  // The subset of our data points that fall within the axes.  These are
  // the only data points that are rendered.
  vector<vtkVector3f> ClippedPoints;

  // Description:
  // This array assigns a color to each data point.
  vtkNew<vtkUnsignedCharArray> Colors;

  // Description:
  // This array assigns a color to each data point which is currently
  // rendered within the axes.
  vtkNew<vtkUnsignedCharArray> ClippedColors;

  // Description:
  // Number of components in our color vectors.  This value is initialized
  // to zero.  It's typically set to 3 or 4 if the points are to be colored.
  int NumberOfComponents;

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
  vtkNew<vtkPlane> Face1;
  vtkNew<vtkPlane> Face2;
  vtkNew<vtkPlane> Face3;
  vtkNew<vtkPlane> Face4;
  vtkNew<vtkPlane> Face5;
  vtkNew<vtkPlane> Face6;

  // Description:
  // Points used to determine whether the axes will fit within the scene as
  // currently sized, regardless of rotation.
  float AxesBoundaryPoints[14][3];

  // Description:
  // This member variable stores the size of the tick labels for each axis.
  // It is used to determine the position of the axis labels.
  float TickLabelOffset[3][2];

  // Description:
  // Distance between two opposing planes (Faces).  Any point further away
  // from a plane than this value is outside our bounding cube and will not
  // be rendered.
  double MaxDistance;

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
  vtkInteractiveChartXYZ(const vtkInteractiveChartXYZ &);    // Not implemented.
  void operator=(const vtkInteractiveChartXYZ &); // Not implemented.
};

#endif
