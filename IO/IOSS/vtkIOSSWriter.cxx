// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkIOSSWriter.h"

#include "vtkCommand.h"
#include "vtkDataArraySelection.h"
#include "vtkDataAssemblyUtilities.h"
#include "vtkDataSet.h"
#include "vtkIOSSModel.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkStreamingDemandDrivenPipeline.h"
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
#include VTK_FMT(fmt/format.h)
// clang-format on

VTK_ABI_NAMESPACE_BEGIN
class vtkIOSSWriter::vtkInternals
{
  Ioss::Init::Initializer io;

public:
  std::unique_ptr<Ioss::Region> Region;
  std::vector<double> TimeSteps;
  std::vector<double> TimeStepsToProcess;
  int CurrentTimeStepIndex{ 0 };
  int RestartIndex{ 0 };

  std::string LastMD5;
  bool LastGlobalIdsCreated = false;
  bool LastGlobalIdsModified = false;
  bool LastElementSideCouldNotBeCreated = false;
  bool LastElementSideCouldNotBeModified = false;
  bool LastElementSideModified = false;

  vtkInternals() = default;
  ~vtkInternals() = default;

  void Initialize()
  {
    this->CurrentTimeStepIndex = 0;
    this->LastGlobalIdsCreated = false;
    this->LastGlobalIdsModified = false;
    this->LastElementSideCouldNotBeCreated = false;
    this->LastElementSideCouldNotBeModified = false;
    this->LastElementSideModified = false;
  }
};

vtkStandardNewMacro(vtkIOSSWriter);
vtkCxxSetObjectMacro(vtkIOSSWriter, Controller, vtkMultiProcessController);
//----------------------------------------------------------------------------
vtkIOSSWriter::vtkIOSSWriter()
  : Internals(new vtkIOSSWriter::vtkInternals)
  , Controller(nullptr)
  , FileName(nullptr)
  , AssemblyName(nullptr)
  , ChooseFieldsToWrite(false)
  , RemoveGhosts(true)
  , OffsetGlobalIds(false)
  , PreserveOriginalIds(false)
  , WriteQAAndInformationRecords(true)
  , DisplacementMagnitude(1.0)
  , TimeStepRange{ 0, VTK_INT_MAX - 1 }
  , TimeStepStride(1)
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
  this->SetAssemblyName(vtkDataAssemblyUtilities::HierarchyName());
  for (int i = 0; i < EntityType::NUMBER_OF_ENTITY_TYPES; ++i)
  {
    this->FieldSelection[i]->AddObserver(vtkCommand::ModifiedEvent, this, &vtkIOSSWriter::Modified);
  }
}

//----------------------------------------------------------------------------
vtkIOSSWriter::~vtkIOSSWriter()
{
  this->SetController(nullptr);
  this->SetFileName(nullptr);
  this->SetAssemblyName(nullptr);
}

//----------------------------------------------------------------------------
vtkDataArraySelection* vtkIOSSWriter::GetFieldSelection(EntityType type)
{
  if (type < EntityType::NUMBER_OF_ENTITY_TYPES)
  {
    return this->FieldSelection[type];
  }
  else
  {
    vtkErrorMacro("Invalid entity type: " << type);
    return nullptr;
  }
}

//------------------------------------------------------------------------------
bool vtkIOSSWriter::AddSelector(vtkIOSSWriter::EntityType entityType, const char* selector)
{
  if (selector)
  {
    if (this->Selectors[entityType].insert(selector).second)
    {
      this->Modified();
      return true;
    }
  }
  return false;
}

