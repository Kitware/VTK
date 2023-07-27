// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkConvertToPointCloud
 * @brief   Convert any dataset to a point cloud
 *
 * This class convert any input dataset into a polydata point cloud
 * containing the same points and point data and either no cells, a single poly vertex cell
 * or as many vertex cell as they are points.
 *
 */

#ifndef vtkConvertToPointCloud_h
#define vtkConvertToPointCloud_h

#include "vtkFiltersPointsModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSPOINTS_EXPORT vtkConvertToPointCloud : public vtkPolyDataAlgorithm
{
public:
  static vtkConvertToPointCloud* New();
  vtkTypeMacro(vtkConvertToPointCloud, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  typedef enum CellGeneration
  {
    NO_CELLS = 0,
    POLYVERTEX_CELL = 1,
    VERTEX_CELLS = 2
  } CellGeneration;

  ///@{
  /**
   * Set/Get the cell generation mode.
   * Available modes are:
   * - NO_CELLS:
   * No cells are generated
   * - POLYVERTEX_CELL:
   * A single polyvertex cell is generated (default)
   * - VERTEX_CELLS:
   * One vertex cell by point, not efficient to generate
   */
  vtkSetMacro(CellGenerationMode, int);
  vtkGetMacro(CellGenerationMode, int);
  ///@}

protected:
  vtkConvertToPointCloud() = default;
  ~vtkConvertToPointCloud() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  int CellGenerationMode = POLYVERTEX_CELL;

private:
  vtkConvertToPointCloud(const vtkConvertToPointCloud&) = delete;
  void operator=(const vtkConvertToPointCloud&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
