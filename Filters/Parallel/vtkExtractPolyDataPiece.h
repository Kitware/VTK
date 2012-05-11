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
// .NAME vtkExtractPolyDataPiece - Return specified piece, including specified
// number of ghost levels.

#ifndef __vtkExtractPolyDataPiece_h
#define __vtkExtractPolyDataPiece_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkIdList;
class vtkIntArray;

class VTKFILTERSPARALLEL_EXPORT vtkExtractPolyDataPiece : public vtkPolyDataAlgorithm
{
public:
  static vtkExtractPolyDataPiece *New();
  vtkTypeMacro(vtkExtractPolyDataPiece, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Turn on/off creating ghost cells (on by default).
  vtkSetMacro(CreateGhostCells, int);
  vtkGetMacro(CreateGhostCells, int);
  vtkBooleanMacro(CreateGhostCells, int);

protected:
  vtkExtractPolyDataPiece();
  ~vtkExtractPolyDataPiece() {};

  // Usual data generation method
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  // A method for labeling which piece the cells belong to.
  void ComputeCellTags(vtkIntArray *cellTags, vtkIdList *pointOwnership,
                       int piece, int numPieces, vtkPolyData *input);

  void AddGhostLevel(vtkPolyData *input, vtkIntArray *cellTags, int ghostLevel);

  int CreateGhostCells;
private:
  vtkExtractPolyDataPiece(const vtkExtractPolyDataPiece&);  // Not implemented.
  void operator=(const vtkExtractPolyDataPiece&);  // Not implemented.
};

#endif
