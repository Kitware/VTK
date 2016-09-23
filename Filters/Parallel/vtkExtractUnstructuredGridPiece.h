/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractUnstructuredGridPiece.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkExtractUnstructuredGridPiece
 * @brief   Return specified piece, including specified
 * number of ghost levels.
*/

#ifndef vtkExtractUnstructuredGridPiece_h
#define vtkExtractUnstructuredGridPiece_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

class vtkIdList;
class vtkIntArray;

class VTKFILTERSPARALLEL_EXPORT vtkExtractUnstructuredGridPiece : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkExtractUnstructuredGridPiece *New();
  vtkTypeMacro(vtkExtractUnstructuredGridPiece, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Turn on/off creating ghost cells (on by default).
   */
  vtkSetMacro(CreateGhostCells, int);
  vtkGetMacro(CreateGhostCells, int);
  vtkBooleanMacro(CreateGhostCells, int);
  //@}

protected:
  vtkExtractUnstructuredGridPiece();
  ~vtkExtractUnstructuredGridPiece() {}

  // Usual data generation method
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  // A method for labeling which piece the cells belong to.
  void ComputeCellTags(vtkIntArray *cellTags, vtkIdList *pointOwnership,
                       int piece, int numPieces, vtkUnstructuredGrid *input);

  void AddGhostLevel(vtkUnstructuredGrid *input, vtkIntArray *cellTags,int ghostLevel);

  int CreateGhostCells;
private:
  void AddFirstGhostLevel(vtkUnstructuredGrid *input, vtkIntArray *cellTags,
                         int piece, int numPieces);

  vtkExtractUnstructuredGridPiece(const vtkExtractUnstructuredGridPiece&) VTK_DELETE_FUNCTION;
  void operator=(const vtkExtractUnstructuredGridPiece&) VTK_DELETE_FUNCTION;
};

#endif
