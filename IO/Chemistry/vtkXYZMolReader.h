// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXYZMolReader
 * @brief   read Molecular Data files
 *
 * vtkXYZMolReader is a source object that reads Molecule files
 * The FileName must be specified
 *
 * @par Thanks:
 * Dr. Jean M. Favre who developed and contributed this class
 */

#ifndef vtkXYZMolReader_h
#define vtkXYZMolReader_h

#include "vtkIOChemistryModule.h" // For export macro
#include "vtkMoleculeReaderBase.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKIOCHEMISTRY_EXPORT vtkXYZMolReader : public vtkMoleculeReaderBase
{
public:
  vtkTypeMacro(vtkXYZMolReader, vtkMoleculeReaderBase);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkXYZMolReader* New();

  /**
   * Test whether the file with the given name can be read by this
   * reader.
   */
  virtual int CanReadFile(VTK_FILEPATH const char* name);

  ///@{
  /**
   * Set the current time step. It should be greater than 0 and smaller than
   * MaxTimeStep.
   */
  vtkSetMacro(TimeStep, int);
  vtkGetMacro(TimeStep, int);
  ///@}

  ///@{
  /**
   * Get the maximum time step.
   */
  vtkGetMacro(MaxTimeStep, int);
  ///@}

protected:
  vtkXYZMolReader();
  ~vtkXYZMolReader() override;

  void ReadSpecificMolecule(FILE* fp) override;

  /**
   * Get next line that is not a comment. It returns the beginning of data on
   * line (skips empty spaces)
   */
  char* GetNextLine(FILE* fp, char* line, int maxlen);

  int GetLine1(const char* line, int* cnt);
  int GetLine2(const char* line, char* name);
  int GetAtom(const char* line, char* atom, float* x);

  void InsertAtom(const char* atom, float* pos);

  vtkSetMacro(MaxTimeStep, int);

  int TimeStep;
  int MaxTimeStep;

private:
  vtkXYZMolReader(const vtkXYZMolReader&) = delete;
  void operator=(const vtkXYZMolReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
