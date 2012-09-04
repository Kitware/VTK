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

class vtkAnnotationLink;
class vtkAxis;
class vtkContext3D;
class vtkContextMouseEvent;
class vtkMatrix4x4;
class vtkPen;
class vtkPlane;
class vtkPlot;
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
  // Paint event for the chart, called whenever the chart needs to be drawn
  virtual bool Paint(vtkContext2D *painter);

  // Description:
  // Set the input for the chart, this should be done in the plot, but keeping
  // things simple while I get everything working...
  virtual void SetInput(vtkTable *input, const vtkStdString &x,
                        const vtkStdString &y, const vtkStdString &z);
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
  // Mouse wheel event. Perform pan or zoom as specified by mouse bindings.
  virtual bool MouseWheelEvent(const vtkContextMouseEvent &mouse, int delta);

  // Description:
  // Key press event.  This allows the user to snap the chart to one of three
  // different 2D views.  "x" changes the view so we're looking down the X axis.
  // Similar behavior occurs for "y" or "z".
  virtual bool KeyPressEvent(const vtkContextKeyEvent &key);
  //ETX

  bool PointShouldBeClipped(vtkVector3f point);

  virtual void SetScene(vtkContextScene *scene);

protected:
  vtkInteractiveChartXYZ();
  ~vtkInteractiveChartXYZ();
  virtual void CalculateTransforms();
  bool Rotate(const vtkContextMouseEvent &mouse);
  bool Pan(const vtkContextMouseEvent &mouse);
  bool Zoom(const vtkContextMouseEvent &mouse);
  bool Spin(const vtkContextMouseEvent &mouse);
  void LookDownX();
  void LookDownY();
  void LookDownZ();
  void LookUpX();
  void LookUpY();
  void LookUpZ();
  void UpdateClippedPoints();
  bool CheckForSceneResize();
  void RescaleAxes();
  void ScaleUpAxes();
  void ScaleDownAxes();
  void ZoomAxes(int delta);
  void InitializeAxesBoundaryPoints();
  void InitializeFutureBox();
  void ComputeDataBounds();
  void DetermineWhichAxesToLabel();
  void DrawTickMarks(vtkContext3D *context);
  void DrawAxesLabels(vtkContext2D *painter);
  void GetOffsetForAxisLabel(int axis, float *bounds, float *offset);

  vtkNew<vtkTransform> Translation;
  vtkNew<vtkTransform> Scale;
  vtkNew<vtkTransform> BoxScale;
  vtkNew<vtkTransform> FutureBox;
  vtkNew<vtkTransform> FutureBoxScale;
  vtkNew<vtkUnsignedCharArray> Colors;
  vtkNew<vtkUnsignedCharArray> ClippedColors;
  int NumberOfComponents;

  std::string XAxisLabel;
  std::string YAxisLabel;
  std::string ZAxisLabel;

  vector<vtkVector3f> clipped_points;

  vtkNew<vtkPlane> Face1;
  vtkNew<vtkPlane> Face2;
  vtkNew<vtkPlane> Face3;
  vtkNew<vtkPlane> Face4;
  vtkNew<vtkPlane> Face5;
  vtkNew<vtkPlane> Face6;
  float AxesBoundaryPoints[14][3];

  vtkNew<vtkMatrix4x4> Modelview;

  double MaxDistance;
  int SceneHeight;
  int SceneWidth;

  // which line to label
  int XAxisToLabel[3];
  int YAxisToLabel[3];
  int ZAxisToLabel[3];

  //what direction the data is from each labeled axis line
  int DirectionToData[3];

  double DataBounds[4];

private:
  float TranslationDebug[3];
  vtkInteractiveChartXYZ(const vtkInteractiveChartXYZ &);    // Not implemented.
  void operator=(const vtkInteractiveChartXYZ &); // Not implemented.
};

#endif
