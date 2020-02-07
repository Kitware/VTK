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
#include "vtkCompositeDataSet.h"
#include "vtkDataArraySelection.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkXMLDataElement.h"

vtkStandardNewMacro(vtkXMLMultiBlockDataReader);

//----------------------------------------------------------------------------
vtkXMLMultiBlockDataReader::vtkXMLMultiBlockDataReader() = default;

//----------------------------------------------------------------------------
vtkXMLMultiBlockDataReader::~vtkXMLMultiBlockDataReader() = default;

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
  vtkCompositeDataSet* composite, const char* filePath, unsigned int& dataSetIndex)
{
  vtkMultiBlockDataSet* mblock = vtkMultiBlockDataSet::SafeDownCast(composite);
  unsigned int numElems = element->GetNumberOfNestedElements();
  for (unsigned int cc = 0; cc < numElems; ++cc)
  {
    vtkXMLDataElement* childXML = element->GetNestedElement(cc);
    if (!childXML || !childXML->GetName() || strcmp(childXML->GetName(), "DataSet") != 0)
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
      vtkMultiBlockDataSet* block = vtkMultiBlockDataSet::SafeDownCast(mblock->GetBlock(group));
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
  vtkCompositeDataSet* composite, const char* filePath, unsigned int& dataSetIndex)
{
  vtkMultiBlockDataSet* mblock = vtkMultiBlockDataSet::SafeDownCast(composite);
  vtkMultiPieceDataSet* mpiece = vtkMultiPieceDataSet::SafeDownCast(composite);
  if (!mblock && !mpiece)
  {
    vtkErrorMacro("Unsupported composite dataset.");
    return;
  }

  if (this->GetFileMajorVersion() < 1)
  {
    // Read legacy file.
    this->ReadVersion0(element, composite, filePath, dataSetIndex);
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
      vtkSmartPointer<vtkDataObject> childDS;
      const char* name = nullptr;
      if (this->ShouldReadDataSet(dataSetIndex))
      {
        // Read
        childDS.TakeReference(this->ReadDataObject(childXML, filePath));
        name = childXML->GetAttribute("name");
      }
      // insert
      if (mblock)
      {
        mblock->SetBlock(index, childDS);
        mblock->GetMetaData(index)->Set(vtkCompositeDataSet::NAME(), name);
      }
      else if (mpiece)
      {
        mpiece->SetPiece(index, childDS);
        mpiece->GetMetaData(index)->Set(vtkCompositeDataSet::NAME(), name);
      }
      dataSetIndex++;
    }
    // Child is a multiblock dataset itself. Create it.
    else if (mblock != nullptr && strcmp(tagName, "Block") == 0)
    {
      vtkMultiBlockDataSet* childDS = vtkMultiBlockDataSet::New();
      this->ReadComposite(childXML, childDS, filePath, dataSetIndex);
      const char* name = childXML->GetAttribute("name");
      mblock->SetBlock(index, childDS);
      mblock->GetMetaData(index)->Set(vtkCompositeDataSet::NAME(), name);
      childDS->Delete();
    }
    // Child is a multipiece dataset. Create it.
    else if (mblock != nullptr && strcmp(tagName, "Piece") == 0)
    {
      // Look ahead to see if there is a nested Piece structure, which can happen when
      // the dataset pieces in a vtkMultiPieceDataSet are themselves split into
      // vtkMultiPieceDataSets when saved in parallel.
      vtkCompositeDataSet* childDS;
      if (childXML->FindNestedElementWithName("Piece"))
      {
        // Create a multiblock to handle a multipiece child
        childDS = vtkMultiBlockDataSet::New();
      }
      else
      {
        // Child is not multipiece, so it is safe to create a vtkMultiPieceDataSet
        childDS = vtkMultiPieceDataSet::New();
      }
      this->ReadComposite(childXML, childDS, filePath, dataSetIndex);
      const char* name = childXML->GetAttribute("name");
      mblock->SetBlock(index, childDS);
      mblock->GetMetaData(index)->Set(vtkCompositeDataSet::NAME(), name);
      childDS->Delete();
    }
    else
    {
      vtkErrorMacro("Syntax error in file.");
      return;
    }
  }
}

