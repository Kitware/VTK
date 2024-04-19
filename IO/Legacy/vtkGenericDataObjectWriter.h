// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGenericDataObjectWriter
 * @brief   writes any type of vtk data object to file
 *
 * vtkGenericDataObjectWriter is a concrete class that writes data objects
 * to disk. The input to this object is any subclass of vtkDataObject.
 */

#ifndef vtkGenericDataObjectWriter_h
#define vtkGenericDataObjectWriter_h

#include "vtkDataWriter.h"
#include "vtkIOLegacyModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKIOLEGACY_EXPORT vtkGenericDataObjectWriter : public vtkDataWriter
{
public:
  static vtkGenericDataObjectWriter* New();
  vtkTypeMacro(vtkGenericDataObjectWriter, vtkDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkGenericDataObjectWriter();
  ~vtkGenericDataObjectWriter() override;

  void WriteData() override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkGenericDataObjectWriter(const vtkGenericDataObjectWriter&) = delete;
  void operator=(const vtkGenericDataObjectWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
