/*=========================================================================

  Program:   ParaView
  Module:    vtkXMLPartitionedDataSetCollectionReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPartitionedDataSetCollectionReader.h"

#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkSmartPointer.h"
#include "vtkXMLDataElement.h"

vtkStandardNewMacro(vtkXMLPartitionedDataSetCollectionReader);

//----------------------------------------------------------------------------
vtkXMLPartitionedDataSetCollectionReader::vtkXMLPartitionedDataSetCollectionReader() {}

//----------------------------------------------------------------------------
vtkXMLPartitionedDataSetCollectionReader::~vtkXMLPartitionedDataSetCollectionReader() {}

//----------------------------------------------------------------------------
void vtkXMLPartitionedDataSetCollectionReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkXMLPartitionedDataSetCollectionReader::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPartitionedDataSetCollection");
  return 1;
}

//----------------------------------------------------------------------------
const char* vtkXMLPartitionedDataSetCollectionReader::GetDataSetName()
{
  return "vtkPartitionedDataSetCollection";
}

//----------------------------------------------------------------------------
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
      if (this->ShouldReadDataSet(dataSetIndex))
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
    }
    else
    {
      vtkErrorMacro("Syntax error in file.");
      return;
    }
  }
}
