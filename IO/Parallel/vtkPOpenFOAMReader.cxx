// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This class was developed by Takuya Oshima at Niigata University,
// Japan (oshima@eng.niigata-u.ac.jp).
//
// Mark Olesen (OpenCFD Ltd.) www.openfoam.com
// has provided various bugfixes, improvements, cleanup
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
#include "vtkDummyController.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h"
#include "vtkSortDataArray.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkUnsignedCharArray.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <numeric>

//------------------------------------------------------------------------------

VTK_ABI_NAMESPACE_BEGIN
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
  // This function is only used in the decomposed case, so we are basically multithreading (if
  // asked) over the processor dirs, but not over all /polyMesh/faces files of each processor dir.
  reader->SetSequentialProcessing(true);

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
    this->SetController(vtkDummyController::New());
    this->NumProcesses = 1;
    this->ProcessId = 0;
  }
  else
  {
    this->NumProcesses = this->Controller->GetNumberOfProcesses();
    this->ProcessId = this->Controller->GetLocalProcessId();
  }
  this->CaseType = RECONSTRUCTED_CASE;
  this->ReadAllFilesToDetermineStructure = true;
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
void vtkPOpenFOAMReader::SetCaseType(int t)
{
  if (this->CaseType != t)
  {
    this->CaseType = static_cast<caseType>(t);
    this->Refresh = true;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkPOpenFOAMReader::SetReadAllFilesToDetermineStructure(bool readAllFilesToDetermineStructure)
{
  if (this->ReadAllFilesToDetermineStructure != readAllFilesToDetermineStructure)
  {
    this->ReadAllFilesToDetermineStructure = readAllFilesToDetermineStructure;
    this->Refresh = true;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
int vtkPOpenFOAMReader::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
#if VTK_OPENFOAM_TIME_PROFILING
  this->InitializeRequestInformation();
#endif
  const bool isRootProc = (this->ProcessId == 0);
  const bool isParallel = (this->NumProcesses > 1);
  int returnCode = 1;

  // Set handle piece request for all cases. Even if the reconstructed case is not actually
  // distributed, we need all processes to go through RequestData every time in order to go through
  // "Gather" functions.
  outputVector->GetInformationObject(0)->Set(CAN_HANDLE_PIECE_REQUEST(), 1);

  if (this->CaseType == RECONSTRUCTED_CASE)
  {
#if VTK_OPENFOAM_TIME_PROFILING
    auto start = std::chrono::high_resolution_clock::now();
#endif
    if (isRootProc)
    {
      returnCode = this->Superclass::RequestInformation(request, inputVector, outputVector);
    }

    if (isParallel)
    {
      this->Controller->Broadcast(&returnCode, 1, 0);

      if (returnCode == 0)
      {
        // Error encountered in process 0 - abort all processes
        vtkErrorMacro(<< "The master process returned an error.");
        return 0;
      }

      vtkDoubleArray* timeValues = nullptr;
      if (isRootProc)
      {
        timeValues = this->Superclass::GetTimeValues();
      }
      else
      {
        timeValues = vtkDoubleArray::New();
      }
      this->Controller->Broadcast(timeValues, 0);
      if (!isRootProc)
      {
        this->Superclass::SetTimeInformation(outputVector, timeValues);
        this->Superclass::Refresh = false;
        timeValues->Delete();
      }
    }

    this->GatherMetaData();
#if VTK_OPENFOAM_TIME_PROFILING
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "vtkPOpenFOAMReader::RequestInformation: Elapsed time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms"
              << std::endl;
    this->Superclass::PrintRequestInformation();
#endif
    return returnCode;
  }

  if (!this->Superclass::FileName || strlen(this->Superclass::FileName) == 0)
  {
    vtkErrorMacro("FileName has to be specified!");
    return 0;
  }

  // Handle the decomposed case

  if (this->Superclass::FileNameOld != this->Superclass::FileName ||
    this->Superclass::ListTimeStepsByControlDict !=
      this->Superclass::ListTimeStepsByControlDictOld ||
    this->Superclass::SkipZeroTime != this->Superclass::SkipZeroTimeOld ||
    this->Superclass::Refresh)
  {
#if VTK_OPENFOAM_TIME_PROFILING
    auto start = std::chrono::high_resolution_clock::now();
#endif
    // retain selection status when just refreshing a case
    if (!this->Superclass::FileNameOld.empty() &&
      this->Superclass::FileNameOld != this->Superclass::FileName)
    {
      // clear selections
      this->Superclass::CellDataArraySelection->RemoveAllArrays();
      this->Superclass::PointDataArraySelection->RemoveAllArrays();
      this->Superclass::LagrangianDataArraySelection->RemoveAllArrays();
      this->Superclass::PatchDataArraySelection->RemoveAllArrays();
    }

    this->Superclass::FileNameOld = this->FileName;
    this->Superclass::Readers.clear();

    // Recreate case information
    vtkStdString masterCasePath, controlDictPath;
    this->Superclass::CreateCasePath(masterCasePath, controlDictPath);

    this->Superclass::CreateCharArrayFromString(
      this->Superclass::CasePath, "CasePath", masterCasePath);

    int nProcessorDirs = 0;
    auto processorDirs = vtkSmartPointer<vtkIntArray>::New();
    vtkStringArray* timeNames = nullptr;
    vtkDoubleArray* timeValues = nullptr;
    std::vector<vtkSmartPointer<vtkTable>> fieldNameInfoPerPrivateReader;
    std::vector<vtkSmartPointer<vtkUnsignedCharArray>>
      populateMeshIndicesFileChecksPerPrivateReader;

    if (isRootProc)
    {
      do
      {
        // Search and list processor subdirectories
        vtkNew<vtkDirectory> dir;
        if (!dir->Open(masterCasePath.c_str()))
        {
          vtkErrorMacro(<< "Cannot open " << masterCasePath);
          returnCode = 0;
          break; // Failed
        }

        processorDirs = ::ScanForProcessorDirs(dir);
        nProcessorDirs = static_cast<int>(processorDirs->GetNumberOfTuples());
#if VTK_OPENFOAM_TIME_PROFILING
        std::cout << "nProcessorDirs: " << nProcessorDirs << std::endl;
#endif
        if (nProcessorDirs)
        {
          // Get times from the first processor subdirectory
          const std::string procDirName = ::ProcessorDirName(processorDirs, 0);
          vtkFoamDebug(<< "First processor dir: " << procDirName << "\n");

          auto masterReader = ::NewFoamReader(this);

          if (!masterReader->MakeInformationVector(outputVector, procDirName) ||
            !masterReader->MakeMetaDataAtTimeStep(true, /*skipComputingMetaData=*/false))
          {
            returnCode = 0;
            break; // Failed
          }
          this->Superclass::Readers.emplace_back(masterReader);
          timeNames = masterReader->GetTimeNames();
          timeValues = masterReader->GetTimeValues();
          if (!this->ReadAllFilesToDetermineStructure)
          {
            fieldNameInfoPerPrivateReader = masterReader->GetMarshalledMetadataPerReader();
            populateMeshIndicesFileChecksPerPrivateReader =
              masterReader->GetPopulateMeshIndicesFileChecksPerReader();
          }
        }
        else
        {
          timeNames = vtkStringArray::New();
          timeValues = vtkDoubleArray::New();
          this->Superclass::SetTimeInformation(outputVector, timeValues);
        }
      } while (false); // End of process 0 only execution scope
    }

    if (isParallel)
    {
      this->Controller->Broadcast(&returnCode, 1, 0);

      if (returnCode == 0)
      {
        // Error encountered in process 0 - abort all processes
        vtkErrorMacro(<< "The master process returned an error.");
        return 0;
      }

      if (!isRootProc)
      {
        timeNames = vtkStringArray::New();
        timeValues = vtkDoubleArray::New();
      }

      this->Controller->Broadcast(processorDirs, 0);
      this->Controller->Broadcast(timeValues, 0);
      this->Broadcast(timeNames);
      if (!this->ReadAllFilesToDetermineStructure)
      {
        size_t numberOfPrivateReaders = fieldNameInfoPerPrivateReader.size();
        this->Controller->Broadcast(&numberOfPrivateReaders, 1, 0);

        fieldNameInfoPerPrivateReader.resize(numberOfPrivateReaders);
        populateMeshIndicesFileChecksPerPrivateReader.resize(numberOfPrivateReaders);
        for (size_t i = 0; i < numberOfPrivateReaders; i++)
        {
          if (!isRootProc)
          {
            fieldNameInfoPerPrivateReader[i] = vtkSmartPointer<vtkTable>::New();
            populateMeshIndicesFileChecksPerPrivateReader[i] =
              vtkSmartPointer<vtkUnsignedCharArray>::New();
          }
          this->Controller->Broadcast(fieldNameInfoPerPrivateReader[i], 0);
          this->Controller->Broadcast(populateMeshIndicesFileChecksPerPrivateReader[i], 0);
        }
        this->BroadcastMetaData();
      }
      if (!isRootProc)
      {
        this->Superclass::SetTimeInformation(outputVector, timeValues);
      }
      nProcessorDirs = static_cast<int>(processorDirs->GetNumberOfTuples());
    }
    else
    {
      if (returnCode == 0)
      {
        // Error encountered (single process). Nothing to cleanup
        return 0;
      }
    }

    // Create reader instances for processor subdirectories,
    // skip first one since it has already been created above

    std::vector<std::string> procDirNames;
    for (int dirIndex = (this->ProcessId ? this->ProcessId : this->NumProcesses);
         dirIndex < nProcessorDirs; dirIndex += this->NumProcesses)
    {
      const std::string procDirName = ::ProcessorDirName(processorDirs, dirIndex);
      vtkFoamDebug(<< "Additional processor dir: " << procDirName << "\n");
      procDirNames.push_back(procDirName);
    }

    std::vector<vtkSmartPointer<vtkObject>> procReaders(procDirNames.size(), nullptr);
    auto readersMakeInformationVector = [&](vtkIdType begin, vtkIdType end) -> void
    {
      for (vtkIdType i = begin; i < end; ++i)
      {
        auto subReader = ::NewFoamReader(this);
        if (this->ReadAllFilesToDetermineStructure)
        {
          // If getting metadata failed, simply skip the reader instance
          if (subReader->MakeInformationVector(nullptr, procDirNames[i], timeNames, timeValues) &&
            subReader->MakeMetaDataAtTimeStep(true, /*skipComputingMetaData=*/false))
          {
            procReaders[i] = subReader;
          }
          else
          {
            vtkWarningMacro(<< "Removing reader for processor subdirectory " << procDirNames[i]);
          }
        }
        else
        {
          const bool makeVector = subReader->MakeInformationVector(nullptr, procDirNames[i],
            timeNames, timeValues, populateMeshIndicesFileChecksPerPrivateReader);
          // the internal readers are created after makeInformationVector is called,
          // so the metadata can now be called.
          subReader->SetMarshalledMetadataPerReader(fieldNameInfoPerPrivateReader);
          // so now we can call MakeMetaDataAtTimeStep, since the internal re
          const bool makeMetaData =
            makeVector && subReader->MakeMetaDataAtTimeStep(true, /*skipComputingMetaData=*/true);
          // If getting metadata failed, simply skip the reader instance
          if (makeMetaData)
          {
            procReaders[i] = subReader;
          }
          else
          {
            vtkWarningMacro(<< "Removing reader for processor subdirectory " << procDirNames[i]);
          }
        }
      }
    };
    if (this->GetSequentialProcessing())
    {
      readersMakeInformationVector(0, static_cast<vtkIdType>(procReaders.size()));
    }
    else
    {
      vtkSMPTools::For(0, static_cast<vtkIdType>(procReaders.size()), readersMakeInformationVector);
    }
    // move valid readers to the main list
    std::copy_if(std::make_move_iterator(procReaders.begin()),
      std::make_move_iterator(procReaders.end()), std::back_inserter(this->Superclass::Readers),
      [](const auto& reader) { return reader != nullptr; });

    // Cleanup
    if (!isRootProc || (nProcessorDirs == 0))
    {
      if (timeNames)
      {
        timeNames->Delete();
      }
      if (timeValues)
      {
        timeValues->Delete();
      }
    }

    if (this->ReadAllFilesToDetermineStructure)
    {
      this->GatherMetaData();
    }
    this->Superclass::Refresh = false;
#if VTK_OPENFOAM_TIME_PROFILING
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "vtkPOpenFOAMReader::RequestInformation: Elapsed time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms"
              << std::endl;
    this->PrintRequestInformation();
#endif
  }

  return returnCode;
}

//------------------------------------------------------------------------------
int vtkPOpenFOAMReader::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
#if VTK_OPENFOAM_TIME_PROFILING
  this->InitializeRequestData();
#endif
  const bool isRootProc = (this->ProcessId == 0);
  const bool isParallel = (this->NumProcesses > 1);
  int returnCode = 1;

  vtkSmartPointer<vtkMultiProcessController> splitController;
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  auto* output = vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (this->CaseType == RECONSTRUCTED_CASE)
  {
#if VTK_OPENFOAM_TIME_PROFILING
    auto start = std::chrono::high_resolution_clock::now();
#endif
    if (isRootProc)
    {
      returnCode = this->Superclass::RequestData(request, inputVector, outputVector);
    }
    this->GatherMetaData();

    if (isParallel)
    {
      this->Controller->Broadcast(&returnCode, 1, 0);

      splitController.TakeReference(this->Controller->PartitionController(1, this->ProcessId));
      vtkNew<vtkMultiBlockDataSet> mb;
      if (isRootProc)
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
#if VTK_OPENFOAM_TIME_PROFILING
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "vtkPOpenFOAMReader::RequestData: Elapsed time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms"
              << std::endl;
    this->Superclass::PrintRequestData();
#endif

    return returnCode;
  }

#if VTK_OPENFOAM_TIME_PROFILING
  auto start = std::chrono::high_resolution_clock::now();
#endif
  if (!this->Superclass::Readers.empty())
  {
    int nTimes = 0; // Also used for logic
    double requestedTimeValue = 0;
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

    vtkNew<vtkAppendCompositeDataLeaves> append;

    std::vector<int> updatedReaders(this->Superclass::Readers.size(), 0);
    auto readersProcessing = [&](vtkIdType begin, vtkIdType end) -> void
    {
      for (vtkIdType i = begin; i < end; ++i)
      {
        vtkOpenFOAMReader* reader = vtkOpenFOAMReader::SafeDownCast(this->Superclass::Readers[i]);
        if (!reader)
        {
          continue;
        }
        // even if the child readers themselves are not modified, mark
        // them as modified if "this" has been modified, since they
        // refer to the property of "this"
        if ((nTimes && reader->SetTimeValue(requestedTimeValue)) ||
          (this->MTimeOld != this->GetMTime()))
        {
          reader->Modified();
        }
        if (reader->MakeMetaDataAtTimeStep(false, /*skipComputingMetaData=*/false))
        {
          reader->Update();
          updatedReaders[i] = 1;
        }
      }
    };
    if (this->GetSequentialProcessing())
    {
      readersProcessing(0, static_cast<vtkIdType>(this->Superclass::Readers.size()));
    }
    else
    {
      vtkSMPTools::For(
        0, static_cast<vtkIdType>(this->Superclass::Readers.size()), readersProcessing);
    }
    for (size_t i = 0; i < this->Superclass::Readers.size(); ++i)
    {
      if (updatedReaders[i])
      {
        append->AddInputConnection(
          vtkOpenFOAMReader::SafeDownCast(this->Superclass::Readers[i])->GetOutputPort());
      }
    }

    this->GatherMetaData();

    if (append->GetNumberOfInputConnections(0) == 0)
    {
      output->Initialize();
      returnCode = 0;
    }
    else
    {
      // reader->RequestInformation() and RequestData() are called
      // for all reader instances without setting UPDATE_TIME_STEPS
      append->Update();
      output->CompositeShallowCopy(append->GetOutput());
    }

    // known issue: output for process without sub-reader will not have CasePath
    output->GetFieldData()->AddArray(this->Superclass::CasePath);

    // Processor 0 needs to broadcast the structure of the multiblock
    // to the processors that didn't have the chance to load something
    // To do so, we split the controller to broadcast only to the interested
    // processors (else case below) and avoid useless communication.
    splitController.TakeReference(
      this->Controller->PartitionController(isRootProc, this->ProcessId));
    if (isRootProc)
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

#if VTK_OPENFOAM_TIME_PROFILING
  auto end = std::chrono::high_resolution_clock::now();
  std::cout << "vtkPOpenFOAMReader::RequestData: Elapsed time: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms"
            << std::endl;
  this->PrintRequestData();
#endif
  return returnCode;
}

//------------------------------------------------------------------------------
// Note: includes guard against non-parallel calls
void vtkPOpenFOAMReader::BroadcastMetaData()
{
  if (this->NumProcesses > 1)
  {
    this->Broadcast(this->Superclass::PatchDataArraySelection);
    this->Broadcast(this->Superclass::CellDataArraySelection);
    this->Broadcast(this->Superclass::PointDataArraySelection);
    this->Broadcast(this->Superclass::LagrangianDataArraySelection);
  }
}

//------------------------------------------------------------------------------
// Note: includes guard against non-parallel calls
void vtkPOpenFOAMReader::GatherMetaData()
{
  if (this->NumProcesses > 1)
  {
    this->AllGather(this->Superclass::PatchDataArraySelection);
    this->AllGather(this->Superclass::CellDataArraySelection);
    this->AllGather(this->Superclass::PointDataArraySelection);
    this->AllGather(this->Superclass::LagrangianDataArraySelection);
  }
}

//------------------------------------------------------------------------------
// Broadcast a vtkStringArray in process 0 to all processes
void vtkPOpenFOAMReader::Broadcast(vtkStringArray* sa)
{
  vtkNew<vtkTable> table;
  if (this->ProcessId == 0)
  {
    table->GetFieldData()->AddArray(sa);
  }

  this->Controller->Broadcast(table.GetPointer(), 0);

  if (this->ProcessId != 0)
  {
    sa->DeepCopy(table->GetFieldData()->GetAbstractArray(0));
  }
}

//------------------------------------------------------------------------------
// Broadcast vtkDataArraySelections in process 0 to all processes
void vtkPOpenFOAMReader::Broadcast(vtkDataArraySelection* dataArraySelection)
{
  vtkNew<vtkTable> table;
  if (this->ProcessId == 0)
  {
    vtkNew<vtkStringArray> patchNames;
    patchNames->SetName("PatchNames");
    vtkNew<vtkIntArray> patchStatus;
    patchStatus->SetName("PatchStatus");
    if (const int numOfArrays = dataArraySelection->GetNumberOfArrays())
    {
      patchNames->SetNumberOfValues(numOfArrays);
      patchStatus->SetNumberOfValues(numOfArrays);
      for (int i = 0; i < numOfArrays; ++i)
      {
        patchNames->SetValue(i, dataArraySelection->GetArrayName(i));
        patchStatus->SetValue(i, dataArraySelection->GetArraySetting(i));
      }
      table->GetFieldData()->AddArray(patchNames);
      table->GetFieldData()->AddArray(patchStatus);
    }
  }
  this->Controller->Broadcast(table, 0);
  if (table->GetFieldData()->GetNumberOfArrays() != 2)
  {
    return;
  }

  if (this->ProcessId != 0)
  {
    dataArraySelection->RemoveAllArrays();
    auto names = vtkStringArray::SafeDownCast(table->GetFieldData()->GetAbstractArray(0));
    auto status = vtkIntArray::SafeDownCast(table->GetFieldData()->GetAbstractArray(1));
    for (vtkIdType i = 0; i < names->GetNumberOfValues(); ++i)
    {
      const int index = dataArraySelection->GetArrayIndex(names->GetValue(i).c_str());
      if (index == -1)
      {
        dataArraySelection->SetArraySetting(names->GetValue(i).c_str(), status->GetValue(i));
      }
      else
      {
        dataArraySelection->SetArraySetting(names->GetValue(i).c_str(), status->GetValue(i));
      }
    }
  }
}

//------------------------------------------------------------------------------
// AllGather vtkStringArray from and to all processes
void vtkPOpenFOAMReader::AllGather(vtkStringArray* sa)
{
  vtkNew<vtkTable> table;
  table->GetFieldData()->AddArray(sa);

  std::vector<vtkSmartPointer<vtkDataObject>> dataObjects;
  this->Controller->AllGather(table, dataObjects);

  sa->Initialize();
  for (auto& dataObject : dataObjects)
  {
    auto array = vtkStringArray::SafeDownCast(dataObject->GetFieldData()->GetAbstractArray(0));
    for (vtkIdType i = 0; i < array->GetNumberOfValues(); ++i)
    {
      if (sa->LookupValue(array->GetValue(i)) == -1)
      {
        sa->InsertNextValue(array->GetValue(i));
      }
    }
  }
}

//------------------------------------------------------------------------------
// AllGather vtkDataArraySelections from/to all processes
void vtkPOpenFOAMReader::AllGather(vtkDataArraySelection* dataArraySelection)
{
  vtkNew<vtkTable> table;
  vtkNew<vtkStringArray> patchNames;
  patchNames->SetName("PatchNames");
  vtkNew<vtkIntArray> patchStatus;
  patchStatus->SetName("PatchStatus");
  if (const int numOfArrays = dataArraySelection->GetNumberOfArrays())
  {
    patchNames->SetNumberOfValues(numOfArrays);
    patchStatus->SetNumberOfValues(numOfArrays);
    for (int i = 0; i < numOfArrays; ++i)
    {
      patchNames->SetValue(i, dataArraySelection->GetArrayName(i));
      patchStatus->SetValue(i, dataArraySelection->GetArraySetting(i));
    }
    table->GetFieldData()->AddArray(patchNames);
    table->GetFieldData()->AddArray(patchStatus);
  }

  std::vector<vtkSmartPointer<vtkDataObject>> dataObjects;
  this->Controller->AllGather(table, dataObjects);

  // do not RemoveAllArrays so that the previous arrays are preserved
  // dataArraySelection->RemoveAllArrays();
  for (auto& dataObject : dataObjects)
  {
    if (dataObject->GetFieldData()->GetNumberOfArrays() != 2)
    {
      continue;
    }
    auto names = vtkStringArray::SafeDownCast(dataObject->GetFieldData()->GetAbstractArray(0));
    auto status = vtkIntArray::SafeDownCast(dataObject->GetFieldData()->GetAbstractArray(1));
    for (vtkIdType i = 0; i < names->GetNumberOfValues(); ++i)
    {
      const auto name = names->GetValue(i).c_str();
      const auto isEnabled = status->GetValue(i);
      // Add new or update current state
      if (!dataArraySelection->AddArray(name, isEnabled))
      {
        dataArraySelection->SetArraySetting(name, isEnabled);
      }
    }
  }
}

//------------------------------------------------------------------------------
double vtkPOpenFOAMReader::ComputeProgress()
{
  // get valid readers
  std::vector<vtkOpenFOAMReader*> readers;
  for (auto& readerObj : this->Superclass::Readers)
  {
    if (auto reader = vtkOpenFOAMReader::SafeDownCast(readerObj))
    {
      readers.push_back(reader);
    }
  }
  // compute the current progress
  if (!readers.empty())
  {
    const double sum = std::accumulate(readers.begin(), readers.end(), 0.0,
      [](double acc, const auto& reader) { return acc + reader->ComputeProgress(); });
    return sum / readers.size();
  }
  return 1.0;
}

#if VTK_OPENFOAM_TIME_PROFILING
//------------------------------------------------------------------------------
void vtkPOpenFOAMReader::InitializeRequestInformation()
{
  this->Superclass::InitializeRequestInformation();
  for (auto& readerObj : this->Readers)
  {
    if (auto reader = vtkOpenFOAMReader::SafeDownCast(readerObj))
    {
      reader->InitializeRequestInformation();
    }
  }
}
//------------------------------------------------------------------------------
void vtkPOpenFOAMReader::InitializeRequestData()
{
  this->Superclass::InitializeRequestData();
  for (auto& readerObj : this->Readers)
  {
    if (auto reader = vtkOpenFOAMReader::SafeDownCast(readerObj))
    {
      reader->InitializeRequestData();
    }
  }
}

//------------------------------------------------------------------------------
void vtkPOpenFOAMReader::PrintRequestInformation()
{
  long long count = 0, minTime = VTK_LONG_LONG_MAX, maxTime = 0;
  size_t minBytes = VTK_UNSIGNED_LONG_LONG_MAX, maxBytes = 0;
  double totalTime = 0, totalBytes = 0;
  for (auto& readerObj : this->Readers)
  {
    if (auto reader = vtkOpenFOAMReader::SafeDownCast(readerObj))
    {
      minTime = std::min(minTime, reader->GetRequestInformationTimeInMicroseconds());
      maxTime = std::max(maxTime, reader->GetRequestInformationTimeInMicroseconds());
      totalTime += reader->GetRequestInformationTimeInMicroseconds();
      minBytes = std::min(minBytes, reader->GetRequestInformationBytes());
      maxBytes = std::max(maxBytes, reader->GetRequestInformationBytes());
      totalBytes += reader->GetRequestInformationBytes();
      count++;
    }
  }
  if (count > 0)
  {
    std::cout << "vtkPOpenFOAMReader::RequestInformation: " << count
              << " Readers' RequestInformation I/O Time: min=" << (minTime / 1000.0)
              << " ms, max=" << (maxTime / 1000.0) << " ms, avg=" << ((totalTime / count) / 1000.0)
              << " ms" << std::endl;
    std::cout << "vtkPOpenFOAMReader::RequestInformation: " << count
              << " Readers' RequestInformation I/O Size: min=" << (minBytes / (1024.0 * 1024.0))
              << " MB, max=" << (maxBytes / (1024.0 * 1024.0))
              << " MB, avg=" << ((totalBytes / count) / (1024.0 * 1024.0)) << " MB" << std::endl;
  }
}

//------------------------------------------------------------------------------
void vtkPOpenFOAMReader::PrintRequestData()
{
  long long count = 0, minTime = VTK_LONG_LONG_MAX, maxTime = 0;
  size_t minBytes = VTK_UNSIGNED_LONG_LONG_MAX, maxBytes = 0;
  double totalTime = 0, totalBytes = 0;
  for (auto& readerObj : this->Readers)
  {
    if (auto reader = vtkOpenFOAMReader::SafeDownCast(readerObj))
    {
      minTime = std::min(minTime, reader->GetRequestDataTimeInMicroseconds());
      maxTime = std::max(maxTime, reader->GetRequestDataTimeInMicroseconds());
      totalTime += reader->GetRequestDataTimeInMicroseconds();
      minBytes = std::min(minBytes, reader->GetRequestDataBytes());
      maxBytes = std::max(maxBytes, reader->GetRequestDataBytes());
      totalBytes += reader->GetRequestDataBytes();
      count++;
    }
  }
  if (count > 0)
  {
    std::cout << "vtkPOpenFOAMReader::RequestData: " << count
              << " Readers' RequestData I/O Time: min=" << (minTime / 1000.0)
              << " ms, max=" << (maxTime / 1000.0) << " ms, avg=" << ((totalTime / count) / 1000.0)
              << " ms" << std::endl;
    std::cout << "vtkPOpenFOAMReader::RequestData: " << count
              << " Readers' RequestData I/O Size: min=" << (minBytes / (1024.0 * 1024.0))
              << " MB, max=" << (maxBytes / (1024.0 * 1024.0))
              << " MB, avg=" << ((totalBytes / count) / (1024.0 * 1024.0)) << " MB" << std::endl;
  }
}
#endif
VTK_ABI_NAMESPACE_END
