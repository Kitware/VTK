/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGLTFUtils.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkGLTFWriterUtils.h"

#include "vtk_jsoncpp.h"

#include "vtkBase64OutputStream.h"
#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkUnsignedIntArray.h"

#include "vtksys/FStream.hxx"
#include "vtksys/SystemTools.hxx"

void vtkGLTFWriterUtils::WriteValues(vtkDataArray* ca, ostream& myFile)
{
  myFile.write(reinterpret_cast<char*>(ca->GetVoidPointer(0)),
    ca->GetNumberOfTuples() * ca->GetNumberOfComponents() * ca->GetElementComponentSize());
}

void vtkGLTFWriterUtils::WriteValues(vtkDataArray* ca, vtkBase64OutputStream* ostr)
{
  ostr->Write(reinterpret_cast<char*>(ca->GetVoidPointer(0)),
    ca->GetNumberOfTuples() * ca->GetNumberOfComponents() * ca->GetElementComponentSize());
}

void vtkGLTFWriterUtils::WriteBufferAndView(vtkDataArray* inda, const char* fileName,
  bool inlineData, Json::Value& buffers, Json::Value& bufferViews)
{
  vtkDataArray* da = inda;

  // gltf does not support doubles so handle that
  if (inda->GetDataType() == VTK_DOUBLE)
  {
    da = vtkFloatArray::New();
    da->DeepCopy(inda);
  }

  // if inline then base64 encode the data
  std::string result;
  if (inlineData)
  {
    result = "data:application/octet-stream;base64,";
    std::ostringstream toString;
    vtkNew<vtkBase64OutputStream> ostr;
    ostr->SetStream(&toString);
    ostr->StartWriting();
    WriteValues(da, ostr);
    ostr->EndWriting();
    result += toString.str();
  }
  else
  {
    // otherwise write binary files
    std::ostringstream toString;
    toString << "buffer" << da->GetMTime() << ".bin";
    result = toString.str();

    std::string fullPath = vtksys::SystemTools::GetFilenamePath(fileName);
    if (!fullPath.empty())
    {
      fullPath += "/";
    }
    fullPath += result;

    // now write the data
    vtksys::ofstream myFile(fullPath.c_str(), ios::out | ios::binary);

    WriteValues(da, myFile);
    myFile.close();
  }

  Json::Value buffer;
  Json::Value view;

  unsigned int count = da->GetNumberOfTuples() * da->GetNumberOfComponents();
  unsigned int byteLength = da->GetElementComponentSize() * count;
  buffer["byteLength"] = static_cast<Json::Value::Int64>(byteLength);
  buffer["uri"] = result;
  buffers.append(buffer);

  // write the buffer views
  view["buffer"] = buffers.size() - 1;
  view["byteOffset"] = 0;
  view["byteLength"] = static_cast<Json::Value::Int64>(byteLength);
  bufferViews.append(view);

  // delete double to float conversion array
  if (da != inda)
  {
    da->Delete();
  }
}

void vtkGLTFWriterUtils::WriteBufferAndView(vtkCellArray* ca, const char* fileName, bool inlineData,
  Json::Value& buffers, Json::Value& bufferViews)
{
  vtkUnsignedIntArray* ia = vtkUnsignedIntArray::New();
  vtkIdType npts;
  const vtkIdType* indx;
  for (ca->InitTraversal(); ca->GetNextCell(npts, indx);)
  {
    for (int j = 0; j < npts; ++j)
    {
      unsigned int value = static_cast<unsigned int>(indx[j]);
      ia->InsertNextValue(value);
    }
  }

  WriteBufferAndView(ia, fileName, inlineData, buffers, bufferViews);
  ia->Delete();
}
