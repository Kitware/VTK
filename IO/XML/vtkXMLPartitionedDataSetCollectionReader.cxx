// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkXMLPartitionedDataSetCollectionReader.h"

#include "vtkBase64Utilities.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataAssembly.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkSmartPointer.h"
#include "vtkXMLDataElement.h"

#include <cctype> // for std::isspace

VTK_ABI_NAMESPACE_BEGIN
namespace
{
vtkSmartPointer<vtkDataAssembly> ReadDataAssembly(
  vtkXMLDataElement* elem, vtkXMLPartitionedDataSetCollectionReader* self)
{
  if (elem->GetAttribute("encoding") == nullptr ||
    strcmp(elem->GetAttribute("encoding"), "base64") != 0 || elem->GetCharacterData() == nullptr)
  {
    vtkWarningWithObjectMacro(self, "Unsupported DataAssembly encoding. Ignoring.");
    return nullptr;
  }

  vtkNew<vtkDataAssembly> assembly;
  const char* encoded_buffer = elem->GetCharacterData();
  size_t len_encoded_data = strlen(encoded_buffer);
  char* decoded_buffer = new char[len_encoded_data];

  // remove leading whitespace, if any.
  while (std::isspace(static_cast<int>(*encoded_buffer)))
  {
    ++encoded_buffer;
    --len_encoded_data;
  }
  auto decoded_buffer_len =
    vtkBase64Utilities::DecodeSafely(reinterpret_cast<const unsigned char*>(encoded_buffer),
      len_encoded_data, reinterpret_cast<unsigned char*>(decoded_buffer), len_encoded_data);
  decoded_buffer[decoded_buffer_len] = '\0';
  assembly->InitializeFromXML(decoded_buffer);
  delete[] decoded_buffer;
  return assembly;
}
}

vtkStandardNewMacro(vtkXMLPartitionedDataSetCollectionReader);

//------------------------------------------------------------------------------
vtkXMLPartitionedDataSetCollectionReader::vtkXMLPartitionedDataSetCollectionReader() = default;

//------------------------------------------------------------------------------
vtkXMLPartitionedDataSetCollectionReader::~vtkXMLPartitionedDataSetCollectionReader() = default;

//------------------------------------------------------------------------------
void vtkXMLPartitionedDataSetCollectionReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
int vtkXMLPartitionedDataSetCollectionReader::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPartitionedDataSetCollection");
  return 1;
}

//------------------------------------------------------------------------------
const char* vtkXMLPartitionedDataSetCollectionReader::GetDataSetName()
{
  return "vtkPartitionedDataSetCollection";
}

//------------------------------------------------------------------------------
void vtkXMLPartitionedDataSetCollectionReader::ReadComposite(vtkXMLDataElement* element,
  vtkCompositeDataSet* composite, const char* filePath, unsigned int& dataSetIndex)
{
  vtkPartitionedDataSetCollection* col = vtkPartitionedDataSetCollection::SafeDownCast(composite);
  vtkPartitionedDataSet* ds = vtkPartitionedDataSet::SafeDownCast(composite);
  if (!col && !ds)
  {
    vtkErrorMacro("Unsupported composite dataset.");
    return;
  }

  // count partitions to guide partition allocation when reading in parallel.
  const unsigned int numberOfParitions =
    ds ? vtkXMLCompositeDataReader::CountNestedElements(element, "DataSet") : 0;

  unsigned int maxElems = element->GetNumberOfNestedElements();
  for (unsigned int cc = 0; cc < maxElems; ++cc)
  {
    vtkXMLDataElement* childXML = element->GetNestedElement(cc);
    if (!childXML || !childXML->GetName())
    {
      continue;
    }

    int index = 0;
    if (!childXML->GetScalarAttribute("index", index))
    // if index not in the structure file, then
    // set up to add at the end
    {
      if (col)
      {
        index = col->GetNumberOfPartitionedDataSets();
      }
      else if (ds)
      {
        index = ds->GetNumberOfPartitions();
      }
    }
    // child is a leaf node, read and insert.
    const char* tagName = childXML->GetName();
    if (strcmp(tagName, "DataSet") == 0)
    {
      vtkSmartPointer<vtkDataObject> childDS;
      if (this->ShouldReadDataSet(dataSetIndex, index, numberOfParitions))
      {
        // Read
        childDS.TakeReference(this->ReadDataObject(childXML, filePath));
      }
      // insert
      ds->SetPartition(index, childDS);
      dataSetIndex++;
    }
    else if (col != nullptr && strcmp(tagName, "Partitions") == 0)
    {
      vtkPartitionedDataSet* childDS = vtkPartitionedDataSet::New();
      this->ReadComposite(childXML, childDS, filePath, dataSetIndex);
      col->SetPartitionedDataSet(index, childDS);
      childDS->Delete();

      // if XML node has name, set read that.
      if (auto name = childXML->GetAttribute("name"))
      {
        col->GetMetaData(index)->Set(vtkCompositeDataSet::NAME(), name);
      }
    }
    else if (col != nullptr && strcmp(tagName, "DataAssembly") == 0)
    {
      auto assembly = ::ReadDataAssembly(childXML, this);
      col->SetDataAssembly(assembly);
    }
    else
    {
      vtkErrorMacro("Syntax error in file.");
      return;
    }
  }
}
VTK_ABI_NAMESPACE_END
