/*=========================================================================

  Project:   Visualization Toolkit
  Module:    vtkExtractParticlesOverTime.cxx

  Copyright (c) Kitware, Inc.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

=========================================================================*/
#include "vtkExtractParticlesOverTime.h"

#include "vtkDataSet.h"
#include "vtkExtractSelection.h"
#include "vtkGenericCell.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSelection.h"
#include "vtkStaticCellLocator.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <array>
#include <set>

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkExtractParticlesOverTime);

namespace
{
enum class State
{
  NOT_EXTRACTED,
  EXTRACTING,
  EXTRACTION_ENDED,
  EXTRACTED
};

enum class IdChannelArrayType
{
  VALID_ID_CHANNEL_ARRAY,
  GLOBAL_IDS,
  NO_ID_CHANNEL_ARRAY
};

//------------------------------------------------------------------------------
vtkDataArray* GetIds(vtkPointData* particlePointData, const std::string& IdChannelArray)
{
  vtkDataArray* dataArray = nullptr;
  if (!IdChannelArray.empty())
  {
    dataArray = particlePointData->GetArray(IdChannelArray.c_str());
  }

  if (!dataArray)
  {
    // Try loading the global ids.
    dataArray = particlePointData->GetGlobalIds();
  }

  return dataArray;
}

} // anonymous namespace

//------------------------------------------------------------------------------
class vtkExtractParticlesOverTimeInternals
{
public:
  double GetProgress() const;
  bool ShouldRestart(vtkMTimeType modifiedTime) const;
  bool GenerateOutput(vtkDataSet* inputDataSet, const std::string& IdChannelArray);

  int NumberOfTimeSteps = 0;
  vtkMTimeType LastModificationTime = 0;
  int CurrentTimeIndex = 0;
  std::set<vtkIdType> ExtractedPoints;
  double RequestedTimeStep = 0;
  vtkNew<vtkExtractSelection> SelectionExtractor;
  State CurrentState = State::NOT_EXTRACTED;
  IdChannelArrayType LastIdChannelArrayType = IdChannelArrayType::NO_ID_CHANNEL_ARRAY;
};

//------------------------------------------------------------------------------
double vtkExtractParticlesOverTimeInternals::GetProgress() const
{
  switch (this->CurrentState)
  {
    case State::NOT_EXTRACTED:
      return 0;
    case State::EXTRACTING:
      if (this->NumberOfTimeSteps <= 0 || this->CurrentTimeIndex < 0)
      {
        return 0;
      }
      else
      {
        return static_cast<double>(this->CurrentTimeIndex) / this->NumberOfTimeSteps;
      }
    case State::EXTRACTION_ENDED:
    case State::EXTRACTED:
      return 1;
    default:
      return 0;
  }
}

//------------------------------------------------------------------------------
bool vtkExtractParticlesOverTimeInternals::ShouldRestart(vtkMTimeType modifiedTime) const
{
  return this->CurrentState == State::NOT_EXTRACTED || this->LastModificationTime < modifiedTime;
}

//------------------------------------------------------------------------------
bool vtkExtractParticlesOverTimeInternals::GenerateOutput(
  vtkDataSet* inputDataSet, const std::string& IdChannelArray)
{
  vtkNew<vtkSelectionNode> particleSelectionNode;
  vtkSmartPointer<vtkDataArray> array;
  particleSelectionNode->SetFieldType(vtkSelectionNode::POINT);
  switch (this->LastIdChannelArrayType)
  {
    case IdChannelArrayType::GLOBAL_IDS:
      particleSelectionNode->SetContentType(vtkSelectionNode::GLOBALIDS);
      array.TakeReference(vtkIdTypeArray::New());
      array->SetName("Extracted Point Ids");
      break;
    case IdChannelArrayType::VALID_ID_CHANNEL_ARRAY:
      particleSelectionNode->SetContentType(vtkSelectionNode::VALUES);
      array.TakeReference(::GetIds(inputDataSet->GetPointData(), IdChannelArray)->NewInstance());
      array->SetName(IdChannelArray.c_str());
      break;
    case IdChannelArrayType::NO_ID_CHANNEL_ARRAY:
      particleSelectionNode->SetContentType(vtkSelectionNode::INDICES);
      array.TakeReference(vtkIdTypeArray::New());
      array->SetName("Extracted Point Ids");
      break;
  }

  array->SetNumberOfTuples(this->ExtractedPoints.size());
  vtkIdType pointIndex = 0;
  for (const auto& pointId : this->ExtractedPoints)
  {
    array->SetTuple1(pointIndex, pointId);
    ++pointIndex;
  }

  particleSelectionNode->SetSelectionList(array);

  vtkNew<vtkSelection> particleSelection;
  particleSelection->AddNode(particleSelectionNode);

  this->SelectionExtractor->SetInputDataObject(0, inputDataSet);
  this->SelectionExtractor->SetInputDataObject(1, particleSelection);
  this->SelectionExtractor->Update();

  return true;
}

//------------------------------------------------------------------------------
vtkExtractParticlesOverTime::vtkExtractParticlesOverTime()
{
  this->Internals = std::make_shared<vtkExtractParticlesOverTimeInternals>();
  this->SetNumberOfInputPorts(2);
}

//------------------------------------------------------------------------------
int vtkExtractParticlesOverTime::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    this->Internals->NumberOfTimeSteps =
      inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  }
  else
  {
    this->Internals->NumberOfTimeSteps = 0;
  }

  this->Internals->CurrentState = State::NOT_EXTRACTED;

  return 1;
}

