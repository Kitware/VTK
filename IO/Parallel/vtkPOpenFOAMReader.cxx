/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPOpenFOAMReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This class was developed by Takuya Oshima at Niigata University,
// Japan (oshima@eng.niigata-u.ac.jp).
//
// ---------------------------------------------------------------------------
//
// Bugs or support questions should be addressed to the discourse forum
// https://discourse.paraview.org/ and/or KitWare
//
// ---------------------------------------------------------------------------
// OpenFOAM decomposed cases have different formats (JAN 2021)
//
// - "Uncollated" with separate directories for each rank
//   processor0 ... processorN
//
// - "Collated" with a single directory for all NN ranks
//   processorsNN
//
// - "Collated" with directories for (inclusive) ranges of ranks
//   processorsNN_first-last, ...
//
// The collated format is not yet supported by the underlying readers
//------------------------------------------------------------------------------

// Support for reading collated format
#define VTK_FOAMFILE_COLLATED_FORMAT 0

//------------------------------------------------------------------------------
// Developer option to debug the reader states
#define VTK_FOAMFILE_DEBUG 0

// Similar to vtkErrorMacro etc.
#if VTK_FOAMFILE_DEBUG
#define vtkFoamDebug(x)                                                                            \
  do                                                                                               \
  {                                                                                                \
    std::cerr << "" x;                                                                             \
  } while (false)
#else
#define vtkFoamDebug(x)                                                                            \
  do                                                                                               \
  {                                                                                                \
  } while (false)
#endif // VTK_FOAMFILE_DEBUG

//------------------------------------------------------------------------------

#include "vtkPOpenFOAMReader.h"

#include "vtkAppendCompositeDataLeaves.h"
#include "vtkCharArray.h"
#include "vtkCollection.h"
#include "vtkDataArraySelection.h"
#include "vtkDirectory.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkSortDataArray.h"
#include "vtkStdString.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"

#include <cctype>
#include <cstring>

//------------------------------------------------------------------------------

vtkStandardNewMacro(vtkPOpenFOAMReader);
vtkCxxSetObjectMacro(vtkPOpenFOAMReader, Controller, vtkMultiProcessController);

//------------------------------------------------------------------------------

// Local Functions

