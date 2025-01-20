// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCompositeDataReader
 * @brief   read vtkCompositeDataSet data file.
 *
 * @warning
 * This is an experimental format. Use XML-based formats for writing composite
 * datasets. Saving composite dataset in legacy VTK format is expected to change
 * in future including changes to the file layout.
 */

#ifndef vtkCompositeDataReader_h
#define vtkCompositeDataReader_h

#include "vtkDataReader.h"
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

class VTKIOLEGACY_EXPORT vtkCompositeDataReader : public vtkDataReader
{
public:
  static vtkCompositeDataReader* New();
  vtkTypeMacro(vtkCompositeDataReader, vtkDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the output of this reader.
   */
  vtkCompositeDataSet* GetOutput();
  vtkCompositeDataSet* GetOutput(int idx);
  void SetOutput(vtkCompositeDataSet* output);
  ///@}

  /**
   * Actual reading happens here
   */
  int ReadMeshSimple(VTK_FILEPATH const std::string& fname, vtkDataObject* output) override;

protected:
  vtkCompositeDataReader();
  ~vtkCompositeDataReader() override;

  vtkDataObject* CreateOutput(vtkDataObject* currentOutput) override;

  int FillOutputPortInformation(int, vtkInformation*) override;

  /**
   * Read the output type information.
   */
  int ReadOutputType();

  bool ReadCompositeData(vtkMultiPieceDataSet*);
  bool ReadCompositeData(vtkMultiBlockDataSet*);
  VTK_DEPRECATED_IN_9_5_0("Please use `vtkOverlappingAMR` version instead.")
  bool ReadCompositeData(vtkHierarchicalBoxDataSet*);
  bool ReadCompositeData(vtkOverlappingAMR*);
  bool ReadCompositeData(vtkPartitionedDataSet*);
  bool ReadCompositeData(vtkPartitionedDataSetCollection*);
  bool ReadCompositeData(vtkNonOverlappingAMR*);
  vtkDataObject* ReadChild();

private:
  vtkCompositeDataReader(const vtkCompositeDataReader&) = delete;
  void operator=(const vtkCompositeDataReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
