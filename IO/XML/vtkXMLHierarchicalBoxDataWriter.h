// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXMLHierarchicalBoxDataWriter
 * @brief   writer for vtkHierarchicalBoxDataSet
 * for backwards compatibility.
 *
 * vtkXMLHierarchicalBoxDataWriter is an empty subclass of
 * vtkXMLUniformGridAMRWriter for writing vtkUniformGridAMR datasets in
 * VTK-XML format.
 */

#ifndef vtkXMLHierarchicalBoxDataWriter_h
#define vtkXMLHierarchicalBoxDataWriter_h

#include "vtkDeprecation.h" // For VTK_DEPRECATED_IN_9_5_0
#include "vtkXMLUniformGridAMRWriter.h"

VTK_ABI_NAMESPACE_BEGIN
class VTK_DEPRECATED_IN_9_5_0("Please use `vtkXMLUniformGridAMRWriter` instead.")
  VTKIOXML_EXPORT vtkXMLHierarchicalBoxDataWriter : public vtkXMLUniformGridAMRWriter
{
public:
  static vtkXMLHierarchicalBoxDataWriter* New();
  vtkTypeMacro(vtkXMLHierarchicalBoxDataWriter, vtkXMLUniformGridAMRWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension() override { return "vth"; }

protected:
  vtkXMLHierarchicalBoxDataWriter();
  ~vtkXMLHierarchicalBoxDataWriter() override;

private:
  vtkXMLHierarchicalBoxDataWriter(const vtkXMLHierarchicalBoxDataWriter&) = delete;
  void operator=(const vtkXMLHierarchicalBoxDataWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