namespace
{

// Create a sub-reader with the current characteristics
vtkSmartPointer<vtkOpenFOAMReader> NewFoamReader(vtkOpenFOAMReader* parent)
{
  auto reader = vtkSmartPointer<vtkOpenFOAMReader>::New();
  reader->SetFileName(parent->GetFileName());
  reader->SetParent(parent);
  reader->SetSkipZeroTime(parent->GetSkipZeroTime());
  reader->SetUse64BitLabels(parent->GetUse64BitLabels());
  reader->SetUse64BitFloats(parent->GetUse64BitFloats());

  return reader;
}

// Generate a processor dirname from number or tuple.
// For 1 component:
//   procNum -> 'processor<procNum>'
//
// For 3 component tuple (nprocs, first, size)
//
// - processors<nprocs>, when first == size == 0
// - processors<nprocs>_<first>-<last>, where last is inclusive
std::string ProcessorDirName(const vtkIntArray* dirs, int index)
{
  if (index < 0 || index >= dirs->GetNumberOfTuples())
  {
    return std::string();
  }
  else if (3 == dirs->GetNumberOfComponents())
  {
    // Collated name
    const auto nprocs = dirs->GetTypedComponent(index, 0);
    const auto first = dirs->GetTypedComponent(index, 1);
    const auto size = dirs->GetTypedComponent(index, 2);

    std::string stem("processors" + std::to_string(nprocs));
    if (size)
    {
      const auto last = (first + size - 1); // inclusive range
      return (stem + "_" + std::to_string(first) + "-" + std::to_string(last));
    }
    return stem;
  }

  // Uncollated name
  return std::string("processor" + std::to_string(dirs->GetValue(index)));
}

#if VTK_FOAMFILE_COLLATED_FORMAT
// Number of processor pieces represented by the tuple
inline int ProcessorsNumPieces(const int procTuple[])
{
  const auto nprocs = procTuple[0];
  // const auto first = procTuple[1];
  const auto size = procTuple[2];

  return (size ? size : nprocs);
}
#endif

// Search and list processor subdirectories
// Detect collated and uncollated processor directories
// - "processor(\d+)"
// - "processors(\d+)"
// - "processors(\d+)_(\d+)-(\d+)"
//
// string parsing logic as per fileOperation / parseProcsNumRange from OpenFOAM-v2012
//
// Return either collated or uncollated directories, never a mix.
// Use the number of components to distinguish
vtkSmartPointer<vtkIntArray> ScanForProcessorDirs(vtkDirectory* dir)
{
  // Uncollated: save processor id
  auto uncollated = vtkSmartPointer<vtkIntArray>::New();
  uncollated->SetNumberOfComponents(1);

  // Collated: save (processor count, first, size) tuple
  auto collated = vtkSmartPointer<vtkIntArray>::New();
  collated->SetNumberOfComponents(3);

  // Sort keys for collated
  vtkNew<vtkIntArray> collatedNums;
  int procTuple[3] = { 0, 0, 0 };

  const vtkIdType nFiles = dir->GetNumberOfFiles();
  for (vtkIdType filei = 0; filei < nFiles; ++filei)
  {
    const char* subdir = dir->GetFile(filei);

    if (strncmp(subdir, "processor", 9) != 0 || !dir->FileIsDirectory(subdir))
    {
      continue;
    }

    if (isdigit(subdir[9]))
    {
      // processor<digits>
      const char* nptr = (subdir + 9);
      char* endptr = nullptr;

      errno = 0;
      long parsed = std::strtol(nptr, &endptr, 10);
      if (errno || nptr == endptr)
      {
        continue; // bad parse
      }
      const auto procId = static_cast<int>(parsed);

      // Require end of string
      if (*endptr == '\0')
      {
        uncollated->InsertNextValue(procId);
      }
    }
    else if (subdir[9] == 's' && isdigit(subdir[10]))
    {
      // processors<digits> or processors<digits>_<digits>-<digits>
      const char* nptr = (subdir + 10);
      char* endptr = nullptr;

      // 1. numProcs
      errno = 0;
      long parsed = std::strtol(nptr, &endptr, 10);
      if (errno || nptr == endptr)
      {
        continue; // bad parse
      }
      const auto nProcs = static_cast<int>(parsed);

      // End of string? Then no range and we are done.
      if (*endptr == '\0')
      {
        procTuple[0] = nProcs;
        procTuple[1] = 0;
        procTuple[2] = 0;

        collated->InsertNextTypedTuple(procTuple);
        collatedNums->InsertNextValue(nProcs);
        continue;
      }

      // Parse point at start of range ('_' character)?
      if (*endptr != '_')
      {
        continue;
      }
      nptr = ++endptr;

      // 2. firstProc
      errno = 0;
      parsed = std::strtol(nptr, &endptr, 10);
      if (errno || nptr == endptr)
      {
        continue; // bad parse
      }
      const auto firstProc = static_cast<int>(parsed);

      // Parse point at range separator ('-' character)?
      if (*endptr != '-')
      {
        continue;
      }
      nptr = ++endptr;

      // 3. lastProc
      errno = 0;
      parsed = std::strtol(nptr, &endptr, 10);
      if (errno || nptr == endptr)
      {
        continue; // bad parse
      }
      const auto lastProc = static_cast<int>(parsed);

      if (
        // Parse point at end of string
        (*endptr == '\0')
        // Input plausibility - accept nProcs == 0 in case that becomes useful in the future
        && (nProcs >= 0 && firstProc >= 0 && firstProc <= lastProc))
      {
        // Convert first/last to start/size
        procTuple[0] = nProcs;
        procTuple[1] = firstProc;
        procTuple[2] = (lastProc - firstProc + 1);

        collated->InsertNextTypedTuple(procTuple);
        collatedNums->InsertNextValue(nProcs);
      }
    }
  }

  collatedNums->Squeeze();
  collated->Squeeze();
  uncollated->Squeeze();

  vtkSortDataArray::Sort(uncollated);
  vtkSortDataArray::Sort(collatedNums, collated);

#if VTK_FOAMFILE_DEBUG
  std::cerr << "processor (";
  for (vtkIdType proci = 0; proci < uncollated->GetNumberOfTuples(); ++proci)
  {
    std::cerr << ' ' << uncollated->GetValue(proci);
  }
  std::cerr << " )\n";

  std::cerr << "processors (";
  for (vtkIdType proci = 0; proci < collated->GetNumberOfTuples(); ++proci)
  {
    collated->GetTypedTuple(proci, procTuple);
    std::cerr << ' ' << procTuple[0];
    if (procTuple[2])
    {
      std::cerr << '_' << procTuple[1] << '-' << (procTuple[1] + procTuple[2] - 1);
    }
  }
  std::cerr << " )\n";
#endif // VTK_FOAMFILE_DEBUG

#if VTK_FOAMFILE_COLLATED_FORMAT
  const int nCollated = static_cast<int>(collated->GetNumberOfTuples());
  if (nCollated)
  {
    // Sanity checks.
    // Same number of processors, check that total number of pieces add up, etc.
    if (collatedNums->GetValue(0) != collatedNums->GetValue(nCollated - 1))
    {
      // Failed
      return uncollated;
    }
    else if (nCollated > 1)
    {
      // Identical nProcs. Now re-sort based on first-last range
      for (int i = 0; i < nCollated; ++i)
      {
        const int firstProc = collated->GetTypedComponent(i, 1);
        collatedNums->SetTValue(i, firstProc);
      }

      vtkSortDataArray::Sort(collatedNums, collated);
    }

    // Done
    return collated;
  }
#endif

  return uncollated;
}

} // End anonymous namespace

