// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkPlotRadViz
 * @brief   Radial plot of multivariate data that show the relations among these variables.
 *
 * Radial plot of multivariate data (tuples with multiple components) that
 * shows the relations among these variables (components). The plot
 * accepts either a vtkTable or a vtkDataSet as input and uses
 * vtkMultiVariateToRGBA to compute the generalized barycentric coordinates (GBC)
 * for the variables.
 *
 * RadViz Deluxe: An Attribute-Aware Display for Multivariate Data.
 * Shenghui Cheng, Wei Xu and Klaus Mueller.
 *
 * @sa vtkPlotParallelCoordinates, vtkMultiVariateToRGBA
 *
 */

#ifndef vtkPlotRadViz_h
#define vtkPlotRadViz_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkPen.h"              // For SetSmartPointerMacro
#include "vtkPlot.h"             // For the base class
#include "vtkPoints2D.h"         // For GetSmartPointerMacro
#include "vtkSmartPointer.h"
#include "vtkTextProperty.h" // For SetSmartPointerMacro
#include "vtkUnsignedCharArray.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

#include <memory>

VTK_ABI_NAMESPACE_BEGIN
class vtkContext2D;
class vtkColorSeries;
class vtkDataObject;
class vtkDataSet;
class vtkDataArray;
class vtkPoints2D;
class vtkTextProperty;
class vtkUnsignedCharArray;

class VTKCHARTSCORE_EXPORT VTK_MARSHALAUTO vtkPlotRadViz : public vtkPlot
{
public:
  vtkTypeMacro(vtkPlotRadViz, vtkPlot);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkPlotRadViz* New();

  void SetInputData(vtkDataSet* dataset);
  /**
   * Paint event for the item.
   */
  bool Paint(vtkContext2D* painter) override;

  /**
   * Set the dimensions of the pie, arguments 1 and 2 are the x and y coordinate
   * of the bottom corner. Arguments 3 and 4 are the width and height.
   */
  void SetDimensions(int arg1, int arg2, int arg3, int arg4);

  /**
   * Set the dimensions of the pie, elements 0 and 1 are the x and y coordinate
   * of the bottom corner. Elements 2 and 3 are the width and height.
   */
  void SetDimensions(const int arg[4]);

  ///@{
  /**
   * Get the dimensions of the pie, elements 0 and 1 are the x and y coordinate
   * of the bottom corner. Elements 2 and 3 are the width and height.
   */
  vtkGetVector4Macro(Dimensions, int);
  ///@}

  ///@{
  /**
   * How many segments do we use when drawing the color wheel. More segments
   * will produce a smoother color wheel. The default is 120.
   */
  vtkSetMacro(ColorWheelSegmentCount, int);
  vtkGetMacro(ColorWheelSegmentCount, int);
  ///@}

  ///@{
  /**
   * What angle is Red placed at. Default is 90 degrees.
   */
  vtkSetMacro(ColorWheelStartAngle, int);
  vtkGetMacro(ColorWheelStartAngle, int);
  ///@}

  ///@{
  /**
   * Colors are laid out in clockwise order if true, otherwise in
   * counter-clockwise order. Default is true.
   */
  vtkSetMacro(ColorWheelClockwiseOrdering, bool);
  vtkGetMacro(ColorWheelClockwiseOrdering, bool);
  vtkBooleanMacro(ColorWheelClockwiseOrdering, bool);
  ///@}

  ///@{
  /**
   * Color wheel is shown if true, and hidden otherwise. Default is true;
   */
  vtkSetMacro(ColorWheelVisibility, bool);
  vtkGetMacro(ColorWheelVisibility, bool);
  vtkBooleanMacro(ColorWheelVisibility, bool);
  ///@}

