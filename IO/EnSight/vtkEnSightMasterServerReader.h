/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEnSightMasterServerReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkEnSightMasterServerReader - reader for compund EnSight files

#ifndef __vtkEnSightMasterServerReader_h
#define __vtkEnSightMasterServerReader_h

#include "vtkIOEnSightModule.h" // For export macro
#include "vtkGenericEnSightReader.h"

class vtkCollection;

class VTKIOENSIGHT_EXPORT vtkEnSightMasterServerReader : public vtkGenericEnSightReader
{
public:
  vtkTypeMacro(vtkEnSightMasterServerReader, vtkGenericEnSightReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkEnSightMasterServerReader* New();

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

  int CanReadFile(const char *fname);

protected:
  vtkEnSightMasterServerReader();
  ~vtkEnSightMasterServerReader();

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **,
                                 vtkInformationVector *);

  vtkSetStringMacro(PieceCaseFileName);
  char* PieceCaseFileName;
  int MaxNumberOfPieces;
  int CurrentPiece;

private:
  vtkEnSightMasterServerReader(const vtkEnSightMasterServerReader&);  // Not implemented.
  void operator=(const vtkEnSightMasterServerReader&);  // Not implemented.
};

#endif
