/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractRectilinearGridPiece.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkExtractRectilinearGridPiece - Takes in RectilinearGrid Data and 
// extracts a region within, producing another RectilinearGrid.

#ifndef __vtkExtractRectilinearGridPiece_h
#define __vtkExtractRectilinearGridPiece_h

#include "vtkRectilinearGridAlgorithm.h"

class VTK_PARALLEL_EXPORT vtkExtractRectilinearGridPiece : public vtkRectilinearGridAlgorithm
{
public:
  static vtkExtractRectilinearGridPiece *New();
  vtkTypeRevisionMacro(vtkExtractRectilinearGridPiece, vtkRectilinearGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
    
protected:
  vtkExtractRectilinearGridPiece() {};
  ~vtkExtractRectilinearGridPiece() {};

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
   
private:
  vtkExtractRectilinearGridPiece(const vtkExtractRectilinearGridPiece&);  // Not implemented.
  void operator=(const vtkExtractRectilinearGridPiece&);  // Not implemented.
};

#endif
