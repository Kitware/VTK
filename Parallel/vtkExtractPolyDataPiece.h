/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractPolyDataPiece.h
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
// .NAME vtkExtractPolyDataPiece - Return specified piece, including specified
// number of ghost levels.

#ifndef __vtkExtractPolyDataPiece_h
#define __vtkExtractPolyDataPiece_h

#include "vtkPolyDataToPolyDataFilter.h"

class VTK_PARALLEL_EXPORT vtkExtractPolyDataPiece : public vtkPolyDataToPolyDataFilter
{
public:
  static vtkExtractPolyDataPiece *New();
  vtkTypeRevisionMacro(vtkExtractPolyDataPiece, vtkPolyDataToPolyDataFilter);
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
  void Execute();
  void ExecuteInformation();
  void ComputeInputUpdateExtents(vtkDataObject *out);
 
  // A method for labeling which piece the cells belong to.
  void ComputeCellTags(vtkIntArray *cellTags, vtkIdList *pointOwnership,
                       int piece, int numPieces);
  
  void AddGhostLevel(vtkPolyData *input, vtkIntArray *cellTags, int ghostLevel);
  
  int CreateGhostCells;
private:
  vtkExtractPolyDataPiece(const vtkExtractPolyDataPiece&);  // Not implemented.
  void operator=(const vtkExtractPolyDataPiece&);  // Not implemented.
};

#endif
