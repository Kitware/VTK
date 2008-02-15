/*=========================================================================

  Program:   ParaView
  Module:    vtkXMLMultiBlockDataReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLMultiBlockDataReader.h"

#include "vtkCompositeDataPipeline.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkXMLDataElement.h"

vtkCxxRevisionMacro(vtkXMLMultiBlockDataReader, "1.4");
vtkStandardNewMacro(vtkXMLMultiBlockDataReader);

//----------------------------------------------------------------------------
vtkXMLMultiBlockDataReader::vtkXMLMultiBlockDataReader()
{
}

//----------------------------------------------------------------------------
vtkXMLMultiBlockDataReader::~vtkXMLMultiBlockDataReader()
{
}

//----------------------------------------------------------------------------
void vtkXMLMultiBlockDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkXMLMultiBlockDataReader::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet");
  return 1;
}

//----------------------------------------------------------------------------
const char* vtkXMLMultiBlockDataReader::GetDataSetName()
{
  return "vtkMultiBlockDataSet";
}

//----------------------------------------------------------------------------
// This version does not support multiblock of multiblocks, so our work is
// simple.
void vtkXMLMultiBlockDataReader::ReadVersion0(vtkXMLDataElement* element, 
  vtkCompositeDataSet* composite, const char* filePath, 
  unsigned int &dataSetIndex)
{
  vtkMultiBlockDataSet* mblock = vtkMultiBlockDataSet::SafeDownCast(composite);
  unsigned int numElems = element->GetNumberOfNestedElements();
  for (unsigned int cc=0; cc < numElems; ++cc)
    {
    vtkXMLDataElement* childXML = element->GetNestedElement(cc);
    if (!childXML || !childXML->GetName() || 
      strcmp(childXML->GetName(), "DataSet") != 0)
      {
      continue;
      }
    int group = 0;
    int index = 0;
    if (childXML->GetScalarAttribute("group", group) &&
      childXML->GetScalarAttribute("dataset", index))
      {
      vtkSmartPointer<vtkDataSet> dataset;
      if (this->ShouldReadDataSet(dataSetIndex))
        {
        dataset.TakeReference(this->ReadDataset(childXML, filePath));
        }
      vtkMultiBlockDataSet* block = vtkMultiBlockDataSet::SafeDownCast(
        mblock->GetBlock(group));
      if (!block)
        {
        block = vtkMultiBlockDataSet::New();
        mblock->SetBlock(group, block);
        block->Delete();
        }
      block->SetBlock(index, dataset);
      }
    dataSetIndex++;
    }
}

//----------------------------------------------------------------------------
void vtkXMLMultiBlockDataReader::ReadComposite(vtkXMLDataElement* element, 
  vtkCompositeDataSet* composite, const char* filePath, 
  unsigned int &dataSetIndex)
{
  vtkMultiBlockDataSet* mblock = vtkMultiBlockDataSet::SafeDownCast(composite);
  if (!mblock)
    {
    vtkErrorMacro("Dataset must be a vtkMultiBlockDataSet.");
    return;
    }

  if (this->GetFileMajorVersion() < 1)
    {
    // Read legacy file.
    this->ReadVersion0(element, composite, filePath, dataSetIndex);
    return;
    }

  unsigned int maxElems = element->GetNumberOfNestedElements();
  for (unsigned int cc=0; cc < maxElems; ++cc)
    {
    vtkXMLDataElement* childXML = element->GetNestedElement(cc);
    if (!childXML || !childXML->GetName())
      {
      continue;
      }

    int index = 0;
    if (!childXML->GetScalarAttribute("index", index))
      {
      index = mblock->GetNumberOfBlocks();
      }

    const char* tagName = childXML->GetName();
    if (strcmp(tagName, "DataSet") == 0)
      {
      vtkSmartPointer<vtkDataSet> childDS;
      if (this->ShouldReadDataSet(dataSetIndex))
        {
        // Read leaf node
        childDS.TakeReference(this->ReadDataset(childXML, filePath));
        }

      mblock->SetBlock(index, childDS);
      dataSetIndex++;
      }
    else if (strcmp(tagName, "Block") == 0)
      {
      // Child is a composite dataset itself. Create it.
      vtkMultiBlockDataSet* childDS = vtkMultiBlockDataSet::New();;
      this->ReadComposite(childXML, childDS, filePath, dataSetIndex);
      mblock->SetBlock(index, childDS);
      childDS->Delete();
      }
    }
}
