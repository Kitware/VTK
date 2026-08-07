// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkPlotRadViz
 *
 * @brief Radial plot of multivariate data that shows the relations
 * among these variables.
 *
 * Radial plot of multivariate data (a tuple with multiple components
 * corresponds to a point) that shows the relations among these
 * variables (components). The plot accepts either a vtkTable or a
 * vtkDataSet as input. It uses vtkMultiVariateToRGBA to compute the
 * point generalized barycentric coordinates (GBC) and the color
 * (RGBA) for a tuple. The point position and color represent the
 * normalized value of each component in relation to each other while
 * the opacity represents the average normalized value for all
 * components. For example, in three-component data, both (0.1, 0.1,
 * 0.2) and (0.4, 0.4, 0.8) will have the same point position and
 * color but the second point will be 4 times more opaque.
 *
 * You can specify a partition of the components (see
 * ComponentsPartition) so that components in a partition are normalized
 * together. By default, all components are in one partition. This can
 * be used when value ranges of different components are very
 * different.
 *
 * You can show an HSV (hue, saturation and value) ColorWheel
 * (ColorWheelVisibility) that can be used to infer the color computed
 * for the point.
 *
 * Alternatively, you can use ScalarVisibility to show the color and
 * opacity on the points.
 *
 * The components are shown as vertices on the outer circle outline of
 * the plot. The vertices can be rotated using VerticesRotation, which
 * changes the points' positions and colors.
 *
 * Points' opacity can be further modified using a lens which modifies
 * the opacity of the points outside the lens (see
 * OutsideLensAlphaCap). The lens can be shown or hidden using
 * LensVisibility.
 *
 * The implementation is based on the paper:
 * RadViz Deluxe: An Attribute-Aware Display for Multivariate Data.
 * Shenghui Cheng, Wei Xu and Klaus Mueller.
 *
 * @sa vtkChartRadViz, vtkMultiVariateToRGBA
 * @sa vtkPlotParallelCoordinates
 *
 */

#ifndef vtkPlotRadViz_h
#define vtkPlotRadViz_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkNew.h"              // For vtkNew
#include "vtkPen.h"              // For SetSmartPointerMacro
#include "vtkPlot.h"             // For the base class
#include "vtkPoints2D.h"         // For GetSmartPointerMacro
#include "vtkSetGet.h"
#include "vtkSmartPointer.h"
#include "vtkTextProperty.h" // For SetSmartPointerMacro
#include "vtkUnsignedCharArray.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

#include <string> // For column names

VTK_ABI_NAMESPACE_BEGIN
class vtkContext2D;
class vtkColorSeries;
class vtkDataObject;
class vtkDataSet;
class vtkDataArray;
class vtkMultiVariateToRGBA;
class vtkPoints2D;
class vtkStringArray;
class vtkTextProperty;
class vtkUnsignedCharArray;

class VTKCHARTSCORE_EXPORT VTK_MARSHALAUTO vtkPlotRadViz : public vtkPlot
{
public:
  vtkTypeMacro(vtkPlotRadViz, vtkPlot);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkPlotRadViz* New();

  virtual void SetInputDataSet(vtkDataSet* dataset);
  void SetInputData(vtkTable* table) override;
  /**
   * Paint event for the item.
   */
  bool Paint(vtkContext2D* painter) override;

  ///@{
  /**
   * Set the visibility of the specified column/component by name. Only
   * visible columns/components are used to compute the plot. This is a
   * passthrough to the underlying vtkMultiVariateToRGBA; see
   * vtkMultiVariateToRGBA::SetColumnVisibility for details. Every
   * column/component of the input set with SetInputDataSet/SetInputData is
   * visible by default.
   */
  void SetColumnVisibility(const std::string& name, bool visible);
  void SetColumnVisibilityAll(bool visible);
  bool GetColumnVisibility(const std::string& name);
  vtkStringArray* GetVisibleColumns();
  void SetVisibleColumns(vtkStringArray* visColumns);
  ///@}

  ///@{
  /**
   * Set/Get the bounding box of the plot. Arguments 1 and 2 are the x and y coordinates
   * of the bottom corner. Arguments 3 and 4 are the width and height.
   * Set is called from vtkChartRadViz.
   */
  void SetDimensions(int arg1, int arg2, int arg3, int arg4);
  void SetDimensions(const int arg[4]);
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
   * Angle in degrees at which the starting color (red) is placed on the
   * color wheel. It is set to 90 to match colors computed by
   * vtkMultiVariateToRGBA.
   */
  vtkGetMacro(ColorWheelStartAngle, int);
  ///@}

  ///@{
  /**
   * Colors are laid out in clockwise order if true, counter-clockwise otherwise.
   * It is set to true to match colors computed by vtkMultiVariateToRGBA.
   */
  vtkGetMacro(ColorWheelClockwiseOrdering, bool);
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
  vtkSetMacro(ScalarVisibility, bool);
  vtkGetMacro(ScalarVisibility, bool);
  vtkBooleanMacro(ScalarVisibility, bool);
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
   * the lens intersects the GBC circle.
   */
  virtual void SetLensCenter(const double* center);
  vtkGetVector2Macro(LensCenter, double);
  /**
   * Radius of the lens in window space.
   */
  virtual void SetLensRadius(double radius);
  vtkGetMacro(LensRadius, double);
  /**
   * Controls how the lens circle outline is drawn.
   */
  vtkSetSmartPointerMacro(LensPen, vtkPen);
  vtkGetSmartPointerMacro(LensPen, vtkPen);
  /**
   * Alpha (opacity) cap for points outside the lens. Each point's alpha is
   * clamped to min(its alpha, OutsideLensAlphaCap). Default is 0
   * (points outside the lens fully transparent).
   */
  vtkSetMacro(OutsideLensAlphaCap, double);
  vtkGetMacro(OutsideLensAlphaCap, double);
  /**
   * Show the lens if true. Default is false.
   */
  vtkSetMacro(LensVisibility, bool);
  vtkGetMacro(LensVisibility, bool);
  vtkBooleanMacro(LensVisibility, bool);
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
   * lens is resized and set to false otherwise.
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
   * Center of the plot in window space.
   */
  const double* GetCenter() const;
  /**
   * Radius of the plot in window space.
   */
  double GetRadius() const;

