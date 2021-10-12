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
  NOT_PROCESSED,
  PROCESSING,
  PROCESSING_DONE
};

//------------------------------------------------------------------------------
const vtkIdTypeArray* GetIds(vtkPointData* particlePointData, const std::string& IdChannelArray)
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

  return vtkIdTypeArray::SafeDownCast(dataArray);
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
  double TimeStepBeforeExecuting = 0;
  vtkNew<vtkExtractSelection> SelectionExtractor;
  State CurrentState = State::NOT_PROCESSED;
};

//------------------------------------------------------------------------------
double vtkExtractParticlesOverTimeInternals::GetProgress() const
{
  switch (this->CurrentState)
  {
    case State::NOT_PROCESSED:
      return 0;
    case State::PROCESSING:
      if (this->NumberOfTimeSteps <= 0 || this->CurrentTimeIndex < 0)
      {
        return 0;
      }
      else
      {
        return static_cast<double>(this->CurrentTimeIndex) / this->NumberOfTimeSteps;
      }
    case State::PROCESSING_DONE:
      return 1;
    default:
      return 0;
  }
}

//------------------------------------------------------------------------------
bool vtkExtractParticlesOverTimeInternals::ShouldRestart(vtkMTimeType modifiedTime) const
{
  return this->CurrentState == State::NOT_PROCESSED || this->LastModificationTime < modifiedTime;
}

//------------------------------------------------------------------------------
bool vtkExtractParticlesOverTimeInternals::GenerateOutput(
  vtkDataSet* inputDataSet, const std::string& IdChannelArray)
{
  vtkNew<vtkSelectionNode> particleSelectionNode;
  vtkNew<vtkIdTypeArray> array;
  particleSelectionNode->SetFieldType(vtkSelectionNode::POINT);
  if (IdChannelArray.empty())
  {
    particleSelectionNode->SetContentType(vtkSelectionNode::GLOBALIDS);
    array->SetName("Extracted Point Ids");
  }
  else
  {
    particleSelectionNode->SetContentType(vtkSelectionNode::VALUES);
    array->SetName(IdChannelArray.c_str());
  }

  array->SetNumberOfComponents(1);
  array->SetNumberOfTuples(2);
  array->Resize(static_cast<vtkIdType>(this->ExtractedPoints.size()));
  vtkIdType pointIndex = 0;
  for (const auto& pointId : this->ExtractedPoints)
  {
    array->SetValue(pointIndex, pointId);
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
  this->SetNumberOfOutputPorts(1);
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

  return 1;
}

//------------------------------------------------------------------------------
int vtkExtractParticlesOverTime::RequestUpdateExtent(vtkInformation*,
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkInformation* inputInformation = inputVector[0]->GetInformationObject(0);

  if (this->Internals->CurrentState != State::PROCESSING)
  {
    this->Internals->TimeStepBeforeExecuting = 0;
    if (inputInformation->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
    {
      this->Internals->TimeStepBeforeExecuting =
        inputInformation->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    }
  }

  const double* timeSteps = inputInformation->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  if (timeSteps && this->Internals->CurrentTimeIndex >= 0)
  {
    double currentTimeStep = timeSteps[this->Internals->CurrentTimeIndex];
    inputInformation->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), currentTimeStep);
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
    return false;
  }

  if (this->Internals->ShouldRestart(this->GetMTime()))
  {
    this->Internals->LastModificationTime = this->GetMTime();
    this->Internals->CurrentTimeIndex = 0;
    request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
    this->Internals->ExtractedPoints.clear();
    this->Internals->CurrentState = State::PROCESSING;
  }

  if (this->Internals->CurrentState == State::PROCESSING)
  {
    vtkPointData* particlePointData = particleDataSet->GetPointData();
    const vtkIdTypeArray* Ids = ::GetIds(particlePointData, IdChannelArray);
    if (!Ids)
    {
      vtkLog(ERROR, "Invalid Ids array in particle input: " << IdChannelArray);
      this->Internals->CurrentState = State::NOT_PROCESSED;
      return false;
    }

    vtkNew<vtkStaticCellLocator> locator;
    locator->SetDataSet(volumeDataSet);
    locator->AutomaticOn();
    locator->BuildLocator();

    vtkNew<vtkGenericCell> resultCell;
    std::array<double, 3> resultPointCoords = {};
    std::array<double, VTK_CELL_SIZE> resultWeights = {};
    double tolerance = 0;

    vtkIdType numberOfPoints = Ids->GetNumberOfTuples();
    for (vtkIdType index = 0; index < numberOfPoints; ++index)
    {
      vtkIdType pointId = 0;
      Ids->GetTypedTuple(index, &pointId);
      if (this->Internals->ExtractedPoints.count(pointId) == 0)
      {
        double* pointCoordinates = particleDataSet->GetPoint(pointId);
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
      request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
      this->Internals->CurrentTimeIndex = 0;

      if (!this->Internals->GenerateOutput(particleDataSet, IdChannelArray))
      {
        this->Internals->CurrentState = State::NOT_PROCESSED;
        return false;
      }

      inputInformation->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(),
        this->Internals->TimeStepBeforeExecuting);
      this->Internals->CurrentState = State::PROCESSING_DONE;
    }
  }

  if (this->Internals->CurrentState == State::PROCESSING_DONE)
  {
    outputDataSet->ShallowCopy(this->Internals->SelectionExtractor->GetOutputDataObject(0));
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
