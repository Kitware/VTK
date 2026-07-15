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
 * barycentric coordinates (GBC) in the vertices polygon) (StoreGBC,
 * StoreVertices, StoreRGBA).
 *
 * Vertices can be rotated (VerticesRotation), which will change the GBC
 * positions and colors.
 *
 * Points' opacity can be further modified using a lens (LensCenter,
 * LensRadius) which modifies the opacity of the points outside the
 * lens (see OutsideLensAlphaCap).
 *
 * You can specify a partition of the components (see
 * ComponentsPartition) so that components in a partition are normalized
 * together. By default, all components are in one partition. This can
 * be used when value ranges of different components are very
 * different.
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
#include "vtkFiltersGeneralModule.h" // For export macro
#include <functional>                // For std::function
#include <vector>                    // For components partition

VTK_ABI_NAMESPACE_BEGIN
class vtkLookupTable;
class vtkInformationVector;

class VTKFILTERSGENERAL_EXPORT vtkMultiVariateToRGBA : public vtkDataObjectAlgorithm
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
   * store the computed GBC as an array named 'gbc'.
   *
   * If StoreRGBA (default is true, in which case we store the computed
   * array named 'rgba') is set to false, we don't compute the color.
   * Setting both parameters to false is an error.
   *
   * @sa vtkChartRadViz, vtkPlotRadViz
   */
  vtkSetMacro(StoreGBC, bool);
  vtkGetMacro(StoreGBC, bool);
  vtkSetMacro(StoreVertices, bool);
  vtkGetMacro(StoreVertices, bool);
  vtkSetMacro(StoreRGBA, bool);
  vtkGetMacro(StoreRGBA, bool);
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
   * clamped to min(its alpha, OutsideLensAlphaCap). Default is 1 (no change).
   * Set to 0 to make points outside the lens fully transparent.
   */
  vtkSetMacro(OutsideLensAlphaCap, double);
  vtkGetMacro(OutsideLensAlphaCap, double);
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
   * If true, negates the Y coordinate for points and vertices, and
   * when computing colors. This is necessary to get the same RadViz graph
   * in VTK (origin bottom-left, Y grows up) and JavaScript (origin
   * top-left, Y grows down). Default is true.
   * @sa vtkPlotRadViz
   */
  vtkSetMacro(FlipY, bool);
  vtkGetMacro(FlipY, bool);
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
  void SetComponentsPartition(const std::vector<std::vector<int>>& partition);
  std::vector<std::vector<int>> GetComponentsPartition() VTK_FUTURE_CONST;

protected:
  vtkMultiVariateToRGBA();
  ~vtkMultiVariateToRGBA() override;

  double VerticesRotation = 0;
  bool StoreGBC = false;
  bool StoreVertices = false;
  bool StoreRGBA = true;
  bool FlipY = true;
  std::vector<std::vector<int>> ComponentsPartition;
  /**
   * Functions to compute the global min/max given the local min/max.
   */
  std::function<double(double)> GlobalMin = nullptr;
  std::function<double(double)> GlobalMax = nullptr;
  /**
   * Where the lens is drawn and its radius
   */
  double LensCenter[2] = { 0.0, 0.0 };
  double LensRadius = 1.0;
  /**
   * Alpha cap for points outside the lens.
   */
  double OutsideLensAlphaCap = 1.0;

  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkMultiVariateToRGBA(const vtkMultiVariateToRGBA&) = delete;
  void operator=(const vtkMultiVariateToRGBA&) = delete;
  std::pair<vtkDataSetAttributes*, vtkSmartPointer<vtkDataArray>> getAttributesAndScalars(
    vtkDataObject* input, vtkInformationVector** inputVector);
};

VTK_ABI_NAMESPACE_END
#endif
