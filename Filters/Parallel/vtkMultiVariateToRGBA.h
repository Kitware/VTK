// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMultiVariateToRGBA
 *
 * @brief Maps multivariate data to color and opacity.
 *
 * Maps multivariate data (multi-component point or cell scalars in a
 * vtkDataSet or one component numeric columns, of any types, in a table)
 * to color and opacity (RGBA). The filter can also generate
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
#include <set>                        // For variable visibility
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
   * If the lens does not intersect the GBC circle, its center is moved
   * toward the circle when the filter executes, so LensCenter and
   * LensRadius can be set in any order.
   */
  void SetLensCenter(const double* center);
  virtual void SetLensCenter(double centerX, double centerY);
  vtkGetVector2Macro(LensCenter, double);
  /**
   * Radius of the lens in GBC space (unit circle centered at the origin).
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
   * Swap inside and outside: cap the alpha of the points inside the lens
   * instead. Default is false.
   */
  virtual void SetInvertLens(bool invert);
  vtkGetMacro(InvertLens, bool);
  vtkBooleanMacro(InvertLens, bool);
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
   * partition group, with min <= max. A pair with min == max, which GetRange
   * reports for a group with a constant value, maps every value of that
   * group to 0, the same as a constant group without a custom range.
   */
  void SetCustomRange(const std::vector<double>& customRange);
  std::vector<double> GetCustomRange() VTK_FUTURE_CONST;
  ///@}

  ///@{
  /**
   * Element-wise, ClientServer-wrappable equivalents of SetCustomRange().
   * ClearCustomRange() empties CustomRange and AddCustomRange() appends
   * one [min, max] pair, for the next partition group.
   */
  void ClearCustomRange();
  void AddCustomRange(double min, double max);
  ///@}

  /**
   * The global [min, max] range actually used to normalize each partition
   * group during the last RequestData, in the same flat, interleaved
   * layout as CustomRange: CustomRange[i] when it was set for that group,
   * otherwise the range computed from the data (and reduced across
   * processes if the Controller is not null). For a group without values in
   * any process, the computed range is [VTK_DOUBLE_MAX, VTK_DOUBLE_MIN], the
   * range of an empty vtkDataArray. Empty until RequestData has run at least
   * once.
   */
  std::vector<double> GetRange() VTK_FUTURE_CONST;

  ///@{
  /**
   * Variables can be partitioned into several groups. After that,
   * all multivariate scalars in a group are normalized together.
   * By default (both PartitionIndices and PartitionOffsets empty), all
   * variables (components) are in one group; setting only one of them is
   * an error.
   *
   * The component indices refer to the visible variables only (see
   * SetVariableVisibility), in their input order: with variables a, b, c, d
   * and b hidden, index 1 is c. Hiding or showing a variable changes the
   * number of components, so a partition set before is then invalid.
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
   * Variable visibility. Variables are the columns of a table or the
   * components of the dataset array processed, and only the visible ones
   * are used to compute the GBC/RGBA values, letting a subset of a
   * multivariate dataset be visualized. A variable is set visible or hidden
   * by name or by index; unnamed variables are named after their index
   * ("0", "1", ...). Names are unique: a name already used, by a named
   * variable or an earlier variable, gets the first free " (2)", " (3)",
   * ... suffix, so a column named "1" keeps its name and an unnamed column
   * 1 is named "1 (2)". By default, every variable is visible.
   *
   * The visibility is stored as a default visibility and the variables
   * that differ from it, which do not depend on the input, so they can be
   * set before the input is connected. Names and indices do not mix: while
   * variables differ from the default by name, setting the visibility by
   * index is an error, and the other way around. SetVariableVisibilityAll(),
   * SetVisibleVariables(), SetVisibleVariableIndices() and
   * ResetVisibleVariableIndices() start over. Indices outside the input are
   * ignored, and negative indices are an error.
   */
  void SetVariableVisibility(const std::string& name, bool visible);
  void SetVariableVisibility(int index, bool visible);
  /**
   * Set the visibility of all variables, including the ones of inputs set
   * later. Passing false leaves the filter with nothing to process until
   * variables are made visible again.
   */
  void SetVariableVisibilityAll(bool visible);
  /**
   * Get the visibility of a variable of the current input, by name or index.
   * A variable that is not in the current input, or any variable when there
   * is no input, is not visible.
   */
  bool GetVariableVisibility(const std::string& name);
  bool GetVariableVisibility(int index);
  /**
   * Get the names of the visible variables of the current input, in their
   * natural order. Empty when the current input has no visible variable.
   * The filter owns the array returned, and every call refills it, so copy
   * it to keep the names of an earlier call.
   */
  virtual vtkStringArray* GetVisibleVariables();
  /**
   * Set the list of visible variables by name: every variable not in this
   * list is hidden. Passing a null or empty array hides every variable.
   */
  virtual void SetVisibleVariables(vtkStringArray* names);
  /**
   * Set the list of visible variables by index: every variable not in this
   * list is hidden. Passing an empty list hides every variable.
   */
  void SetVisibleVariableIndices(const std::vector<int>& indices);
  /**
   * Get the indices of the visible variables of the current input, in
   * increasing order.
   */
  std::vector<int> GetVisibleVariableIndices();
  /**
   * Build the list of visible variables by index, for the ParaView server
   * manager XML, which calls a method without arguments (clean_command) and
   * then a method once per index (command), like AddPartitionIndex(). From
   * C++ or Python, use SetVisibleVariableIndices() or
   * SetVariableVisibility(int, bool) instead.
   *
   * ResetVisibleVariableIndices() makes every variable visible, the same as
   * SetVariableVisibilityAll(true), so an empty list shows every variable.
   * The first AddVisibleVariableIndex() after it, or on a new filter, hides
   * every other variable, so only the added ones are visible. After any other
   * visibility setter, AddVisibleVariableIndex(i) is the same as
   * SetVariableVisibility(i, true).
   */
  void ResetVisibleVariableIndices();
  void AddVisibleVariableIndex(int index);
  /**
   * Returns the names of every variable of the current input, visible or
   * not, in their natural order.
   */
  vtkSmartPointer<vtkStringArray> GetAllVariableNames();
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
  bool InvertLens = false;
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
   * Variable visibility: a variable is visible when
   * VariablesVisibleByDefault is true, unless its name or index is in the
   * exceptions, which flip it. At most one of the exceptions is not empty.
   * For example, for variables a, b, c: SetVariableVisibility("b", false)
   * gives {true, {b}}, so a and c are visible; SetVisibleVariables({a, c})
   * gives {false, {a, c}}, the same visible variables.
   */
  bool VariablesVisibleByDefault = true;
  std::set<std::string> NameVisibilityExceptions;
  std::set<int> IndexVisibilityExceptions;
  /**
   * True after ResetVisibleVariableIndices() or on a new filter, until the
   * next visibility setter: the next AddVisibleVariableIndex() hides every
   * other variable. It does not change the visibility, so it is not part of
   * the MTime.
   */
  bool StartVisibleIndexList = true;

  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkMultiVariateToRGBA(const vtkMultiVariateToRGBA&) = delete;
  void operator=(const vtkMultiVariateToRGBA&) = delete;
  /**
   * Returns the attributes that receive the output arrays and the scalars
   * to process, keeping only the visible columns or components. For a table,
   * the attributes are its row data, and the scalars combine its visible
   * columns, which must be one component numeric arrays, into one array with
   * a component per column. For a dataset, they are the point or cell data
   * that hold the input array to process, and the scalars are that array.
   * On an error, which is reported, the scalars are null; the attributes are
   * also null if the dataset lacks the array or the input is neither a table
   * nor a dataset.
   */
  std::pair<vtkDataSetAttributes*, vtkSmartPointer<vtkDataArray>> GetAttributesAndScalars(
    vtkDataObject* input, vtkInformationVector** inputVector);
  /**
   * Returns a read-only view exposing only the visible components of
   * 'array' (or 'array' itself, unmodified, if every
   * component of 'array' happens to be visible).
   */
  vtkSmartPointer<vtkDataArray> GetVisibleComponentsArray(vtkDataArray* array);
  /**
   * Returns true if the variable with this index and name is visible.
   */
  bool IsVariableVisible(int index, const std::string& name) const;
  /**
   * Backing storage for the pointer returned by GetVisibleVariables().
   */
  vtkSmartPointer<vtkStringArray> VisibleVariablesCache;

  ///@{
  /**
   * Lens-only changes call LensModified() instead of Modified(), which
   * keeps the results of the last execution (Cached*) valid.
   */
  void LensModified();
  bool InLensModified = false;
  vtkTimeStamp NonLensModifiedTime;
  vtkSmartPointer<vtkDoubleArray> CachedGBC;
  // before the lens; CachedRGB is null for StoreA
  vtkSmartPointer<vtkDoubleArray> CachedRGB;
  vtkSmartPointer<vtkDoubleArray> CachedAlpha;
  vtkSmartPointer<vtkDoubleArray> CachedVertices;
  vtkWeakPointer<vtkDataObject> CachedInput;
  // association of the attributes that received the cached arrays
  int CachedAssociation = 0;
  vtkTimeStamp CacheTime;
  ///@}
  /**
   * Returns a new 'rgba' array: CachedRGB (or 0 for StoreA) and
   * CachedAlpha with the lens applied.
   */
  vtkSmartPointer<vtkDataArray> ApplyLens();
};

VTK_ABI_NAMESPACE_END
#endif
