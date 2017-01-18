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
/**
 * @class   vtkEnSightMasterServerReader
 * @brief   reader for compund EnSight files
*/

#ifndef vtkEnSightMasterServerReader_h
#define vtkEnSightMasterServerReader_h

#include "vtkIOEnSightModule.h" // For export macro
#include "vtkGenericEnSightReader.h"

class vtkCollection;

class VTKIOENSIGHT_EXPORT vtkEnSightMasterServerReader : public vtkGenericEnSightReader
{
public:
  vtkTypeMacro(vtkEnSightMasterServerReader, vtkGenericEnSightReader);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  static vtkEnSightMasterServerReader* New();

  /**
   * Determine which file should be read for piece
   */
  int DetermineFileName(int piece);

  //@{
  /**
   * Get the file name that will be read.
   */
  vtkGetStringMacro(PieceCaseFileName);
  //@}

  //@{
  /**
   * Set or get the current piece.
   */
  vtkSetMacro(CurrentPiece, int);
  vtkGetMacro(CurrentPiece, int);
  //@}

  int CanReadFile(const char *fname);

protected:
  vtkEnSightMasterServerReader();
  ~vtkEnSightMasterServerReader() VTK_OVERRIDE;

  int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *) VTK_OVERRIDE;
  int RequestInformation(vtkInformation *, vtkInformationVector **,
                                 vtkInformationVector *) VTK_OVERRIDE;

  vtkSetStringMacro(PieceCaseFileName);
  char* PieceCaseFileName;
  int MaxNumberOfPieces;
  int CurrentPiece;

private:
  vtkEnSightMasterServerReader(const vtkEnSightMasterServerReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkEnSightMasterServerReader&) VTK_DELETE_FUNCTION;
};

#endif
