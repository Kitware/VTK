// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkEnSightSOSGoldReader.h"

#include "core/EnSightFile.h"
#include "vtkDataArraySelection.h"
#include "vtkDataAssembly.h"
#include "vtkEnSightGoldCombinedReader.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkSetGet.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtksys/SystemTools.hxx"

#include <string>
#include <vector>

namespace
{
void sanitize(std::string& str)
{
  char quotes = '\"';
  size_t found = str.find(quotes);
  if (found != std::string::npos)
  {
    str.erase(std::remove(str.begin(), str.end(), quotes), str.end());
  }

  // remove whitespace at the end of the string and before the string
  std::string whitespaces(" \t\n\r");
  found = str.find_last_not_of(whitespaces);
  if (found != std::string::npos)
  {
    str.erase(found + 1);
  }
  found = str.find_first_not_of(whitespaces);
  if (found != std::string::npos)
  {
    str.erase(0, found);
  }
}

void addSelectionArrays(
  vtkDataArraySelection* readerSelection, vtkDataArraySelection* fullSelection)
{
  for (int i = 0; i < readerSelection->GetNumberOfArrays(); i++)
  {
    fullSelection->AddArray(readerSelection->GetArrayName(i));
  }
}

void updateSelectionArrays(
  vtkDataArraySelection* readerSelection, vtkDataArraySelection* fullSelection)
{
  for (int i = 0; i < readerSelection->GetNumberOfArrays(); i++)
  {
    auto name = readerSelection->GetArrayName(i);
    readerSelection->SetArraySetting(name, fullSelection->ArrayIsEnabled(name));
  }
}

} // end anon namespace

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
struct vtkEnSightSOSGoldReader::ReaderImpl
{
  std::vector<vtkSmartPointer<vtkEnSightGoldCombinedReader>> Readers;
  ensight_gold::EnSightFile SOSFile;
  std::string FilePath;
  std::vector<std::string> CaseFileNames;
  int NumberOfPieces = 0;
  vtkNew<vtkDataArraySelection> PartSelection;
  vtkNew<vtkDataArraySelection> PointArraySelection;
  vtkNew<vtkDataArraySelection> CellArraySelection;
  vtkNew<vtkDataArraySelection> FieldArraySelection;

  int CheckVersion(const char* filename)
  {
    if (!this->SOSFile.SetFileNamePattern(filename, true))
    {
      vtkGenericWarningMacro("EnSight SOS file " << filename << " could not be opened");
      return false;
    }

    auto result = this->SOSFile.ReadNextLine();
    bool formatFound = false;
    do
    {
      if (result.second.find("FORMAT") != std::string::npos)
      {
        formatFound = true;
      }
      else if (formatFound && result.second.find("type") != std::string::npos)
      {
        if (result.second.find("master_server") != std::string::npos ||
          result.second.find("gold") != std::string::npos)
        {
          return true;
        }
      }
      result = this->SOSFile.ReadNextLine();
    } while (result.first);
    return false;
  }

  bool ParseSOSFile(const char* filename)
  {
    this->CaseFileNames.clear();
    if (!this->SOSFile.SetFileNamePattern(filename, true))
    {
      vtkGenericWarningMacro("SOS file " << filename << " could not be opened");
      return false;
    }
    this->FilePath = vtksys::SystemTools::GetParentDirectory(filename);

    bool foundServersSection = false;
    int numServers = 0;

    auto result = this->SOSFile.ReadNextLine();
    while (result.first)
    {
      std::string& line = result.second;
      if (line.find("FORMAT") != std::string::npos)
      {
        // nothing to do here
      }
      else if (line.find("type") != std::string::npos)
      {
        if (line.find("master_server") == std::string::npos ||
          line.find("gold") == std::string::npos)
        {
          vtkGenericWarningMacro("vtkEnSightSOSGoldReader only reads SOS files for Gold format");
          return false;
        }
      }
      else if (line.find("SERVERS") != std::string::npos)
      {
        foundServersSection = true;
      }
      else if (foundServersSection && line.find("number of servers") != std::string::npos)
      {
        auto pos = line.find(':');
        auto value = line.substr(pos + 1);
        sanitize(value);
        try
        {
          numServers = std::stoi(value);
        }
        catch (std::invalid_argument&)
        {
          vtkGenericWarningMacro("Couldn't convert " << value << " to an int");
          return false;
        }
      }
      else if (foundServersSection && line.find("casefile") != std::string::npos)
      {
        auto lineParts = vtksys::SystemTools::SplitString(result.second, ':');
        if (lineParts.size() != 2)
        {
          vtkGenericWarningMacro("line " << result.second << " could not be read properly");
          return false;
        }
        sanitize(lineParts[1]);
        this->CaseFileNames.push_back(lineParts[1]);
      }
      result = this->SOSFile.ReadNextLine();
    }

    if (this->CaseFileNames.size() != static_cast<size_t>(numServers))
    {
      vtkGenericWarningMacro(
        "The 'number of servers' is not equal to the number of listed servers");
      return false;
    }

    this->NumberOfPieces = numServers;

    return true;
  }

