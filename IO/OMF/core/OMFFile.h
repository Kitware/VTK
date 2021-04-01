/*=========================================================================

  Program:   Visualization Toolkit
  Module:    OMFFile.h
  Language:  C++

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef OMFFile_h
#define OMFFile_h

#include "vtkSmartPointer.h"

#include "vtk_jsoncpp_fwd.h"

#include <memory> // for std::unique_ptr
#include <string>
#include <vector>

class vtkDataArray;
class vtkImageData;

namespace omf
{

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

} // end namespace omf
#endif
