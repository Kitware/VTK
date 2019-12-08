/*=========================================================================

  Program:   ParaView
  Module:    vtkXMLPartitionedDataSetReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

vtkStandardNewMacro(vtkXMLPartitionedDataSetReader);

//----------------------------------------------------------------------------
vtkXMLPartitionedDataSetReader::vtkXMLPartitionedDataSetReader() {}

//----------------------------------------------------------------------------
vtkXMLPartitionedDataSetReader::~vtkXMLPartitionedDataSetReader() {}

//----------------------------------------------------------------------------
void vtkXMLPartitionedDataSetReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkXMLPartitionedDataSetReader::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPartitionedDataSet");
  return 1;
}

//----------------------------------------------------------------------------
const char* vtkXMLPartitionedDataSetReader::GetDataSetName()
{
  return "vtkPartitionedDataSet";
}

//----------------------------------------------------------------------------
void vtkXMLPartitionedDataSetReader::ReadComposite(vtkXMLDataElement* element,
  vtkCompositeDataSet* composite, const char* filePath, unsigned int& dataSetIndex)
{
  vtkPartitionedDataSet* pds = vtkPartitionedDataSet::SafeDownCast(composite);
  if (!pds)
  {
    vtkErrorMacro("Unsupported composite dataset.");
    return;
  }

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
      if (this->ShouldReadDataSet(dataSetIndex))
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
