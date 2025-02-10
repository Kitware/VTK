// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXMLHierarchicalDataReader
 * @brief   Reader for hierarchical datasets
 *
 * vtkXMLHierarchicalDataReader reads the VTK XML hierarchical data file
 * format. XML hierarchical data files are meta-files that point to a list
 * of serial VTK XML files. When reading in parallel, it will distribute
 * sub-blocks among processor. If the number of sub-blocks is less than
 * the number of processors, some processors will not have any sub-blocks
 * for that level. If the number of sub-blocks is larger than the
 * number of processors, each processor will possibly have more than
 * 1 sub-block.
 */

#ifndef vtkXMLHierarchicalDataReader_h
#define vtkXMLHierarchicalDataReader_h

#include "vtkDeprecation.h" // For VTK_DEPRECATED_IN_9_5_0
#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLMultiGroupDataReader.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkHierarchicalDataSet;

class VTK_DEPRECATED_IN_9_5_0(
  "Please use `vtkXMLMultiBlockDataReader` instead.") VTKIOXML_EXPORT vtkXMLHierarchicalDataReader
  : public vtkXMLMultiGroupDataReader
{
public:
  static vtkXMLHierarchicalDataReader* New();
  vtkTypeMacro(vtkXMLHierarchicalDataReader, vtkXMLMultiGroupDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkXMLHierarchicalDataReader();
  ~vtkXMLHierarchicalDataReader() override;

  // Get the name of the data set being read.
  const char* GetDataSetName() override { return "vtkHierarchicalDataSet"; }

private:
  vtkXMLHierarchicalDataReader(const vtkXMLHierarchicalDataReader&) = delete;
  void operator=(const vtkXMLHierarchicalDataReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
