// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMultiVariateToRGBA
 *
 * @brief Maps multivariate data to color and opacity.
 *
 * Maps multivariate data (multi-component point or cell scalars (AOS
 * array) in a vtkDataSet or multiple columns in a table (an SOA
 * array)) to color and opacity (RGBA). The filter can also generate
 * two additional set of point coordinates. First, point coordinates
 * for vertices associated with the components on the outline of a
 * circle centered at (0, 0) with radius 1 (these points form a
 * polygon). Second, points associated with the data (generalized
 * barycentric coordinates (GBC) inside the vertices polygon, scaled
 * to cover the circle) (see StoreRGBA, StoreVertices, StoreGBC).
 * The color and opacity can be used to volume render the dataset.
 *
 * The first vertex is at the top and the following vertices are placed
 * clockwise, in the same order as the colors of the color wheel, so the
 * first component is red (see VerticesClockwiseOrdering). Vertices can be
 * rotated (VerticesRotation), which will change the GBC positions and
 * colors.
 *
 * Points' opacity can be further modified using a lens (LensCenter,
 * LensRadius) which modifies the opacity of the points outside the
 * lens (see OutsideLensAlpha). When only the lens changes, the filter
 * reuses the results of its last execution and only recomputes the
 * opacity, so that the lens can be dragged interactively.
 *
 * You can specify a partition of the set of components in groups (subsets)
 * (see SetPartitionIndices) so that components in each group (subset) are
 * normalized together. By default, all components are in one
 * group. This can be used when value ranges of different
 * components are very different.
 *
 * The implementation is based on the papers:
 * RadVolViz: An Information Display-Inspired Transfer Function Editor for Multivariate Volume
 * Visualization
 * Ayush Kumar, Xinyu Zhang, Huolin L. Xin, Hanfei Yan, Xiaojing Huang, Wei Xu,
 * and Klaus Mueller
 * and
 * RadViz Deluxe: An Attribute-Aware Display for Multivariate Data
 * Shenghui Cheng, Wei Xu and Klaus Mueller
 */

#ifndef vtkMultiVariateToRGBA_h
#define vtkMultiVariateToRGBA_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkNew.h"                   // For vtkNew
#include "vtkSmartPointer.h"          // For vtkSmartPointer
#include "vtkTimeStamp.h"             // For vtkTimeStamp
#include "vtkWeakPointer.h"           // For vtkWeakPointer
#include <array>                      // For std::array
#include <string>                     // For column names
#include <vector>                     // For components partition

VTK_ABI_NAMESPACE_BEGIN
class vtkDoubleArray;
class vtkInformationVector;
class vtkMultiProcessController;
class vtkStringArray;

