// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkEnSightSOSGoldReader.h"

#include "core/EnSightFile.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArraySelection.h"
#include "vtkDataAssembly.h"
#include "vtkDataSet.h"
#include "vtkEnSightGoldCombinedReader.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkSetGet.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtksys/SystemTools.hxx"

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPICommunicator.h"
#endif

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
  for (int i = 0; i < fullSelection->GetNumberOfArrays(); i++)
  {
    // this may add parts to this reader's selection, if the part name was not found
    // during this reader's GetPartInfo, but this shouldn't cause any issues. In this case,
    // it helps the reader know that it should create an empty vtkPartitionedDataSet for this part
    // (if the part is enabled) so that the vtkPartitionedDataSetCollection structure matches across
    // ranks when running in parallel.
    auto name = fullSelection->GetArrayName(i);
    readerSelection->SetArraySetting(name, fullSelection->ArrayIsEnabled(name));
  }
}

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
template <class T>
bool syncValues(T* data, int numValues, int numPieces, vtkMultiProcessController* controller)
{
  // Compare values on all processes that will read real pieces.
  // Returns whether the values match.  If they match, all processes'
  // values are modified to match that of node 0.  This will leave the
  // values unchanged on processes that will read real data, but
  // inform the other processes of the proper values.

  if (!controller)
  {
    return true;
  }

  vtkMPICommunicator* communicator =
    vtkMPICommunicator::SafeDownCast(controller->GetCommunicator());

  if (!communicator)
  {
    if (controller->GetNumberOfProcesses() == 1)
    {
      return true;
    }
    else
    {
      return false;
    }
  }

  int numProcs = controller->GetNumberOfProcesses();
  int myid = controller->GetLocalProcessId();

  // Collect all the values to node 0.
  T* values = new T[numProcs * numValues];
  communicator->Gather(data, values, numValues, 0);

  int result = VTK_OK;
  // Node 0 compares its values to those from other processes that
  // will actually be reading data.
  if (myid == 0)
  {
    for (int i = 1; result && (i < numPieces); i++)
    {
      for (int j = 0; result && (j < numValues); j++)
      {
        if (values[i * numValues + j] != values[j])
        {
          result = VTK_ERROR;
        }
      }
    }
  }

  // Free buffer where values were collected.
  delete[] values;

  // Broadcast result of comparison to all processes.
  communicator->Broadcast(&result, 1, 0);

  // If the results were okay, broadcast the correct values to all
  // processes so that those that will not read can have the correct
  // values.
  if (result == VTK_OK)
  {
    communicator->Broadcast(data, numValues, 0);
  }

  return true;
}
#else
template <class T>
bool syncValues(T*, int, int, vtkMultiProcessController*)
{
  return true;
}
#endif

} // end anon namespace

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
struct vtkEnSightSOSGoldReader::ReaderImpl
{
  std::vector<vtkSmartPointer<vtkEnSightGoldCombinedReader>> Readers;
  ensight_gold::EnSightFile SOSFile;
  std::string FilePath;
  std::vector<std::string> CaseFileNames;

  // the index in the output vtkPartitionedDataSetCollection for each part.
  // an element is -1 if that part will not be loaded. This is passed on to
  // EnSightDataSet so that every rank can put the parts in the vtkPartitionedDataSetCollection in
  // the same way
  vtkNew<vtkIdTypeArray> PartPDCIndex;

  int Rank;
  int NumberOfProcesses;

  // this is the total number of casefiles for this dataset
  // but not necessarily how many casefiles this rank will read
  int TotalNumberOfCaseFiles = 0;

  // CaseFileStart and CaseFileEnd determine which casefiles we'll actually read on this rank
  int CaseFileStart = -1;
  int CaseFileEnd = -1;

  // total number of unique parts across all casefiles.
  int TotalNumberOfParts = 0;

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

    this->TotalNumberOfCaseFiles = numServers;

