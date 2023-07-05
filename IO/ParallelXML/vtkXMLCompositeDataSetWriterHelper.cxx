// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkXMLCompositeDataSetWriterHelper.h"

#include "vtkDataObject.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkXMLDataObjectWriter.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkXMLCompositeDataSetWriterHelper);
vtkCxxSetObjectMacro(vtkXMLCompositeDataSetWriterHelper, Writer, vtkXMLWriterBase);
//----------------------------------------------------------------------------
vtkXMLCompositeDataSetWriterHelper::vtkXMLCompositeDataSetWriterHelper()
  : Writer(nullptr)
{
}

//----------------------------------------------------------------------------
vtkXMLCompositeDataSetWriterHelper::~vtkXMLCompositeDataSetWriterHelper()
{
  this->SetWriter(nullptr);
}

//----------------------------------------------------------------------------
std::string vtkXMLCompositeDataSetWriterHelper::WriteDataSet(
  const std::string& path, const std::string& prefix, vtkDataObject* data)
{
  if (!data)
  {
    return {};
  }

  bool isEmpty = true;
  for (int attrType = vtkDataObject::POINT;
       attrType < vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES && isEmpty; ++attrType)
  {
    isEmpty = (data->GetNumberOfElements(attrType) == 0);
  }

  if (isEmpty)
  {
    return {};
  }

  auto writer = this->GetWriter(data->GetDataObjectType());
  if (!writer)
  {
    vtkLogF(WARNING, "skipping dataset of unsupported type '%s'.", data->GetClassName());
    return {};
  }

  std::string fname = prefix + "." + writer->GetDefaultFileExtension();
  writer->SetInputDataObject(data);
  writer->SetFileName(path.empty() ? fname.c_str() : (path + "/" + fname).c_str());
  writer->Write();
  writer->SetInputDataObject(nullptr);

  vtkLogF(TRACE, "wrote leaf %s", fname.c_str());
  return fname;
}

//----------------------------------------------------------------------------
vtkXMLWriterBase* vtkXMLCompositeDataSetWriterHelper::GetWriter(int dataType)
{
  auto iter = this->WriterCache.find(dataType);
  if (iter != this->WriterCache.end())
  {
    return iter->second;
  }

  if (auto writer = vtkXMLDataObjectWriter::NewWriter(dataType))
  {
    // Copy settings to the writer.
    writer->SetDebug(this->Writer->GetDebug());
    writer->SetByteOrder(this->Writer->GetByteOrder());
    writer->SetCompressor(this->Writer->GetCompressor());
    writer->SetBlockSize(this->Writer->GetBlockSize());
    writer->SetDataMode(this->Writer->GetDataMode());
    writer->SetEncodeAppendedData(this->Writer->GetEncodeAppendedData());
    writer->SetHeaderType(this->Writer->GetHeaderType());
    writer->SetIdType(this->Writer->GetIdType());
    this->WriterCache[dataType].TakeReference(writer);
    return writer;
  }

  return nullptr;
}

//----------------------------------------------------------------------------
void vtkXMLCompositeDataSetWriterHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
