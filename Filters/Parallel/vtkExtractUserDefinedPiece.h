/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractUserDefinedPiece.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

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

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkExtractUnstructuredGridPiece.h"

class VTKFILTERSPARALLEL_EXPORT vtkExtractUserDefinedPiece : public vtkExtractUnstructuredGridPiece
{
public:
  vtkTypeMacro(vtkExtractUserDefinedPiece, vtkExtractUnstructuredGridPiece);
  static vtkExtractUserDefinedPiece *New();
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  typedef int (*UserDefFunc)(vtkIdType cellID, vtkUnstructuredGrid *grid, void *constantData);

  // Set the function used to identify the piece.  The function should
  // return 1 if the cell is in the piece, and 0 otherwise.
  void SetPieceFunction(UserDefFunc func) {this->InPiece = func; this->Modified();}

  // Set constant data to be used by the piece identifying function.
  void SetConstantData(void *data, int len);

  // Get constant data to be used by the piece identifying function.
  // Return the length of the data buffer.
  int GetConstantData(void **data);

  // The function should return 1 if the cell
  // is in the piece, and 0 otherwise.

protected:

  vtkExtractUserDefinedPiece();
  ~vtkExtractUserDefinedPiece() VTK_OVERRIDE;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;

  void ComputeCellTagsWithFunction(vtkIntArray *tags, vtkIdList *pointOwnership,
                                   vtkUnstructuredGrid *input);

private:
  vtkExtractUserDefinedPiece(const vtkExtractUserDefinedPiece&) VTK_DELETE_FUNCTION;
  void operator=(const vtkExtractUserDefinedPiece&) VTK_DELETE_FUNCTION;

  void *ConstantData;
  int ConstantDataLen;

  UserDefFunc InPiece;
};
#endif