//------------------------------------------------------------------------------
int vtkExtractParticlesOverTime::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkInformation* inputInformation = inputVector[0]->GetInformationObject(0);
  double* timeSteps = nullptr;

  switch (this->Internals->CurrentState)
  {
    case State::NOT_EXTRACTED:
    case State::EXTRACTED:
      // Save the requested time step.
      this->Internals->RequestedTimeStep = 0;
      if (inputInformation->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
      {
        this->Internals->RequestedTimeStep =
          inputInformation->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
      }
      break;

    case State::EXTRACTING:
      // Update time step to continue executing.
      timeSteps = inputInformation->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
      if (timeSteps && this->Internals->CurrentTimeIndex >= 0)
      {
        double currentTimeStep = timeSteps[this->Internals->CurrentTimeIndex];
        inputInformation->Set(
          vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), currentTimeStep);
      }
      break;

    case State::EXTRACTION_ENDED:
      // Restore requested time step for final extraction.
      inputInformation->Set(
        vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), this->Internals->RequestedTimeStep);
      this->Internals->CurrentState = State::EXTRACTED;
      break;
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkExtractParticlesOverTime::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{

  vtkInformation* inputInformation = inputVector[0]->GetInformationObject(0);
  vtkDataSet* particleDataSet =
    vtkDataSet::SafeDownCast(inputInformation->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation* volumeInformation = inputVector[1]->GetInformationObject(0);
  vtkDataSet* volumeDataSet =
    vtkDataSet::SafeDownCast(volumeInformation->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation* outputInformation = outputVector->GetInformationObject(0);
  vtkDataSet* outputDataSet =
    vtkDataSet::SafeDownCast(outputInformation->Get(vtkDataObject::DATA_OBJECT()));

  if (this->Internals->NumberOfTimeSteps <= 0)
  {
    vtkLog(ERROR, "No time steps in input data!");
    return 0;
  }

  if (this->Internals->ShouldRestart(this->GetMTime()))
  {
    this->Internals->LastModificationTime = this->GetMTime();
    this->Internals->CurrentTimeIndex = 0;
    request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
    this->Internals->ExtractedPoints.clear();
    this->Internals->CurrentState = State::EXTRACTING;
    return 1;
  }

  if (this->Internals->CurrentState == State::EXTRACTING)
  {
    auto* particlePointData = particleDataSet->GetPointData();

    vtkDataArray* ids = nullptr;
    this->Internals->LastIdChannelArrayType = IdChannelArrayType::NO_ID_CHANNEL_ARRAY;
    if (!IdChannelArray.empty())
    {
      ids = particlePointData->GetArray(IdChannelArray.c_str());
    }

    if (ids)
    {
      this->Internals->LastIdChannelArrayType = IdChannelArrayType::VALID_ID_CHANNEL_ARRAY;
    }
    else
    {
      // Try loading the global ids.
      ids = particlePointData->GetGlobalIds();
      if (ids)
      {
        this->Internals->LastIdChannelArrayType = IdChannelArrayType::GLOBAL_IDS;
      }
    }

    vtkNew<vtkStaticCellLocator> locator;
    locator->SetDataSet(volumeDataSet);
    locator->AutomaticOn();
    locator->BuildLocator();

    vtkNew<vtkGenericCell> resultCell;
    std::array<double, 3> resultPointCoords = {};
    std::array<double, VTK_CELL_SIZE> resultWeights = {};
    double tolerance = 0;

    vtkIdType numberOfPoints = 0;
    if (ids)
    {
      numberOfPoints = ids->GetNumberOfTuples();
    }
    else
    {
      numberOfPoints = particleDataSet->GetNumberOfPoints();
    }

    for (vtkIdType index = 0; index < numberOfPoints; ++index)
    {
      vtkIdType pointId = index;
      if (ids)
      {
        pointId = static_cast<vtkIdType>(ids->GetTuple1(index));
      }

      if (this->Internals->ExtractedPoints.count(pointId) == 0)
      {
        double* pointCoordinates = particleDataSet->GetPoint(index);
        vtkIdType findResult = locator->FindCell(
          pointCoordinates, tolerance, resultCell, resultPointCoords.data(), resultWeights.data());
        if (findResult != -1)
        {
          this->Internals->ExtractedPoints.emplace(pointId);
        }
      }
    }

    this->Internals->CurrentTimeIndex++;

    if (this->Internals->CurrentTimeIndex == this->Internals->NumberOfTimeSteps)
    {
      this->Internals->CurrentTimeIndex = 0;
      this->Internals->CurrentState = State::EXTRACTION_ENDED;
    }
  }

  if (this->Internals->CurrentState == State::EXTRACTED)
  {
    if (!this->Internals->GenerateOutput(particleDataSet, IdChannelArray))
    {
      this->Internals->CurrentState = State::NOT_EXTRACTED;
      return 0;
    }

    outputDataSet->ShallowCopy(this->Internals->SelectionExtractor->GetOutputDataObject(0));

    if (request->Has(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING()))
    {
      request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
    }
  }

  this->UpdateProgress(this->Internals->GetProgress());

  return 1;
}

//------------------------------------------------------------------------------
void vtkExtractParticlesOverTime::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent
     << "IdChannelArray: " << (this->IdChannelArray.empty() ? "None" : this->IdChannelArray)
     << std::endl;
}
