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
 * Angelos Angelopoulos and Spiros Tsalikis for revisions
 */

#ifndef vtkMoleculeReaderBase_h
#define vtkMoleculeReaderBase_h

#include "vtkIOGeometryModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"
#include "vtkUnsignedIntArray.h"

class vtkCellArray;
class vtkFloatArray;
class vtkDataArray;
class vtkIdTypeArray;
class vtkUnsignedCharArray;
class vtkPoints;
class vtkStringArray;
class vtkMolecule;
class vtkPeriodicTable;

class VTKIOGEOMETRY_EXPORT vtkMoleculeReaderBase : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkMoleculeReaderBase, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

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

  vtkGetMacro(NumberOfModels, unsigned int);

protected:
  vtkMoleculeReaderBase();
  ~vtkMoleculeReaderBase() override;

  char* FileName;
  double BScale;
  double HBScale;
  int NumberOfAtoms;
  unsigned int NumberOfModels;

  int FillOutputPortInformation(int, vtkInformation*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int ReadMolecule(FILE* fp, vtkPolyData* output);
  static unsigned int MakeAtomType(const char* atomType);
  unsigned int MakeBonds(vtkPoints*, vtkIdTypeArray*, vtkCellArray*);

  static vtkSmartPointer<vtkPeriodicTable> PeriodicTable;
  vtkSmartPointer<vtkMolecule> Molecule;
  vtkSmartPointer<vtkPoints> Points;
  vtkSmartPointer<vtkUnsignedCharArray> RGB;
  vtkSmartPointer<vtkFloatArray> Radii;
  vtkSmartPointer<vtkIdTypeArray> AtomType;
  vtkSmartPointer<vtkStringArray> AtomTypeStrings;
  vtkSmartPointer<vtkIdTypeArray> Residue;
  vtkSmartPointer<vtkUnsignedCharArray> Chain;
  vtkSmartPointer<vtkUnsignedCharArray> SecondaryStructures;
  vtkSmartPointer<vtkUnsignedCharArray> SecondaryStructuresBegin;
  vtkSmartPointer<vtkUnsignedCharArray> SecondaryStructuresEnd;
  vtkSmartPointer<vtkUnsignedCharArray> IsHetatm;
  vtkSmartPointer<vtkUnsignedIntArray> Model;

  virtual void ReadSpecificMolecule(FILE* fp) = 0;

private:
  vtkMoleculeReaderBase(const vtkMoleculeReaderBase&) = delete;
  void operator=(const vtkMoleculeReaderBase&) = delete;
};

#endif
