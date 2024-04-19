// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef OMFProject_h
#define OMFProject_h

#include "vtkABINamespace.h"

#include <memory> // for std::unique_ptr

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArraySelection;
class vtkPartitionedDataSetCollection;
VTK_ABI_NAMESPACE_END

namespace omf
{
VTK_ABI_NAMESPACE_BEGIN

class OMFProject
{
public:
  OMFProject();
  ~OMFProject();

  /**
   * Checks that the file can be read.
   * This reads the OMF header as well as the JSON
   * to ensure both can be read
   */
  bool CanParseFile(const char* filename, vtkDataArraySelection* selection);

  /**
   * This actually processes the JSON, storing the created
   * datasets in output.
   */
  bool ProcessJSON(vtkPartitionedDataSetCollection* output, vtkDataArraySelection* selection,
    bool writeOutTextures, bool columnMajorOrdering);

private:
  struct ProjectImpl;
  std::unique_ptr<ProjectImpl> Impl;
};

VTK_ABI_NAMESPACE_END
} // end namespace omf

#endif
