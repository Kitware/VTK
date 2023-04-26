/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIOSSWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkIOSSWriter.h"

#include "vtkIOSSModel.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVersion.h"

// Ioss includes
#include <vtk_ioss.h>
// clang-format off
#include VTK_IOSS(Ionit_Initializer.h)
#include VTK_IOSS(Ioss_Assembly.h)
#include VTK_IOSS(Ioss_DatabaseIO.h)
#include VTK_IOSS(Ioss_EdgeBlock.h)
#include VTK_IOSS(Ioss_EdgeSet.h)
#include VTK_IOSS(Ioss_ElementBlock.h)
#include VTK_IOSS(Ioss_ElementSet.h)
#include VTK_IOSS(Ioss_FaceBlock.h)
#include VTK_IOSS(Ioss_FaceSet.h)
#include VTK_IOSS(Ioss_IOFactory.h)
#include VTK_IOSS(Ioss_NodeBlock.h)
#include VTK_IOSS(Ioss_NodeSet.h)
#include VTK_IOSS(Ioss_Region.h)
#include VTK_IOSS(Ioss_SideBlock.h)
#include VTK_IOSS(Ioss_SideSet.h)
#include VTK_IOSS(Ioss_StructuredBlock.h)
// clang-format on

#include <algorithm>
#include <string>
#include <vector>

// clang-format off
#include <vtk_fmt.h> // needed for `fmt`
#include VTK_FMT(fmt/core.h)
// clang-format on

VTK_ABI_NAMESPACE_BEGIN
class vtkIOSSWriter::vtkInternals
{
  Ioss::Init::Initializer io;

public:
  std::unique_ptr<Ioss::Region> Region;
  std::vector<double> TimeSteps;
  std::vector<double> TimeStepsToProcess;
  int CurrentTimeStep{ 0 };
  int RestartIndex{ 0 };
  std::string LastMD5;

  vtkInternals() = default;
  ~vtkInternals() = default;
};

vtkStandardNewMacro(vtkIOSSWriter);
vtkCxxSetObjectMacro(vtkIOSSWriter, Controller, vtkMultiProcessController);
//----------------------------------------------------------------------------
vtkIOSSWriter::vtkIOSSWriter()
  : Internals(new vtkIOSSWriter::vtkInternals)
  , Controller(nullptr)
  , FileName(nullptr)
  , OffsetGlobalIds(false)
  , PreserveInputEntityGroups(false)
  , DisplacementMagnitude(1.0)
  , TimeStepRange{ 0, VTK_INT_MAX - 1 }
  , TimeStepStride(1)
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkIOSSWriter::~vtkIOSSWriter()
{
  this->SetController(nullptr);
  this->SetFileName(nullptr);
}

//------------------------------------------------------------------------------
int vtkIOSSWriter::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSetCollection");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
  return 1;
}

//------------------------------------------------------------------------------
vtkTypeBool vtkIOSSWriter::ProcessRequest(
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
  // generate the data
  else if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    return this->RequestData(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkIOSSWriter::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  auto& internals = (*this->Internals);
  auto* inInfo = inputVector[0]->GetInformationObject(0);
  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    const auto numTimesteps = inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    const auto* timesteps = inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    internals.TimeSteps.resize(numTimesteps);
    std::copy(timesteps, timesteps + numTimesteps, internals.TimeSteps.begin());
    if (this->TimeStepRange[0] >= this->TimeStepRange[1] || this->TimeStepStride < 1)
    {
      internals.TimeStepsToProcess = internals.TimeSteps;
    }
    else
    {
      const auto begin = std::max(this->TimeStepRange[0], 0);
      const auto end = std::min(this->TimeStepRange[1] + 1, static_cast<int>(numTimesteps));
      const auto stride = this->TimeStepStride;
      internals.TimeStepsToProcess.clear();
      for (int i = begin; i < end; i += stride)
      {
        internals.TimeStepsToProcess.push_back(internals.TimeSteps[i]);
      }
    }
  }
  else
  {
    internals.TimeSteps.clear();
    internals.TimeStepsToProcess.clear();
  }
  internals.CurrentTimeStep = 0;

  return 1;
}