//------------------------------------------------------------------------------
void vtkIOSSWriter::ClearSelectors(vtkIOSSWriter::EntityType entityType)
{
  if (!this->Selectors[entityType].empty())
  {
    this->Selectors[entityType].clear();
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkIOSSWriter::SetSelector(vtkIOSSWriter::EntityType entity, const char* selector)
{
  if (selector)
  {
    if (this->Selectors[entity].size() == 1 && *this->Selectors[entity].begin() == selector)
    {
      return;
    }
    this->Selectors[entity].clear();
    this->Selectors[entity].insert(selector);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
int vtkIOSSWriter::GetNumberOfSelectors(vtkIOSSWriter::EntityType entity) const
{
  if (entity < EntityType::NUMBER_OF_ENTITY_TYPES)
  {
    return static_cast<int>(this->Selectors[entity].size());
  }
  return 0;
}

//------------------------------------------------------------------------------
const char* vtkIOSSWriter::GetSelector(vtkIOSSWriter::EntityType entityType, int index) const
{
  if (index >= 0 && index < this->GetNumberOfSelectors(entityType))
  {
    auto& selectors = this->Selectors[entityType];
    auto it = selectors.begin();
    std::advance(it, index);
    return it->c_str();
  }
  return nullptr;
}

//------------------------------------------------------------------------------
std::set<std::string> vtkIOSSWriter::GetSelectors(vtkIOSSWriter::EntityType entityType) const
{
  if (entityType < EntityType::NUMBER_OF_ENTITY_TYPES)
  {
    return this->Selectors[entityType];
  }
  return std::set<std::string>();
}

//------------------------------------------------------------------------------
int vtkIOSSWriter::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSetCollection");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
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
  internals.Initialize();

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
  if (internals.CurrentTimeStepIndex >= 0 &&
    internals.CurrentTimeStepIndex < static_cast<int>(internals.TimeSteps.size()))
  {
    info->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(),
      internals.TimeSteps[internals.CurrentTimeStepIndex]);
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
    const auto& currentTime = internals.TimeSteps[internals.CurrentTimeStepIndex];
    // check if timestep should be processed or skipped
    const bool processTimeStep =
      std::find(internals.TimeStepsToProcess.begin(), internals.TimeStepsToProcess.end(),
        currentTime) != internals.TimeStepsToProcess.end();
    if (!processTimeStep)
    {
      ++internals.CurrentTimeStepIndex;
      if (static_cast<size_t>(internals.CurrentTimeStepIndex) < internals.TimeSteps.size())
      {
        request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
      }
      else
      {
        internals.Initialize();
        request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
      }
      return 1;
    }
  }

  this->WriteData();

  ++internals.CurrentTimeStepIndex;
  if (static_cast<size_t>(internals.CurrentTimeStepIndex) < internals.TimeSteps.size())
  {
    request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
  }
  else
  {
    internals.Initialize();
    request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkIOSSWriter::WriteData()
{
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

  auto inputPDC = vtkPartitionedDataSetCollection::SafeDownCast(inputDO);
  if (!inputPDC)
  {
    vtkErrorMacro("Incorrect input type!");
    return;
  }

  auto& internals = (*this->Internals);
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
  // checks for global ids and element_side warnings
  if (model.GlobalIdsCreated() && model.GlobalIdsCreated() != internals.LastGlobalIdsCreated)
  {
    internals.LastGlobalIdsCreated = model.GlobalIdsCreated();
    vtkWarningMacro("Point or Cell Global IDs were not present. They have been created "
                    "assuming uniqueness.");
  }
  else if (model.GlobalIdsModified() &&
    model.GlobalIdsModified() != internals.LastGlobalIdsModified)
  {
    internals.LastGlobalIdsModified = model.GlobalIdsModified();
    vtkWarningMacro(
      "Point or Cell Global IDs were invalid. They have been re-created assuming uniqueness.");
  }
  // checks for element_side
  if (model.ElementSideCouldNotBeCreated() &&
    model.ElementSideCouldNotBeCreated() != internals.LastElementSideCouldNotBeCreated)
  {
    internals.LastElementSideCouldNotBeCreated = model.ElementSideCouldNotBeCreated();
    vtkWarningMacro(
      "Sets' element_side was not present. Edge, Face Element, Side sets have been skipped.");
  }
  else if (model.ElementSideCouldNotBeModified() &&
    model.ElementSideCouldNotBeModified() != internals.LastElementSideCouldNotBeModified)
  {
    internals.LastElementSideCouldNotBeModified = model.ElementSideCouldNotBeModified();
    vtkWarningMacro(
      "Sets' element_side was invalid and could not be re-created either because the "
      "original Cell Global IDs were not present, or because there were sets that were pointing "
      "to block cells that were not present. Edge, Face, Element, Side sets have been skipped.");
  }
  else if (model.ElementSideModified() &&
    model.ElementSideModified() != internals.LastElementSideModified)
  {
    internals.LastElementSideModified = model.ElementSideModified();
    vtkWarningMacro(
      "Sets' element_side was invalid. It was re-created using the original Cell Global IDs.");
  }

  if (internals.CurrentTimeStepIndex == this->TimeStepRange[0] || structureChanged)
  {
    internals.RestartIndex =
      internals.CurrentTimeStepIndex == this->TimeStepRange[0] ? 0 : (internals.RestartIndex + 1);

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
    // tell the writer to writer all blocks, even if empty
    properties.add(Ioss::Property("RETAIN_EMPTY_BLOCKS", "on"));
    // Do not convert variable names to lower case. The default is on.
    // For ex: this resolves a misunderstanding b/w T (temperature) vs t (time)
    properties.add(Ioss::Property("LOWER_CASE_VARIABLE_NAMES", "off"));
    if (!this->GetWriteQAAndInformationRecords())
    {
      properties.add(Ioss::Property("OMIT_INFO_RECORDS", true));
      properties.add(Ioss::Property("OMIT_QA_RECORDS", true));
    }
    const auto fname = internals.RestartIndex > 0
      ? fmt::format("{}-s{:04}", this->FileName, internals.RestartIndex)
      : std::string(this->FileName);

#ifdef SEACAS_HAVE_MPI
    // As of now netcdf mpi support is not working for IOSSWriter,
    // because mpi calls are called inside the writer instead of the ioss library
    // so we are using comm_null(), instead of comm_world().
    // In the future, when comm_world() is used and SEACAS_HAVE_MPI is on
    // my_processor and processor_count properties should be removed for exodus.
    // For more info. see Ioex::DatabaseIO::DatabaseIO in the ioss library.
    auto parallelUtilsComm = Ioss::ParallelUtils::comm_null();
#else
    auto parallelUtilsComm = Ioss::ParallelUtils::comm_world();
#endif
    Ioss::DatabaseIO* dbase =
      Ioss::IOFactory::create("exodus", fname, Ioss::WRITE_RESTART, parallelUtilsComm, properties);
    if (dbase == nullptr || !dbase->ok(true))
    {
      vtkErrorMacro("Could not open database '" << fname << "'");
      return;
    }

    // note: region takes ownership of `dbase` pointer.
    internals.Region.reset(new Ioss::Region(dbase, "region_1"));
    // Ioss automatically adds the information records
    if (this->GetWriteQAAndInformationRecords())
    {
      internals.Region->property_add(Ioss::Property("code_name", "VTK"));
      internals.Region->property_add(Ioss::Property("code_version", vtkVersion::GetVTKVersion()));
    }

    model.DefineModel(*internals.Region);
    model.DefineTransient(*internals.Region);
    model.Model(*internals.Region);
    internals.LastMD5 = md5;
  }

  auto inputInfo = inputDO->GetInformation();
  const double currentTimeStep = inputInfo->Has(vtkDataObject::DATA_TIME_STEP())
    ? inputInfo->Get(vtkDataObject::DATA_TIME_STEP())
    : 0.0;

  model.Transient(*internals.Region, /*time=*/currentTimeStep);
}

//----------------------------------------------------------------------------
void vtkIOSSWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(nullptr)") << endl;
  os << indent << "AssemblyName: " << (this->AssemblyName ? this->AssemblyName : "(nullptr)")
     << endl;

  os << indent << "ChooseFieldsToWrite: " << (this->ChooseFieldsToWrite ? "On" : "Off") << endl;
  // Skip NodeBlock for selectors
  for (int i = EntityType::EDGEBLOCK; i < EntityType::NUMBER_OF_ENTITY_TYPES; ++i)
  {
    os << indent << vtkIOSSReader::GetDataAssemblyNodeNameForEntityType(i)
       << " selectors: " << endl;
    for (const auto& selector : this->Selectors[i])
    {
      os << indent << selector << "  ";
    }
    os << endl;
  }
  if (this->ChooseFieldsToWrite)
  {
    for (int i = EntityType::NODEBLOCK; i < EntityType::NUMBER_OF_ENTITY_TYPES; ++i)
    {
      os << indent << vtkIOSSReader::GetDataAssemblyNodeNameForEntityType(i)
         << " fields to write: " << endl;
      this->FieldSelection[i]->PrintSelf(os, indent.GetNextIndent());
      os << endl;
    }
  }
  os << indent << "RemoveGhosts: " << (this->RemoveGhosts ? "On" : "Off") << endl;
  os << indent << "Controller: " << this->Controller << endl;
  os << indent << "OffsetGlobalIds: " << OffsetGlobalIds << endl;
  os << indent << "PreserveOriginalIds: " << (this->PreserveOriginalIds ? "On" : "Off") << endl;
  os << indent
     << "WriteQAAndInformationRecords: " << (this->WriteQAAndInformationRecords ? "On" : "Off")
     << endl;
  os << indent << "DisplacementMagnitude: " << this->DisplacementMagnitude << endl;
  os << indent << "TimeStepRange: " << this->TimeStepRange[0] << ", " << this->TimeStepRange[1]
     << endl;
  os << indent << "TimeStepStride: " << this->TimeStepStride << endl;
}
VTK_ABI_NAMESPACE_END