  bool SetupReaders()
  {
    this->Readers.clear();
    if (this->NumberOfPieces < 0)
    {
      return false;
    }

    this->Readers.resize(this->NumberOfPieces);
    for (int i = 0; i < this->NumberOfPieces; i++)
    {
      if (!this->Readers[i])
      {
        this->Readers[i] = vtkSmartPointer<vtkEnSightGoldCombinedReader>::New();
      }
      this->Readers[i]->SetCaseFileName(this->CaseFileNames[i].c_str());
      this->Readers[i]->SetFilePath(this->FilePath.c_str());
      this->Readers[i]->UpdateInformation();
    }
    return true;
  }

  void AddSelections()
  {
    for (vtkEnSightGoldCombinedReader* reader : this->Readers)
    {
      auto selection = reader->GetPartSelection();
      addSelectionArrays(selection, this->PartSelection);
      selection = reader->GetPointArraySelection();
      addSelectionArrays(selection, this->PointArraySelection);
      selection = reader->GetCellArraySelection();
      addSelectionArrays(selection, this->CellArraySelection);
      selection = reader->GetFieldArraySelection();
      addSelectionArrays(selection, this->FieldArraySelection);
    }
  }

  void UpdateSelections()
  {
    for (vtkEnSightGoldCombinedReader* reader : this->Readers)
    {
      auto selection = reader->GetPartSelection();
      updateSelectionArrays(selection, this->PartSelection);
      selection = reader->GetPointArraySelection();
      updateSelectionArrays(selection, this->PointArraySelection);
      selection = reader->GetCellArraySelection();
      updateSelectionArrays(selection, this->CellArraySelection);
      selection = reader->GetFieldArraySelection();
      updateSelectionArrays(selection, this->FieldArraySelection);
    }
  }
};

vtkStandardNewMacro(vtkEnSightSOSGoldReader);

//------------------------------------------------------------------------------
vtkEnSightSOSGoldReader::vtkEnSightSOSGoldReader()
{
  this->SetNumberOfInputPorts(0);
  this->CaseFileName = nullptr;
  this->Impl = new ReaderImpl;
}

//------------------------------------------------------------------------------
vtkEnSightSOSGoldReader::~vtkEnSightSOSGoldReader()
{
  this->SetCaseFileName(nullptr);
  delete this->Impl;
}

