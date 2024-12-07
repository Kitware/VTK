// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkEnSightGoldCombinedReader.h"
#include "core/EnSightDataSet.h"

#include "vtkDataArraySelection.h"
#include "vtkDataAssembly.h"
#include "vtkDataObjectMeshCache.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPICommunicator.h"
#endif

#include <algorithm>
#include <string>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

namespace
{

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
template <typename T>
bool broadcastValues(T* data, vtkIdType numValues, vtkMultiProcessController* controller)
{
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
    return false;
  }

  if (!communicator->Broadcast(data, numValues, 0))
  {
    return false;
  }
  return true;
}
#else
template <typename T>
bool broadcastValues(T*, vtkIdType, vtkMultiProcessController*)
{
  return true;
}
#endif

bool broadcastSelection(vtkDataArraySelection* selection, vtkMultiProcessController* controller)
{
  if (!selection)
  {
    return false;
  }

  if (!controller || selection->GetNumberOfArrays() == 0)
  {
    return true;
  }

  std::vector<int> selectedArrays(selection->GetNumberOfArrays(), 0);
  for (int i = 0; i < selection->GetNumberOfArrays(); i++)
  {
    selectedArrays[i] = selection->GetArraySetting(i);
  }

  if (!broadcastValues(
        selectedArrays.data(), static_cast<vtkIdType>(selectedArrays.size()), controller))
  {
    return false;
  }

  // now update the selection for ranks that aren't rank 0
  if (controller->GetLocalProcessId() == 0)
  {
    return true;
  }

  for (int i = 0; i < selection->GetNumberOfArrays(); i++)
  {
    const char* name = selection->GetArrayName(i);
    selection->SetArraySetting(name, selectedArrays[i]);
  }
  return true;
}

} // end anon namespace

struct vtkEnSightGoldCombinedReader::ReaderImpl
{
  ensight_gold::EnSightDataSet Reader;
  vtkNew<vtkDataArraySelection> PartSelection;
  vtkNew<vtkDataArraySelection> PointArraySelection;
  vtkNew<vtkDataArraySelection> CellArraySelection;
  vtkNew<vtkDataArraySelection> FieldArraySelection;
  std::vector<double> TimeSteps;

  // PartNames contains all the parts found in this casefile during the
  // call to EnSightDataSet::GetPartInfo. This can be used by vtkEnSightSOSGoldReader
  // to determine the index of each loaded part in the output vtkPartitionedDataSetCollection
  vtkNew<vtkStringArray> PartNames;
};

vtkStandardNewMacro(vtkEnSightGoldCombinedReader);

vtkCxxSetObjectMacro(vtkEnSightGoldCombinedReader, Controller, vtkMultiProcessController);
vtkMultiProcessController* vtkEnSightGoldCombinedReader::GetController()
{
  return this->Controller;
}

//----------------------------------------------------------------------------
vtkEnSightGoldCombinedReader::vtkEnSightGoldCombinedReader()
{
  this->SetNumberOfInputPorts(0);
  this->CaseFileName = nullptr;
  this->FilePath = nullptr;
  this->TimeValue = 0.0;
  this->Impl = new ReaderImpl;
  this->Controller = nullptr;
  this->SetController(vtkMultiProcessController::GetGlobalController());
  this->PartOfSOSFile = false;
}

//----------------------------------------------------------------------------
vtkEnSightGoldCombinedReader::~vtkEnSightGoldCombinedReader()
{
  this->SetCaseFileName(nullptr);
  this->SetFilePath(nullptr);
  delete this->Impl;
  this->SetController(nullptr);
}

//------------------------------------------------------------------------------
int vtkEnSightGoldCombinedReader::CanReadFile(const char* casefilename)
{
  return (this->Impl->Reader.CheckVersion(casefilename) ? 1 : 0);
}

