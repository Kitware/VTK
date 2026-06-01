// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMultiVariateToRGBA
 * @brief   generalized histograms up to 4 dimensions
 *
 * vtkMultiVariateToRGBA takes a 3 component RGB image as
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

/**
 * Maps multivariate data (multi-component scalars and tables) to
 * color and opacity (RGBA).
 *
 * Uses
 * RadVolViz: An Information Display-Inspired Transfer Function Editor for Multivariate Volume
 * Visualization
 * Ayush Kumar, Xinyu Zhang, Huolin L. Xin, Hanfei Yan, Xiaojing Huang, Wei Xu,
 * and Klaus Mueller
 * and
 * RadViz Deluxe: An Attribute-Aware Display for Multivariate Data
 * Shenghui Cheng, Wei Xu and Klaus Mueller
 */
class VTKFILTERSGENERAL_EXPORT vtkMultiVariateToRGBA : public vtkDataObjectAlgorithm
{
public:
  static vtkMultiVariateToRGBA* New();
  vtkTypeMacro(vtkMultiVariateToRGBA, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Angle in degrees to rotate the vertices coding the multivariate attributes
   * Default is 0 degrees. This angle affects the GBC positions and the computed
   * colors.
   */
  vtkSetMacro(VerticesRotation, double);
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
   * If true, negates the Y coordinates for points, vertices and in
   * color computation. This is necessary to get the same RadViz graph
   * in VTK (origin bottom-left, Y grows up) and JavaScript (origin
   * top-left, Y grows down). Default is true.
   * @sa vtkPlotRadViz
   */
  vtkSetMacro(FlipY, bool);
  vtkGetMacro(FlipY, bool);
  ///@}

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