//------------------------------------------------------------------------------
int vtkEnSightSOSGoldReader::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  if (!this->Impl->ParseSOSFile(this->CaseFileName))
  {
    vtkErrorMacro("Problem parsing the SOS file");
    return 0;
  }

  if (!this->Impl->SetupReaders())
  {
    vtkErrorMacro("Problem setting up the readers");
    return 0;
  }

  this->Impl->AddSelections();

  auto timeSteps = this->Impl->Readers[0]->GetAllTimeSteps();

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (timeSteps && timeSteps->GetNumberOfValues() > 0)
  {
    int numSteps = timeSteps->GetNumberOfValues();
    double timeRange[2];
    timeRange[0] = timeSteps->GetValue(0);
    timeRange[1] = timeSteps->GetValue(numSteps - 1);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);

    double* times = new double[numSteps];
    for (int i = 0; i < numSteps; i++)
    {
      times[i] = timeSteps->GetValue(i);
    }
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &times[0], numSteps);
    delete[] times;
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkEnSightSOSGoldReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkPartitionedDataSetCollection* output =
    vtkPartitionedDataSetCollection::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int tsLength = 0;
  double* steps = nullptr;
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    tsLength = outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    steps = outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  }

  double timeValue = 0.0;
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()) && steps != nullptr &&
    tsLength > 0)
  {
    double requestedTimeStep = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    // find the first time value larger than requested time value
    // this logic could be improved
    int cnt = 0;
    while (cnt < tsLength - 1 && steps[cnt] < requestedTimeStep)
    {
      cnt++;
    }
    timeValue = steps[cnt];
  }

  this->Impl->UpdateSelections();

  // since vtkPartitionedDataSetCollection can't hold other vtkPartitionedDataSetCollections
  // we have to create a new PDSC that holds all of the partitioned data sets, then
  // we'll create a new assembly that will provide the hierarchy similar to the old reader's
  // mutliblock output
  int outputIdx = 0;
  vtkNew<vtkDataAssembly> fullAssembly;
  fullAssembly->SetRootNodeName("vtkPartitionedDataSetCollection");
  for (size_t block = 0; block < this->Impl->Readers.size(); block++)
  {
    this->Impl->Readers[block]->SetTimeValue(timeValue);
    this->Impl->Readers[block]->UpdateInformation();
    this->Impl->Readers[block]->Update();
    vtkPartitionedDataSetCollection* pdsc = this->Impl->Readers[block]->GetOutput();

    std::string blockName = "PartitionedDataSet" + std::to_string(block);
    auto validName = vtkDataAssembly::MakeValidNodeName(blockName.c_str());
    auto blockNode = fullAssembly->AddNode(validName.c_str());
    auto oldAssembly = pdsc->GetDataAssembly();
    for (unsigned int pidx = 0; pidx < pdsc->GetNumberOfPartitionedDataSets(); pidx++)
    {
      auto partName = pdsc->GetMetaData(pidx)->Get(vtkCompositeDataSet::NAME());
      auto pds = pdsc->GetPartitionedDataSet(pidx);
      if (pds && partName)
      {
        auto validPartName = vtkDataAssembly::MakeValidNodeName(partName);
        auto oldNodes = oldAssembly->FindNodesWithName(validPartName.c_str());
        auto oldNode = oldNodes[0];
        output->SetPartitionedDataSet(outputIdx, pds);
        auto childNode = fullAssembly->AddNode(oldAssembly->GetNodeName(oldNode), blockNode);
        fullAssembly->AddDataSetIndex(childNode, outputIdx);
        outputIdx++;
      }
    }
  }
  output->SetDataAssembly(fullAssembly);

  return 1;
}

//------------------------------------------------------------------------------
int vtkEnSightSOSGoldReader::CanReadFile(const char* fname)
{
  return this->Impl->CheckVersion(fname);
}

//------------------------------------------------------------------------------
vtkDataArraySelection* vtkEnSightSOSGoldReader::GetPartSelection()
{
  return this->Impl->PartSelection;
}

//------------------------------------------------------------------------------
vtkDataArraySelection* vtkEnSightSOSGoldReader::GetPointArraySelection()
{
  return this->Impl->PointArraySelection;
}

//------------------------------------------------------------------------------
vtkDataArraySelection* vtkEnSightSOSGoldReader::GetCellArraySelection()
{
  return this->Impl->CellArraySelection;
}

//------------------------------------------------------------------------------
vtkDataArraySelection* vtkEnSightSOSGoldReader::GetFieldArraySelection()
{
  return this->Impl->FieldArraySelection;
}

//------------------------------------------------------------------------------
vtkMTimeType vtkEnSightSOSGoldReader::GetMTime()
{
  auto maxVal = std::max(this->Superclass::GetMTime(), this->Impl->PartSelection->GetMTime());
  maxVal = std::max(maxVal, this->Impl->PointArraySelection->GetMTime());
  maxVal = std::max(maxVal, this->Impl->CellArraySelection->GetMTime());
  return std::max(maxVal, this->Impl->FieldArraySelection->GetMTime());
}

//------------------------------------------------------------------------------
void vtkEnSightSOSGoldReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SOS filename: " << this->CaseFileName << endl;
}
VTK_ABI_NAMESPACE_END