  ///@{
  /**
   * Get the vtkTextProperty that governs how the vertex labels are displayed.
   */
  vtkSetSmartPointerMacro(LabelsProperties, vtkTextProperty);
  vtkGetSmartPointerMacro(LabelsProperties, vtkTextProperty);
  ///@}

  /**
   * Variables can be partitioned into several groups. After that,
   * all multivariate scalars in a group are normalized together.
   * By default, all variables (components) are in one group.
   *
   * For example, if indices `[0, 1, 2]` are the same partition, we
   * will normalize them all together, meaning we compute the min/max
   * over all three scalars.
   */
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_NOT_SUPPORTED)
  void SetComponentsPartition(const std::vector<std::vector<int>>& partition);
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_NOT_SUPPORTED)
  std::vector<std::vector<int>> GetComponentsPartition() VTK_FUTURE_CONST;

  /**
   * Returns the angle in degrees between vertex and mousePos, measured
   * at the plot's center. All these positions are in window space.
   */
  double GetAngle(double vertex[2], double mousePos[2]);

protected:
  vtkPlotRadViz();
  ~vtkPlotRadViz() override;
  void DrawColorWheel(vtkContext2D* painter);
  /**
   * Draw the points using the 'pen' attributes and 'color' instead of
   * the 'pen' color. The 'points' are in the [-1, 1] space.
   */
  vtkSmartPointer<vtkPoints2D> DrawColoredMarkers(
    vtkContext2D* painter, vtkDataArray* points, vtkPen* pen, vtkDataArray* color = nullptr);
  /**
   * Draw the points using the 'pen' attributes. The 'points' are in
   * the [-1, 1] space. If activeIndex >= 0, draw that point using the ActiveColor.
   */
  vtkSmartPointer<vtkPoints2D> DrawMarkers(
    vtkContext2D* painter, vtkDataArray* points, vtkPen* pen, int activeIndex = -1);
  void DrawLabels(vtkContext2D* painter, vtkDataArray* vertices, vtkPen* pen);
  void DrawLens(vtkContext2D* painter);
  void ComputeAutoLabels();
  vtkSmartPointer<vtkPoints2D> TransformPoints(vtkDataArray* points);
  vtkSmartPointer<vtkUnsignedCharArray> TransformColors(vtkDataArray* rgba);
  /**
   * Returns the horizontal and vertical text justification for xy coordinates.
   */
  static std::pair<int, int> GetJustification(double xy[2]);
  void ShiftLabelByPenWidth(double width, double transformedXy[2]);
  ///@{
  /**
   * Returns the arrays named 'vertices', 'gbc' and 'rgba' from a table or dataset, or throws
   * std::runtime_error if not found.
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

  std::vector<std::vector<int>> ComponentsPartition;
  int Dimensions[4] = { 0, 0, 0, 0 };
  /**
   * Controls how the vertices are drawn.
   * These are the markers on the circle for the variables.
   */
  vtkSmartPointer<vtkPen> VerticesPen = vtkSmartPointer<vtkPen>::New();
  /**
   * Controls how the lens circle outline is drawn.
   */
  vtkSmartPointer<vtkPen> LensPen = vtkSmartPointer<vtkPen>::New();
  /**
   * Pen used for active vertex, active lens circle or active lens center.
   * Only the color is used.
   */
  vtkColor4ub ActiveColor = vtkColor4ub(255, 0, 0, 255); // red
  /**
   * The index of the active vertex (mouse over it or rotating the
   * vertices by dragging that vertex), -1 when no vertex is active.
   */
  int ActiveVertexIndex = -1;
  /**
   * Set to true when the mouse is over the lens outline or when the
   * lens is resized and set to false otherwise.
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
  double OutsideLensAlphaCap = 0.0;
  /**
   * Show the lens if true.
   */
  bool LensVisibility = false;
  /**
   * Where the lens is drawn and its radius in window coordinates.
   */
  double LensCenter[2] = { 0.0, 0.0 };
  double LensRadius = 0.0;

  vtkSmartPointer<vtkPoints2D> Vertices;
  double VerticesRotation = 0.0;

  /**
   * The text properties associated with the vertex labels.
   */
  vtkSmartPointer<vtkTextProperty> LabelsProperties = vtkSmartPointer<vtkTextProperty>::New();
  int ColorWheelSegmentCount = 120;
  int ColorWheelStartAngle = 90;
  bool ColorWheelClockwiseOrdering = true;
  bool ColorWheelVisibility = true;
  bool ScalarVisibility = false;

  /**
   * Persistent filter used to compute the GBC/RGBA values, and that also
   * owns the column/component visibility configuration (see
   * SetColumnVisibility). Kept alive across Paint() calls so that the
   * visibility configuration and the vtkAlgorithm modified-time tracking
   * both persist between renders.
   */
  vtkNew<vtkMultiVariateToRGBA> ComputeGBC;

private:
  vtkPlotRadViz(const vtkPlotRadViz&) = delete;
  void operator=(const vtkPlotRadViz&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
};

VTK_ABI_NAMESPACE_END
#endif // vtkPlotRadViz_h
