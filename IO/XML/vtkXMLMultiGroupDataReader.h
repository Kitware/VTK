// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXMLMultiGroupDataReader
 * @brief   Reader for multi-block datasets
 *
 * vtkXMLMultiGroupDataReader is a legacy reader that reads multi group files
 * into multiblock datasets.
 */

#ifndef vtkXMLMultiGroupDataReader_h
#define vtkXMLMultiGroupDataReader_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLMultiBlockDataReader.h"

VTK_ABI_NAMESPACE_BEGIN
class VTK_DEPRECATED_IN_9_5_0(
  "Please use `vtkXMLMultiBlockDataReader` instead.") VTKIOXML_EXPORT vtkXMLMultiGroupDataReader
  : public vtkXMLMultiBlockDataReader
{
public:
  static vtkXMLMultiGroupDataReader* New();
  vtkTypeMacro(vtkXMLMultiGroupDataReader, vtkXMLMultiBlockDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkXMLMultiGroupDataReader();
  ~vtkXMLMultiGroupDataReader() override;

  // Get the name of the data set being read.
  const char* GetDataSetName() override { return "vtkMultiGroupDataSet"; }

private:
  vtkXMLMultiGroupDataReader(const vtkXMLMultiGroupDataReader&) = delete;
  void operator=(const vtkXMLMultiGroupDataReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
