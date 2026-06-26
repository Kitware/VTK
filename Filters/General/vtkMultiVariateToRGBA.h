// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMultiVariateToRGBA
 *
 * @brief Maps multivariate data to color and opacity.
 *
 * Maps multivariate data (multi-component scalars (AOS array) in a vtkDataSet or
 * multiple columns in a table (an SOA array)) to color and opacity (RGBA).
 * Uses
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
#include <vector>                    // For modality groups

VTK_ABI_NAMESPACE_BEGIN
class vtkLookupTable;
class vtkInformationVector;

class VTKFILTERSGENERAL_EXPORT vtkMultiVariateToRGBA : public vtkDataObjectAlgorithm
{
public:
  static vtkMultiVariateToRGBA* New();
  vtkTypeMacro(vtkMultiVariateToRGBA, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Angle in degrees to rotate the vertices encoding the multivariate attributes.
   * Default is 0 degrees. This angle affects the GBC positions and the computed colors.
   * The rotation will be clamped to [0, 360).
   */
  virtual void SetVerticesRotation(double rotation);
  vtkGetMacro(VerticesRotation, double);

  ///@{
  /**
   *
   * This filter computes colors (RGBA) that can be used to volume
   * render the data. To do that, it computes several intermediate
   * arrays which include vertices positions used to encode variables and
   * generalized barycentric coordinates (GBC). All arrays are stored as vtkDoubleArray.
   *
   * If StoreVertices (default is false) is set to true
   * we store the computed vertices as a field array named 'vertices'.
   *
   * If StoreGBC (default is false) is set to true we
   * store the computed attribute as an array named 'gbc'.
   *
   * If StoreRGBA (default is true, in which case we store the computed
   * array named 'rgba') is set to false, we don't proceed to
   * computing the color.  Setting both parameters to false is an
   * error.
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

  ///@{
  /**
   * Center of the lens in GBC space (unit circle centered at the origin).
   * The lens must intersect the GBC circle.
   */
  virtual void SetLensCenter(const float* center);
  vtkGetVector2Macro(LensCenter, float);
  /**
   * Radius of the lens in GBC space (unit circle centered at the origin).
   * The lens must intersect the GBC circle.
   */
  virtual void SetLensRadius(float radius);
  vtkGetMacro(LensRadius, float);
  /**
   * Clamp the lens circle (lensCenter, lensRadius) so it intersects the
   * reference circle (center, radius). GetLensRadius returns the adjusted
   * radius; GetLensCenter returns the adjusted center.
   */
  static float GetLensRadius(
    float lensRadius, float radius, const float* lensCenter, const float* center);
  static std::array<float, 2> GetLensCenter(
    float lensRadius, float radius, const float* lensCenter, const float* center);
  ///@}

  ///@{
  /**
   * If true, negates the Y coordinates for points, vertices and in
   * color computation. This is necessary to get the same RadViz graph
   * in VTK (origin bottom-left, Y grows up) and JavaScript (origin
   * top-left, Y grows down). Default is true.
   * @sa vtkPlotRadViz
   */
  vtkSetMacro(FlipY, bool);
  vtkGetMacro(FlipY, bool);
  ///@}

  /**
   * Variables can be grouped in several groups. After that,
   * all multivariate scalars in a group are normalized together.
   *
   * For example, if indices `[0, 1, 2]` are the same modality, we
   * will normalize them all together, meaning we compute the min/max
   * over all three scalars.
   */
  void SetModalityGroups(std::vector<std::vector<int>>& modalityGroups);
  std::vector<std::vector<int>> GetModalityGroups() VTK_FUTURE_CONST;

protected:
  vtkMultiVariateToRGBA();
  ~vtkMultiVariateToRGBA() override;

  double VerticesRotation = 0;
  bool StoreGBC = false;
  bool StoreVertices = false;
  bool StoreRGBA = true;
  bool FlipY = true;
  std::vector<std::vector<int>> ModalityGroups;
  std::function<double(double)> GlobalMin = nullptr;
  std::function<double(double)> GlobalMax = nullptr;
  /**
   * Where the lens is drawn and its radius
   */
  float LensCenter[2];
  float LensRadius;

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