//------------------------------------------------------------------------------
vtkPOpenFOAMReader::vtkPOpenFOAMReader()
{
  this->Controller = nullptr;
  this->SetController(vtkMultiProcessController::GetGlobalController());
  if (this->Controller == nullptr)
  {
    this->NumProcesses = 1;
    this->ProcessId = 0;
  }
  else
  {
    this->NumProcesses = this->Controller->GetNumberOfProcesses();
    this->ProcessId = this->Controller->GetLocalProcessId();
  }
  this->CaseType = RECONSTRUCTED_CASE;
  this->MTimeOld = 0;
}

//------------------------------------------------------------------------------
vtkPOpenFOAMReader::~vtkPOpenFOAMReader()
{
  this->SetController(nullptr);
}

//------------------------------------------------------------------------------
void vtkPOpenFOAMReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Case Type: " << this->CaseType << endl;
  os << indent << "MTimeOld: " << this->MTimeOld << endl;
  os << indent << "Number of Processes: " << this->NumProcesses << endl;
  os << indent << "Process Id: " << this->ProcessId << endl;
  os << indent << "Controller: " << this->Controller << endl;
}

//------------------------------------------------------------------------------
void vtkPOpenFOAMReader::SetCaseType(const int t)
{
  if (this->CaseType != t)
  {
    this->CaseType = static_cast<caseType>(t);
    this->Refresh = true;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
int vtkPOpenFOAMReader::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (this->CaseType == RECONSTRUCTED_CASE)
  {
    int ret = 1;
    if (this->ProcessId == 0)
    {
      ret = this->Superclass::RequestInformation(request, inputVector, outputVector);
    }
    if (this->NumProcesses > 1)
    {
      // if there was an error in process 0 abort all processes
      this->BroadcastStatus(ret);
      if (ret == 0)
      {
        vtkErrorMacro(<< "The master process returned an error.");
        return 0;
      }

      vtkDoubleArray* timeValues;
      if (this->ProcessId == 0)
      {
        timeValues = this->Superclass::GetTimeValues();
      }
      else
      {
        timeValues = vtkDoubleArray::New();
      }
      this->Controller->Broadcast(timeValues, 0);
      if (this->ProcessId != 0)
      {
        this->Superclass::SetTimeInformation(outputVector, timeValues);
        timeValues->Delete();
        this->Superclass::Refresh = false;
      }
      this->GatherMetaData(); // pvserver deadlocks without this
    }

    return ret;
  }

  if (!this->Superclass::FileName || strlen(this->Superclass::FileName) == 0)
  {
    vtkErrorMacro("FileName has to be specified!");
    return 0;
  }

  // Handle the decomposed case

  if (*this->Superclass::FileNameOld != this->Superclass::FileName ||
    this->Superclass::ListTimeStepsByControlDict !=
      this->Superclass::ListTimeStepsByControlDictOld ||
    this->Superclass::SkipZeroTime != this->Superclass::SkipZeroTimeOld ||
    this->Superclass::Refresh)
  {
    // retain selection status when just refreshing a case
    if (!this->Superclass::FileNameOld->empty() &&
      *this->Superclass::FileNameOld != this->Superclass::FileName)
    {
      // clear selections
      this->Superclass::CellDataArraySelection->RemoveAllArrays();
      this->Superclass::PointDataArraySelection->RemoveAllArrays();
      this->Superclass::LagrangianDataArraySelection->RemoveAllArrays();
      this->Superclass::PatchDataArraySelection->RemoveAllArrays();
    }

    *this->Superclass::FileNameOld = vtkStdString(this->FileName);
    this->Superclass::Readers->RemoveAllItems();
    this->Superclass::NumberOfReaders = 0;

    // Recreate case information
    vtkStdString masterCasePath, controlDictPath;
    this->Superclass::CreateCasePath(masterCasePath, controlDictPath);

    this->Superclass::CreateCharArrayFromString(
      this->Superclass::CasePath, "CasePath", masterCasePath);

    int nProcessorDirs = 0;
    auto processorDirs = vtkSmartPointer<vtkIntArray>::New();
    vtkStringArray* timeNames;
    vtkDoubleArray* timeValues;

    int ret = 1;
    if (this->ProcessId == 0)
    {
      // Search and list processor subdirectories
      vtkNew<vtkDirectory> dir;
      if (!dir->Open(masterCasePath.c_str()))
      {
        vtkErrorMacro(<< "Cannot open " << masterCasePath);
        this->BroadcastStatus(ret = 0);
        return 0;
      }

      processorDirs = ::ScanForProcessorDirs(dir);
      nProcessorDirs = static_cast<int>(processorDirs->GetNumberOfTuples());

      if (nProcessorDirs)
      {
        // Get times from the first processor subdirectory
        const std::string procDirName = ::ProcessorDirName(processorDirs, 0);
        vtkFoamDebug(<< "First processor dir: " << procDirName << "\n");

        auto masterReader = ::NewFoamReader(this);

        if (!masterReader->MakeInformationVector(outputVector, procDirName) ||
          !masterReader->MakeMetaDataAtTimeStep(true))
        {
          this->BroadcastStatus(ret = 0);
          return 0;
        }
        this->Superclass::Readers->AddItem(masterReader);
        timeNames = masterReader->GetTimeNames();
        timeValues = masterReader->GetTimeValues();
      }
      else
      {
        timeNames = vtkStringArray::New();
        timeValues = vtkDoubleArray::New();
        this->Superclass::SetTimeInformation(outputVector, timeValues);
      }
    }
    else
    {
      timeNames = vtkStringArray::New();
      timeValues = vtkDoubleArray::New();
    }

    if (this->NumProcesses > 1)
    {
      // if there was an error in process 0 abort all processes
      this->BroadcastStatus(ret);
      if (ret == 0)
      {
        vtkErrorMacro(<< "The master process returned an error.");
        timeValues->Delete(); // don't have to care about process 0
        return 0;
      }

      this->Controller->Broadcast(processorDirs, 0);
      this->Controller->Broadcast(timeValues, 0);
      this->Broadcast(timeNames);
      if (this->ProcessId != 0)
      {
        this->Superclass::SetTimeInformation(outputVector, timeValues);
      }
      nProcessorDirs = static_cast<int>(processorDirs->GetNumberOfTuples());
    }

    // Create reader instances for processor subdirectories,
    // skip first one since it has already been created above

    for (int dirIndex = (this->ProcessId ? this->ProcessId : this->NumProcesses);
         dirIndex < nProcessorDirs; dirIndex += this->NumProcesses)
    {
      const std::string procDirName = ::ProcessorDirName(processorDirs, dirIndex);
      vtkFoamDebug(<< "Additional processor dir: " << procDirName << "\n");

      auto subReader = ::NewFoamReader(this);

      // If getting metadata failed, simply skip the reader instance
      if (subReader->MakeInformationVector(nullptr, procDirName, timeNames, timeValues) &&
        subReader->MakeMetaDataAtTimeStep(true))
      {
        this->Superclass::Readers->AddItem(subReader);
      }
      else
      {
        vtkWarningMacro(<< "Removing reader for processor subdirectory " << procDirName);
      }
    }

    // Cleanup
    if (this->ProcessId != 0 || (nProcessorDirs == 0))
    {
      timeNames->Delete();
      timeValues->Delete();
    }

    this->GatherMetaData();
    this->Superclass::Refresh = false;
  }

  outputVector->GetInformationObject(0)->Set(CAN_HANDLE_PIECE_REQUEST(), 1);

  return 1;
}

//------------------------------------------------------------------------------
int vtkPOpenFOAMReader::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkSmartPointer<vtkMultiProcessController> splitController;
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  auto* output = vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (this->CaseType == RECONSTRUCTED_CASE)
  {
    int ret = 1;
    if (this->ProcessId == 0)
    {
      ret = this->Superclass::RequestData(request, inputVector, outputVector);
    }
    this->BroadcastStatus(ret);
    this->GatherMetaData();

    if (this->NumProcesses > 1)
    {
      splitController.TakeReference(this->Controller->PartitionController(1, this->ProcessId));
      vtkNew<vtkMultiBlockDataSet> mb;
      if (this->ProcessId == 0)
      {
        mb->CopyStructure(output);
        splitController->Broadcast(mb, 0);
      }
      else
      {
        splitController->Broadcast(mb, 0);
        output->CopyStructure(mb);
      }
    }
    return ret;
  }

  int ret = 1;
  if (this->Superclass::Readers->GetNumberOfItems())
  {
    int nTimes(0); // Also used for logic
    double requestedTimeValue(0);
    if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
    {
      nTimes = outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

      // UPDATE_TIME_STEP is unreliable if there is only one time-step
      requestedTimeValue =
        (1 == nTimes ? outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), 0)
                     : outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()));

      if (nTimes)
      {
        outInfo->Set(vtkDataObject::DATA_TIME_STEP(), requestedTimeValue);
      }
    }

    // NOTE: do not call SetTimeValue() directly here

    vtkAppendCompositeDataLeaves* append = vtkAppendCompositeDataLeaves::New();
    // append->AppendFieldDataOn();

    vtkOpenFOAMReader* reader;
    this->Superclass::CurrentReaderIndex = 0;
    this->Superclass::Readers->InitTraversal();
    while ((reader = vtkOpenFOAMReader::SafeDownCast(
              this->Superclass::Readers->GetNextItemAsObject())) != nullptr)
    {
      // even if the child readers themselves are not modified, mark
      // them as modified if "this" has been modified, since they
      // refer to the property of "this"
      if ((nTimes && reader->SetTimeValue(requestedTimeValue)) ||
        (this->MTimeOld != this->GetMTime()))
      {
        reader->Modified();
      }
      if (reader->MakeMetaDataAtTimeStep(false))
      {
        append->AddInputConnection(reader->GetOutputPort());
      }
    }

    this->GatherMetaData();

    if (append->GetNumberOfInputConnections(0) == 0)
    {
      output->Initialize();
      ret = 0;
    }
    else
    {
      // reader->RequestInformation() and RequestData() are called
      // for all reader instances without setting UPDATE_TIME_STEPS
      append->Update();
      output->ShallowCopy(append->GetOutput());
    }
    append->Delete();

    // known issue: output for process without sub-reader will not have CasePath
    output->GetFieldData()->AddArray(this->Superclass::CasePath);

    // Processor 0 needs to broadcast the structure of the multiblock
    // to the processors that didn't have the chance to load something
    // To do so, we split the controller to broadcast only to the interested
    // processors (else case below) and avoid useless communication.
    splitController.TakeReference(
      this->Controller->PartitionController(this->ProcessId == 0, this->ProcessId));
    if (this->ProcessId == 0)
    {
      vtkNew<vtkMultiBlockDataSet> mb;
      mb->CopyStructure(output);
      splitController->Broadcast(mb, 0);
    }
  }
  else
  {
    this->GatherMetaData();

    // This rank did not receive anything so data structure is void.
    // Let's receive the empty but structured multiblock from processor 0
    splitController.TakeReference(this->Controller->PartitionController(true, this->ProcessId));
    vtkNew<vtkMultiBlockDataSet> mb;
    splitController->Broadcast(mb, 0);
    output->CopyStructure(mb);
  }

  this->Superclass::UpdateStatus();
  this->MTimeOld = this->GetMTime();

  return ret;
}