//------------------------------------------------------------------------------
int vtkEnSightGoldCombinedReader::RequestInformation(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkLogScopeFunction(TRACE);
  if (!this->CaseFileName)
  {
    vtkErrorMacro("CaseFileName is null");
    return 0;
  }

  std::string fullFileName;
  if (this->FilePath)
  {
    fullFileName = this->FilePath;
    fullFileName += "/";
  }
  fullFileName += this->CaseFileName;
  if (!this->Impl->Reader.ParseCaseFile(fullFileName.c_str()))
  {
    vtkErrorMacro("Case file " << this->CaseFileName << " could not be parsed without error");
    return 0;
  }
  if (this->Impl->Reader.UseStaticMeshCache())
  {
    this->Impl->Reader.GetMeshCache()->SetConsumer(this);
  }

  // the rigid body files need to be read here because it's possible that there's no time step
  // information in the rest of the files, so we'll need to use the info in the eet file to get
  // time values.
  if (this->Impl->Reader.HasRigidBodyFile())
  {
    if (!this->Impl->Reader.ReadRigidBodyGeometryFile())
    {
      vtkErrorMacro("Error reading rigid body file. Will attempt to continue reading EnSight "
                    "files, without applying rigid body transformations.");
    }
  }

  this->Impl->TimeSteps = this->Impl->Reader.GetTimeSteps();
  if (this->Impl->TimeSteps.empty() && this->Impl->Reader.UseRigidBodyTimeSteps())
  {
    // we'll fall back on using time step info from rigid body files
    this->Impl->TimeSteps = this->Impl->Reader.GetEulerTimeSteps();
    if (this->Impl->TimeSteps.empty())
    {
      vtkErrorMacro("UseEulerTimeSteps is true, but there are no time steps saved.");
      return 0;
    }
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);

  if (!this->Impl->TimeSteps.empty())
  {
    if (!this->AllTimeSteps)
    {
      this->AllTimeSteps = vtkSmartPointer<vtkDoubleArray>::New();
    }
    this->AllTimeSteps->SetArray(this->Impl->TimeSteps.data(), this->Impl->TimeSteps.size(), 1);

    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), this->Impl->TimeSteps.data(),
      static_cast<int>(this->Impl->TimeSteps.size()));

    double timeRange[2];
    timeRange[0] = this->Impl->TimeSteps[0];
    timeRange[1] = this->Impl->TimeSteps[this->Impl->TimeSteps.size() - 1];
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
  }

  this->Impl->Reader.SetPartOfSOSFile(this->PartOfSOSFile);

  this->Impl->Reader.GetPartInfo(this->Impl->PartSelection, this->Impl->PointArraySelection,
    this->Impl->CellArraySelection, this->Impl->FieldArraySelection, this->Impl->PartNames);

  // If we're not reading this as part of an SOS file, but we're running in parallel, we need to
  // coordinate the selections across ranks
  if (!this->PartOfSOSFile)
  {
    if (!broadcastSelection(this->Impl->PartSelection, this->Controller))
    {
      vtkErrorMacro("broadcasting part selections failed");
      return 0;
    }
    if (!broadcastSelection(this->Impl->PointArraySelection, this->Controller))
    {
      vtkErrorMacro("broadcasting point array selections failed");
      return 0;
    }
    if (!broadcastSelection(this->Impl->CellArraySelection, this->Controller))
    {
      vtkErrorMacro("broadcasting cell array selections failed");
      return 0;
    }
    if (!broadcastSelection(this->Impl->FieldArraySelection, this->Controller))
    {
      vtkErrorMacro("broadcasting field array selections failed");
      return 0;
    }
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkEnSightGoldCombinedReader::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkLogScopeFunction(TRACE);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  int piece = outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER())
    ? outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER())
    : 0;
  int npieces = outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES())
    ? outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES())
    : 1;
  vtkLog(TRACE, "piece " << piece << " of " << npieces << " pieces");

  vtkPartitionedDataSetCollection* output =
    vtkPartitionedDataSetCollection::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkNew<vtkDataAssembly> assembly;
  output->SetDataAssembly(assembly);

  int tsLength = outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  double* steps = outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

  double actualTimeValue = this->TimeValue;
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()) && tsLength > 0)
  {
    // Get the requested time step. We only support requests of a single time
    // step in this reader right now
    double requestedTimeStep = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

    // find the first time value larger than requested time value
    // this logic could be improved
    int cnt = 0;
    while (cnt < tsLength - 1 && steps[cnt] < requestedTimeStep)
    {
      cnt++;
    }
    actualTimeValue = steps[cnt];
  }
  output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), actualTimeValue);
  this->Impl->Reader.SetActualTimeValue(actualTimeValue);

  bool outputStructureOnly = false;
  if (piece > 0)
  {
    // This reader will eventually have a couple of decomposition strategies for running in
    // parallel. Currently there is one implemented, which is assigning a full casefile to a rank.
    // This means that if we're running in parallel when trying to load a casefile directly (instead
    // of an SOS file), we don't need to read on every rank, but the other ranks DO need to create
    // the same vtkPartitionedDataSetCollection structure.
    outputStructureOnly = true;
  }

  if (!this->Impl->Reader.ReadGeometry(output, this->Impl->PartSelection, outputStructureOnly))
  {
    vtkErrorMacro("Geometry file could not be read");
    return 0;
  }

  if (!this->Impl->Reader.ReadMeasuredGeometry(
        output, this->Impl->PartSelection, outputStructureOnly))
  {
    vtkErrorMacro("Measured geometry file could not be read");
    return 0;
  }

  if (outputStructureOnly)
  {
    // Reading variables is not necessary in this case
    return 1;
  }

  if (!this->Impl->Reader.ReadVariables(output, this->Impl->PartSelection,
        this->Impl->PointArraySelection, this->Impl->CellArraySelection,
        this->Impl->FieldArraySelection))
  {
    vtkErrorMacro("Variable file(s) could not be read");
    return 0;
  }

  return 1;
}

