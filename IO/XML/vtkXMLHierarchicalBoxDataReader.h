// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXMLHierarchicalBoxDataReader
 * @brief   Reader for hierarchical datasets
 * (for backwards compatibility).
 *
 *
 * vtkXMLHierarchicalBoxDataReader is an empty subclass of
 * vtkXMLUniformGridAMRReader. This is only for backwards compatibility. Newer
 * code should simply use vtkXMLUniformGridAMRReader.
 *
 * @warning
 * The reader supports reading v1.1 and above. For older versions, use
 * vtkXMLHierarchicalBoxDataFileConverter.
 */

#ifndef vtkXMLHierarchicalBoxDataReader_h
#define vtkXMLHierarchicalBoxDataReader_h

#include "vtkDeprecation.h" // For VTK_DEPRECATED_IN_9_5_0
#include "vtkXMLUniformGridAMRReader.h"

VTK_ABI_NAMESPACE_BEGIN
class VTK_DEPRECATED_IN_9_5_0("Please use `vtkXMLUniformGridAMRReader` instead.")
  VTKIOXML_EXPORT vtkXMLHierarchicalBoxDataReader : public vtkXMLUniformGridAMRReader
{
public:
  static vtkXMLHierarchicalBoxDataReader* New();
  vtkTypeMacro(vtkXMLHierarchicalBoxDataReader, vtkXMLUniformGridAMRReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkXMLHierarchicalBoxDataReader();
  ~vtkXMLHierarchicalBoxDataReader() override;

private:
  vtkXMLHierarchicalBoxDataReader(const vtkXMLHierarchicalBoxDataReader&) = delete;
  void operator=(const vtkXMLHierarchicalBoxDataReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