//------------------------------------------------------------------------------
void vtkPOpenFOAMReader::BroadcastStatus(int& status)
{
  if (this->NumProcesses > 1)
  {
    this->Controller->Broadcast(&status, 1, 0);
  }
}

//------------------------------------------------------------------------------
void vtkPOpenFOAMReader::GatherMetaData()
{
  if (this->NumProcesses > 1)
  {
    this->AllGather(this->Superclass::PatchDataArraySelection);
    this->AllGather(this->Superclass::CellDataArraySelection);
    this->AllGather(this->Superclass::PointDataArraySelection);
    this->AllGather(this->Superclass::LagrangianDataArraySelection);
    // omit removing duplicated entries of LagrangianPaths as well
    // when the number of processes is 1 assuming there's no duplicate
    // entry within a process
    this->AllGather(this->Superclass::LagrangianPaths);
  }
}

//------------------------------------------------------------------------------
// Broadcast a vtkStringArray in process 0 to all processes
void vtkPOpenFOAMReader::Broadcast(vtkStringArray* sa)
{
  vtkIdType lengths[2];
  if (this->ProcessId == 0)
  {
    lengths[0] = sa->GetNumberOfTuples();
    lengths[1] = 0;
    for (int strI = 0; strI < sa->GetNumberOfTuples(); strI++)
    {
      lengths[1] += static_cast<vtkIdType>(sa->GetValue(strI).length()) + 1;
    }
  }
  this->Controller->Broadcast(lengths, 2, 0);
  char* contents = new char[lengths[1]];
  if (this->ProcessId == 0)
  {
    for (int strI = 0, idx = 0; strI < sa->GetNumberOfTuples(); strI++)
    {
      const int len = static_cast<int>(sa->GetValue(strI).length()) + 1;
      memmove(contents + idx, sa->GetValue(strI).c_str(), len);
      idx += len;
    }
  }
  this->Controller->Broadcast(contents, lengths[1], 0);
  if (this->ProcessId != 0)
  {
    sa->Initialize();
    sa->SetNumberOfTuples(lengths[0]);
    for (int strI = 0, idx = 0; strI < lengths[0]; strI++)
    {
      sa->SetValue(strI, contents + idx);
      idx += static_cast<int>(sa->GetValue(strI).length()) + 1;
    }
  }
  delete[] contents;
}

