// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkXMLMultiBlockDataReader.h"

#include "vtkCallbackCommand.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataAssembly.h"
#include "vtkDataAssemblyUtilities.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkXMLDataElement.h"

#include <algorithm>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkXMLMultiBlockDataReader);
//------------------------------------------------------------------------------
vtkXMLMultiBlockDataReader::vtkXMLMultiBlockDataReader()
{
  this->SetSelector("/"); // Default to read everything and maintain backwards compatibility
}

//------------------------------------------------------------------------------
vtkXMLMultiBlockDataReader::~vtkXMLMultiBlockDataReader() = default;

//------------------------------------------------------------------------------
void vtkXMLMultiBlockDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "Assembly: ";
  if (this->Assembly)
  {
    this->Assembly->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "is nullptr" << std::endl;
  }

  os << "AssemblyTag: " << this->AssemblyTag << std::endl;

  os << "Selectors:";
  for (const auto& selector : this->Selectors)
  {
    os << "\n" << selector;
  }
  os << std::endl;
}

//------------------------------------------------------------------------------
int vtkXMLMultiBlockDataReader::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet");
  return 1;
}

//------------------------------------------------------------------------------
const char* vtkXMLMultiBlockDataReader::GetDataSetName()
{
  return "vtkMultiBlockDataSet";
}

//------------------------------------------------------------------------------
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
bool vtkXMLMultiBlockDataReader::AddSelector(const char* selector)
{
  if (selector && this->Selectors.insert(selector).second)
  {
    this->Modified();
    return true;
  }

  return false;
}

