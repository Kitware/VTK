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

#include <functional> // For std::function
#include <vector>     // For modality groups

VTK_ABI_NAMESPACE_BEGIN
class vtkLookupTable;

class VTKFILTERSGENERAL_EXPORT vtkMultiVariateToRGBA : public vtkDataObjectAlgorithm
{
public:
  static vtkMultiVariateToRGBA* New();
  vtkTypeMacro(vtkMultiVariateToRGBA, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Angle in degrees to rotate the vertices coding the multivariate attributes
   * Default is 0 degrees.
   */
  vtkSetMacro(ColorWheelRotation, double);
  vtkGetMacro(ColorWheelRotation, double);

  ///@{
  /**
   * This filter computes generalized barycentric coordinates (GBC)
   * and uses that to compute colors (RGBA) that can be used to volume
   * render the data. If StoreGBC (default is false) is set to true we
   * store the computed attribute as an array named 'gbc'. If
   * StoreRGBA (default is true, in which case we store the computed
   * array named 'rgba') is set to false, we don't proceed to
   * computing the color.  Setting both parameters to false is an
   * error.  @sa vtkChartRadViz, vtkPlotRadViz
   */
  vtkSetMacro(StoreGBC, bool);
  vtkGetMacro(StoreGBC, bool);
  vtkSetMacro(StoreRGBA, bool);
  vtkGetMacro(StoreRGBA, bool);
  ///@}

  void SetModalityGroups(std::vector<std::vector<int>>& modalityGroups);
  std::vector<std::vector<int>> GetModalityGroups() VTK_FUTURE_CONST;

protected:
  vtkMultiVariateToRGBA();
  ~vtkMultiVariateToRGBA() override;

  double ColorWheelRotation = 0;
  bool StoreGBC = false;
  bool StoreRGBA = true;
  std::vector<std::vector<int>> ModalityGroups;
  std::function<double(double)> GlobalMin = nullptr;
  std::function<double(double)> GlobalMax = nullptr;

  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkMultiVariateToRGBA(const vtkMultiVariateToRGBA&) = delete;
  void operator=(const vtkMultiVariateToRGBA&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
