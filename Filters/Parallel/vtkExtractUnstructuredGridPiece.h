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
// .NAME vtkExtractUnstructuredGridPiece - Return specified piece, including specified
// number of ghost levels.

#ifndef __vtkExtractUnstructuredGridPiece_h
#define __vtkExtractUnstructuredGridPiece_h

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

  // Description:
  // Turn on/off creating ghost cells (on by default).
  vtkSetMacro(CreateGhostCells, int);
  vtkGetMacro(CreateGhostCells, int);
  vtkBooleanMacro(CreateGhostCells, int);

protected:
  vtkExtractUnstructuredGridPiece();
  ~vtkExtractUnstructuredGridPiece() {};

  // Usual data generation method
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  // A method for labeling which piece the cells belong to.
  void ComputeCellTags(vtkIntArray *cellTags, vtkIdList *pointOwnership,
                       int piece, int numPieces, vtkUnstructuredGrid *input);

  void AddGhostLevel(vtkUnstructuredGrid *input, vtkIntArray *cellTags,int ghostLevel);

  int CreateGhostCells;
private:
  void AddFirstGhostLevel(vtkUnstructuredGrid *input, vtkIntArray *cellTags,
                         int piece, int numPieces);

  vtkExtractUnstructuredGridPiece(const vtkExtractUnstructuredGridPiece&);  // Not implemented.
  void operator=(const vtkExtractUnstructuredGridPiece&);  // Not implemented.
};

#endif