class VTKFILTERSPARALLEL_EXPORT vtkMultiVariateToRGBA : public vtkDataObjectAlgorithm
{
public:
  static vtkMultiVariateToRGBA* New();
  vtkTypeMacro(vtkMultiVariateToRGBA, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * This filter computes colors (RGBA) that can be used to volume
   * render the data. To do that, it computes several intermediate
   * arrays which include vertex positions used to encode variables and
   * generalized barycentric coordinates (GBC). All arrays are stored as vtkDoubleArray.
   *
   * If StoreVertices (default is false) is set to true,
   * we store the computed vertices as a field array named 'vertices'.
   *
   * If StoreGBC (default is false) is set to true, we
   * store the computed GBC as a point/cell array named 'gbc'.
   *
   * If StoreRGBA (default is true, in which case we store the computed point/cell
   * array named 'rgba') is set to false, we don't compute the color.
   *
   * If StoreA (default is false) is set to true, we store a point/cell
   * array named 'rgba' that only has its alpha (A) component computed; the
   * RGB part is initialized to 0. This is a cheaper alternative to StoreRGBA
   * when only opacity is needed. StoreRGBA and StoreA cannot both be true.
   *
   * Setting StoreGBC, StoreRGBA and StoreA all to false is an error.
   *
   * @sa vtkChartRadViz, vtkPlotRadViz
   */
  vtkSetMacro(StoreGBC, bool);
  vtkGetMacro(StoreGBC, bool);
  vtkSetMacro(StoreVertices, bool);
  vtkGetMacro(StoreVertices, bool);
  vtkSetMacro(StoreRGBA, bool);
  vtkGetMacro(StoreRGBA, bool);
  vtkSetMacro(StoreA, bool);
  vtkGetMacro(StoreA, bool);
  ///@}

  /**
   * Angle in degrees to rotate the vertices encoding the multivariate attributes.
   * Default is 0 degrees. This angle affects the GBC positions and the computed colors.
   * The rotation will be clamped to [0, 360).
   */
  virtual void SetVerticesRotation(double rotation);
  vtkGetMacro(VerticesRotation, double);

  ///@{
  /**
   * If true, the vertices are placed clockwise, otherwise counter-clockwise.
   * The first vertex is at the top in both cases, before VerticesRotation is
   * applied. Default is true: the vertices follow the colors of the color
   * wheel, so component i of n has the hue i / n and the first component is
   * red.
   */
  vtkSetMacro(VerticesClockwiseOrdering, bool);
  vtkGetMacro(VerticesClockwiseOrdering, bool);
  vtkBooleanMacro(VerticesClockwiseOrdering, bool);
  ///@}

  ///@{
  /**
   * Center of the lens in GBC space (unit circle centered at the origin).
   * The lens must intersect the GBC circle.
   */
  void SetLensCenter(const double* center);
  virtual void SetLensCenter(double centerX, double centerY);
  vtkGetVector2Macro(LensCenter, double);
  /**
   * Radius of the lens in GBC space (unit circle centered at the origin).
   * The lens must intersect the GBC circle.
   */
  virtual void SetLensRadius(double radius);
  vtkGetMacro(LensRadius, double);
  /**
   * Alpha (opacity) cap for points outside the lens. Each point's alpha is
   * clamped to min(its alpha, OutsideLensAlpha). Default is 0
   * (points outside the lens fully transparent).
   */
  virtual void SetOutsideLensAlpha(double alpha);
  vtkGetMacro(OutsideLensAlpha, double);
  /**
   * Clamp the lens circle (lensCenter, lensRadius) so that it intersects the
   * reference circle (center, radius). GetLensRadius returns the adjusted
   * radius; GetLensCenter returns the adjusted center.
   */
  static double GetLensRadius(
    double lensRadius, double radius, const double* lensCenter, const double* center);
  static std::array<double, 2> GetLensCenter(
    double lensRadius, double radius, const double* lensCenter, const double* center);
  ///@}

  ///@{
  /**
   * Convert hue, saturation and lightness (all in [0, 1]) to red, green
   * and blue (in [0, 1]) using the same conversion as the computed
   * colors. ColorLightness is the lightness used for the computed colors.
   * @sa vtkPlotRadViz
   */
  static void HSLToRGB(double h, double s, double l, double rgb[3]);
  static constexpr double ColorLightness = 0.55;
  ///@}

  ///@{
  /**
   * Describe the color wheel that matches the computed colors.
   * GetColorWheelStartAngle returns the angle at which red, the first
   * color, is placed: 90 degrees, measured counter-clockwise from the X
   * axis. GetColorWheelClockwiseOrdering returns true, as the colors
   * follow each other clockwise around the wheel.
   * @sa vtkPlotRadViz
   */
  int GetColorWheelStartAngle() { return 90; }
  bool GetColorWheelClockwiseOrdering() { return true; }
  ///@}

  ///@{
  /**
   * Set and get the controller.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

  ///@{
  /**
   * Custom ranges used as the global min/max when normalizing scalars,
   * instead of the range computed from the data (see NormalizeWorker), one
   * [min, max] range per partition group (see SetPartitionIndices). Since
   * these ranges are meant to be the global ranges, when they are set no
   * AllReduce is performed, even if the Controller is not null. Values
   * outside a custom range are clamped to it.
   *
   * CustomRange is a flat array, interleaved as [min0, max0, min1, max1,
   * ...]. An empty CustomRange (the default) means no custom range is set,
   * in which case the global min/max is computed from the data (and
   * reduced across processes if the Controller is not null). Otherwise,
   * CustomRange must hold exactly two values (one [min, max] pair) per
   * partition group.
   */
  void SetCustomRange(const std::vector<double>& customRange);
  std::vector<double> GetCustomRange() VTK_FUTURE_CONST;
  ///@}

  /**
   * The global [min, max] range actually used to normalize each partition
   * group during the last RequestData, in the same flat, interleaved
   * layout as CustomRange: CustomRange[i] when it was set for that group,
   * otherwise the range computed from the data (and reduced across
   * processes if the Controller is not null). Empty until RequestData has
   * run at least once.
   */
  std::vector<double> GetRange() VTK_FUTURE_CONST;

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
   * Set the visibility of the specified column/component by name (a table
   * column name, or a component name of the dataset array being
   * processed). Only visible (i.e. not hidden) columns/components are used
   * to compute the RGBA/GBC values, letting a subset of a multivariate
   * dataset be visualized. By default (an empty HiddenColumns list), every
   * column/component is visible.
   */
  void SetColumnVisibility(const std::string& name, bool visible);
  /**
   * Set the visibility of all columns/components. Passing true clears the
   * hidden list, so every column/component of the current input is
   * visible. Passing false marks every column/component of the current
   * input as hidden, so the filter has nothing to process.
   */
  void SetColumnVisibilityAll(bool visible);
  /**
   * Get the visibility of the specified column/component. Returns true
   * unless the column/component has been explicitly marked hidden.
   */
  bool GetColumnVisibility(const std::string& name);
  /**
   * Get the list of the columns/components of the current input that are
   * visible (i.e. not hidden), in their natural order. Empty when the
   * current input has no visible column/component.
   */
  virtual vtkStringArray* GetVisibleColumns();
  /**
   * Set the list of visible columns/components: every column/component of
   * the current input not in this list is marked hidden. Passing a null or
   * empty array marks every column/component of the current input hidden.
   */
  virtual void SetVisibleColumns(vtkStringArray* visColumns);
  /**
   * Returns the names of the columns/components that will actually be used
   * to compute the RGBA/GBC values: the columns/components of the current
   * input that are visible (i.e. not hidden), in their natural order.
   */
  vtkSmartPointer<vtkStringArray> GetProcessedColumnNames();
  ///@}

  ///@{
  /**
   * Data type of the 'rgba' array: VTK_DOUBLE (the default) or VTK_FLOAT.
   * VTK_FLOAT uses half the memory and is faster to upload as a volume
   * texture, which matters when the lens is dragged.
   */
  vtkSetClampMacro(OutputScalarType, int, VTK_FLOAT, VTK_DOUBLE);
  vtkGetMacro(OutputScalarType, int);
  ///@}

  /**
   * Overridden to tell lens-only changes apart from all other changes.
   */
  void Modified() override;

protected:
  vtkMultiVariateToRGBA();
  ~vtkMultiVariateToRGBA() override;

  vtkMultiProcessController* Controller = nullptr;
  double VerticesRotation = 0;
  bool VerticesClockwiseOrdering = true;
  bool StoreGBC = false;
  bool StoreVertices = false;
  bool StoreRGBA = true;
  bool StoreA = false;
  std::vector<int> PartitionIndices;
  std::vector<int> PartitionOffsets;
  /**
   * Where the lens is drawn and its radius
   */
  double LensCenter[2] = { 0.0, 0.0 };
  double LensRadius = 1.0;
  /**
   * Alpha cap for points outside the lens.
   */
  double OutsideLensAlpha = 0.0;
  int OutputScalarType = VTK_DOUBLE;
  /**
   * Custom ranges used as the global min/max when normalizing scalars,
   * interleaved as [min0, max0, min1, max1, ...], one pair per partition
   * group. Empty means no custom range is set.
   */
  std::vector<double> CustomRange;
  /**
   * The global [min, max] range actually used to normalize each partition
   * group during the last RequestData, in the same flat, interleaved
   * layout as CustomRange. Set at the end of RequestData.
   */
  std::vector<double> Range;
  /**
   * Strongly owned internal data for the column/component visibility,
   * storing the names of the columns/components that are hidden. An empty
   * array (the default) means every column/component is visible.
   */
  vtkNew<vtkStringArray> HiddenColumns;

  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkMultiVariateToRGBA(const vtkMultiVariateToRGBA&) = delete;
  void operator=(const vtkMultiVariateToRGBA&) = delete;
  std::pair<vtkDataSetAttributes*, vtkSmartPointer<vtkDataArray>> getAttributesAndScalars(
    vtkDataObject* input, vtkInformationVector** inputVector);
  /**
   * Returns the names of every column (table) or component (dataset array)
   * found in the current input, in their natural order.
   */
  vtkSmartPointer<vtkStringArray> GetAllColumnNames();
  /**
   * Returns a read-only view exposing only the visible (i.e. not hidden)
   * named components of 'array' (or 'array' itself, unmodified, if every
   * component of 'array' happens to be visible).
   */
  vtkSmartPointer<vtkDataArray> GetVisibleComponentsArray(vtkDataArray* array);
  /**
   * Backing storage for the pointer returned by GetVisibleColumns().
   */
  vtkSmartPointer<vtkStringArray> VisibleColumnsCache;

  ///@{
  /**
   * Lens-only changes call LensModified() instead of Modified(), which
   * keeps the results of the last execution (Cached*) valid.
   */
  void LensModified();
  bool InLensModified = false;
  vtkTimeStamp NonLensModifiedTime;
  vtkSmartPointer<vtkDoubleArray> CachedGBC;
  vtkSmartPointer<vtkDoubleArray> CachedRGBA; // before the lens
  vtkSmartPointer<vtkDoubleArray> CachedVertices;
  vtkWeakPointer<vtkDataObject> CachedInput;
  vtkTimeStamp CacheTime;
  ///@}
  /**
   * Returns a new 'rgba' array: CachedRGBA with the lens applied.
   */
  vtkSmartPointer<vtkDataArray> ApplyLens();
};

VTK_ABI_NAMESPACE_END
#endif
