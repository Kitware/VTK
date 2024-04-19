// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSplitSharpEdgesPolyData
 * @brief   Split sharp edges in a polygonal mesh
 *
 * vtkSplitSharpEdgesPolyData is a filter that splits sharp edges. When sharp
 * edges are present, the edges are split and new duplicated points are generated
 * to prevent blurry edges (due to Gouraud shading), and give crisp (rendered) surface definition.
 */
#ifndef vtkSplitSharpEdgesPolyData_h
#define vtkSplitSharpEdgesPolyData_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSCORE_EXPORT vtkSplitSharpEdgesPolyData : public vtkPolyDataAlgorithm
{
public:
  static vtkSplitSharpEdgesPolyData* New();
  vtkTypeMacro(vtkSplitSharpEdgesPolyData, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify the angle that defines a sharp edge. If the difference in
   * angle across neighboring polygons is greater than this value, the
   * shared edge is considered "sharp".
   *
   * Default is 30 degrees.
   */
  vtkSetClampMacro(FeatureAngle, double, 0.0, 180.0);
  vtkGetMacro(FeatureAngle, double);
  ///@}

  ///@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   *
   * Default is DEFAULT_PRECISION.
   */
  vtkSetClampMacro(OutputPointsPrecision, int, SINGLE_PRECISION, DEFAULT_PRECISION);
  vtkGetMacro(OutputPointsPrecision, int);
  ///@}

protected:
  vtkSplitSharpEdgesPolyData();
  ~vtkSplitSharpEdgesPolyData() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkSplitSharpEdgesPolyData(const vtkSplitSharpEdgesPolyData&) = delete;
  void operator=(const vtkSplitSharpEdgesPolyData&) = delete;

  struct MarkAndSplitFunctor;

  double FeatureAngle;
  int OutputPointsPrecision;
};
VTK_ABI_NAMESPACE_END

#endif // vtkSplitSharpEdgesPolyData_h
