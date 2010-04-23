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
#include "vtkMultiPieceDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkXMLDataElement.h"

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
  vtkMultiPieceDataSet* mpiece = vtkMultiPieceDataSet::SafeDownCast(composite);
  if (!mblock && !mpiece)
    {
    vtkErrorMacro("Unsuported composite dataset.");
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
    // if index not in the structure file, then 
    // set up to add at the end
      {
      if (mblock)
        {
        index = mblock->GetNumberOfBlocks();
        }
      else if (mpiece)
        {
        index = mpiece->GetNumberOfPieces();
        }
      }
    // child is a leaf node, read and insert.
    const char* tagName = childXML->GetName();
    if (strcmp(tagName, "DataSet") == 0)
      {
      vtkSmartPointer<vtkDataSet> childDS;
      if (this->ShouldReadDataSet(dataSetIndex))
        {
        // Read
        childDS.TakeReference(this->ReadDataset(childXML, filePath));
        }
      // insert
      if (mblock)
        {
        mblock->SetBlock(index, childDS);
        }
      else if (mpiece)
        {
        mpiece->SetPiece(index, childDS);
        }
      dataSetIndex++;
      }
    // Child is a multiblock dataset itself. Create it.
    else if (mblock != 0
             && strcmp(tagName, "Block") == 0)
      {
      vtkMultiBlockDataSet* childDS = vtkMultiBlockDataSet::New();;
      this->ReadComposite(childXML, childDS, filePath, dataSetIndex);
      if (mblock)
        {
        mblock->SetBlock(index, childDS);
        }
      else if (mpiece)
        {
        vtkErrorMacro("Multipiece data can't have composite children.");
        return;
        }
      childDS->Delete();
      }
    // Child is a multipiece dataset. Create it.
    else if (mblock!=0
             && strcmp(tagName, "Piece") == 0)
      {
      vtkMultiPieceDataSet* childDS = vtkMultiPieceDataSet::New();;
      this->ReadComposite(childXML, childDS, filePath, dataSetIndex);
      mblock->SetBlock(index, childDS);
      childDS->Delete();
      }
    else
      {
      vtkErrorMacro("Syntax error in file.");
      return;
      }
    }
}