    return true;
  }

  bool SetupReaders()
  {
    vtkLogScopeFunction(TRACE);
    this->Readers.clear();
    if (this->TotalNumberOfCaseFiles < 0)
    {
      return false;
    }

    // determine which files we will actually read on this rank
    int localNumberOfCaseFiles = this->TotalNumberOfCaseFiles;
    int remainder = localNumberOfCaseFiles % this->NumberOfProcesses;
    localNumberOfCaseFiles /= this->NumberOfProcesses;
    this->CaseFileStart = this->Rank * localNumberOfCaseFiles + std::min(this->Rank, remainder);
    this->CaseFileEnd =
      (this->Rank + 1) * localNumberOfCaseFiles + std::min(this->Rank + 1, remainder);
    vtkLog(TRACE, "casefile start " << CaseFileStart << ", casefile end " << CaseFileEnd);
    localNumberOfCaseFiles = this->CaseFileEnd - this->CaseFileStart;

    // We set up readers for all casefiles on all ranks so we can get the metadata (part names,
    // array names) on all ranks.
    // When we actually read, we'll use CaseFileStart and CaseFileEnd to only read the appropriate
    // casefile(s)
    this->Readers.resize(this->TotalNumberOfCaseFiles);
    for (int i = 0; i < this->TotalNumberOfCaseFiles; i++)
    {
      if (!this->Readers[i])
      {
        this->Readers[i] = vtkSmartPointer<vtkEnSightGoldCombinedReader>::New();
      }
      this->Readers[i]->SetCaseFileName(this->CaseFileNames[i].c_str());
      this->Readers[i]->SetFilePath(this->FilePath.c_str());
      this->Readers[i]->SetPartOfSOSFile(true);
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

  vtkSmartPointer<vtkStringArray> UpdatePartIndices()
  {
    vtkLogScopeFunction(TRACE);
    vtkNew<vtkStringArray> loadedPartNames;
    loadedPartNames->Initialize();
    this->PartPDCIndex->Initialize();
    this->PartPDCIndex->SetNumberOfTuples(this->TotalNumberOfParts);
    this->PartPDCIndex->FillValue(-1);
    int pdcIndex = 0;
    for (vtkEnSightGoldCombinedReader* reader : this->Readers)
    {
      vtkLogScopeF(TRACE, "reader loop");
      vtkSmartPointer<vtkStringArray> partNames = reader->GetPartNames();
      for (int i = 0; i < partNames->GetNumberOfValues(); i++)
      {
        auto part = partNames->GetValue(i);
        vtkLog(TRACE, "partName: " << part);
        if (part.empty())
        {
          continue;
        }
        if (this->PartSelection->ArrayIsEnabled(part.c_str()) &&
          this->PartPDCIndex->GetValue(i) == -1)
        {
          this->PartPDCIndex->SetValue(i, pdcIndex);
          vtkLog(TRACE, "part " << part << " has a PDC index of " << PartPDCIndex[i]);
          loadedPartNames->InsertNextValue(part);
          pdcIndex++;
        }
      }
    }

    return loadedPartNames;
  }
};

vtkStandardNewMacro(vtkEnSightSOSGoldReader);

vtkCxxSetObjectMacro(vtkEnSightSOSGoldReader, Controller, vtkMultiProcessController);
vtkMultiProcessController* vtkEnSightSOSGoldReader::GetController()
{
  return this->Controller;
}

//------------------------------------------------------------------------------
vtkEnSightSOSGoldReader::vtkEnSightSOSGoldReader()
{
  this->SetNumberOfInputPorts(0);
  this->CaseFileName = nullptr;
  this->Impl = new ReaderImpl;
  this->Controller = nullptr;
  this->SetController(vtkMultiProcessController::GetGlobalController());
  this->Impl->Rank = this->Controller ? this->Controller->GetLocalProcessId() : 0;
  this->Impl->NumberOfProcesses = this->Controller ? this->Controller->GetNumberOfProcesses() : 1;
}

//------------------------------------------------------------------------------
vtkEnSightSOSGoldReader::~vtkEnSightSOSGoldReader()
{
  this->SetCaseFileName(nullptr);
  delete this->Impl;
  this->SetController(nullptr);
}

//------------------------------------------------------------------------------
int vtkEnSightSOSGoldReader::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkLogScopeFunction(TRACE);

  int parseResults = this->Impl->ParseSOSFile(this->CaseFileName);
  if (!syncValues(&parseResults, 1, this->Impl->NumberOfProcesses, this->Controller) ||
    !parseResults)
  {
    vtkErrorMacro("Problem parsing the SOS file");
    return 0;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);

  parseResults = this->Impl->SetupReaders();
  if (!syncValues(&parseResults, 1, this->Impl->NumberOfProcesses, this->Controller) ||
    !parseResults)
  {
    vtkErrorMacro("Problem setting up the readers");
    return 0;
  }

  this->Impl->AddSelections();

  // now we can set the total number of parts in the dataset
  // this includes the measured part if any
  this->Impl->TotalNumberOfParts = this->Impl->PartSelection->GetNumberOfArrays();
  vtkLog(TRACE, "total number of parts " << this->Impl->TotalNumberOfParts);

  auto timeSteps = this->Impl->Readers[0]->GetAllTimeSteps();

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
  vtkLogScopeFunction(TRACE);
  if (this->Impl->Readers.empty())
  {
    // we don't have anything to read
    return 1;
  }
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  int piece = outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER())
    ? outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER())
    : 0;
  int npieces = outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES())
    ? outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES())
    : 1;
  vtkLog(TRACE, "piece: " << piece << ", number of pieces: " << npieces);

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
  auto partNames = this->Impl->UpdatePartIndices();

  // With SOS files, each casefile must contain all parts of the dataset, however the part in a
  // given casefile does not have to be the full data, and can even be empty. Thus each portion of a
  // part in a casefile is merely a partition of the full part. The structure of the
  // vtkMultiBlockDataSet in the old readers misunderstands this. It creates a block for each
  // casefile, and then creates blocks under it for each part. Thus it looks like the partitions of
  // each part are actually separate parts that just happen to have the same name. In addition, the
  // old readers then create their own decomposition of the data and split all partitions of parts
  // among all available ranks, which results in a pretty inefficient distribution of data across
  // ranks. I think it's safe to assume that a lot of users will already have a good partitioning in
  // their ensight files that is output from their solvers and we should respect that when running
  // in parallel. In vtkPartitionedDataSetCollection terms, each part in a dataset is a
  // vtkPartitionedDataSet and each portion of a part in a casefile will be a partiion of its
  // respective vtkPartitionedDataSet.
  vtkNew<vtkDataAssembly> fullAssembly;
  fullAssembly->SetRootNodeName("vtkPartitionedDataSetCollection");
  for (int block = this->Impl->CaseFileStart; block < this->Impl->CaseFileEnd; block++)
  {
    this->Impl->Readers[block]->SetTimeValue(timeValue);
    this->Impl->Readers[block]->UpdateInformation();
    this->Impl->Readers[block]->SetPDCInfoForLoadedParts(this->Impl->PartPDCIndex, partNames);
    this->Impl->Readers[block]->Update();

    vtkPartitionedDataSetCollection* readerPDSC = this->Impl->Readers[block]->GetOutput();
    auto readerAssembly = readerPDSC->GetDataAssembly();

    for (unsigned int pdsIdx = 0; pdsIdx < readerPDSC->GetNumberOfPartitionedDataSets(); pdsIdx++)
    {
      auto readerPDS = readerPDSC->GetPartitionedDataSet(pdsIdx);
      if (!readerPDS)
      {
        // this should be an error, since EnSightDataSet makes sure there's at least an empty PDC
        vtkErrorMacro("the partitioned dataset should not be null");
        return 0;
      }

      // now check to see if we already have a PDS at this index in the output
      vtkSmartPointer<vtkPartitionedDataSet> sosPDS = output->GetPartitionedDataSet(pdsIdx);
      if (!sosPDS)
      {
        sosPDS = vtkSmartPointer<vtkPartitionedDataSet>::New();
        sosPDS->CompositeShallowCopy(readerPDS);
        output->SetPartitionedDataSet(pdsIdx, sosPDS);
      }
      else
      {
        unsigned int currentCount = sosPDS->GetNumberOfPartitions();
        // add the partitions from this reader's PDS to the new PDS
        for (unsigned int partition = 0; partition < readerPDS->GetNumberOfPartitions();
             partition++)
        {
          sosPDS->SetPartition(currentCount + partition, readerPDS->GetPartition(partition));
        }
      }

      std::string partName;
      // part name may not be set for empty partitioned datasets
      if (readerPDSC->GetMetaData(pdsIdx)->Has(vtkCompositeDataSet::NAME()))
      {
        partName = readerPDSC->GetMetaData(pdsIdx)->Get(vtkCompositeDataSet::NAME());
      }

      std::string sosPartName;
      if (output->GetMetaData(pdsIdx)->Has(vtkCompositeDataSet::NAME()))
      {
        sosPartName = output->GetMetaData(pdsIdx)->Get(vtkCompositeDataSet::NAME());
      }

      if (!partName.empty() && sosPartName.empty())
      {
        // need to update the metadata and the assembly
        output->GetMetaData(pdsIdx)->Set(vtkCompositeDataSet::NAME(), partName);
        auto validPartName = vtkDataAssembly::MakeValidNodeName(partName.c_str());
        auto readerNodes = readerAssembly->FindNodesWithName(validPartName.c_str());
        auto sosNode = fullAssembly->AddNode(readerAssembly->GetNodeName(readerNodes[0]));
        fullAssembly->AddDataSetIndex(sosNode, pdsIdx);
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
  os << indent << "Total number of case files: " << this->Impl->TotalNumberOfCaseFiles << endl;
  os << indent << "Case file start index: " << this->Impl->CaseFileStart << endl;
  os << indent << "Case file end index: " << this->Impl->CaseFileEnd << endl;
}
VTK_ABI_NAMESPACE_END
