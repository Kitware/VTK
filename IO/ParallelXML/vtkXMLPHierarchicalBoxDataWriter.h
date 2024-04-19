// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXMLPHierarchicalBoxDataWriter
 * @brief   parallel writer for
 * vtkHierarchicalBoxDataSet for backwards compatibility.
 *
 * vtkXMLPHierarchicalBoxDataWriter is an empty subclass of
 * vtkXMLPUniformGridAMRWriter for backwards compatibility.
 */

#ifndef vtkXMLPHierarchicalBoxDataWriter_h
#define vtkXMLPHierarchicalBoxDataWriter_h

#include "vtkIOParallelXMLModule.h" // For export macro
#include "vtkXMLPUniformGridAMRWriter.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKIOPARALLELXML_EXPORT vtkXMLPHierarchicalBoxDataWriter : public vtkXMLPUniformGridAMRWriter
{
public:
  static vtkXMLPHierarchicalBoxDataWriter* New();
  vtkTypeMacro(vtkXMLPHierarchicalBoxDataWriter, vtkXMLPUniformGridAMRWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkXMLPHierarchicalBoxDataWriter();
  ~vtkXMLPHierarchicalBoxDataWriter() override;

private:
  vtkXMLPHierarchicalBoxDataWriter(const vtkXMLPHierarchicalBoxDataWriter&) = delete;
  void operator=(const vtkXMLPHierarchicalBoxDataWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
