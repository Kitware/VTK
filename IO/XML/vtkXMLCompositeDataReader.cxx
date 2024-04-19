// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: BSD-3-Clause
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
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
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

#include <algorithm>
#include <map>
#include <set>
#include <vtksys/SystemTools.hxx>

VTK_ABI_NAMESPACE_BEGIN
struct vtkXMLCompositeDataReaderEntry
{
  const char* extension;
  const char* name;
};

struct vtkXMLCompositeDataReaderInternals
{
  vtkXMLCompositeDataReaderInternals() { this->ResetUpdateInformation(); }

  void ResetUpdateInformation()
  {
    this->UpdatePiece = 0;
    this->UpdateNumberOfPieces = 1;
    this->NumDataSets = 1;
    this->HasUpdateRestriction = false;
  }

  vtkSmartPointer<vtkXMLDataElement> Root;
  typedef std::map<std::string, vtkSmartPointer<vtkXMLReader>> ReadersType;
  ReadersType Readers;
  static const vtkXMLCompositeDataReaderEntry ReaderList[];
  int UpdatePiece;
  int UpdateNumberOfPieces;
  unsigned int NumDataSets;
  std::set<int> UpdateIndices;
  bool HasUpdateRestriction;
};

//------------------------------------------------------------------------------
vtkXMLCompositeDataReader::vtkXMLCompositeDataReader()
  : PieceDistribution(Block)
{
  this->Internal = new vtkXMLCompositeDataReaderInternals;
}

//------------------------------------------------------------------------------
vtkXMLCompositeDataReader::~vtkXMLCompositeDataReader()
{
  delete this->Internal;
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
const char* vtkXMLCompositeDataReader::GetDataSetName()
{
  return "vtkCompositeDataSet";
}

//------------------------------------------------------------------------------
void vtkXMLCompositeDataReader::SetupEmptyOutput()
{
  this->GetCurrentOutput()->Initialize();
}

//------------------------------------------------------------------------------
int vtkXMLCompositeDataReader::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkCompositeDataSet");
  return 1;
}

//------------------------------------------------------------------------------
vtkExecutive* vtkXMLCompositeDataReader::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//------------------------------------------------------------------------------
std::string vtkXMLCompositeDataReader::GetFilePath()
{
  std::string filePath = this->FileName;
  std::string::size_type pos = filePath.find_last_of("/\\");
  if (pos != std::string::npos)
  {
    filePath = filePath.substr(0, pos);
  }
  else
  {
    filePath = "";
  }

  return filePath;
}

//------------------------------------------------------------------------------
vtkCompositeDataSet* vtkXMLCompositeDataReader::GetOutput()
{
  return this->GetOutput(0);
}

