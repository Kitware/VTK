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
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkPDBReader *New();

protected:
  vtkPDBReader();
  ~vtkPDBReader() override;

  void ReadSpecificMolecule(FILE* fp) override;

private:
  vtkPDBReader(const vtkPDBReader&) = delete;
  void operator=(const vtkPDBReader&) = delete;
};

#endif
