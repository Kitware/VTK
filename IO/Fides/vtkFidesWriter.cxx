// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkFidesWriter.h"

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
#include "vtkmlib/DataSetConverters.h"
#include "vtksys/SystemTools.hxx"

#ifdef IOFIDES_HAVE_MPI
#include "vtkMPI.h"
#include "vtkMPIController.h"
#endif

#include <fides/DataSetWriter.h>

#include <viskores/cont/DataSet.h>
#include <viskores/cont/PartitionedDataSet.h>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkFidesWriter);
vtkCxxSetObjectMacro(vtkFidesWriter, Controller, vtkMultiProcessController);

struct vtkFidesWriter::FidesWriterImpl
{
  std::vector<double> TimeSteps;
  std::vector<double> TimeStepsToProcess;
  int CurrentTimeStepIndex{ 0 };

  std::map<std::string, fides::io::DataSetAppendWriter> Writers;

  FidesWriterImpl() = default;
  ~FidesWriterImpl()
  {
    for (auto& writer : this->Writers)
    {
      writer.second.Close();
    }
  }

  void Initialize() { this->CurrentTimeStepIndex = 0; }
};

//------------------------------------------------------------------------------
vtkFidesWriter::vtkFidesWriter()
  : Impl(new FidesWriterImpl)
  , Controller(nullptr)
  , FileName(nullptr)
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

  this->WriteData();
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
  return 1;
}

//------------------------------------------------------------------------------
void vtkFidesWriter::WriteData()
{
  vtkLogScopeFunction(TRACE);

  vtkSmartPointer<vtkDataObject> inputDO = this->GetInput();
  if (vtkDataSet::SafeDownCast(inputDO))
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
    // convert MB to PDC.
    vtkNew<vtkDataAssembly> hierarchyUnused;
    vtkNew<vtkPartitionedDataSetCollection> pdc;
    if (!vtkDataAssemblyUtilities::GenerateHierarchy(mb, hierarchyUnused, pdc))
    {
      vtkErrorMacro("Failed to generate hierarchy for input!");
      return;
    }
    inputDO = pdc;
  }

  auto inputPDC = vtkPartitionedDataSetCollection::SafeDownCast(inputDO);
  if (!inputPDC)
  {
    vtkErrorMacro("Invalid input data object!");
    return;
  }

  std::vector<std::string> pathComponents;
  vtksys::SystemTools::SplitPath(this->FileName, pathComponents);
  std::string fileExt = vtksys::SystemTools::GetFilenameLastExtension(this->FileName);
  std::string fileBase = vtksys::SystemTools::GetFilenameWithoutLastExtension(this->FileName);

  for (unsigned int pdsIdx = 0; pdsIdx < inputPDC->GetNumberOfPartitionedDataSets(); ++pdsIdx)
  {
    vtkLogScopeF(TRACE, "pdsIdx %d", pdsIdx);
    // process PDS
    auto inputPDS = inputPDC->GetPartitionedDataSet(pdsIdx);
    assert(inputPDS != nullptr);

    viskores::cont::PartitionedDataSet vtkmPDS;
    std::vector<std::string> fieldsToWrite;
    for (unsigned int partIdx = 0; partIdx < inputPDS->GetNumberOfPartitions(); ++partIdx)
    {
      vtkLogScopeF(TRACE, "partIdx %d", partIdx);
      auto partition = inputPDS->GetPartition(partIdx);

      if (partIdx == 0)
      {
        // determine the fields that should be written
        // we're handling POINTS, CELLS and NONE
        for (int association = 0; association < 3; ++association)
        {
          auto fd = partition->GetAttributesAsFieldData(association);
          auto selection = this->GetArraySelection(association);

          if (!fd || !selection)
          {
            continue;
          }

          for (int idx = 0; idx < fd->GetNumberOfArrays(); ++idx)
          {
            auto inarray = fd->GetAbstractArray(idx);
            if (inarray && inarray->GetName() &&
              (!this->ChooseFieldsToWrite || selection->ArrayIsEnabled(inarray->GetName())))
            {
              fieldsToWrite.emplace_back(inarray->GetName());
            }
          }
        }
      }

      viskores::cont::DataSet ds = tovtkm::Convert(partition, tovtkm::FieldsFlag::PointsAndCells);
      vtkmPDS.AppendPartition(ds);
    }

    std::string fname = this->FileName;
    if (inputPDC->GetNumberOfPartitionedDataSets() > 1)
    {
      pathComponents[pathComponents.size() - 1] =
        fileBase + "-p" + std::to_string(pdsIdx) + fileExt;
      fname = vtksys::SystemTools::JoinPath(pathComponents);
    }
    vtkLog(TRACE, "fname " << fname);

    if (this->Impl->Writers.count(fname) == 0)
    {
#ifdef IOFIDES_HAVE_MPI
      if (this->Controller)
      {
        vtkMPICommunicator* vtkComm =
          vtkMPICommunicator::SafeDownCast(this->Controller->GetCommunicator());
        if (!vtkComm->GetMPIComm())
        {
          return;
        }
        MPI_Comm comm = *(vtkComm->GetMPIComm()->GetHandle());
        this->Impl->Writers.insert(std::pair<std::string, fides::io::DataSetAppendWriter>(
          fname, fides::io::DataSetAppendWriter(fname, comm)));
      }
      else
      {
        this->Impl->Writers.insert(std::pair<std::string, fides::io::DataSetAppendWriter>(
          fname, fides::io::DataSetAppendWriter(fname)));
      }
#else
      this->Impl->Writers.insert(std::pair<std::string, fides::io::DataSetAppendWriter>(
        fname, fides::io::DataSetAppendWriter(fname)));
#endif
    }
    auto it = this->Impl->Writers.find(fname);
    if (it == this->Impl->Writers.end())
    {
      continue;
    }
    auto& writer = it->second;
    writer.SetWriteFields(fieldsToWrite);

    std::string mode = "BPFile";
    if (this->Engine != EngineTypes::BPFile)
    {
      vtkErrorMacro("Unsupported engine type: " << this->Engine);
      return;
    }
    try
    {
      writer.Write(vtkmPDS, mode);
    }
    catch (const std::exception& e)
    {
      vtkErrorMacro("Exception encountered when trying to write data: " << e.what());
    }
  }
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
