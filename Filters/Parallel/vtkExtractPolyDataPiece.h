// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkExtractPolyDataPiece
 * @brief   Return specified piece, including specified
 * number of ghost levels.
 */

#ifndef vtkExtractPolyDataPiece_h
#define vtkExtractPolyDataPiece_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkIdList;
class vtkIntArray;

class VTKFILTERSPARALLEL_EXPORT vtkExtractPolyDataPiece : public vtkPolyDataAlgorithm
{
public:
  static vtkExtractPolyDataPiece* New();
  vtkTypeMacro(vtkExtractPolyDataPiece, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Turn on/off creating ghost cells (on by default).
   */
  vtkSetMacro(CreateGhostCells, vtkTypeBool);
  vtkGetMacro(CreateGhostCells, vtkTypeBool);
  vtkBooleanMacro(CreateGhostCells, vtkTypeBool);
  ///@}

protected:
  vtkExtractPolyDataPiece();
  ~vtkExtractPolyDataPiece() override = default;

  // Usual data generation method
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  // A method for labeling which piece the cells belong to.
  void ComputeCellTags(
    vtkIntArray* cellTags, vtkIdList* pointOwnership, int piece, int numPieces, vtkPolyData* input);

  void AddGhostLevel(vtkPolyData* input, vtkIntArray* cellTags, int ghostLevel);

  vtkTypeBool CreateGhostCells;

private:
  vtkExtractPolyDataPiece(const vtkExtractPolyDataPiece&) = delete;
  void operator=(const vtkExtractPolyDataPiece&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
