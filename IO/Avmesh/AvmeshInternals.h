// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef AvmeshInternals_h
#define AvmeshInternals_h

#include <stdexcept>
#include <string>
#include <vtkABINamespace.h>

VTK_ABI_NAMESPACE_BEGIN

class vtkPartitionedDataSetCollection;

void ReadAvmesh(vtkPartitionedDataSetCollection* output, std::string fname, bool SurfaceOnly,
  bool BuildConnectivityIteratively);

class AvmeshError : public std::runtime_error
{
public:
  AvmeshError(std::string msg);
};

VTK_ABI_NAMESPACE_END
#endif // AvmeshInternals_h
