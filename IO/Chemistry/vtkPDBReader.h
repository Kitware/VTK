// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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

#include "vtkIOChemistryModule.h" // For export macro
#include "vtkMoleculeReaderBase.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKIOCHEMISTRY_EXPORT vtkPDBReader : public vtkMoleculeReaderBase
{
public:
  vtkTypeMacro(vtkPDBReader, vtkMoleculeReaderBase);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkPDBReader* New();

protected:
  vtkPDBReader();
  ~vtkPDBReader() override;

  void ReadSpecificMolecule(FILE* fp) override;

private:
  vtkPDBReader(const vtkPDBReader&) = delete;
  void operator=(const vtkPDBReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