  ///@{
  /**
   * Turn on/off flag to control whether scalar data is used to color objects.
   * The RGBA colors are automatically computed by vtkMultiVariateToRGBA.
   */
  vtkSetMacro(ScalarVisibility, vtkTypeBool);
  vtkGetMacro(ScalarVisibility, vtkTypeBool);
  vtkBooleanMacro(ScalarVisibility, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Controls how the Lens is drawn. Points outside the circle
   * are set transparent (invisible).
   */
  vtkSetSmartPointerMacro(LensPen, vtkPen);
  vtkGetSmartPointerMacro(LensPen, vtkPen);
  ///@}

  ///@{
  /**
   * Controls how the vertices is drawn.
   * These are the markers on the circle for the variables.
   */
  vtkSetSmartPointerMacro(VerticesPen, vtkPen);
  vtkGetSmartPointerMacro(VerticesPen, vtkPen);
  /**
   * Return the vertices of the plot. Valid after the first Paint.
   */
  vtkGetSmartPointerMacro(Vertices, vtkPoints2D);
  /**
   * Angle in degrees, to rotate the vertices
   * coding the multivariate attributes Default is 0 degrees. This
   * angle affects the GBC positions and the computed colors.
   * The angle stored will be between [0, 360).
   */
  virtual void SetVerticesRotation(double rotation);
  vtkGetMacro(VerticesRotation, double);
  ///@}

  ///@{
  /**
   * Center of the lens in window coordinates
   */
  virtual void SetLensCenter(const float* center);
  vtkGetVector2Macro(LensCenter, float);
  /**
   * Radius of the lens
   */
  virtual void SetLensRadius(float radius);
  vtkGetMacro(LensRadius, float);
  ///@}

  /**
   * Center of the plot
   */
  const float* GetCenter() const;
  /**
   * Radius of the plot
   */
  float GetRadius() const;

  ///@{
  /**
   * Get the vtkTextProperty that governs how the vertices labels are displayed
   */
  vtkSetSmartPointerMacro(LabelsProperties, vtkTextProperty);
  vtkGetSmartPointerMacro(LabelsProperties, vtkTextProperty);
  ///@}

  double GetAngle(double vertex[2], double mousePos[2]);

protected:
  vtkPlotRadViz();
  ~vtkPlotRadViz() override = default;
  void DrawColorWheel(vtkContext2D* painter);
  // draw the points either using the 'color' if its not nullptr or using the 'pen' color.
  vtkSmartPointer<vtkPoints2D> DrawMarkers(
    vtkContext2D* painter, vtkDataArray* gbc, vtkPen* pen, vtkDataArray* color = nullptr);
  void DrawLabels(vtkContext2D* painter, vtkDataArray* vertices, vtkPen* pen);
  void DrawLens(vtkContext2D* painter);
  void ComputeAutoLabels(vtkDataObject* input, vtkDataArray* array);
  vtkSmartPointer<vtkPoints2D> TransformPoints(vtkDataArray* points);
  vtkSmartPointer<vtkUnsignedCharArray> TransformColors(vtkDataArray* rgba);
  /**
   * Returns the horizontal and vertical text justification for xy coordinates.
   */
  static std::pair<int, int> GetJustification(double xy[2]);
  void ShiftLabelByPenWidth(float width, double transformedXy[2]);
  std::tuple<vtkDataArray*, vtkDataArray*, vtkDataArray*> GetOutputArrays(vtkDataObject* output);
  static vtkDataArray* GetOutputArray(vtkTable* output, const char* name);
  static vtkDataArray* GetOutputArray(vtkDataSet* dataset, const char* name);

  int Dimensions[4];
  /**
   * Controls how the vertices is drawn.
   * These are the markers on the circle for the variables.
   */
  vtkSmartPointer<vtkPen> VerticesPen;
  /**
   * Controls how the Lens is drawn. Points outside the circle
   * are set transparent (invisible).
   */
  vtkSmartPointer<vtkPen> LensPen;
  /**
   * Where the lens is drawn and its radius in Window coordinates
   */
  float LensCenter[2];
  float LensRadius;

  vtkSmartPointer<vtkPoints2D> Vertices;
  double VerticesRotation;

  /**
   * The text properties associated with the vertices labels
   */
  vtkSmartPointer<vtkTextProperty> LabelsProperties;
  int ColorWheelSegmentCount;
  int ColorWheelStartAngle;
  bool ColorWheelClockwiseOrdering;
  bool ColorWheelVisibility;
  vtkTypeBool ScalarVisibility;

private:
  vtkPlotRadViz(const vtkPlotRadViz&) = delete;
  void operator=(const vtkPlotRadViz&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

VTK_ABI_NAMESPACE_END
#endif // vtkPlotRadViz_h
