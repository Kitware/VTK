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

//------------------------------------------------------------------------------
class vtkExtractParticlesOverTimeInternals : public vtkObject
{
public:
  static vtkExtractParticlesOverTimeInternals* New();
  vtkTypeMacro(vtkExtractParticlesOverTimeInternals, vtkObject);

  bool ProcessRequestUpdateExtent(vtkInformation* inputInformation);

  bool ProcessRequestData(vtkMTimeType modifiedTime, vtkInformation* request,
    vtkDataSet* particleDataSet, vtkDataSet* volumeDataSet, const std::string& IdChannelArray,
    vtkInformation* inputInformation, vtkDataSet* outputParticleDataSet);

  double GetProgress() const;
  int NumberOfTimeSteps = 0;

private:
  bool ShouldRestart(vtkMTimeType modifiedTime) const;

  static const vtkIdTypeArray* GetIds(
    vtkPointData* particlePointData, const std::string& IdChannelArray);
  bool GenerateOutput(vtkDataSet* inputDataSet, const std::string& IdChannelArray);
  bool FillExtractedPointSelection(
    const std::string& IdChannelArray, vtkSelection* selection) const;

  vtkMTimeType LastModificationTime = 0;
  int CurrentTimeIndex = 0;
  std::set<vtkIdType> ExtractedPoints;
  double TimeStepBeforeExecuting = 0;
  vtkNew<vtkExtractSelection> SelectionExtractor;

  enum State
  {
    NOT_PROCESSED,
    PROCESSING,
    PROCESSING_DONE
  };
  State CurrentState = NOT_PROCESSED;
};
vtkStandardNewMacro(vtkExtractParticlesOverTimeInternals);

//------------------------------------------------------------------------------
bool vtkExtractParticlesOverTimeInternals::ProcessRequestUpdateExtent(
  vtkInformation* inputInformation)
{
  if (this->CurrentState != PROCESSING)
  {
    this->TimeStepBeforeExecuting = 0;
    if (inputInformation->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
    {
      this->TimeStepBeforeExecuting =
        inputInformation->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    }
  }

  const double* timeSteps = inputInformation->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  if (timeSteps && this->CurrentTimeIndex >= 0)
  {
    double currentTimeStep = timeSteps[this->CurrentTimeIndex];
    inputInformation->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), currentTimeStep);
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkExtractParticlesOverTimeInternals::ProcessRequestData(vtkMTimeType modifiedTime,
  vtkInformation* request, vtkDataSet* particleDataSet, vtkDataSet* volumeDataSet,
  const std::string& IdChannelArray, vtkInformation* inputInformation,
  vtkDataSet* outputParticleDataSet)
{
  if (this->NumberOfTimeSteps <= 0)
  {
    vtkErrorMacro("No time steps in input data!");
    return false;
  }

  if (this->ShouldRestart(modifiedTime))
  {
    this->LastModificationTime = modifiedTime;
    this->CurrentTimeIndex = 0;
    request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
    this->ExtractedPoints.clear();
    this->CurrentState = PROCESSING;
  }

  if (this->CurrentState == PROCESSING)
  {
    vtkPointData* particlePointData = particleDataSet->GetPointData();
    if (!particlePointData)
    {
      vtkLog(ERROR, "Invalid Point Data in particle input.");
      this->CurrentState = NOT_PROCESSED;
      return false;
    }

    const vtkIdTypeArray* Ids = this->GetIds(particlePointData, IdChannelArray);
    if (!Ids)
    {
      vtkLog(ERROR, "Invalid Ids array in particle input: " << IdChannelArray);
      this->CurrentState = NOT_PROCESSED;
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
      if (this->ExtractedPoints.count(pointId) == 0)
      {
        double* pointCoordinates = particleDataSet->GetPoint(pointId);
        vtkIdType findResult = locator->FindCell(
          pointCoordinates, tolerance, resultCell, resultPointCoords.data(), resultWeights.data());
        if (findResult != -1)
        {
          this->ExtractedPoints.emplace(pointId);
        }
      }
    }

    this->CurrentTimeIndex++;

    if (this->CurrentTimeIndex == this->NumberOfTimeSteps)
    {
      request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
      this->CurrentTimeIndex = 0;

      if (!GenerateOutput(particleDataSet, IdChannelArray))
      {
        this->CurrentState = NOT_PROCESSED;
        return false;
      }

      inputInformation->Set(
        vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), this->TimeStepBeforeExecuting);
      this->CurrentState = PROCESSING_DONE;
    }
  }

  if (this->CurrentState == PROCESSING_DONE)
  {
    outputParticleDataSet->ShallowCopy(this->SelectionExtractor->GetOutputDataObject(0));
  }

  return true;
}

//------------------------------------------------------------------------------
double vtkExtractParticlesOverTimeInternals::GetProgress() const
{
  switch (this->CurrentState)
  {
    case NOT_PROCESSED:
      return 0;
    case PROCESSING:
      if (this->NumberOfTimeSteps <= 0 || this->CurrentTimeIndex < 0)
      {
        return 0;
      }
      else
      {
        return static_cast<double>(this->CurrentTimeIndex) / this->NumberOfTimeSteps;
      }
    case PROCESSING_DONE:
      return 1;
    default:
      return 0;
  }
}

//------------------------------------------------------------------------------
bool vtkExtractParticlesOverTimeInternals::ShouldRestart(vtkMTimeType modifiedTime) const
{
  return this->CurrentState == NOT_PROCESSED || this->LastModificationTime < modifiedTime;
}

//------------------------------------------------------------------------------
bool vtkExtractParticlesOverTimeInternals::GenerateOutput(
  vtkDataSet* inputDataSet, const std::string& IdChannelArray)
{
  vtkNew<vtkSelection> particleSelection;
  if (!FillExtractedPointSelection(IdChannelArray, particleSelection))
  {
    vtkLog(ERROR, "Error while generating particle selection.");
    return false;
  }

  this->SelectionExtractor->SetInputDataObject(0, inputDataSet);
  this->SelectionExtractor->SetInputDataObject(1, particleSelection);
  this->SelectionExtractor->Update();

  return true;
}

//------------------------------------------------------------------------------
bool vtkExtractParticlesOverTimeInternals::FillExtractedPointSelection(
  const std::string& IdChannelArray, vtkSelection* selection) const
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

  array->Allocate(static_cast<vtkIdType>(this->ExtractedPoints.size()));
  for (const auto& pointId : this->ExtractedPoints)
  {
    array->InsertNextValue(pointId);
  }
  particleSelectionNode->SetSelectionList(array);
  selection->AddNode(particleSelectionNode);

  return true;
}

//------------------------------------------------------------------------------
const vtkIdTypeArray* vtkExtractParticlesOverTimeInternals::GetIds(
  vtkPointData* particlePointData, const std::string& IdChannelArray)
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

//------------------------------------------------------------------------------
vtkExtractParticlesOverTime::vtkExtractParticlesOverTime()
{
  this->Internals = vtkSmartPointer<vtkExtractParticlesOverTimeInternals>::New();

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
  if (!this->Internals->ProcessRequestUpdateExtent(inputInformation))
  {
    vtkLog(ERROR, "Error while processing request update extent.");
    return 0;
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

  if (!this->Internals->ProcessRequestData(this->GetMTime(), request, particleDataSet,
        volumeDataSet, this->IdChannelArray, inputInformation, outputDataSet))
  {
    vtkLog(ERROR, "Error while processing extraction.");
    return 0;
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
