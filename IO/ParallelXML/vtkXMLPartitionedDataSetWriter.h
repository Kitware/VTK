// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkXMLPartitionedDataSetWriter
 * @brief XML writer for vtkPartitionedDataSet
 *
 * vtkXMLPartitionedDataSetWriter is a writer for vtkPartitionedDataSet.
 * vtkXMLPartitionedDataSetWriter supports distributed use-cases. Use
 * `SetController` to set the controller to use in case of distributed
 * execution. In that case, the meta-file is only written out on the root node.
 */

#ifndef vtkXMLPartitionedDataSetWriter_h
#define vtkXMLPartitionedDataSetWriter_h

#include "vtkIOParallelXMLModule.h" // For export macro
#include "vtkXMLWriter2.h"

#include <string> // for std::string
#include <vector> // for std::vector

VTK_ABI_NAMESPACE_BEGIN
class vtkPartitionedDataSet;

class VTKIOPARALLELXML_EXPORT vtkXMLPartitionedDataSetWriter : public vtkXMLWriter2
{
public:
  static vtkXMLPartitionedDataSetWriter* New();
  vtkTypeMacro(vtkXMLPartitionedDataSetWriter, vtkXMLWriter2);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Primarily for backwards compatibility. `SetInputDataObject` is the
   * preferred API to use to set input.
   */
  void SetInputData(vtkPartitionedDataSet* pd);

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension() override { return "vtpd"; }

protected:
  vtkXMLPartitionedDataSetWriter();
  ~vtkXMLPartitionedDataSetWriter() override;

  ///@{
  /**
   * Methods to define the file's major and minor version numbers.
   * Major version incremented since v0.1 composite data readers cannot read
   * the files written by this new reader.
   */
  int GetDataSetMajorVersion() override { return 1; }
  int GetDataSetMinorVersion() override { return 0; }
  ///@}

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkXMLPartitionedDataSetWriter(const vtkXMLPartitionedDataSetWriter&) = delete;
  void operator=(const vtkXMLPartitionedDataSetWriter&) = delete;

  bool WriteSummaryXML(vtkPartitionedDataSet* input, const std::vector<std::string>& allFilenames);
};

VTK_ABI_NAMESPACE_END
#endif
