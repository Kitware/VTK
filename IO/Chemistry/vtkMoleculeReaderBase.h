// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMoleculeReaderBase
 * @brief   Read molecular data files
 *
 * vtkMoleculeReaderBase is a source object that reads molecule files.
 * The FileName must be specified
 *
 * @par Thanks:
 * Dr. Jean M. Favre who originally developed and contributed this class
 * Angelos Angelopoulos and Spiros Tsalikis for revisions
 */

#ifndef vtkMoleculeReaderBase_h
#define vtkMoleculeReaderBase_h

#include "vtkIOChemistryModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkCellArray;
class vtkFloatArray;
class vtkDataArray;
class vtkIdTypeArray;
class vtkUnsignedCharArray;
class vtkUnsignedIntArray;
class vtkPoints;
class vtkStringArray;
class vtkMolecule;
class vtkPeriodicTable;

class VTKIOCHEMISTRY_EXPORT vtkMoleculeReaderBase : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkMoleculeReaderBase, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);

  ///@{
  /**
   * A scaling factor to compute bonds between non-hydrogen atoms
   */
  vtkSetMacro(BScale, double);
  vtkGetMacro(BScale, double);
  ///@}

  ///@{
  /**
   * A scaling factor to compute bonds with hydrogen atoms.
   */
  vtkSetMacro(HBScale, double);
  vtkGetMacro(HBScale, double);
  ///@}

  /**
   * Number of atoms in the molecule.
   */
  vtkGetMacro(NumberOfAtoms, vtkIdType);

  /**
   * Number of models that make up the molecule.
   */
  vtkGetMacro(NumberOfModels, unsigned int);

protected:
  vtkMoleculeReaderBase();
  ~vtkMoleculeReaderBase() override;

  char* FileName;
  double BScale;
  double HBScale;
  vtkIdType NumberOfAtoms;
  unsigned int NumberOfModels;

  int FillOutputPortInformation(int, vtkInformation*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Reads a molecule from the passed file pointer and creates a vtkPolyData.
   *
   * @param fp Molecule file pointer.
   * @param output Pointer to output vtkPolyData.
   *
   * @return Zero upon successfully reading a molecule.
   */
  int ReadMolecule(FILE* fp, vtkPolyData* output);

  /**
   * Given a string for the type (name) of an atom, returns a unique
   * number for that atom.
   *
   * @param atomType A string for the type (name) of an atom, e.g. "He" for a helium atom.
   *
   * @return Unique number for the atom type.
   */
  unsigned int MakeAtomType(const char* atomType);

  /**
   * Creates molecular bonds (VTK cells) given atomic coordinates (VTK points) and atom types.
   *
   * @param points Atomic (VTK points) coordinates.
   * @param atomTypes Array containing the atom type numbers.
   * @param newBonds Output bonds.
   *
   * @return Number of bonds.
   */
  unsigned int MakeBonds(vtkPoints* points, vtkIdTypeArray* atomTypes, vtkCellArray* newBonds);

  vtkNew<vtkPeriodicTable> PeriodicTable;
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

VTK_ABI_NAMESPACE_END
#endif
