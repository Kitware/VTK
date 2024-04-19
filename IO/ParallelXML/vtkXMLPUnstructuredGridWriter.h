// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXMLPUnstructuredGridWriter
 * @brief   Write PVTK XML UnstructuredGrid files.
 *
 * vtkXMLPUnstructuredGridWriter writes the PVTK XML UnstructuredGrid
 * file format.  One unstructured grid input can be written into a
 * parallel file format with any number of pieces spread across files.
 * The standard extension for this writer's file format is "pvtu".
 * This writer uses vtkXMLUnstructuredGridWriter to write the
 * individual piece files.
 *
 * @sa
 * vtkXMLUnstructuredGridWriter
 */

#ifndef vtkXMLPUnstructuredGridWriter_h
#define vtkXMLPUnstructuredGridWriter_h

#include "vtkIOParallelXMLModule.h" // For export macro
#include "vtkXMLPUnstructuredDataWriter.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkUnstructuredGridBase;

class VTKIOPARALLELXML_EXPORT vtkXMLPUnstructuredGridWriter : public vtkXMLPUnstructuredDataWriter
{
public:
  static vtkXMLPUnstructuredGridWriter* New();
  vtkTypeMacro(vtkXMLPUnstructuredGridWriter, vtkXMLPUnstructuredDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get/Set the writer's input.
   */
  vtkUnstructuredGridBase* GetInput();

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension() override;

protected:
  vtkXMLPUnstructuredGridWriter();
  ~vtkXMLPUnstructuredGridWriter() override;

  // see algorithm for more info
  int FillInputPortInformation(int port, vtkInformation* info) override;

  const char* GetDataSetName() override;
  vtkXMLUnstructuredDataWriter* CreateUnstructuredPieceWriter() override;

private:
  vtkXMLPUnstructuredGridWriter(const vtkXMLPUnstructuredGridWriter&) = delete;
  void operator=(const vtkXMLPUnstructuredGridWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