//------------------------------------------------------------------------------
// AllGather vtkStringArray from and to all processes
void vtkPOpenFOAMReader::AllGather(vtkStringArray* s)
{
  vtkIdType length = 0;
  for (int strI = 0; strI < s->GetNumberOfTuples(); strI++)
  {
    length += static_cast<vtkIdType>(s->GetValue(strI).length()) + 1;
  }
  vtkIdType* lengths = new vtkIdType[this->NumProcesses];
  this->Controller->AllGather(&length, lengths, 1);
  vtkIdType totalLength = 0;
  vtkIdType* offsets = new vtkIdType[this->NumProcesses];
  for (int procI = 0; procI < this->NumProcesses; procI++)
  {
    offsets[procI] = totalLength;
    totalLength += lengths[procI];
  }
  char *allContents = new char[totalLength], *contents = new char[length];
  for (int strI = 0, idx = 0; strI < s->GetNumberOfTuples(); strI++)
  {
    const int len = static_cast<int>(s->GetValue(strI).length()) + 1;
    memmove(contents + idx, s->GetValue(strI).c_str(), len);
    idx += len;
  }
  this->Controller->AllGatherV(contents, allContents, length, lengths, offsets);
  delete[] contents;
  delete[] lengths;
  delete[] offsets;
  s->Initialize();
  for (int idx = 0; idx < totalLength; idx += static_cast<int>(strlen(allContents + idx)) + 1)
  {
    const char* str = allContents + idx;
    // insert only when the same string is not found
    if (s->LookupValue(str) == -1)
    {
      s->InsertNextValue(str);
    }
  }
  s->Squeeze();
  delete[] allContents;
}

