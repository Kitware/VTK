// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkFidesWriter.h"

#include "vtkCellGrid.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArraySelection.h"
#include "vtkDataAssembly.h"
#include "vtkDataAssemblyUtilities.h"
#include "vtkDataSet.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringFormatter.h"
#include "vtksys/SystemTools.hxx"

// Fides includes
#include <vtk_fides.h>
// clang-format off
#include VTK_FIDES(fides/FidesDataSetWriter.h)
// clang-format on

#ifdef IOFIDES_HAVE_MPI
#include "vtkMPI.h"
#include "vtkMPIController.h"
#endif

#include <cassert>
#include <memory>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkFidesWriter);
vtkCxxSetObjectMacro(vtkFidesWriter, Controller, vtkMultiProcessController);

struct vtkFidesWriter::FidesWriterImpl
{
  std::vector<double> TimeSteps;
  std::vector<double> TimeStepsToProcess;
  int CurrentTimeStepIndex{ 0 };

  std::map<std::string, std::unique_ptr<fides::io::FidesDataSetWriter>> Writers;

  FidesWriterImpl() = default;
  ~FidesWriterImpl()
  {
    for (auto& writer : this->Writers)
    {
      writer.second->Close();
    }
  }

  void Initialize() { this->CurrentTimeStepIndex = 0; }
};

//------------------------------------------------------------------------------
vtkFidesWriter::vtkFidesWriter()
  : Impl(new FidesWriterImpl)
  , Controller(nullptr)
  , FileName(nullptr)
  , AdiosConfigFile(nullptr)
  , ChooseFieldsToWrite(false)
  , TimeStepRange{ 0, VTK_INT_MAX - 1 }
  , TimeStepStride(1)
  , Engine(EngineTypes::BPFile)
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(0);
}

//------------------------------------------------------------------------------
vtkFidesWriter::~vtkFidesWriter()
{
  this->SetFileName(nullptr);
  this->SetAdiosConfigFile(nullptr);
  this->SetController(nullptr);
}

//------------------------------------------------------------------------------
vtkDataArraySelection* vtkFidesWriter::GetArraySelection(int association)
{
  if (association < 3)
  {
    return this->ArraySelection[association];
  }
  vtkErrorMacro("Invalid association: " << association);
  return nullptr;
}

//------------------------------------------------------------------------------
vtkDataArraySelection* vtkFidesWriter::GetPointDataArraySelection()
{
  return this->GetArraySelection(vtkDataObject::FIELD_ASSOCIATION_POINTS);
}

//------------------------------------------------------------------------------
vtkDataArraySelection* vtkFidesWriter::GetCellDataArraySelection()
{
  return this->GetArraySelection(vtkDataObject::FIELD_ASSOCIATION_CELLS);
}

//------------------------------------------------------------------------------
vtkDataArraySelection* vtkFidesWriter::GetFieldDataArraySelection()
{
  return this->GetArraySelection(vtkDataObject::FIELD_ASSOCIATION_NONE);
}

//------------------------------------------------------------------------------
int vtkFidesWriter::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSetCollection");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCellGrid");
  return 1;
}

