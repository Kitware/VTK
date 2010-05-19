/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXYZMolReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXYZMolReader - read Molecular Data files
// .SECTION Description
// vtkXYZMolReader is a source object that reads Molecule files
// The FileName must be specified
//
// .SECTION Thanks
// Dr. Jean M. Favre who developed and contributed this class

#ifndef __vtkXYZMolReader_h
#define __vtkXYZMolReader_h

#include "vtkMoleculeReaderBase.h"


class VTK_IO_EXPORT vtkXYZMolReader : public vtkMoleculeReaderBase
{
public:
  vtkTypeMacro(vtkXYZMolReader,vtkMoleculeReaderBase);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkXYZMolReader *New();

  // Description:
  // Test whether the file with the given name can be read by this
  // reader.
  virtual int CanReadFile(const char* name);

  // Description:
  // Set the current time step. It should be greater than 0 and smaller than
  // MaxTimeStep.
  vtkSetMacro(TimeStep, int);
  vtkGetMacro(TimeStep, int);

  // Description:
  // Get the maximum time step.
  vtkGetMacro(MaxTimeStep, int);

protected:
  vtkXYZMolReader();
  ~vtkXYZMolReader();

  void ReadSpecificMolecule(FILE* fp);

  // Description:
  // Get next line that is not a comment. It returns the beginning of data on
  // line (skips empty spaces)
  char* GetNextLine(FILE* fp, char* line, int maxlen);

  int GetLine1(const char* line, int *cnt);
  int GetLine2(const char* line, char *name);
  int GetAtom(const char* line, char* atom, float *x);

  void InsertAtom(const char* atom, float *pos);

  vtkSetMacro(MaxTimeStep, int);

  int TimeStep;
  int MaxTimeStep;

private:
  vtkXYZMolReader(const vtkXYZMolReader&);  // Not implemented.
  void operator=(const vtkXYZMolReader&);  // Not implemented.
};

#endif
