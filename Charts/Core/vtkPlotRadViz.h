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
 * You can specify a partition of the set of components in groups (subsets)
 * (see SetPartitionIndices) so that components in each group (subset) are
 * normalized together. By default, all components are in one
 * group. This can be used when value ranges of different
 * components are very different.
 *
 * You can show an HSL (hue, saturation and lightness=0.55) ColorWheel
 * (ColorWheelVisibility) that shows the color computed for a point at
 * that position using the same computation as for vtkMultiVariateToRGBA.
 *
 * Alternatively, you can use ScalarVisibility to show the color on
 * the points. If ScalarVisibility is off, all points use the color of
 * the pen. The opacity of the points is computed from the data in both
 * cases, the opacity of the pen is not used.
 *
 * The components are shown as vertices on the outer circle outline of
 * the plot. The first vertex is at the top and the following vertices
 * are placed clockwise, in the same order as the colors of the color
 * wheel (see VerticesClockwiseOrdering). The vertices can be rotated
 * using VerticesRotation, which changes the points' positions and colors.
 *
 * Points' opacity can be further modified using a lens which modifies
 * the opacity of the points outside the lens (see
 * OutsideLensAlpha). By default the lens does not change the
 * points' opacity. The lens can be shown or hidden using
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
class vtkDoubleArray;
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
  using vtkPlot::SetInputData;
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
   * Number of concentric rings used when drawing the color wheel. More
   * rings produce a smoother gradient from the center outward. The
   * default is 16.
   */
  vtkSetMacro(ColorWheelRingCount, int);
  vtkGetMacro(ColorWheelRingCount, int);
  ///@}

  ///@{
  /**
   * Angle in degrees at which the starting color (red) is placed on the
   * color wheel. It is read from vtkMultiVariateToRGBA, so that the color
   * wheel matches the colors computed for the points.
   */
  int GetColorWheelStartAngle();
  ///@}

  ///@{
  /**
   * Colors are laid out in clockwise order if true, counter-clockwise otherwise.
   * It is read from vtkMultiVariateToRGBA, so that the color wheel matches
   * the colors computed for the points.
   */
  bool GetColorWheelClockwiseOrdering();
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
   * If off, the points use the color of the pen. The opacity of the points
   * is the computed one (A) in both cases. Default is off.
   */
  vtkSetMacro(ScalarVisibility, bool);
  vtkGetMacro(ScalarVisibility, bool);
  vtkBooleanMacro(ScalarVisibility, bool);
  ///@}

  ///@{
  /**
   * Width in pixels of the black outline drawn around the data points
   * and the vertices. The outline is drawn inside the
   * marker, so it does not change the marker size (the pen width).
   * All outlines are drawn before all fills, so overlapping markers
   * share one outline. Default is 1, use 0 for no outline.
   */
  vtkSetClampMacro(MarkerOutlineWidth, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(MarkerOutlineWidth, double);
  ///@}

  ///@{
  /**
   * Controls how the vertices are drawn. Default is white, with a
   * width (marker diameter) of 10.
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
   * If true, the vertices are placed clockwise, otherwise counter-clockwise.
   * The first vertex is at the top in both cases, before VerticesRotation is
   * applied. Default is true: the vertices follow the colors of the color
   * wheel.
   * @sa vtkMultiVariateToRGBA
   */
  vtkSetMacro(VerticesClockwiseOrdering, bool);
  vtkGetMacro(VerticesClockwiseOrdering, bool);
  vtkBooleanMacro(VerticesClockwiseOrdering, bool);
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
   * Controls how the lens circle outline is drawn. Its width is also the
   * size of the lens center dot, which is shown only when
   * ActiveLensInterior is true, using ActiveColor.
   * Default is half-transparent red, with a width of 8.
   */
  vtkSetSmartPointerMacro(LensPen, vtkPen);
  vtkGetSmartPointerMacro(LensPen, vtkPen);
  /**
   * Alpha (opacity) cap for points outside the lens. Each point's alpha is
   * clamped to min(its alpha, OutsideLensAlpha). Default is 1
   * (points outside the lens keep their computed opacity). Set a lower
   * value to fade or hide the points outside the lens.
   */
  vtkSetMacro(OutsideLensAlpha, double);
  vtkGetMacro(OutsideLensAlpha, double);
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
   * lens is dragged and set to false otherwise. The lens center dot is
   * shown only when this is true.
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
   * Default is bold, black Arial with a font size of 16.
   */
  vtkSetSmartPointerMacro(LabelsProperties, vtkTextProperty);
  vtkGetSmartPointerMacro(LabelsProperties, vtkTextProperty);
  ///@}

  ///@{
  /**
   * Variables can be partitioned into several groups. After that,
   * all multivariate scalars in a group are normalized together.
   * By default, all variables (components) are in one group.
   *
   * PartitionIndices holds component indices for each group, with groups
   * stored contiguously. PartitionOffsets holds, for each group, the
   * offset of the (last_element + 1) of that group in PartitionIndices,
   * in the order the groups appear in PartitionIndices.
   *
   * For example, to normalize indices `[0, 1, 2]` together and `[3, 4]`
   * together, set PartitionIndices to `[0, 1, 2, 3, 4]` and PartitionOffsets to
   * `[3, 5]`.
   */
  void SetPartitionIndices(const std::vector<int>& partitionIndices);
  std::vector<int> GetPartitionIndices() VTK_FUTURE_CONST;
  void SetPartitionOffsets(const std::vector<int>& partitionOffsets);
  std::vector<int> GetPartitionOffsets() VTK_FUTURE_CONST;
  ///@}

  ///@{
  /**
   * Element-wise, ClientServer-wrappable equivalents of SetPartitionIndices()
   * and SetPartitionOffsets(). The ParaView server manager XML cannot invoke
   * a method taking a std::vector argument, so it instead calls
   * ClearPartitionIndices()/ClearPartitionOffsets() once to empty the list,
   * followed by one AddPartitionIndex()/AddPartitionOffset() call per value.
   */
  void ClearPartitionIndices();
  void AddPartitionIndex(int index);
  void ClearPartitionOffsets();
  void AddPartitionOffset(int offset);
  ///@}

  ///@{
  /**
   * Custom ranges used as the global min/max when normalizing scalars,
   * instead of the range computed from the data, one [min, max] range per
   * partition group. This is a passthrough to the underlying
   * vtkMultiVariateToRGBA; see vtkMultiVariateToRGBA::SetCustomRange for
   * details, including the flat [min0, max0, min1, max1, ...] layout.
   *
   * This has to be set when running in ParaView Catalyst to enable
   * using a global range for all ranks. vtkMultiVariateToRGBA::GetRange
   * can be used to compute the global range from data.
   */
  void SetCustomRange(const std::vector<double>& customRange);
  std::vector<double> GetCustomRange() VTK_FUTURE_CONST;
  ///@}

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
  /**
   * Recomputes the plot circle's center and radius from Dimensions,
   * shrinking the radius so that there is enough room around the circle
   * for the vertex labels (measured using LabelsProperties and painter)
   * to fit within Dimensions without being clipped. This keeps the lens
   * position and size proportional to the plot circle when the radius
   * changes.
   */
  void UpdateCenterAndRadius(vtkContext2D* painter);
  vtkSmartPointer<vtkPoints2D> TransformPoints(vtkDataArray* points);
  vtkSmartPointer<vtkUnsignedCharArray> TransformColors(vtkDataArray* rgba);
  /**
   * Returns a per-point RGBA array using 'pen' color for RGB and 'rgba's A
   * component for alpha. Used to apply the already computed opacity
   * (including the lens's OutsideLensAlpha) to a single-color plot when
   * ScalarVisibility is off.
   */
  vtkSmartPointer<vtkDoubleArray> ApplyPenColor(vtkPen* pen, vtkDataArray* rgba);
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

  std::vector<int> PartitionIndices;
  std::vector<int> PartitionOffsets;
  /**
   * Custom ranges used as the global min/max when normalizing scalars,
   * interleaved as [min0, max0, min1, max1, ...], one pair per partition
   * group. Empty means no custom range is set.
   */
  std::vector<double> CustomRange;
  int Dimensions[4] = { 0, 0, 0, 0 };
  /**
   * Controls how the vertices are drawn.
   * These are the markers on the circle for the variables.
   */
  vtkSmartPointer<vtkPen> VerticesPen = vtkSmartPointer<vtkPen>::New();
  /**
   * Width of the black outline of the markers.
   */
  double MarkerOutlineWidth = 1.0;
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
  double OutsideLensAlpha = 1.0;
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
  bool VerticesClockwiseOrdering = true;

  /**
   * The text properties associated with the vertex labels.
   */
  vtkSmartPointer<vtkTextProperty> LabelsProperties = vtkSmartPointer<vtkTextProperty>::New();
  int ColorWheelSegmentCount = 120;
  int ColorWheelRingCount = 16;
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