//------------------------------------------------------------------------------
vtkDataArraySelection* vtkEnSightGoldCombinedReader::GetPartSelection()
{
  return this->Impl->PartSelection;
}

//------------------------------------------------------------------------------
vtkDataArraySelection* vtkEnSightGoldCombinedReader::GetPointArraySelection()
{
  return this->Impl->PointArraySelection;
}

//------------------------------------------------------------------------------
vtkDataArraySelection* vtkEnSightGoldCombinedReader::GetCellArraySelection()
{
  return this->Impl->CellArraySelection;
}

//------------------------------------------------------------------------------
vtkDataArraySelection* vtkEnSightGoldCombinedReader::GetFieldArraySelection()
{
  return this->Impl->FieldArraySelection;
}

//------------------------------------------------------------------------------
vtkMTimeType vtkEnSightGoldCombinedReader::GetMTime()
{
  auto maxVal = std::max(this->Superclass::GetMTime(), this->Impl->PartSelection->GetMTime());
  maxVal = std::max(maxVal, this->Impl->PointArraySelection->GetMTime());
  maxVal = std::max(maxVal, this->Impl->CellArraySelection->GetMTime());
  return std::max(maxVal, this->Impl->FieldArraySelection->GetMTime());
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkStringArray> vtkEnSightGoldCombinedReader::GetPartNames()
{
  return this->Impl->PartNames;
}

//------------------------------------------------------------------------------
void vtkEnSightGoldCombinedReader::SetPDCInfoForLoadedParts(
  vtkSmartPointer<vtkIdTypeArray> indices, vtkSmartPointer<vtkStringArray> names)
{
  this->Impl->Reader.SetPDCInfoForLoadedParts(indices, names);
}

//------------------------------------------------------------------------------
void vtkEnSightGoldCombinedReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Case FileName: " << (this->CaseFileName ? this->CaseFileName : "(none)") << endl;
  os << indent << "File path: " << (this->FilePath ? this->FilePath : "(none)") << endl;
}
VTK_ABI_NAMESPACE_END