//----------------------------------------------------------------------------
void vtkXMLMultiBlockDataReader::ClearSelectors()
{
  if (!this->Selectors.empty())
  {
    this->Selectors.clear();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkXMLMultiBlockDataReader::SetSelector(const char* selector)
{
  this->ClearSelectors();
  this->AddSelector(selector);
}

//----------------------------------------------------------------------------
int vtkXMLMultiBlockDataReader::GetNumberOfSelectors() const
{
  return static_cast<int>(this->Selectors.size());
}

//----------------------------------------------------------------------------
const char* vtkXMLMultiBlockDataReader::GetSelector(int index) const
{
  if (index >= 0 && index < this->GetNumberOfSelectors())
  {
    auto iter = std::next(this->Selectors.begin(), index);
    return iter->c_str();
  }
  return nullptr;
}

//------------------------------------------------------------------------------
bool vtkXMLMultiBlockDataReader::IsBlockSelected(unsigned int compositeIndex)
{
  return std::find(this->SelectedCompositeIds.begin(), this->SelectedCompositeIds.end(),
           compositeIndex) != this->SelectedCompositeIds.end();
}

//------------------------------------------------------------------------------
void vtkXMLMultiBlockDataReader::ReadCompositeInternal(vtkXMLDataElement* element,
  vtkCompositeDataSet* composite, const char* filePath, unsigned int& dataSetIndex,
  unsigned int& compositeIndex)
{
  vtkMultiBlockDataSet* mb = vtkMultiBlockDataSet::SafeDownCast(composite);
  vtkMultiPieceDataSet* mp = vtkMultiPieceDataSet::SafeDownCast(composite);
  if (!mb && !mp)
  {
    if (composite)
    {
      vtkErrorMacro("Unsupported composite dataset.");
    }
    return;
  }
  compositeIndex++;

  // count how may piece in total are there when reading a multi-piece.
  // This helps with distribution of the pieces.
  const unsigned int numPieces = (mp && this->DistributePiecesInMultiPieces)
    ? vtkXMLCompositeDataReader::CountNestedElements(element, "DataSet")
    : 0;

  const unsigned int maxElems = element->GetNumberOfNestedElements();
  for (unsigned int cc = 0; cc < maxElems; ++cc)
  {
    vtkXMLDataElement* childXML = element->GetNestedElement(cc);
    if (!childXML || !childXML->GetName())
    {
      continue;
    }
    const char* tagName = childXML->GetName();

    int index = 0;
    if (!childXML->GetScalarAttribute("index", index))
    {
      // if index not in the structure file, then set up to add at the end
      if (mb)
      {
        index = mb->GetNumberOfBlocks();
      }
      else if (mp)
      {
        index = mp->GetNumberOfPieces();
      }
    }
    // child is a leaf node, read and insert.
    if (strcmp(tagName, "DataSet") == 0)
    {
      vtkSmartPointer<vtkDataObject> childDS;
      if (this->ShouldReadDataSet(dataSetIndex, index, numPieces))
      {
        if (this->IsBlockSelected(compositeIndex))
        {
          // Read
          childDS.TakeReference(this->ReadDataObject(childXML, filePath));
        }
      }
      // insert
      if (mb)
      {
        mb->SetBlock(index, childDS);
      }
      else if (mp)
      {
        mp->SetPiece(index, childDS);
      }
      dataSetIndex++;
      compositeIndex++;
    }
    // Child is a multiblock dataset itself.
    else if (mb && strcmp(tagName, "Block") == 0)
    {
      auto childDS = vtkMultiBlockDataSet::SafeDownCast(mb->GetBlock(cc));
      this->ReadCompositeInternal(childXML, childDS, filePath, dataSetIndex, compositeIndex);
    }
    // Child is a multipiece dataset.
    else if (mb && strcmp(tagName, "Piece") == 0)
    {
      // child can either be a vtkMultiBlockDataSet or vtkMultiPieceDataSet
      // see in FillMetaData why this can happen
      auto childDS = vtkCompositeDataSet::SafeDownCast(mb->GetBlock(cc));
      this->ReadCompositeInternal(childXML, childDS, filePath, dataSetIndex, compositeIndex);
    }
    else
    {
      vtkErrorMacro("Syntax error in file.");
      return;
    }
  }
}

//------------------------------------------------------------------------------
void vtkXMLMultiBlockDataReader::ReadComposite(vtkXMLDataElement* element,
  vtkCompositeDataSet* composite, const char* filePath, unsigned int& dataSetIndex)
{
  vtkMultiBlockDataSet* mb = vtkMultiBlockDataSet::SafeDownCast(composite);
  if (!mb)
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

  this->SelectedCompositeIds = vtkDataAssemblyUtilities::GetSelectedCompositeIds(
    { this->Selectors.begin(), this->Selectors.end() }, this->Assembly, /*data*/ nullptr,
    /*leaf nodes*/ true);

  unsigned int compositeIndex = 0;
  this->ReadCompositeInternal(element, composite, filePath, dataSetIndex, compositeIndex);
}

//------------------------------------------------------------------------------
int vtkXMLMultiBlockDataReader::FillMetaData(
  vtkCompositeDataSet* metadata, vtkXMLDataElement* element)
{
  vtkMultiBlockDataSet* mb = vtkMultiBlockDataSet::SafeDownCast(metadata);
  vtkMultiPieceDataSet* mp = vtkMultiPieceDataSet::SafeDownCast(metadata);

  const unsigned int maxElems = element->GetNumberOfNestedElements();
  for (unsigned int cc = 0; cc < maxElems; ++cc)
  {
    vtkXMLDataElement* childXML = element->GetNestedElement(cc);
    if (!childXML || !childXML->GetName())
    {
      continue;
    }
    const char* tagName = childXML->GetName();

    int index = 0;
    if (!childXML->GetScalarAttribute("index", index))
    {
      // if index not in the structure file, then set up to add at the end
      if (mb)
      {
        index = mb->GetNumberOfBlocks();
      }
      else if (mp)
      {
        index = mp->GetNumberOfPieces();
      }
    }
    // child is a leaf node, read and insert.
    if (strcmp(tagName, "DataSet") == 0)
    {
      vtkInformation* childInformation = nullptr;
      if (mb)
      {
        mb->SetBlock(index, nullptr);
        childInformation = mb->GetMetaData(index);
      }
      else if (mp)
      {
        mp->SetPiece(index, nullptr);
        childInformation = mp->GetMetaData(index);
      }
      if (childInformation)
      {
        if (auto name = childXML->GetAttribute("name"))
        {
          childInformation->Set(vtkCompositeDataSet::NAME(), name);
        }
        double bounding_box[6];
        if (childXML->GetVectorAttribute("bounding_box", 6, bounding_box) == 6)
        {
          childInformation->Set(vtkDataObject::BOUNDING_BOX(), bounding_box, 6);
        }
        int extent[6];
        if (childXML->GetVectorAttribute("extent", 6, extent) == 6)
        {
          childInformation->Set(vtkDataObject::PIECE_EXTENT(), extent, 6);
        }
      }
    }
    // Child is a multiblock dataset itself. Create it.
    else if (mb && strcmp(tagName, "Block") == 0)
    {
      vtkNew<vtkMultiBlockDataSet> childDS;
      this->FillMetaData(childDS, childXML);
      mb->SetBlock(index, childDS);
      if (auto name = childXML->GetAttribute("name"))
      {
        mb->GetMetaData(index)->Set(vtkCompositeDataSet::NAME(), name);
      }
    }
    // Child is a multipiece dataset. Create it.
    else if (mb && strcmp(tagName, "Piece") == 0)
    {
      // Look ahead to see if there is a nested Piece structure, which can happen when
      // the dataset pieces in a vtkMultiPieceDataSet are themselves split into
      // vtkMultiPieceDataSets when saved in parallel.
      vtkSmartPointer<vtkCompositeDataSet> childDS;
      if (childXML->FindNestedElementWithName("Piece"))
      {
        // Create a multiblock to handle a multipiece child
        childDS = vtkSmartPointer<vtkMultiBlockDataSet>::New();
      }
      else
      {
        // Child is not multipiece, so it is safe to create a vtkMultiPieceDataSet
        childDS = vtkSmartPointer<vtkMultiPieceDataSet>::New();
      }

      this->FillMetaData(childDS, childXML);
      mb->SetBlock(index, childDS);
      if (auto name = childXML->GetAttribute("name"))
      {
        mb->GetMetaData(index)->Set(vtkCompositeDataSet::NAME(), name);
      }
      int whole_extent[6];
      if (childXML->GetVectorAttribute("whole_extent", 6, whole_extent) == 6)
      {
        mb->GetMetaData(index)->Set(
          vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), whole_extent, 6);
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

//------------------------------------------------------------------------------
void vtkXMLMultiBlockDataReader::PrepareToCreateMetaData(vtkXMLDataElement* ePrimary)
{
  if (this->GetFileMajorVersion() < 1)
  {
    return;
  }
  // HACK: for now, if this is a multiblock of multi-pieces alone,
  // we can use a piece-based strategy for distributing pieces.
  bool is_multiblock_of_multipieces = true;
  for (unsigned int cc = 0, max = ePrimary->GetNumberOfNestedElements();
       is_multiblock_of_multipieces && cc < max; ++cc)
  {
    auto childXML = ePrimary->GetNestedElement(cc);
    if (childXML == nullptr || childXML->GetName() == nullptr)
    {
      continue;
    }

    is_multiblock_of_multipieces = (strcmp(childXML->GetName(), "Piece") == 0);
  }
  this->DistributePiecesInMultiPieces = is_multiblock_of_multipieces;
}

//------------------------------------------------------------------------------
void vtkXMLMultiBlockDataReader::CreateMetaData(vtkXMLDataElement* ePrimary)
{
  if (this->GetFileMajorVersion() < 1)
  {
    return;
  }
  this->Metadata = vtkSmartPointer<vtkMultiBlockDataSet>::New();
  this->FillMetaData(this->Metadata, ePrimary);

  vtkNew<vtkDataAssembly> hierarchy;
  vtkDataAssemblyUtilities::GenerateHierarchy(this->Metadata, hierarchy);
  const std::string assemblyXMLContents = hierarchy->SerializeToXML(vtkIndent());
  const std::string existingAssemblyXMLContents = this->Assembly->SerializeToXML(vtkIndent());
  if (existingAssemblyXMLContents != assemblyXMLContents)
  {
    // Assembly has been updated
    this->Assembly->InitializeFromXML(assemblyXMLContents.c_str());
    // Update Assembly widget
    ++this->AssemblyTag;
  }
}

//------------------------------------------------------------------------------
void vtkXMLMultiBlockDataReader::SyncCompositeDataArraySelections(
  vtkCompositeDataSet* composite, vtkXMLDataElement* element, const std::string& filePath)
{
  auto mb = vtkMultiBlockDataSet::SafeDownCast(composite);
  auto mp = vtkMultiPieceDataSet::SafeDownCast(composite);

  for (int cc = 0; cc < element->GetNumberOfNestedElements(); ++cc)
  {
    vtkXMLDataElement* childXML = element->GetNestedElement(cc);
    if (!childXML || !childXML->GetName())
    {
      continue;
    }
    const char* tagName = childXML->GetName();

    int index = 0;
    if (!childXML->GetScalarAttribute("index", index))
    {
      // if index not in the structure file, then set up to add at the end
      if (mb)
      {
        index = mb->GetNumberOfBlocks();
      }
      else if (mp)
      {
        index = mp->GetNumberOfPieces();
      }
    }
    if (strcmp(tagName, "DataSet") == 0)
    {
      if (mp && index > 0)
      {
        // don't read array selections for multi-pieces except the first one
        // since that is not expected to change across datasets in a multipiece.
      }
      else
      {
        this->SyncDataArraySelections(this, childXML, filePath);
      }
    }
    else
    {
      auto childComposite = mb ? vtkCompositeDataSet::SafeDownCast(mb->GetBlock(cc)) : nullptr;
      if (childComposite)
      {
        this->SyncCompositeDataArraySelections(childComposite, childXML, filePath);
      }
    }
  }
}
VTK_ABI_NAMESPACE_END
