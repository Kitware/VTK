/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractStructuredGridPiece.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkExtractStructuredGridPiece - Takes in a StructuredGrid and 
// extracts a region within, producing another StructuredGrid.

#ifndef __vtkExtractStructuredGridPiece_h
#define __vtkExtractStructuredGridPiece_h

#include "vtkStructuredGridAlgorithm.h"

class VTK_PARALLEL_EXPORT vtkExtractStructuredGridPiece : public vtkStructuredGridAlgorithm
{
public:
  static vtkExtractStructuredGridPiece *New();
  vtkTypeRevisionMacro(vtkExtractStructuredGridPiece, vtkStructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
    
protected:
  vtkExtractStructuredGridPiece() {};
  ~vtkExtractStructuredGridPiece() {};

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
   
private:
  vtkExtractStructuredGridPiece(const vtkExtractStructuredGridPiece&);  // Not implemented.
  void operator=(const vtkExtractStructuredGridPiece&);  // Not implemented.
};

#endif
