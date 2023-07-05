// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkExtractCellsAlongPolyLine
 * @brief   Extracts input cells that are intersected by a list of input lines or poly lines
 *
 * This filter extracts input cells that are intersected by a list of input lines or poly lines.
 * The lines / poly lines input is set on input port 1, as a source connection, while the input data
 * set on which cells are extracted is to be set on input port 0. The produced output is a
 * `vtkUnstructuredGrid`. Input lines can be either set inside a `vtkPolyData` or a
 * `vtkUnstructuredGrid`. If the input type has an explicit geometry, i.e. the input is not a
 * `vtkPointSet`, then the user can set the output
 * points precision using `OutputPointsPrecisions`. Otherwise, the point precision is set to be the
 * same as the input.
 *
 * This filter uses multi-threading if available.
 */

#ifndef vtkExtractCellsAlongPolyLine_h
#define vtkExtractCellsAlongPolyLine_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSCORE_EXPORT vtkExtractCellsAlongPolyLine : public vtkUnstructuredGridAlgorithm
{
public:
  vtkTypeMacro(vtkExtractCellsAlongPolyLine, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkExtractCellsAlongPolyLine* New();

  /**
   * Set the source for creating the lines to probe from. Only cells of type VTK_LINE or
   * VTK_POLY_LINE will be processed.
   */
  virtual void SetSourceConnection(vtkAlgorithmOutput* input);

  ///@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings. OutputPointsPrecision is DEFAULT_PRECISION
   * by default. However, if the input is polymorphic to `vtkPointSet`, then
   * the points precision of the input's points is used instead.
   */
  vtkSetMacro(OutputPointsPrecision, int);
  vtkGetMacro(OutputPointsPrecision, int);
  ///@}

protected:
  vtkExtractCellsAlongPolyLine();
  ~vtkExtractCellsAlongPolyLine() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  int OutputPointsPrecision;

private:
  vtkExtractCellsAlongPolyLine(const vtkExtractCellsAlongPolyLine&) = delete;
  void operator=(const vtkExtractCellsAlongPolyLine&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
