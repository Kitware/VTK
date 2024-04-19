// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCMLMoleculeReader
 * @brief   Read a CML file and output a
 * vtkMolecule object
 *
 */

#ifndef vtkCMLMoleculeReader_h
#define vtkCMLMoleculeReader_h

#include "vtkIOChemistryModule.h" // For export macro
#include "vtkMoleculeAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkMolecule;

class VTKIOCHEMISTRY_EXPORT vtkCMLMoleculeReader : public vtkMoleculeAlgorithm
{
public:
  static vtkCMLMoleculeReader* New();
  vtkTypeMacro(vtkCMLMoleculeReader, vtkMoleculeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the output (vtkMolecule) that the reader will fill
   */
  vtkMolecule* GetOutput();
  void SetOutput(vtkMolecule*) override;
  ///@}

  ///@{
  /**
   * Get/Set the name of the CML file
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

protected:
  vtkCMLMoleculeReader();
  ~vtkCMLMoleculeReader() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillOutputPortInformation(int, vtkInformation*) override;

  char* FileName;

private:
  vtkCMLMoleculeReader(const vtkCMLMoleculeReader&) = delete;
  void operator=(const vtkCMLMoleculeReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
