/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractUnstructuredGridPiece.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
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

#include "vtkUnstructuredGridToUnstructuredGridFilter.h"

class VTK_PARALLEL_EXPORT vtkExtractUnstructuredGridPiece : public vtkUnstructuredGridToUnstructuredGridFilter
{
public:
  static vtkExtractUnstructuredGridPiece *New();
  vtkTypeRevisionMacro(vtkExtractUnstructuredGridPiece, vtkUnstructuredGridToUnstructuredGridFilter);
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
  void Execute();
  void ExecuteInformation();
  void ComputeInputUpdateExtents(vtkDataObject *out);
 
  // A method for labeling which piece the cells belong to.
  void ComputeCellTags(vtkIntArray *cellTags, vtkIdList *pointOwnership,
                       int piece, int numPieces);
  
  void AddGhostLevel(vtkUnstructuredGrid *input, vtkIntArray *cellTags, int ghostLevel);
  
  int CreateGhostCells;
private:
  vtkExtractUnstructuredGridPiece(const vtkExtractUnstructuredGridPiece&);  // Not implemented.
  void operator=(const vtkExtractUnstructuredGridPiece&);  // Not implemented.
};

#endif
