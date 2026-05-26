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
#include "vtkTextProperty.h"     // For SetSmartPointerMacro
#include "vtkWrappingHints.h"    // For VTK_MARSHALAUTO

#include <memory>

VTK_ABI_NAMESPACE_BEGIN
class vtkContext2D;
class vtkColorSeries;
class vtkDataObject;
class vtkDataSet;
class vtkFloatArray;
class vtkPoints2D;
class vtkTextProperty;

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
  ///@}

  ///@{
  /**
   * Get the dimensions of the pie, elements 0 and 1 are the x and y coordinate
   * of the bottom corner. Elements 2 and 3 are the width and height.
   */
  vtkSetSmartPointerMacro(VerticesPen, vtkPen);
  vtkGetSmartPointerMacro(VerticesPen, vtkPen);
  ///@}

  ///@{
  /**
   * Get the vtkTextProperty that governs how the vertices labels are displayed
   */
  vtkSetSmartPointerMacro(LabelsProperties, vtkTextProperty);
  vtkGetSmartPointerMacro(LabelsProperties, vtkTextProperty);
  ///@}

protected:
  vtkPlotRadViz();
  ~vtkPlotRadViz() override = default;
  void DrawColorWheel(vtkContext2D* painter);
  void DrawMarkers(vtkContext2D* painter, vtkFloatArray* gbc, vtkPen* pen);
  void DrawLabels(vtkContext2D* painter, vtkFloatArray* vertices, vtkPen* pen);
  void ComputeAutoLabels(vtkDataObject* input, vtkDataArray* array);
  vtkSmartPointer<vtkPoints2D> Transform(vtkFloatArray* points);
  /**
   * Returns the horizontal and vertical text justification for xy coordinates.
   */
  static std::pair<int, int> GetJustification(float xy[2]);
  void ShiftLabelByPenWidth(float width, double transformedXy[2]);

  int Dimensions[4];
  /**
   * This object stores the vtkPen that controls how the vertices is drawn.
   * These are the markers on the circle for the variables.
   */
  vtkSmartPointer<vtkPen> VerticesPen;
  /**
   * The text properties associated with the vertices labels
   */
  vtkSmartPointer<vtkTextProperty> LabelsProperties;
  int ColorWheelSegmentCount;
  int ColorWheelStartAngle;
  bool ColorWheelClockwiseOrdering;

private:
  vtkPlotRadViz(const vtkPlotRadViz&) = delete;
  void operator=(const vtkPlotRadViz&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

VTK_ABI_NAMESPACE_END
#endif // vtkPlotRadViz_h
