/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMoleculeReaderBase.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMoleculeReaderBase - read Molecular Data files
// .SECTION Description
// vtkMoleculeReaderBase is a source object that reads Molecule files
// The FileName must be specified
//
// .SECTION Thanks
// Dr. Jean M. Favre who developed and contributed this class

#ifndef __vtkMoleculeReaderBase_h
#define __vtkMoleculeReaderBase_h

#include "vtkPolyDataSource.h"

class vtkCellArray;
class vtkFloatArray;
class vtkDataArray;
class vtkIdTypeArray;
class vtkUnsignedCharArray;
class vtkPoints;

class VTK_IO_EXPORT vtkMoleculeReaderBase : public vtkPolyDataSource 
{
public:
  vtkTypeRevisionMacro(vtkMoleculeReaderBase,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  vtkSetMacro(BScale, double);
  vtkGetMacro(BScale, double);

  vtkSetMacro(HBScale, double);
  vtkGetMacro(HBScale, double);

  vtkGetMacro(NumberOfAtoms, int);

protected:
  vtkMoleculeReaderBase();
  ~vtkMoleculeReaderBase();

  char *FileName;
  double BScale;  
  // a scaling factor to compute bonds between non-hydrogen atoms
  double HBScale; 
  // a scaling factor to compute bonds with hydrogen atoms
  int NumberOfAtoms;

  virtual void Execute();
  int ReadMolecule(FILE *fp);
  int MakeAtomType(const char *atype);
  int MakeBonds(vtkPoints*, vtkIdTypeArray*, vtkCellArray*);

  vtkPoints *Points;
  vtkUnsignedCharArray *RGB;
  vtkFloatArray *Radii;
  vtkIdTypeArray *AtomType;

  virtual void ReadSpecificMolecule(FILE* fp) = 0;
  
private:
  vtkMoleculeReaderBase(const vtkMoleculeReaderBase&);  // Not implemented.
  void operator=(const vtkMoleculeReaderBase&);  // Not implemented.
};

#endif
