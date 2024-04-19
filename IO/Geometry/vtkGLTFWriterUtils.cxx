// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkGLTFWriterUtils.h"

#include "vtkBase64OutputStream.h"
#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkUnsignedIntArray.h"

#include "vtksys/FStream.hxx"
#include "vtksys/SystemTools.hxx"

#include <sstream>
#include <string>

VTK_ABI_NAMESPACE_BEGIN
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
  bool inlineData, nlohmann::json& buffers, nlohmann::json& bufferViews, int bufferViewTarget)
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

  nlohmann::json buffer;
  nlohmann::json view;

  unsigned int count = da->GetNumberOfTuples() * da->GetNumberOfComponents();
  unsigned int byteLength = da->GetElementComponentSize() * count;
  buffer["byteLength"] = byteLength;
  buffer["uri"] = result;
  buffers.emplace_back(buffer);

  // write the buffer views
  view["buffer"] = buffers.size() - 1;
  view["byteOffset"] = 0;
  view["byteLength"] = byteLength;
  view["target"] = bufferViewTarget;
  bufferViews.emplace_back(view);

  // delete double to float conversion array
  if (da != inda)
  {
    da->Delete();
  }
}

void vtkGLTFWriterUtils::WriteCellBufferAndView(vtkCellArray* ca, const char* fileName,
  bool inlineData, nlohmann::json& buffers, nlohmann::json& bufferViews)
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

  WriteBufferAndView(ia, fileName, inlineData, buffers, bufferViews, GLTF_ELEMENT_ARRAY_BUFFER);
  ia->Delete();
}
VTK_ABI_NAMESPACE_END