//----------------------------------------------------------------------------
int vtkIOSSWriter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  auto* info = inputVector[0]->GetInformationObject(0);
  if (auto* controller = this->GetController())
  {
    const int rank = controller->GetLocalProcessId();
    const int numRanks = controller->GetNumberOfProcesses();

    info->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), rank);
    info->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), numRanks);
  }

  auto& internals = (*this->Internals);
  if (internals.CurrentTimeStep >= 0 &&
    internals.CurrentTimeStep < static_cast<int>(internals.TimeSteps.size()))
  {
    info->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(),
      internals.TimeSteps[internals.CurrentTimeStep]);
  }
  else
  {
    info->Remove(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkIOSSWriter::RequestData(vtkInformation* request,
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* vtkNotUsed(outputVector))
{
  if (!this->FileName || !this->FileName[0])
  {
    vtkErrorMacro("Cannot write without a valid filename!");
    return 0;
  }
  auto& internals = (*this->Internals);
  if (!internals.TimeSteps.empty())
  {
    const auto& currentTime = internals.TimeSteps[internals.CurrentTimeStep];
    // check if timestep should be processed or skipped
    const bool processTimeStep =
      std::find(internals.TimeStepsToProcess.begin(), internals.TimeStepsToProcess.end(),
        currentTime) != internals.TimeStepsToProcess.end();
    if (!processTimeStep)
    {
      ++internals.CurrentTimeStep;
      if (static_cast<size_t>(internals.CurrentTimeStep) < internals.TimeSteps.size())
      {
        request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
      }
      else
      {
        internals.CurrentTimeStep = 0;
        request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
      }
      return 1;
    }
  }

  this->WriteData();

  ++internals.CurrentTimeStep;
  if (static_cast<size_t>(internals.CurrentTimeStep) < internals.TimeSteps.size())
  {
    request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
  }
  else
  {
    internals.CurrentTimeStep = 0;
    request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkIOSSWriter::WriteData()
{
  auto& internals = (*this->Internals);
  vtkSmartPointer<vtkDataObject> inputDO = this->GetInput();
  if (vtkUnstructuredGrid::SafeDownCast(inputDO))
  {
    vtkNew<vtkPartitionedDataSet> pd;
    pd->SetPartition(0, inputDO);
    inputDO = pd;
  }

  if (auto* pd = vtkPartitionedDataSet::SafeDownCast(inputDO))
  {
    vtkNew<vtkPartitionedDataSetCollection> pdc;
    pdc->SetPartitionedDataSet(0, pd);
    inputDO = pdc;
  }

  auto* inputPDC = vtkPartitionedDataSetCollection::SafeDownCast(inputDO);
  if (!inputPDC)
  {
    vtkErrorMacro("Incorrect input type!");
    return;
  }

  auto* controller = this->GetController();

  const vtkIOSSModel model(inputPDC, this);
  const auto md5 = model.MD5();
  vtkLogF(TRACE, "MD5: %s", md5.c_str());

  int structureChanged = internals.LastMD5 != md5;
  // ensure that all processes agree on whether the structure changed
  if (controller && controller->GetNumberOfProcesses() > 1)
  {
    int globalStructureChanged;
    controller->AllReduce(&structureChanged, &globalStructureChanged, 1, vtkCommunicator::MAX_OP);
    structureChanged = globalStructureChanged;
  }

  if (internals.CurrentTimeStep == this->TimeStepRange[0] || structureChanged)
  {
    internals.RestartIndex =
      internals.CurrentTimeStep == this->TimeStepRange[0] ? 0 : (internals.RestartIndex + 1);

    Ioss::PropertyManager properties;
    // if I use "8" here, then I get the following error:
    //     EXODUS: ERROR: 64-bit integer storage requested, but the netcdf library
    //     does not support the required netcdf-4 or 64BIT_DATA extensions.
    // So until we can track that down, just leaving this at 32-bit (4 byte).
    properties.add(Ioss::Property("INTEGER_SIZE_API", 4));
    properties.add(Ioss::Property("FIELD_SUFFIX_SEPARATOR", "_"));
    if (controller && controller->GetNumberOfProcesses() > 1)
    {
      properties.add(Ioss::Property("my_processor", controller->GetLocalProcessId()));
      properties.add(Ioss::Property("processor_count", controller->GetNumberOfProcesses()));
    }
    const auto fname = internals.RestartIndex > 0
      ? fmt::format("{}-s{:04}", this->FileName, internals.RestartIndex)
      : std::string(this->FileName);

    Ioss::DatabaseIO* dbase = Ioss::IOFactory::create(
      "exodus", fname, Ioss::WRITE_RESTART, Ioss::ParallelUtils::comm_world(), properties);
    if (dbase == nullptr || !dbase->ok(true))
    {
      vtkErrorMacro("Could not open database '" << fname << "'");
      return;
    }

    // note: region takes ownership of `dbase` pointer.
    internals.Region.reset(new Ioss::Region(dbase, "region_1"));
    internals.Region->property_add(Ioss::Property("code_name", std::string("VTK")));
    internals.Region->property_add(
      Ioss::Property("code_version", std::string(vtkVersion::GetVTKVersion())));

    model.DefineModel(*internals.Region);
    model.DefineTransient(*internals.Region);
    model.Model(*internals.Region);
    internals.LastMD5 = md5;
  }

  auto inputInfo = inputDO->GetInformation();
  const double time = inputInfo->Has(vtkDataObject::DATA_TIME_STEP())
    ? inputInfo->Get(vtkDataObject::DATA_TIME_STEP())
    : 0.0;

  model.Transient(*internals.Region, /*time=*/time);
}

//----------------------------------------------------------------------------
void vtkIOSSWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(nullptr)") << endl;
  os << indent << "Controller: " << this->Controller << endl;
  os << indent << "OffsetGlobalIds: " << OffsetGlobalIds << endl;
  os << indent << "PreserveInputEntityGroups: " << this->PreserveInputEntityGroups << endl;
  os << indent << "DisplacementMagnitude: " << this->DisplacementMagnitude << endl;
  os << indent << "TimeStepRange: " << this->TimeStepRange[0] << ", " << this->TimeStepRange[1]
     << endl;
  os << indent << "TimeStepStride: " << this->TimeStepStride << endl;
}
VTK_ABI_NAMESPACE_END
