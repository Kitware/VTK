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
#include "vtkSetGet.h"
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
   * Set the bounding box of the plot. Arguments 1 and 2 are the x and y coordinate
   * of the bottom corner. Arguments 3 and 4 are the width and height.
   */
  void SetDimensions(int arg1, int arg2, int arg3, int arg4);

  /**
   * Set the bounding box of the plot. Elements 0 and 1 are the x and y coordinate
   * of the bottom corner. Elements 2 and 3 are the width and height.
   */
  void SetDimensions(const int arg[4]);

  ///@{
  /**
   * Get the bounding box of the plot. Elements 0 and 1 are the x and y coordinate
   * of the bottom corner. Elements 2 and 3 are the width and height.
   */
  vtkGetVector4Macro(Dimensions, int);
  ///@}

  ///@{
  /**
   * Number of segments used when drawing the color wheel. More segments
   * produce a smoother gradient. The default is 120.
   */
  vtkSetMacro(ColorWheelSegmentCount, int);
  vtkGetMacro(ColorWheelSegmentCount, int);
  ///@}

  ///@{
  /**
   * Angle in degrees at which red is placed on the color wheel. Default is 90.
   */
  vtkSetMacro(ColorWheelStartAngle, int);
  vtkGetMacro(ColorWheelStartAngle, int);
  ///@}

  ///@{
  /**
   * Colors are laid out in clockwise order if true, counter-clockwise otherwise.
   * Default is true.
   */
  vtkSetMacro(ColorWheelClockwiseOrdering, bool);
  vtkGetMacro(ColorWheelClockwiseOrdering, bool);
  vtkBooleanMacro(ColorWheelClockwiseOrdering, bool);
  ///@}

  ///@{
  /**
   * Show the color wheel if true, hide it otherwise. Default is true.
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
   * Controls how the vertices are drawn.
   */
  vtkSetSmartPointerMacro(VerticesPen, vtkPen);
  vtkGetSmartPointerMacro(VerticesPen, vtkPen);
  /**
   * Return the vertex positions of the plot. Valid after the first Paint.
   */
  vtkGetSmartPointerMacro(Vertices, vtkPoints2D);
  /**
   * Angle in degrees to rotate the vertices encoding the multivariate attributes.
   * Default is 0 degrees. This angle affects the GBC positions and the computed colors.
   * The angle stored will be between [0, 360).
   */
  virtual void SetVerticesRotation(double rotation);
  vtkGetMacro(VerticesRotation, double);
  ///@}

  ///@{
  /**
   * Center of the lens in window space. The set function ensures that
   * the lens intersects the the GBC circle.
   */
  virtual void SetLensCenter(const float* center);
  vtkGetVector2Macro(LensCenter, float);
  /**
   * Radius of the lens in window space
   */
  virtual void SetLensRadius(float radius);
  vtkGetMacro(LensRadius, float);
  /**
   * Controls how the lens circle outline is drawn.
   */
  vtkSetSmartPointerMacro(LensPen, vtkPen);
  vtkGetSmartPointerMacro(LensPen, vtkPen);
  /**
   * Alpha (opacity) cap for points outside the lens. Each point's alpha is
   * clamped to min(its alpha, OutsideLensAlpha). Default is 1 (no change).
   * Set to 0 to make points outside the lens fully transparent.
   */
  vtkSetMacro(OutsideLensAlphaCap, double);
  vtkGetMacro(OutsideLensAlphaCap, double);
  ///@}

  ///@{
  /**
   * Color used for active vertex, active lens circle or active lens center.
   */
  vtkSetMacro(ActiveColor, vtkColor4ub);
  vtkGetMacro(ActiveColor, vtkColor4ub);
  /**
   * The index of the active vertex (mouse over it or rotating the
   * vertices by dragging that vertex), -1 when no vertex is active.
   */
  vtkSetMacro(ActiveVertexIndex, int);
  vtkGetMacro(ActiveVertexIndex, int);
  /**
   * Set to true when the mouse is over the lens outline or when the
   * lens is resided and set to false otherwise.
   */
  vtkSetMacro(ActiveLensOutline, bool);
  vtkGetMacro(ActiveLensOutline, bool);
  /**
   * Set to true when the mouse is over the lens interior or when the
   * lens is dragged and set to false otherwise.
   */
  vtkSetMacro(ActiveLensInterior, bool);
  vtkGetMacro(ActiveLensInterior, bool);
  ///@}

  /**
   * Center of the plot in window space
   */
  const float* GetCenter() const;
  /**
   * Radius of the plot in window space
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
  /**
   * Draw the points using the 'pen' attributes and  'color' instead of
   * the 'pen' color. The 'points' are in the [-1, 1] space.
   *
   */
  vtkSmartPointer<vtkPoints2D> DrawColoredMarkers(
    vtkContext2D* painter, vtkDataArray* points, vtkPen* pen, vtkDataArray* color = nullptr);
  /**
   * Draw the points using the 'pen' attributes. The 'points' are in
   * the [-1, 1] space. If activeIndex >= 0, draw that point using the ActiveColor
   *
   */
  vtkSmartPointer<vtkPoints2D> DrawMarkers(
    vtkContext2D* painter, vtkDataArray* points, vtkPen* pen, int activeIndex = -1);
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
  ///@{
  /**
   * Returns the arrays named 'vertices', 'gbc', 'rgba' from a table or dataset or throws
   * std::runtime_error if not found
   */
  std::tuple<vtkDataArray*, vtkDataArray*, vtkDataArray*> GetVerticesGbcRgba(vtkDataObject* output);
  /**
   * Returns the column array 'name' or throws std::runtime_error if the array is not found.
   */
  static vtkDataArray* GetArray(vtkTable* output, const char* name);
  /**
   * Returns the point or cell array 'name' or throws std::runtime_error if the array is not found.
   */
  static vtkDataArray* GetArray(vtkDataSet* dataset, const char* name);
  ///@}

  int Dimensions[4];
  /**
   * Controls how the vertices are drawn.
   * These are the markers on the circle for the variables.
   */
  vtkSmartPointer<vtkPen> VerticesPen;
  /**
   * Controls how the lens circle outline is drawn.
   */
  vtkSmartPointer<vtkPen> LensPen;
  /**
   * Pen used for active vertex, active lens circle or active lens center.
   * Only the color is used.
   */
  vtkColor4ub ActiveColor;
  /**
   * The index of the active vertex (mouse over it or rotating the
   * vertices by dragging that vertex), -1 when no vertex is active.
   */
  int ActiveVertexIndex = -1;
  /**
   * Set to true when the mouse is over the lens outline or when the
   * lens is resided and set to false otherwise.
   */
  bool ActiveLensOutline = false;
  /**
   * Set to true when the mouse is over the lens interior or when the
   * lens is dragged and set to false otherwise.
   */
  bool ActiveLensInterior = false;
  /**
   * Alpha cap for points outside the lens.
   */
  double OutsideLensAlphaCap;
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