namespace
{
vtkInformation* CreateMetaDataIfNecessary(
  vtkMultiBlockDataSet* mblock, vtkMultiPieceDataSet* mpiece, int index)
{
  vtkInformation* piece_metadata = nullptr;
  if (mblock)
  {
    mblock->SetBlock(index, nullptr);
    piece_metadata = mblock->GetMetaData(index);
  }
  else if (mpiece)
  {
    mpiece->SetPiece(index, nullptr);
    piece_metadata = mpiece->GetMetaData(index);
  }
  return piece_metadata;
}

}

//----------------------------------------------------------------------------
int vtkXMLMultiBlockDataReader::FillMetaData(vtkCompositeDataSet* metadata,
  vtkXMLDataElement* element, const std::string& filePath, unsigned int& dataSetIndex)
{
  vtkMultiBlockDataSet* mblock = vtkMultiBlockDataSet::SafeDownCast(metadata);
  vtkMultiPieceDataSet* mpiece = vtkMultiPieceDataSet::SafeDownCast(metadata);

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
      vtkInformation* piece_metadata = CreateMetaDataIfNecessary(mblock, mpiece, index);
      double bounding_box[6];
      if (childXML->GetVectorAttribute("bounding_box", 6, bounding_box) == 6)
      {
        if (piece_metadata)
        {
          piece_metadata->Set(vtkDataObject::BOUNDING_BOX(), bounding_box, 6);
        }
      }
      int extent[6];
      if (childXML->GetVectorAttribute("extent", 6, extent) == 6)
      {
        if (piece_metadata)
        {
          piece_metadata->Set(vtkDataObject::PIECE_EXTENT(), extent, 6);
        }
      }
      if (this->ShouldReadDataSet(dataSetIndex))
      {
        this->SyncDataArraySelections(this, childXML, filePath);
      }
      dataSetIndex++;
    }
    // Child is a multiblock dataset itself. Create it.
    else if (mblock != nullptr && strcmp(tagName, "Block") == 0)
    {
      vtkMultiBlockDataSet* childDS = vtkMultiBlockDataSet::New();
      this->FillMetaData(childDS, childXML, filePath, dataSetIndex);
      if (mblock)
      {
        mblock->SetBlock(index, childDS);
      }
      else if (mpiece)
      {
        vtkErrorMacro("Multipiece data can't have composite children.");
        return 0;
      }
      childDS->Delete();
    }
    // Child is a multipiece dataset. Create it.
    else if (mblock != nullptr && strcmp(tagName, "Piece") == 0)
    {
      // Look ahead to see if there is a nested Piece structure, which can happen when
      // the dataset pieces in a vtkMultiPieceDataSet are themselves split into
      // vtkMultiPieceDataSets when saved in parallel.
      vtkCompositeDataSet* childDS;
      if (childXML->FindNestedElementWithName("Piece"))
      {
        // Create a multiblock to handle a multipiece child
        childDS = vtkMultiBlockDataSet::New();
      }
      else
      {
        // Child is not multipiece, so it is safe to create a vtkMultiPieceDataSet
        childDS = vtkMultiPieceDataSet::New();
      }

      this->FillMetaData(childDS, childXML, filePath, dataSetIndex);
      mblock->SetBlock(index, childDS);
      childDS->Delete();
      int whole_extent[6];
      if (childXML->GetVectorAttribute("whole_extent", 6, whole_extent) == 6)
      {
        vtkInformation* piece_metadata = mblock->GetMetaData(index);
        piece_metadata->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), whole_extent, 6);
      }
    }
    else
    {
      vtkErrorMacro("Syntax error in file.");
      return 0;
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLMultiBlockDataReader::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  this->Superclass::RequestInformation(request, inputVector, outputVector);

  if (this->GetFileMajorVersion() < 1)
  {
    return 1;
  }

  const std::string filePath = this->GetFilePath();
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkSmartPointer<vtkMultiBlockDataSet> metadata = vtkSmartPointer<vtkMultiBlockDataSet>::New();
  unsigned int dataSetIndex = 0;
  if (!this->FillMetaData(metadata, this->GetPrimaryElement(), filePath, dataSetIndex))
  {
    return 0;
  }
  info->Set(vtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA(), metadata);

  return 1;
}
