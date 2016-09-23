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
/**
 * @class   vtkMoleculeReaderBase
 * @brief   read Molecular Data files
 *
 * vtkMoleculeReaderBase is a source object that reads Molecule files
 * The FileName must be specified
 *
 * @par Thanks:
 * Dr. Jean M. Favre who developed and contributed this class
*/

#ifndef vtkMoleculeReaderBase_h
#define vtkMoleculeReaderBase_h

#include "vtkIOGeometryModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkCellArray;
class vtkFloatArray;
class vtkDataArray;
class vtkIdTypeArray;
class vtkUnsignedCharArray;
class vtkPoints;
class vtkStringArray;
class vtkMolecule;

class VTKIOGEOMETRY_EXPORT vtkMoleculeReaderBase : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkMoleculeReaderBase,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  //@{
  /**
   * A scaling factor to compute bonds between non-hydrogen atoms
   */
  vtkSetMacro(BScale, double);
  vtkGetMacro(BScale, double);
  //@}

  //@{
  /**
   * A scaling factor to compute bonds with hydrogen atoms.
   */
  vtkSetMacro(HBScale, double);
  vtkGetMacro(HBScale, double);
  //@}

  vtkGetMacro(NumberOfAtoms, int);

protected:
  vtkMoleculeReaderBase();
  ~vtkMoleculeReaderBase();

  char *FileName;
  double BScale;
  double HBScale;
  int NumberOfAtoms;

  virtual int FillOutputPortInformation(int, vtkInformation *);
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  int ReadMolecule(FILE *fp, vtkPolyData *output);
  int MakeAtomType(const char *atype);
  int MakeBonds(vtkPoints*, vtkIdTypeArray*, vtkCellArray*);

  vtkMolecule *Molecule;
  vtkPoints *Points;
  vtkUnsignedCharArray *RGB;
  vtkFloatArray *Radii;
  vtkIdTypeArray *AtomType;
  vtkStringArray *AtomTypeStrings;
  vtkIdTypeArray *Residue;
  vtkUnsignedCharArray *Chain;
  vtkUnsignedCharArray *SecondaryStructures;
  vtkUnsignedCharArray *SecondaryStructuresBegin;
  vtkUnsignedCharArray *SecondaryStructuresEnd;
  vtkUnsignedCharArray *IsHetatm;

  virtual void ReadSpecificMolecule(FILE* fp) = 0;

private:
  vtkMoleculeReaderBase(const vtkMoleculeReaderBase&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMoleculeReaderBase&) VTK_DELETE_FUNCTION;
};

#endif
