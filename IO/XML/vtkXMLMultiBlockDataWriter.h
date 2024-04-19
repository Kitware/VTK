// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXMLMultiBlockDataWriter
 * @brief   writer for vtkMultiBlockDataSet.
 *
 * vtkXMLMultiBlockDataWriter is a vtkXMLCompositeDataWriter subclass to handle
 * vtkMultiBlockDataSet.
 */

#ifndef vtkXMLMultiBlockDataWriter_h
#define vtkXMLMultiBlockDataWriter_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLCompositeDataWriter.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKIOXML_EXPORT vtkXMLMultiBlockDataWriter : public vtkXMLCompositeDataWriter
{
public:
  static vtkXMLMultiBlockDataWriter* New();
  vtkTypeMacro(vtkXMLMultiBlockDataWriter, vtkXMLCompositeDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension() override { return "vtm"; }

protected:
  vtkXMLMultiBlockDataWriter();
  ~vtkXMLMultiBlockDataWriter() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  // Internal method called recursively to create the xml tree for the children
  // of compositeData.
  int WriteComposite(
    vtkCompositeDataSet* compositeData, vtkXMLDataElement* parent, int& writerIdx) override;

private:
  vtkXMLMultiBlockDataWriter(const vtkXMLMultiBlockDataWriter&) = delete;
  void operator=(const vtkXMLMultiBlockDataWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
