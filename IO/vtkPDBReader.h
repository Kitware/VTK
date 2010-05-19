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
// .NAME vtkPDBReader - read Molecular Data files
// .SECTION Description
// vtkPDBReader is a source object that reads Molecule files
// The FileName must be specified
//
// .SECTION Thanks
// Dr. Jean M. Favre who developed and contributed this class

#ifndef __vtkPDBReader_h
#define __vtkPDBReader_h

#include "vtkMoleculeReaderBase.h"


class VTK_IO_EXPORT vtkPDBReader : public vtkMoleculeReaderBase
{
public:
  vtkTypeMacro(vtkPDBReader,vtkMoleculeReaderBase);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkPDBReader *New();

protected:
  vtkPDBReader();
  ~vtkPDBReader();

  void ReadSpecificMolecule(FILE* fp);
  
private:
  vtkPDBReader(const vtkPDBReader&);  // Not implemented.
  void operator=(const vtkPDBReader&);  // Not implemented.
};

#endif
