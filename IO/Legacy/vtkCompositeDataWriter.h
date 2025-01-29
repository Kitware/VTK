// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCompositeDataWriter
 * @brief   legacy VTK file writer for vtkCompositeDataSet
 * subclasses.
 *
 * vtkCompositeDataWriter is a writer for writing legacy VTK files for
 * vtkCompositeDataSet and subclasses.
 * @warning
 * This is an experimental format. Use XML-based formats for writing composite
 * datasets. Saving composite dataset in legacy VTK format is expected to change
 * in future including changes to the file layout.
 */

#ifndef vtkCompositeDataWriter_h
#define vtkCompositeDataWriter_h

#include "vtkDataWriter.h"
#include "vtkIOLegacyModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkCompositeDataSet;
class vtkHierarchicalBoxDataSet;
class vtkMultiBlockDataSet;
class vtkMultiPieceDataSet;
class vtkNonOverlappingAMR;
class vtkOverlappingAMR;
class vtkPartitionedDataSet;
class vtkPartitionedDataSetCollection;

class VTKIOLEGACY_EXPORT vtkCompositeDataWriter : public vtkDataWriter
{
public:
  static vtkCompositeDataWriter* New();
  vtkTypeMacro(vtkCompositeDataWriter, vtkDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the input to this writer.
   */
  vtkCompositeDataSet* GetInput();
  vtkCompositeDataSet* GetInput(int port);
  ///@}

protected:
  vtkCompositeDataWriter();
  ~vtkCompositeDataWriter() override;

  ///@{
  /**
   * Performs the actual writing.
   */
  void WriteData() override;
  int FillInputPortInformation(int port, vtkInformation* info) override;
  ///@}

  bool WriteCompositeData(ostream*, vtkMultiBlockDataSet*);
  bool WriteCompositeData(ostream*, vtkMultiPieceDataSet*);
  VTK_DEPRECATED_IN_9_5_0("Please use `vtkOverlappingAMR` version instead.")
  bool WriteCompositeData(ostream*, vtkHierarchicalBoxDataSet*);
  bool WriteCompositeData(ostream*, vtkOverlappingAMR*);
  bool WriteCompositeData(ostream*, vtkNonOverlappingAMR*);
  bool WriteCompositeData(ostream*, vtkPartitionedDataSet*);
  bool WriteCompositeData(ostream*, vtkPartitionedDataSetCollection*);
  bool WriteBlock(ostream* fp, vtkDataObject* block);

private:
  vtkCompositeDataWriter(const vtkCompositeDataWriter&) = delete;
  void operator=(const vtkCompositeDataWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
