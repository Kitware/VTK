// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPieceRequestFilter
 * @brief   Sets the piece request for upstream filters.
 *
 * Sends the piece and number of pieces to upstream filters; passes the input
 * to the output unmodified.
 */

#ifndef vtkPieceRequestFilter_h
#define vtkPieceRequestFilter_h

#include "vtkAlgorithm.h"
#include "vtkFiltersParallelModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkDataObject;

class VTKFILTERSPARALLEL_EXPORT vtkPieceRequestFilter : public vtkAlgorithm
{
public:
  static vtkPieceRequestFilter* New();
  vtkTypeMacro(vtkPieceRequestFilter, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * The total number of pieces.
   */
  vtkSetClampMacro(NumberOfPieces, int, 0, VTK_INT_MAX);
  vtkGetMacro(NumberOfPieces, int);
  ///@}

  ///@{
  /**
   * The piece to extract.
   */
  vtkSetClampMacro(Piece, int, 0, VTK_INT_MAX);
  vtkGetMacro(Piece, int);
  ///@}

  ///@{
  /**
   * Get the output data object for a port on this algorithm.
   */
  vtkDataObject* GetOutput();
  vtkDataObject* GetOutput(int);
  ///@}

  ///@{
  /**
   * Set an input of this algorithm.
   */
  void SetInputData(vtkDataObject*);
  void SetInputData(int, vtkDataObject*);
  ///@}

  /**
   * see vtkAlgorithm for details
   */
  vtkTypeBool ProcessRequest(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

protected:
  vtkPieceRequestFilter();
  ~vtkPieceRequestFilter() override = default;

  virtual int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  virtual int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  virtual int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  int FillOutputPortInformation(int port, vtkInformation* info) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  int NumberOfPieces;
  int Piece;

private:
  vtkPieceRequestFilter(const vtkPieceRequestFilter&) = delete;
  void operator=(const vtkPieceRequestFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
