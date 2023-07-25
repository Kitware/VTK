// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXMLPDataSetWriter
 * @brief   Write any type of PVTK XML file.
 *
 * vtkXMLPDataSetWriter is a wrapper around the PVTK XML file format
 * writers.  Given an input vtkDataSet, the correct writer is
 * automatically selected based on the type of input.
 *
 * @sa
 * vtkXMLPImageDataWriter vtkXMLPStructuredGridWriter
 * vtkXMLPRectilinearGridWriter vtkXMLPPolyDataWriter
 * vtkXMLPUnstructuredGridWriter
 */

#ifndef vtkXMLPDataSetWriter_h
#define vtkXMLPDataSetWriter_h

#include "vtkIOParallelXMLModule.h" // For export macro
#include "vtkXMLPDataWriter.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKIOPARALLELXML_EXPORT vtkXMLPDataSetWriter : public vtkXMLPDataWriter
{
public:
  vtkTypeMacro(vtkXMLPDataSetWriter, vtkXMLPDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkXMLPDataSetWriter* New();

  /**
   * Get/Set the writer's input.
   */
  vtkDataSet* GetInput();

protected:
  vtkXMLPDataSetWriter();
  ~vtkXMLPDataSetWriter() override;

  // see algorithm for more info
  int FillInputPortInformation(int port, vtkInformation* info) override;

  // Override writing method from superclass.
  int WriteInternal() override;

  // Dummies to satisfy pure virtuals from superclass.
  const char* GetDataSetName() override;
  const char* GetDefaultFileExtension() override;
  vtkXMLWriter* CreatePieceWriter(int index) override;

private:
  vtkXMLPDataSetWriter(const vtkXMLPDataSetWriter&) = delete;
  void operator=(const vtkXMLPDataSetWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
