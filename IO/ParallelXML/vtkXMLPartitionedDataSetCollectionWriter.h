// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkXMLPartitionedDataSetCollectionWriter
 * @brief writer for vtkPartitionedDataSetCollection.
 *
 * vtkXMLPartitionedDataSetCollectionWriter is a writer for vtkPartitionedDataSetCollection.
 * This writer supports distributed use-cases as well. Use `SetController` to set the
 * controller to use in case of distributed execution. In that case, the meta-file is written
 * only on the root node.
 */

#ifndef vtkXMLPartitionedDataSetCollectionWriter_h
#define vtkXMLPartitionedDataSetCollectionWriter_h

#include "vtkIOParallelXMLModule.h" // For export macro
#include "vtkXMLWriter2.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkPartitionedDataSetCollection;

class VTKIOPARALLELXML_EXPORT vtkXMLPartitionedDataSetCollectionWriter : public vtkXMLWriter2
{
public:
  static vtkXMLPartitionedDataSetCollectionWriter* New();
  vtkTypeMacro(vtkXMLPartitionedDataSetCollectionWriter, vtkXMLWriter2);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Primarily for backwards compatibility. `SetInputDataObject` is the
   * preferred API to use to set input.
   */
  void SetInputData(vtkPartitionedDataSetCollection* pd);

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension() override { return "vtpc"; }

protected:
  vtkXMLPartitionedDataSetCollectionWriter();
  ~vtkXMLPartitionedDataSetCollectionWriter() override;

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
  vtkXMLPartitionedDataSetCollectionWriter(
    const vtkXMLPartitionedDataSetCollectionWriter&) = delete;
  void operator=(const vtkXMLPartitionedDataSetCollectionWriter&) = delete;

  bool WriteSummaryXML(vtkPartitionedDataSetCollection* input,
    const std::vector<std::vector<std::string>>& allFilenames);
};

VTK_ABI_NAMESPACE_END
#endif
