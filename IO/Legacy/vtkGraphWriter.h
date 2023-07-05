// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGraphWriter
 * @brief   write vtkGraph data to a file
 *
 * vtkGraphWriter is a sink object that writes ASCII or binary
 * vtkGraph data files in vtk format. See text for format details.
 * @warning
 * Binary files written on one system may not be readable on other systems.
 */

#ifndef vtkGraphWriter_h
#define vtkGraphWriter_h

#include "vtkDataWriter.h"
#include "vtkIOLegacyModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkGraph;
class vtkMolecule;

class VTKIOLEGACY_EXPORT vtkGraphWriter : public vtkDataWriter
{
public:
  static vtkGraphWriter* New();
  vtkTypeMacro(vtkGraphWriter, vtkDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the input to this writer.
   */
  vtkGraph* GetInput();
  vtkGraph* GetInput(int port);
  ///@}

protected:
  vtkGraphWriter() = default;
  ~vtkGraphWriter() override = default;

  void WriteData() override;

  void WriteMoleculeData(ostream* fp, vtkMolecule* m);

  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkGraphWriter(const vtkGraphWriter&) = delete;
  void operator=(const vtkGraphWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
