// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXMLPUnstructuredDataWriter
 * @brief   Superclass for PVTK XML unstructured data writers.
 *
 * vtkXMLPUnstructuredDataWriter provides PVTK XML writing
 * functionality that is common among all the parallel unstructured
 * data formats.
 */

#ifndef vtkXMLPUnstructuredDataWriter_h
#define vtkXMLPUnstructuredDataWriter_h

#include "vtkDeprecation.h"         // For VTK_DEPRECATED_IN_9_5_0
#include "vtkIOParallelXMLModule.h" // For export macro
#include "vtkXMLPDataWriter.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkPointSet;
class vtkXMLUnstructuredDataWriter;

class VTKIOPARALLELXML_EXPORT vtkXMLPUnstructuredDataWriter : public vtkXMLPDataWriter
{
public:
  vtkTypeMacro(vtkXMLPUnstructuredDataWriter, vtkXMLPDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkXMLPUnstructuredDataWriter();
  ~vtkXMLPUnstructuredDataWriter() override;

  vtkPointSet* GetPointSetInput();
  VTK_DEPRECATED_IN_9_5_0("Use GetPointSetInput() instead.")
  vtkPointSet* GetInputAsPointSet() { return this->GetPointSetInput(); }
  virtual vtkXMLUnstructuredDataWriter* CreateUnstructuredPieceWriter() = 0;
  vtkXMLWriter* CreatePieceWriter(int index) override;
  void WritePData(vtkIndent indent) override;

private:
  vtkXMLPUnstructuredDataWriter(const vtkXMLPUnstructuredDataWriter&) = delete;
  void operator=(const vtkXMLPUnstructuredDataWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
