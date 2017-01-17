/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPDBReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPDBReader
 * @brief   read Molecular Data files
 *
 * vtkPDBReader is a source object that reads Molecule files
 * The FileName must be specified
 *
 * @par Thanks:
 * Dr. Jean M. Favre who developed and contributed this class
*/

#ifndef vtkPDBReader_h
#define vtkPDBReader_h

#include "vtkIOGeometryModule.h" // For export macro
#include "vtkMoleculeReaderBase.h"


class VTKIOGEOMETRY_EXPORT vtkPDBReader : public vtkMoleculeReaderBase
{
public:
  vtkTypeMacro(vtkPDBReader,vtkMoleculeReaderBase);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  static vtkPDBReader *New();

protected:
  vtkPDBReader();
  ~vtkPDBReader() VTK_OVERRIDE;

  void ReadSpecificMolecule(FILE* fp) VTK_OVERRIDE;

private:
  vtkPDBReader(const vtkPDBReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPDBReader&) VTK_DELETE_FUNCTION;
};

#endif