//------------------------------------------------------------------------------
int vtkFidesWriter::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    return this->RequestInformation(request, inputVector, outputVector);
  }
  else if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
  }
  else if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    return this->RequestData(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//------------------------------------------------------------------------------
int vtkFidesWriter::RequestUpdateExtent(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector*)
{
  // setup pipeline request
  auto* info = inputVector[0]->GetInformationObject(0);
  if (auto* controller = this->GetController())
  {
    const int rank = controller->GetLocalProcessId();
    const int numRanks = controller->GetNumberOfProcesses();

    info->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), rank);
    info->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), numRanks);
    vtkLog(TRACE, "piece " << rank << " of " << numRanks);
  }

  if (this->Impl->CurrentTimeStepIndex >= 0 &&
    this->Impl->CurrentTimeStepIndex < static_cast<int>(this->Impl->TimeSteps.size()))
  {
    info->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(),
      this->Impl->TimeSteps[this->Impl->CurrentTimeStepIndex]);
    vtkLog(TRACE,
      "time step " << this->Impl->CurrentTimeStepIndex << " of " << this->Impl->TimeSteps.size());
  }
  else
  {
    info->Remove(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkFidesWriter::RequestInformation(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector*)
{
  vtkLogScopeFunction(TRACE);

  auto inInfo = inputVector[0]->GetInformationObject(0);
  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    const auto numTimesteps = inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    const auto timesteps = inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    vtkLog(TRACE, "found " << numTimesteps << " time steps");
    vtkLog(TRACE, "time steps: " << timesteps[0] << " to " << timesteps[numTimesteps - 1]);

    this->Impl->TimeSteps.resize(numTimesteps);
    std::copy(timesteps, timesteps + numTimesteps, this->Impl->TimeSteps.begin());
    if (this->TimeStepRange[0] >= this->TimeStepRange[1] || this->TimeStepStride < 1)
    {
      this->Impl->TimeStepsToProcess = this->Impl->TimeSteps;
    }
    else
    {
      const auto begin = std::max(this->TimeStepRange[0], 0);
      const auto end = std::min(this->TimeStepRange[1] + 1, static_cast<int>(numTimesteps));
      const auto stride = this->TimeStepStride;
      this->Impl->TimeStepsToProcess.clear();
      for (int i = begin; i < end; i += stride)
      {
        this->Impl->TimeStepsToProcess.push_back(this->Impl->TimeSteps[i]);
      }
    }
  }
  else
  {
    this->Impl->TimeSteps.clear();
    this->Impl->TimeStepsToProcess.clear();
  }
  this->Impl->Initialize();
  return 1;
}

//------------------------------------------------------------------------------
int vtkFidesWriter::RequestData(
  vtkInformation* request, vtkInformationVector**, vtkInformationVector*)
{
  vtkLogScopeFunction(TRACE);

  if (!this->FileName || !this->FileName[0])
  {
    vtkErrorMacro("Cannot write without a valid filename!");
    return 0;
  }

  if (!this->Impl->TimeSteps.empty())
  {
    const auto& currentTime = this->Impl->TimeSteps[this->Impl->CurrentTimeStepIndex];
    vtkLog(TRACE, "current time step " << currentTime);

    // check if timestep should be processed or skipped
    const bool processTimeStep =
      std::find(this->Impl->TimeStepsToProcess.begin(), this->Impl->TimeStepsToProcess.end(),
        currentTime) != this->Impl->TimeStepsToProcess.end();
    if (!processTimeStep)
    {
      ++this->Impl->CurrentTimeStepIndex;
      if (static_cast<size_t>(this->Impl->CurrentTimeStepIndex) < this->Impl->TimeSteps.size())
      {
        request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
      }
      else
      {
        this->Impl->Initialize();
        request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
      }
      return 1;
    }
  }

  bool ret = this->WriteDataAndReturn();
  vtkLog(TRACE, "wrote data for time step " << this->Impl->CurrentTimeStepIndex);

  ++this->Impl->CurrentTimeStepIndex;
  if (static_cast<size_t>(this->Impl->CurrentTimeStepIndex) < this->Impl->TimeSteps.size())
  {
    request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
  }
  else
  {
    this->Impl->Initialize();
    request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
  }
  return ret ? 1 : 0;
}

//------------------------------------------------------------------------------
namespace
{
// Whether a collection item name is acceptable as-is for the Fides collection
// schema (it becomes a vtkDataAssembly node name on read-back). This mirrors
// Fides' own parse-time validation: it must be a valid vtkDataAssembly node
// name, and additionally must not start with "da" while being longer than two
// characters -- Fides reserves "da*" (e.g. "data", "dataset") for its internal
// <dataset> tags, a rule stricter than vtkDataAssembly::IsNodeNameReserved.
bool IsAcceptableItemName(const char* name)
{
  if (!name || !vtkDataAssembly::IsNodeNameValid(name))
  {
    return false;
  }
  const std::string s(name);
  return !(s.size() > 2 && s[0] == 'd' && s[1] == 'a');
}

// Collect the names of the selection-enabled arrays for a partitioned dataset.
// Only called when ChooseFieldsToWrite is on (when off, the writer leaves the
// Fides default write-all behavior in place). The vtkDataObject-typed accessor
// is used so vtkCellGrid partitions (which aren't vtkDataSet) are visible;
// GetPartition() returns null for those.
void CollectFieldsToWrite(
  vtkPartitionedDataSet* pds, vtkFidesWriter* self, std::vector<std::string>& fieldsToWrite)
{
  if (!pds || pds->GetNumberOfPartitions() == 0)
  {
    return;
  }
  auto partition = pds->GetPartitionAsDataObject(0);
  if (!partition)
  {
    return;
  }
  for (int association = 0; association < 3; ++association)
  {
    auto fd = partition->GetAttributesAsFieldData(association);
    auto selection = self->GetArraySelection(association);
    if (!fd || !selection)
    {
      continue;
    }
    for (int idx = 0; idx < fd->GetNumberOfArrays(); ++idx)
    {
      auto inarray = fd->GetAbstractArray(idx);
      if (inarray && inarray->GetName() && selection->ArrayIsEnabled(inarray->GetName()))
      {
        fieldsToWrite.emplace_back(inarray->GetName());
      }
    }
  }
}
}

//------------------------------------------------------------------------------
bool vtkFidesWriter::WriteDataAndReturn()
{
  vtkLogScopeFunction(TRACE);

  vtkSmartPointer<vtkDataObject> inputDO = this->GetInput();
  // Bare vtkDataSet or vtkCellGrid: wrap as a single-partition PDS so the
  // common PDS/PDSC path below handles both uniformly. vtkCellGrid isn't a
  // vtkDataSet (it derives directly from vtkDataObject) and so doesn't match
  // the vtkDataSet downcast.
  if (vtkDataSet::SafeDownCast(inputDO) || vtkCellGrid::SafeDownCast(inputDO))
  {
    vtkNew<vtkPartitionedDataSet> pd;
    pd->SetPartition(0, inputDO);
    inputDO = pd;
  }

  if (auto pd = vtkPartitionedDataSet::SafeDownCast(inputDO))
  {
    vtkNew<vtkPartitionedDataSetCollection> pdc;
    pdc->SetPartitionedDataSet(0, pd);
    inputDO = pdc;
  }

  if (auto mb = vtkMultiBlockDataSet::SafeDownCast(inputDO))
  {
    // convert MB to PDC, preserving the hierarchy as a vtkDataAssembly so it
    // can be serialized into the Fides collection schema below.
    vtkNew<vtkDataAssembly> hierarchy;
    vtkNew<vtkPartitionedDataSetCollection> pdc;
    if (!vtkDataAssemblyUtilities::GenerateHierarchy(mb, hierarchy, pdc))
    {
      vtkErrorMacro("Failed to generate hierarchy for input!");
      return false;
    }
    pdc->SetDataAssembly(hierarchy);
    inputDO = pdc;
  }

  auto inputPDC = vtkPartitionedDataSetCollection::SafeDownCast(inputDO);
  if (!inputPDC)
  {
    vtkErrorMacro("Invalid input data object!");
    return false;
  }

  // Read DATA_TIME_STEP from the original input object the upstream producer
  // handed us, not from the per-PDS slices we walk below -- upstream stamps the
  // time once on the top-level data object's info, and that's the only place
  // the executive's pipeline machinery propagates it to. Must be set before the
  // first Write so Fides' schema generator emits the step variable.
  bool haveCurrentTime = false;
  double currentTime = 0.0;
  if (auto* originalInfo = this->GetInput() ? this->GetInput()->GetInformation() : nullptr)
  {
    if (originalInfo->Has(vtkDataObject::DATA_TIME_STEP()))
    {
      currentTime = originalInfo->Get(vtkDataObject::DATA_TIME_STEP());
      haveCurrentTime = true;
    }
  }

  const unsigned int numPDS = inputPDC->GetNumberOfPartitionedDataSets();

  // Detect cellgrid partitions anywhere in the collection. The Fides VTK
  // backend's WriteCollection cannot carry cellgrid items, so collections that
  // contain them fall back to the per-PDS path below.
  bool hasCellGrid = false;
  for (unsigned int i = 0; i < numPDS && !hasCellGrid; ++i)
  {
    auto pds = inputPDC->GetPartitionedDataSet(i);
    for (unsigned int j = 0; pds && j < pds->GetNumberOfPartitions(); ++j)
    {
      if (vtkCellGrid::SafeDownCast(pds->GetPartitionAsDataObject(j)))
      {
        hasCellGrid = true;
        break;
      }
    }
  }

  const std::string mode = this->Engine == EngineTypes::BPFile ? "BPFile" : "SST";

  // Create (on first step) or look up the FidesDataSetWriter for a given output
  // file, run the per-step protocol (BeginStep -> SetCurrentTime -> write ->
  // EndStep), and report success. SetWriteFields / SetAdiosConfigFile must
  // happen before the first BeginStep, so they only run when the writer is
  // first created. doWrite performs the actual Write/WriteCollection call.
  auto writeTarget = [&](const std::string& fname, const std::vector<std::string>& fieldsToWrite,
                       auto&& doWrite) -> bool
  {
    const bool firstStep = this->Impl->Writers.count(fname) == 0;
    if (firstStep)
    {
#ifdef IOFIDES_HAVE_MPI
      if (this->Controller)
      {
        vtkMPICommunicator* vtkComm =
          vtkMPICommunicator::SafeDownCast(this->Controller->GetCommunicator());
        if (!vtkComm || !vtkComm->GetMPIComm())
        {
          vtkErrorMacro("Controller has no MPI communicator");
          return false;
        }
        MPI_Comm comm = *(vtkComm->GetMPIComm()->GetHandle());
        this->Impl->Writers.emplace(
          fname, std::make_unique<fides::io::FidesDataSetWriter>(fname, comm, mode));
      }
      else
      {
        this->Impl->Writers.emplace(
          fname, std::make_unique<fides::io::FidesDataSetWriter>(fname, mode));
      }
#else
      this->Impl->Writers.emplace(
        fname, std::make_unique<fides::io::FidesDataSetWriter>(fname, mode));
#endif
    }
    auto it = this->Impl->Writers.find(fname);
    if (it == this->Impl->Writers.end())
    {
      return true;
    }
    auto& writer = *it->second;

    if (firstStep)
    {
      if (this->AdiosConfigFile && this->AdiosConfigFile[0])
      {
        writer.SetAdiosConfigFile(this->AdiosConfigFile);
      }
      // When not explicitly choosing fields, leave the writer's default
      // write-all behavior in place rather than constraining it to a list.
      if (this->ChooseFieldsToWrite)
      {
        writer.SetWriteFields(fieldsToWrite);
      }
    }

    try
    {
      writer.BeginStep();
      if (haveCurrentTime)
      {
        writer.SetCurrentTime(currentTime);
      }
      doWrite(writer);
      writer.EndStep();
    }
    catch (const std::exception& e)
    {
      vtkErrorMacro("Exception encountered when trying to write data: " << e.what());
      return false;
    }
    return true;
  };

  // True collection output: more than one partitioned dataset and no cellgrid
  // items (which WriteCollection can't carry). The whole collection -- the
  // partitions plus the data assembly -- is serialized into a single Fides
  // dataset. A single partitioned dataset is not a collection and takes the
  // simpler per-dataset path below.
  if (!hasCellGrid && numPDS > 1)
  {
    // Collection item names become vtkDataAssembly node names, which have strict
    // validity rules (and "da*" is reserved). Fides also names unnamed items
    // "dataset_<i>", which is itself reserved. Replace any missing, invalid, or
    // reserved name with a synthesized "block<i>" up front, on a shallow copy so
    // the pipeline input is left untouched.
    vtkNew<vtkPartitionedDataSetCollection> namedPDC;
    namedPDC->ShallowCopy(inputPDC);
    std::vector<std::string> fieldsToWrite;
    for (unsigned int pdsIdx = 0; pdsIdx < numPDS; ++pdsIdx)
    {
      const bool hasName = namedPDC->HasMetaData(pdsIdx) &&
        namedPDC->GetMetaData(pdsIdx)->Has(vtkCompositeDataSet::NAME());
      const char* name =
        hasName ? namedPDC->GetMetaData(pdsIdx)->Get(vtkCompositeDataSet::NAME()) : nullptr;
      if (!IsAcceptableItemName(name))
      {
        namedPDC->GetMetaData(pdsIdx)->Set(
          vtkCompositeDataSet::NAME(), ("block" + vtk::to_string(pdsIdx)).c_str());
      }
      if (this->ChooseFieldsToWrite)
      {
        CollectFieldsToWrite(namedPDC->GetPartitionedDataSet(pdsIdx), this, fieldsToWrite);
      }
    }
    return writeTarget(this->FileName, fieldsToWrite,
      [&](fides::io::FidesDataSetWriter& writer) { writer.WriteCollection(namedPDC); });
  }

  // Otherwise write each partitioned dataset to its own file. The common
  // single-PDS case (a plain mesh or a cellgrid) writes to FileName directly;
  // multi-PDS cellgrid collections get a -p<idx> suffix per dataset.
  std::vector<std::string> pathComponents;
  vtksys::SystemTools::SplitPath(this->FileName, pathComponents);
  std::string fileExt = vtksys::SystemTools::GetFilenameLastExtension(this->FileName);
  std::string fileBase = vtksys::SystemTools::GetFilenameWithoutLastExtension(this->FileName);

  bool ok = true;
  for (unsigned int pdsIdx = 0; pdsIdx < numPDS; ++pdsIdx)
  {
    vtkLogScopeF(TRACE, "pdsIdx %u", pdsIdx);
    auto inputPDS = inputPDC->GetPartitionedDataSet(pdsIdx);
    assert(inputPDS != nullptr);

    std::vector<std::string> fieldsToWrite;
    if (this->ChooseFieldsToWrite)
    {
      CollectFieldsToWrite(inputPDS, this, fieldsToWrite);
    }

    std::string fname = this->FileName;
    if (numPDS > 1)
    {
      pathComponents[pathComponents.size() - 1] =
        fileBase + "-p" + vtk::to_string(pdsIdx) + fileExt;
      fname = vtksys::SystemTools::JoinPath(pathComponents);
    }
    vtkLog(TRACE, "fname " << fname);

    ok = writeTarget(fname, fieldsToWrite,
           [&](fides::io::FidesDataSetWriter& writer) { writer.Write(inputPDS); }) &&
      ok;
  }
  return ok;
}

//------------------------------------------------------------------------------
void vtkFidesWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Filename: " << this->FileName << endl;
  os << indent << "Controller: " << this->Controller << endl;
  os << indent << "TimeStepRange: " << this->TimeStepRange[0] << ", " << this->TimeStepRange[1]
     << endl;
  os << indent << "TimeStepStride: " << this->TimeStepStride << endl;
  os << indent << "Engine: " << this->Engine << endl;
}

VTK_ABI_NAMESPACE_END
