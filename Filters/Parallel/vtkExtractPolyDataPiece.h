/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractPolyDataPiece.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkExtractPolyDataPiece
 * @brief   Return specified piece, including specified
 * number of ghost levels.
*/

#ifndef vtkExtractPolyDataPiece_h
#define vtkExtractPolyDataPiece_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkIdList;
class vtkIntArray;

class VTKFILTERSPARALLEL_EXPORT vtkExtractPolyDataPiece : public vtkPolyDataAlgorithm
{
public:
  static vtkExtractPolyDataPiece *New();
  vtkTypeMacro(vtkExtractPolyDataPiece, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Turn on/off creating ghost cells (on by default).
   */
  vtkSetMacro(CreateGhostCells, int);
  vtkGetMacro(CreateGhostCells, int);
  vtkBooleanMacro(CreateGhostCells, int);
  //@}

protected:
  vtkExtractPolyDataPiece();
  ~vtkExtractPolyDataPiece() VTK_OVERRIDE {}

  // Usual data generation method
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;
  int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;

  // A method for labeling which piece the cells belong to.
  void ComputeCellTags(vtkIntArray *cellTags, vtkIdList *pointOwnership,
                       int piece, int numPieces, vtkPolyData *input);

  void AddGhostLevel(vtkPolyData *input, vtkIntArray *cellTags, int ghostLevel);

  int CreateGhostCells;
private:
  vtkExtractPolyDataPiece(const vtkExtractPolyDataPiece&) VTK_DELETE_FUNCTION;
  void operator=(const vtkExtractPolyDataPiece&) VTK_DELETE_FUNCTION;
};

#endif
