// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkExtractUserDefinedPiece
 * @brief   Return user specified piece with ghost cells
 *
 *
 * Provided a function that determines which cells are zero-level
 * cells ("the piece"), this class outputs the piece with the
 * requested number of ghost levels.  The only difference between
 * this class and the class it is derived from is that the
 * zero-level cells are specified by a function you provide,
 * instead of determined by dividing up the cells based on cell Id.
 *
 * @sa
 * vtkExtractUnstructuredGridPiece
 */

#ifndef vtkExtractUserDefinedPiece_h
#define vtkExtractUserDefinedPiece_h

#include "vtkExtractUnstructuredGridPiece.h"
#include "vtkFiltersParallelModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSPARALLEL_EXPORT vtkExtractUserDefinedPiece : public vtkExtractUnstructuredGridPiece
{
public:
  vtkTypeMacro(vtkExtractUserDefinedPiece, vtkExtractUnstructuredGridPiece);
  static vtkExtractUserDefinedPiece* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  typedef int (*UserDefFunc)(vtkIdType cellID, vtkUnstructuredGrid* grid, void* constantData);

  // Set the function used to identify the piece.  The function should
  // return 1 if the cell is in the piece, and 0 otherwise.
  void SetPieceFunction(UserDefFunc func)
  {
    this->InPiece = func;
    this->Modified();
  }

  // Set constant data to be used by the piece identifying function.
  void SetConstantData(void* data, int len);

  // Get constant data to be used by the piece identifying function.
  // Return the length of the data buffer.
  int GetConstantData(void** data);

  // The function should return 1 if the cell
  // is in the piece, and 0 otherwise.

protected:
  vtkExtractUserDefinedPiece();
  ~vtkExtractUserDefinedPiece() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  void ComputeCellTagsWithFunction(
    vtkIntArray* tags, vtkIdList* pointOwnership, vtkUnstructuredGrid* input);

private:
  vtkExtractUserDefinedPiece(const vtkExtractUserDefinedPiece&) = delete;
  void operator=(const vtkExtractUserDefinedPiece&) = delete;

  void* ConstantData;
  int ConstantDataLen;

  UserDefFunc InPiece;
};
VTK_ABI_NAMESPACE_END
#endif