//------------------------------------------------------------------------------
vtkCompositeDataSet* vtkXMLCompositeDataReader::GetOutput(int port)
{
  vtkDataObject* output =
    vtkCompositeDataPipeline::SafeDownCast(this->GetExecutive())->GetCompositeOutputData(port);
  return vtkCompositeDataSet::SafeDownCast(output);
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
vtkXMLDataElement* vtkXMLCompositeDataReader::GetPrimaryElement()
{
  return this->Internal->Root;
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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
  for (const vtkXMLCompositeDataReaderEntry* readerEntry =
         vtkXMLCompositeDataReaderInternals::ReaderList;
       !rname && readerEntry->extension; ++readerEntry)
  {
    if (ext == readerEntry->extension)
    {
      rname = readerEntry->name;
    }
  }

  return this->GetReaderOfType(rname);
}

//------------------------------------------------------------------------------
unsigned int vtkXMLCompositeDataReader::CountNestedElements(
  vtkXMLDataElement* element, const std::string& tagName, const std::set<std::string>& exclusions)
{
  if (tagName.empty() || element == nullptr)
  {
    return 0;
  }

  unsigned int count = 0;
  for (unsigned int cc = 0, max = element->GetNumberOfNestedElements(); cc < max; ++cc)
  {
    auto child = element->GetNestedElement(cc);
    if (child && child->GetName())
    {
      if (child->GetName() == tagName)
      {
        ++count;
      }
      else if (exclusions.find(child->GetName()) == exclusions.end())
      {
        count += vtkXMLCompositeDataReader::CountNestedElements(child, tagName, exclusions);
      }
    }
  }
  return count;
}

//------------------------------------------------------------------------------
void vtkXMLCompositeDataReader::ReadXMLData()
{
  vtkLogF(TRACE, "vtkXMLCompositeDataReader::ReadXMLData");
  vtkInformation* info = this->GetCurrentOutputInformation();

  this->Internal->UpdatePiece = info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  this->Internal->UpdateNumberOfPieces =
    info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  this->Internal->NumDataSets = this->CountNestedElements(this->GetPrimaryElement(), "DataSet");

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

//------------------------------------------------------------------------------
int vtkXMLCompositeDataReader::ShouldReadDataSet(
  unsigned int idx, unsigned int pieceIndex /*=0*/, unsigned int numPieces /*=0*/)
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

  unsigned int datasetIndex = idx;
  unsigned int numDatasets = this->Internal->NumDataSets;

  if (pieceIndex < numPieces)
  {
    // This dataset is part of a vtkParititionedDataSet or a
    // vtkMultiPieceDataset. Handle is differently.
    datasetIndex = pieceIndex;
    numDatasets = numPieces;
  }

  int assignment = -1;
  switch (this->PieceDistribution)
  {
    case vtkXMLCompositeDataReader::Block:
      assignment = vtkXMLCompositeDataReader::GetPieceAssignmentForBlockStrategy(
        datasetIndex, numDatasets, this->Internal->UpdateNumberOfPieces);
      break;

    case vtkXMLCompositeDataReader::Interleave:
      assignment = vtkXMLCompositeDataReader::GetPieceAssignmentForInterleaveStrategy(
        datasetIndex, numDatasets, this->Internal->UpdateNumberOfPieces);
      break;

    default:
      vtkErrorMacro("Invalid PieceDistribution setting: " << this->PieceDistribution);
      break;
  }

  return (assignment == this->Internal->UpdatePiece) ? 1 : 0;
}

//------------------------------------------------------------------------------
int vtkXMLCompositeDataReader::GetPieceAssignmentForBlockStrategy(
  unsigned int idx, unsigned int numDatasets, int numPieces)
{
  numPieces = std::max(1, numPieces);
  // Use signed integers for the modulus -- otherwise weird things like
  // (-1 % 3) == 0 will happen!
  const int gid = static_cast<int>(idx);
  const int div = static_cast<int>(numDatasets) / numPieces;
  const int mod = static_cast<int>(numDatasets) % numPieces;
  const int piece = gid / (div + 1);
  if (piece < mod)
  {
    return piece;
  }
  else
  {
    return mod + (gid - (div + 1) * mod) / div;
  }
}

//------------------------------------------------------------------------------
int vtkXMLCompositeDataReader::GetPieceAssignmentForInterleaveStrategy(
  unsigned int idx, unsigned int vtkNotUsed(numDatasets), int numPieces)
{
  // Use signed integers for the modulus -- otherwise weird things like
  // (-1 % 3) == 0 will happen!
  const int i = static_cast<int>(idx);
  return (i % numPieces);
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
vtkDataSet* vtkXMLCompositeDataReader::ReadDataset(vtkXMLDataElement* xmlElem, const char* filePath)
{
  return vtkDataSet::SafeDownCast(ReadDataObject(xmlElem, filePath));
}

//------------------------------------------------------------------------------
void vtkXMLCompositeDataReader::SetFileName(const char* fname)
{
  if (fname == nullptr || this->GetFileName() == nullptr || strcmp(fname, this->GetFileName()) != 0)
  {
    // this is a disaster, but a necessary temporary workaround for paraview/paraview#20179:
    // if filename changed, reset information about update-piece so that
    // RequestInformation stage does not rely on potentially obsolete
    // information.
    this->Internal->ResetUpdateInformation();
  }
  this->Superclass::SetFileName(fname);
}

//------------------------------------------------------------------------------
int vtkXMLCompositeDataReader::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  this->Superclass::RequestInformation(request, inputVector, outputVector);
  vtkInformation* info = outputVector->GetInformationObject(0);
  info->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
  return 1;
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
const vtkXMLCompositeDataReaderEntry vtkXMLCompositeDataReaderInternals::ReaderList[] = {
  { "vtp", "vtkXMLPolyDataReader" }, { "vtu", "vtkXMLUnstructuredGridReader" },
  { "vti", "vtkXMLImageDataReader" }, { "vtr", "vtkXMLRectilinearGridReader" },
  { "vts", "vtkXMLStructuredGridReader" }, { "vtt", "vtkXMLTableReader" },
  { "htg", "vtkXMLHyperTreeGridReader" }, { nullptr, nullptr }
};
VTK_ABI_NAMESPACE_END
