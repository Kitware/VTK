/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEnSightCompoundReader.h
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
// .NAME vtkEnSightCompoundReader - reader for compund EnSight files

#ifndef __vtkEnSightCompoundReader_h
#define __vtkEnSightCompoundReader_h

#include "vtkGenericEnSightReader.h"

class vtkCollection;

class VTK_IO_EXPORT vtkEnSightCompoundReader : public vtkGenericEnSightReader
{
public:
  vtkTypeRevisionMacro(vtkEnSightCompoundReader, vtkGenericEnSightReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkEnSightCompoundReader* New();

  virtual void UpdateInformation();

  // Description:
  // Determine which file should be read for piece
  int DetermineFileName(int piece);

  // Description:
  // Get the file name that will be read.
  vtkGetStringMacro(PieceCaseFileName);

  // Description:
  // Set or get the current piece.
  vtkSetMacro(CurrentPiece, int);
  vtkGetMacro(CurrentPiece, int);
  
protected:
  vtkEnSightCompoundReader();
  ~vtkEnSightCompoundReader();
  
  void Execute();
  void ExecuteInformation();

  vtkSetStringMacro(PieceCaseFileName);
  char* PieceCaseFileName;
  int MaxNumberOfPieces;
  int CurrentPiece;

private:
  vtkEnSightCompoundReader(const vtkEnSightCompoundReader&);  // Not implemented.
  void operator=(const vtkEnSightCompoundReader&);  // Not implemented.
};

#endif