//------------------------------------------------------------------------------
// AllGather vtkDataArraySelections from and to all processes
void vtkPOpenFOAMReader::AllGather(vtkDataArraySelection* s)
{
  vtkIdType length = 0;
  for (int strI = 0; strI < s->GetNumberOfArrays(); strI++)
  {
    length += static_cast<vtkIdType>(strlen(s->GetArrayName(strI))) + 2;
  }
  vtkIdType* lengths = new vtkIdType[this->NumProcesses];
  this->Controller->AllGather(&length, lengths, 1);
  vtkIdType totalLength = 0;
  vtkIdType* offsets = new vtkIdType[this->NumProcesses];
  for (int procI = 0; procI < this->NumProcesses; procI++)
  {
    offsets[procI] = totalLength;
    totalLength += lengths[procI];
  }
  char *allContents = new char[totalLength], *contents = new char[length];
  for (int strI = 0, idx = 0; strI < s->GetNumberOfArrays(); strI++)
  {
    const char* arrayName = s->GetArrayName(strI);
    contents[idx] = s->ArrayIsEnabled(arrayName);
    const int len = static_cast<int>(strlen(arrayName)) + 1;
    memmove(contents + idx + 1, arrayName, len);
    idx += len + 1;
  }
  this->Controller->AllGatherV(contents, allContents, length, lengths, offsets);
  delete[] contents;
  delete[] lengths;
  delete[] offsets;
  // do not RemoveAllArray so that the previous arrays are preserved
  // s->RemoveAllArrays();
  for (int idx = 0; idx < totalLength; idx += static_cast<int>(strlen(allContents + idx + 1)) + 2)
  {
    const char* arrayName = allContents + idx + 1;
    s->AddArray(arrayName);
    if (allContents[idx] == 0)
    {
      s->DisableArray(arrayName);
    }
    else
    {
      s->EnableArray(arrayName);
    }
  }
  delete[] allContents;
}
