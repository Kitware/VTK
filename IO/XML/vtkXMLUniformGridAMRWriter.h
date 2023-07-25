// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXMLUniformGridAMRWriter
 * @brief   writer for vtkUniformGridAMR.
 *
 * vtkXMLUniformGridAMRWriter is a vtkXMLCompositeDataWriter subclass to
 * handle vtkUniformGridAMR datasets (including vtkNonOverlappingAMR and
 * vtkOverlappingAMR).
 */

#ifndef vtkXMLUniformGridAMRWriter_h
#define vtkXMLUniformGridAMRWriter_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLCompositeDataWriter.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKIOXML_EXPORT vtkXMLUniformGridAMRWriter : public vtkXMLCompositeDataWriter
{
public:
  static vtkXMLUniformGridAMRWriter* New();
  vtkTypeMacro(vtkXMLUniformGridAMRWriter, vtkXMLCompositeDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension() override { return "vth"; }

protected:
  vtkXMLUniformGridAMRWriter();
  ~vtkXMLUniformGridAMRWriter() override;

  /**
   * Methods to define the file's major and minor version numbers.
   * VTH/VTHB version number 1.1 is used for overlapping/non-overlapping AMR
   * datasets.
   */
  int GetDataSetMajorVersion() override { return 1; }
  int GetDataSetMinorVersion() override { return 1; }

  int FillInputPortInformation(int port, vtkInformation* info) override;

  // Internal method called recursively to create the xml tree for the children
  // of compositeData.
  int WriteComposite(
    vtkCompositeDataSet* compositeData, vtkXMLDataElement* parent, int& writerIdx) override;

private:
  vtkXMLUniformGridAMRWriter(const vtkXMLUniformGridAMRWriter&) = delete;
  void operator=(const vtkXMLUniformGridAMRWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
