/*=========================================================================

  Program:   ParaView
  Module:    vtkXMLCompositeDataReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLCompositeDataReader.h"

#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArraySelection.h"
#include "vtkDataSet.h"
#include "vtkEventForwarderCommand.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkUniformGrid.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLDataParser.h"
#include "vtkXMLHyperTreeGridReader.h"
#include "vtkXMLImageDataReader.h"
#include "vtkXMLPolyDataReader.h"
#include "vtkXMLRectilinearGridReader.h"
#include "vtkXMLStructuredGridReader.h"
#include "vtkXMLTableReader.h"
#include "vtkXMLUnstructuredGridReader.h"

#include <map>
#include <set>
#include <string>
#include <vector>
#include <vtksys/SystemTools.hxx>

struct vtkXMLCompositeDataReaderEntry
{
  const char* extension;
  const char* name;
};

struct vtkXMLCompositeDataReaderInternals
{
  vtkXMLCompositeDataReaderInternals()
  {
    this->Piece = 0;
    this->NumPieces = 1;
    this->NumDataSets = 1;
    this->HasUpdateRestriction = false;
  }

  vtkSmartPointer<vtkXMLDataElement> Root;
  typedef std::map<std::string, vtkSmartPointer<vtkXMLReader> > ReadersType;
  ReadersType Readers;
  static const vtkXMLCompositeDataReaderEntry ReaderList[];
  unsigned int Piece;
  unsigned int NumPieces;
  unsigned int NumDataSets;
  std::set<int> UpdateIndices;
  bool HasUpdateRestriction;
};

//----------------------------------------------------------------------------
vtkXMLCompositeDataReader::vtkXMLCompositeDataReader()
  : PieceDistribution(Block)
{
  this->Internal = new vtkXMLCompositeDataReaderInternals;
}

//----------------------------------------------------------------------------
vtkXMLCompositeDataReader::~vtkXMLCompositeDataReader()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkXMLCompositeDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "PieceDistribution: ";
  switch (this->PieceDistribution)
  {
    case Block:
      os << "Block\n";
      break;

    case Interleave:
      os << "Interleave\n";
      break;

    default:
      os << "Invalid (!!)\n";
      break;
  }

  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
const char* vtkXMLCompositeDataReader::GetDataSetName()
{
  return "vtkCompositeDataSet";
}

//----------------------------------------------------------------------------
void vtkXMLCompositeDataReader::SetupEmptyOutput()
{
  this->GetCurrentOutput()->Initialize();
}

//----------------------------------------------------------------------------
int vtkXMLCompositeDataReader::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkCompositeDataSet");
  return 1;
}

//----------------------------------------------------------------------------
vtkExecutive* vtkXMLCompositeDataReader::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//----------------------------------------------------------------------------
std::string vtkXMLCompositeDataReader::GetFilePath()
{
  std::string filePath = this->FileName;
  std::string::size_type pos = filePath.find_last_of("/\\");
  if (pos != filePath.npos)
  {
    filePath = filePath.substr(0, pos);
  }
  else
  {
    filePath = "";
  }

  return filePath;
}

//----------------------------------------------------------------------------
vtkCompositeDataSet* vtkXMLCompositeDataReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkCompositeDataSet* vtkXMLCompositeDataReader::GetOutput(int port)
{
  vtkDataObject* output =
    vtkCompositeDataPipeline::SafeDownCast(this->GetExecutive())->GetCompositeOutputData(port);
  return vtkCompositeDataSet::SafeDownCast(output);
}

//----------------------------------------------------------------------------
int vtkXMLCompositeDataReader::ReadPrimaryElement(vtkXMLDataElement* ePrimary)
{
  if (!this->Superclass::ReadPrimaryElement(ePrimary))
  {
    return 0;
  }

  vtkXMLDataElement* root = this->XMLParser->GetRootElement();
  int numNested = root->GetNumberOfNestedElements();
  for (int i = 0; i < numNested; ++i)
  {
    vtkXMLDataElement* eNested = root->GetNestedElement(i);
    if (strcmp(eNested->GetName(), "FieldData") == 0)
    {
      this->FieldDataElement = eNested;
    }
  }

  // Simply save the XML tree. We'll iterate over it later.
  this->Internal->Root = ePrimary;
  return 1;
}

//----------------------------------------------------------------------------
vtkXMLDataElement* vtkXMLCompositeDataReader::GetPrimaryElement()
{
  return this->Internal->Root;
}

//----------------------------------------------------------------------------
std::string vtkXMLCompositeDataReader::GetFileNameFromXML(
  vtkXMLDataElement* xmlElem, const std::string& filePath)
{
  // Construct the name of the internal file.
  const char* file = xmlElem->GetAttribute("file");
  if (!file)
  {
    return std::string();
  }

  std::string fileName;
  if (!(file[0] == '/' || file[1] == ':'))
  {
    fileName = filePath;
    if (fileName.length())
    {
      fileName += "/";
    }
  }
  fileName += file;

  return fileName;
}

//----------------------------------------------------------------------------
vtkXMLReader* vtkXMLCompositeDataReader::GetReaderOfType(const char* type)
{
  if (!type)
  {
    return nullptr;
  }

  vtkXMLCompositeDataReaderInternals::ReadersType::iterator iter =
    this->Internal->Readers.find(type);
  if (iter != this->Internal->Readers.end())
  {
    return iter->second;
  }

  vtkXMLReader* reader = nullptr;
  if (strcmp(type, "vtkXMLImageDataReader") == 0)
  {
    reader = vtkXMLImageDataReader::New();
  }
  else if (strcmp(type, "vtkXMLUnstructuredGridReader") == 0)
  {
    reader = vtkXMLUnstructuredGridReader::New();
  }
  else if (strcmp(type, "vtkXMLPolyDataReader") == 0)
  {
    reader = vtkXMLPolyDataReader::New();
  }
  else if (strcmp(type, "vtkXMLRectilinearGridReader") == 0)
  {
    reader = vtkXMLRectilinearGridReader::New();
  }
  else if (strcmp(type, "vtkXMLStructuredGridReader") == 0)
  {
    reader = vtkXMLStructuredGridReader::New();
  }
  else if (strcmp(type, "vtkXMLTableReader") == 0)
  {
    reader = vtkXMLTableReader::New();
  }
  else if (strcmp(type, "vtkXMLHyperTreeGridReader") == 0)
  {
    reader = vtkXMLHyperTreeGridReader::New();
  }
  if (reader)
  {
    if (this->GetParserErrorObserver())
    {
      reader->SetParserErrorObserver(this->GetParserErrorObserver());
    }
    if (this->HasObserver("ErrorEvent"))
    {
      vtkNew<vtkEventForwarderCommand> fwd;
      fwd->SetTarget(this);
      reader->AddObserver("ErrorEvent", fwd);
    }
    this->Internal->Readers[type] = reader;
    reader->Delete();
  }
  return reader;
}

//----------------------------------------------------------------------------
vtkXMLReader* vtkXMLCompositeDataReader::GetReaderForFile(const std::string& fileName)
{
  // Get the file extension.
  std::string ext = vtksys::SystemTools::GetFilenameLastExtension(fileName);
  if (!ext.empty())
  {
    // remove "." from the extension.
    ext.erase(0, 1);
  }

  // Search for the reader matching this extension.
  const char* rname = nullptr;
  for (const vtkXMLCompositeDataReaderEntry* readerEntry = this->Internal->ReaderList;
       !rname && readerEntry->extension; ++readerEntry)
  {
    if (ext == readerEntry->extension)
    {
      rname = readerEntry->name;
    }
  }

  return this->GetReaderOfType(rname);
}

//----------------------------------------------------------------------------
unsigned int vtkXMLCompositeDataReader::CountLeaves(vtkXMLDataElement* elem)
{
  unsigned int count = 0;
  if (elem)
  {
    unsigned int max = elem->GetNumberOfNestedElements();
    for (unsigned int cc = 0; cc < max; ++cc)
    {
      vtkXMLDataElement* child = elem->GetNestedElement(cc);
      if (child && child->GetName())
      {
        if (strcmp(child->GetName(), "DataSet") == 0)
        {
          count++;
        }
        else
        {
          count += this->CountLeaves(child);
        }
      }
    }
  }
  return count;
}

//----------------------------------------------------------------------------
void vtkXMLCompositeDataReader::ReadXMLData()
{
  vtkInformation* info = this->GetCurrentOutputInformation();

  this->Internal->Piece =
    static_cast<unsigned int>(info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  this->Internal->NumPieces = static_cast<unsigned int>(
    info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
  this->Internal->NumDataSets = this->CountLeaves(this->GetPrimaryElement());

  vtkDataObject* doOutput = info->Get(vtkDataObject::DATA_OBJECT());
  vtkCompositeDataSet* composite = vtkCompositeDataSet::SafeDownCast(doOutput);
  if (!composite)
  {
    return;
  }

  this->ReadFieldData();

  // Find the path to this file in case the internal files are
  // specified as relative paths.
  std::string filePath = this->GetFilePath();

  vtkInformation* outInfo = this->GetCurrentOutputInformation();
  if (outInfo->Has(vtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES()))
  {
    this->Internal->HasUpdateRestriction = true;
    this->Internal->UpdateIndices = std::set<int>();
    int length = outInfo->Length(vtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES());
    if (length > 0)
    {
      int* idx = outInfo->Get(vtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES());
      this->Internal->UpdateIndices = std::set<int>(idx, idx + length);

      // Change the total number of datasets so that we'll properly load
      // balance across the valid datasets.
      this->Internal->NumDataSets = length;
    }
  }
  else
  {
    this->Internal->HasUpdateRestriction = false;
  }

  // All processes create the entire tree structure however, but each one only
  // reads the datasets assigned to it.
  unsigned int dataSetIndex = 0;
  this->ReadComposite(this->GetPrimaryElement(), composite, filePath.c_str(), dataSetIndex);
}

//----------------------------------------------------------------------------
int vtkXMLCompositeDataReader::ShouldReadDataSet(unsigned int idx)
{
  // Apply the update restriction:
  if (this->Internal->HasUpdateRestriction)
  {
    auto iter = this->Internal->UpdateIndices.find(idx);
    if (iter == this->Internal->UpdateIndices.end())
    {
      return 0;
    }

    // Update the dataset index to its position in the update indices:
    idx = std::distance(this->Internal->UpdateIndices.begin(), iter);
  }

  int result = 0;

  switch (this->PieceDistribution)
  {
    case vtkXMLCompositeDataReader::Block:
      result = this->DataSetIsValidForBlockStrategy(idx) ? 1 : 0;
      break;

    case vtkXMLCompositeDataReader::Interleave:
      result = this->DataSetIsValidForInterleaveStrategy(idx) ? 1 : 0;
      break;

    default:
      vtkErrorMacro("Invalid PieceDistribution setting: " << this->PieceDistribution);
      break;
  }

  return result;
}

//------------------------------------------------------------------------------
bool vtkXMLCompositeDataReader::DataSetIsValidForBlockStrategy(unsigned int idx)
{
  // Minimum number of datasets per block:
  unsigned int blockSize = 1;

  // Number of blocks with an extra dataset due to overflow:
  unsigned int overflowBlocks = 0;

  // Adjust values if overflow is detected:
  if (this->Internal->NumPieces < this->Internal->NumDataSets)
  {
    blockSize = this->Internal->NumDataSets / this->Internal->NumPieces;
    overflowBlocks = this->Internal->NumDataSets % this->Internal->NumPieces;
  }

  // Size of an overflow block:
  const unsigned int blockSizeOverflow = blockSize + 1;

  unsigned int minDS; // Minimum valid dataset index
  unsigned int maxDS; // Maximum valid dataset index
  if (this->Internal->Piece < overflowBlocks)
  {
    minDS = blockSizeOverflow * this->Internal->Piece;
    maxDS = minDS + blockSizeOverflow;
  }
  else
  {
    // Account for earlier blocks that have an overflowed dataset:
    const unsigned int overflowOffset = blockSizeOverflow * overflowBlocks;
    // Number of preceding blocks that don't overflow:
    const unsigned int regularBlocks = this->Internal->Piece - overflowBlocks;
    // Offset due to regular blocks:
    const unsigned int regularOffset = blockSize * regularBlocks;

    minDS = overflowOffset + regularOffset;
    maxDS = minDS + blockSize;
  }

  return idx >= minDS && idx < maxDS;
}

//------------------------------------------------------------------------------
bool vtkXMLCompositeDataReader::DataSetIsValidForInterleaveStrategy(unsigned int idx)
{
  // Use signed integers for the modulus -- otherwise weird things like
  // (-1 % 3) == 0 will happen!
  int i = static_cast<int>(idx);
  int p = static_cast<int>(this->Internal->Piece);
  int n = static_cast<int>(this->Internal->NumPieces);

  return ((i - p) % n) == 0;
}

//----------------------------------------------------------------------------
vtkDataObject* vtkXMLCompositeDataReader::ReadDataObject(
  vtkXMLDataElement* xmlElem, const char* filePath)
{
  // Get the reader for this file
  std::string fileName = this->GetFileNameFromXML(xmlElem, filePath);
  if (fileName.empty())
  { // No filename in XML element. Not necessarily an error.
    return nullptr;
  }
  vtkXMLReader* reader = this->GetReaderForFile(fileName);
  if (!reader)
  {
    vtkErrorMacro("Could not create reader for " << fileName);
    return nullptr;
  }
  reader->SetFileName(fileName.c_str());
  reader->GetPointDataArraySelection()->CopySelections(this->PointDataArraySelection);
  reader->GetCellDataArraySelection()->CopySelections(this->CellDataArraySelection);
  reader->GetColumnArraySelection()->CopySelections(this->ColumnArraySelection);
  reader->Update();
  vtkDataObject* output = reader->GetOutputDataObject(0);
  if (!output)
  {
    return nullptr;
  }

  vtkDataObject* outputCopy = output->NewInstance();
  outputCopy->ShallowCopy(output);
  return outputCopy;
}

//----------------------------------------------------------------------------
vtkDataSet* vtkXMLCompositeDataReader::ReadDataset(vtkXMLDataElement* xmlElem, const char* filePath)
{
  return vtkDataSet::SafeDownCast(ReadDataObject(xmlElem, filePath));
}

//----------------------------------------------------------------------------
int vtkXMLCompositeDataReader::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  this->Superclass::RequestInformation(request, inputVector, outputVector);

  vtkInformation* info = outputVector->GetInformationObject(0);
  info->Set(CAN_HANDLE_PIECE_REQUEST(), 1);

  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLCompositeDataReader::SyncDataArraySelections(
  vtkXMLReader* accum, vtkXMLDataElement* xmlElem, const std::string& filePath)
{
  // Get the reader for this file
  std::string fileName = this->GetFileNameFromXML(xmlElem, filePath);
  if (fileName.empty())
  { // No filename in XML element. Not necessarily an error.
    return;
  }
  vtkXMLReader* reader = this->GetReaderForFile(fileName);
  if (!reader)
  {
    vtkErrorMacro("Could not create reader for " << fileName);
    return;
  }
  reader->SetFileName(fileName.c_str());
  // initialize array selection so we don't have any residual array selections
  // from previous use of the reader.
  reader->GetPointDataArraySelection()->RemoveAllArrays();
  reader->GetCellDataArraySelection()->RemoveAllArrays();
  reader->GetColumnArraySelection()->RemoveAllArrays();
  reader->UpdateInformation();

  // Merge the arrays:
  accum->GetPointDataArraySelection()->Union(reader->GetPointDataArraySelection());
  accum->GetCellDataArraySelection()->Union(reader->GetCellDataArraySelection());
  accum->GetColumnArraySelection()->Union(reader->GetColumnArraySelection());
}

//----------------------------------------------------------------------------
const vtkXMLCompositeDataReaderEntry vtkXMLCompositeDataReaderInternals::ReaderList[] = {
  { "vtp", "vtkXMLPolyDataReader" }, { "vtu", "vtkXMLUnstructuredGridReader" },
  { "vti", "vtkXMLImageDataReader" }, { "vtr", "vtkXMLRectilinearGridReader" },
  { "vts", "vtkXMLStructuredGridReader" }, { "vtt", "vtkXMLTableReader" },
  { "htg", "vtkXMLHyperTreeGridReader" }, { nullptr, nullptr }
};
