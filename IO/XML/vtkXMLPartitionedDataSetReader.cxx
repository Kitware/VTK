// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkXMLPartitionedDataSetReader.h"

#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkSmartPointer.h"
#include "vtkXMLDataElement.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkXMLPartitionedDataSetReader);

//------------------------------------------------------------------------------
vtkXMLPartitionedDataSetReader::vtkXMLPartitionedDataSetReader() = default;

//------------------------------------------------------------------------------
vtkXMLPartitionedDataSetReader::~vtkXMLPartitionedDataSetReader() = default;

//------------------------------------------------------------------------------
void vtkXMLPartitionedDataSetReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
int vtkXMLPartitionedDataSetReader::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPartitionedDataSet");
  return 1;
}

//------------------------------------------------------------------------------
const char* vtkXMLPartitionedDataSetReader::GetDataSetName()
{
  return "vtkPartitionedDataSet";
}

//------------------------------------------------------------------------------
void vtkXMLPartitionedDataSetReader::ReadComposite(vtkXMLDataElement* element,
  vtkCompositeDataSet* composite, const char* filePath, unsigned int& dataSetIndex)
{
  vtkPartitionedDataSet* pds = vtkPartitionedDataSet::SafeDownCast(composite);
  if (!pds)
  {
    vtkErrorMacro("Unsupported composite dataset.");
    return;
  }

  const unsigned int numberOfParitions =
    vtkXMLCompositeDataReader::CountNestedElements(element, "Dataset");
  unsigned int maxElems = element->GetNumberOfNestedElements();
  for (unsigned int cc = 0; cc < maxElems; ++cc)
  {
    vtkXMLDataElement* childXML = element->GetNestedElement(cc);
    if (!childXML || !childXML->GetName())
    {
      continue;
    }

    int index = pds->GetNumberOfPartitions();

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
      pds->SetPartition(index, childDS);
      dataSetIndex++;
    }
    else
    {
      vtkErrorMacro("Syntax error in file.");
      return;
    }
  }
}
VTK_ABI_NAMESPACE_END
