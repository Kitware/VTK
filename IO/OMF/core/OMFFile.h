// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef OMFFile_h
#define OMFFile_h

#include "vtkSmartPointer.h"

#include "vtk_jsoncpp_fwd.h"

#include <memory> // for std::unique_ptr
#include <string>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;
class vtkImageData;
VTK_ABI_NAMESPACE_END

namespace omf
{
VTK_ABI_NAMESPACE_BEGIN

struct OMFFile
{
  OMFFile();
  ~OMFFile();

  bool OpenStream(const char* filename);

  std::string GetFileName();

  bool ReadHeader(std::string& uid);

  bool ParseJSON();

  const Json::Value& JSONRoot();

  vtkSmartPointer<vtkDataArray> ReadArrayFromStream(const std::string& uid, int numComponents = -1);

  vtkSmartPointer<vtkImageData> ReadPNGFromStream(const Json::Value& json);

  std::vector<std::string> ReadStringArrayFromStream(const std::string& uid);

private:
  struct FileImpl;
  std::unique_ptr<FileImpl> Impl;
};

VTK_ABI_NAMESPACE_END
} // end namespace omf
#endif
