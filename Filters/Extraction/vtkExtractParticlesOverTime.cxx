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
    vtkDataSet* particleDataSet, vtkDataSet* volumeDataSet, char* IdChannelArray,
    vtkInformation* inputInformation, vtkDataSet* outputParticleDataSet);

  void SetNumberOfTimeSteps(int numberOfTimeSteps);
  double GetProgress() const;

private:
  bool ShouldRestart(vtkMTimeType modifiedTime) const;

  const vtkIdTypeArray* GetIds(vtkPointData* particlePointData, char* IdChannelArray) const;
  bool GenerateOutput(vtkDataSet* inputDataSet, char* IdChannelArray);
  bool FillExtractedPointSelection(char* IdChannelArray, vtkSelection* selection) const;

  vtkMTimeType LastModificationTime = 0;
  int NumberOfTimeSteps = 0;
  int CurrentTimeIndex = 0;
  std::set<vtkIdType> ExtractedPoints;
  double TimeStepBeforeExecuting = 0;
  vtkSmartPointer<vtkDataSet> CachedOutputDataSet;

  enum State
  {
    Not_Processed,
    Processing,
    Processing_Done
  };
  State CurrentState = Not_Processed;
};
vtkStandardNewMacro(vtkExtractParticlesOverTimeInternals);

//------------------------------------------------------------------------------
bool vtkExtractParticlesOverTimeInternals::ProcessRequestUpdateExtent(
  vtkInformation* inputInformation)
{
  if (this->CurrentState != Processing)
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
  char* IdChannelArray, vtkInformation* inputInformation, vtkDataSet* outputParticleDataSet)
{
  if (this->NumberOfTimeSteps <= 0)
  {
    vtkErrorMacro("No time steps in input data!");
    return false;
  }

  if (ShouldRestart(modifiedTime))
  {
    this->LastModificationTime = modifiedTime;
    this->CurrentTimeIndex = 0;
    request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
    this->ExtractedPoints.clear();
    this->CurrentState = Processing;
  }

  if (this->CurrentState == Processing)
  {
    vtkPointData* particlePointData = particleDataSet->GetPointData();
    if (!particlePointData)
    {
      vtkLog(ERROR, "Invalid Point Data in particle input.");
      this->CurrentState = Not_Processed;
      return false;
    }

    const vtkIdTypeArray* Ids = GetIds(particlePointData, IdChannelArray);
    if (!Ids)
    {
      vtkLog(ERROR, "Invalid Ids array in particle input.");
      this->CurrentState = Not_Processed;
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
          this->ExtractedPoints.insert(pointId);
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
        this->CurrentState = Not_Processed;
        return false;
      }

      inputInformation->Set(
        vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), this->TimeStepBeforeExecuting);
      this->CurrentState = Processing_Done;
    }
  }

  if (this->CurrentState == Processing_Done)
  {
    outputParticleDataSet->DeepCopy(this->CachedOutputDataSet);
  }

  return true;
}

//------------------------------------------------------------------------------
void vtkExtractParticlesOverTimeInternals::SetNumberOfTimeSteps(int numberOfTimeSteps)
{
  this->NumberOfTimeSteps = numberOfTimeSteps;
}

//------------------------------------------------------------------------------
double vtkExtractParticlesOverTimeInternals::GetProgress() const
{
  switch (this->CurrentState)
  {
    case Not_Processed:
      return 0;
    case Processing:
      if (this->NumberOfTimeSteps <= 0 || this->CurrentTimeIndex < 0)
      {
        return 0;
      }
      else
      {
        return static_cast<double>(this->CurrentTimeIndex) / this->NumberOfTimeSteps;
      }
    case Processing_Done:
      return 1;
    default:
      return 0;
  }
}

//------------------------------------------------------------------------------
bool vtkExtractParticlesOverTimeInternals::ShouldRestart(vtkMTimeType modifiedTime) const
{
  return this->CurrentState == Not_Processed || this->LastModificationTime < modifiedTime;
}

//------------------------------------------------------------------------------
bool vtkExtractParticlesOverTimeInternals::GenerateOutput(
  vtkDataSet* inputDataSet, char* IdChannelArray)
{
  vtkNew<vtkSelection> particleSelection;
  if (!FillExtractedPointSelection(IdChannelArray, particleSelection))
  {
    vtkLog(ERROR, "Error while generating particle selection.");
    return false;
  }

  vtkNew<vtkExtractSelection> extractor;
  extractor->SetInputDataObject(0, inputDataSet);
  extractor->SetInputDataObject(1, particleSelection);
  extractor->Update();

  auto extractedDataSet = extractor->GetOutputDataObject(0);
  if (!extractedDataSet)
  {
    vtkLog(ERROR, "Wrong output in extract selection.");
    return false;
  }

  this->CachedOutputDataSet =
    vtkSmartPointer<vtkDataSet>(vtkDataSet::SafeDownCast(extractedDataSet->NewInstance()));
  this->CachedOutputDataSet->DeepCopy(extractedDataSet);

  return true;
}

//------------------------------------------------------------------------------
bool vtkExtractParticlesOverTimeInternals::FillExtractedPointSelection(
  char* IdChannelArray, vtkSelection* selection) const
{
  vtkNew<vtkSelectionNode> particleSelectionNode;
  particleSelectionNode->SetFieldType(vtkSelectionNode::POINT);
  if (IdChannelArray)
  {
    particleSelectionNode->SetContentType(vtkSelectionNode::VALUES);
  }
  else
  {
    particleSelectionNode->SetContentType(vtkSelectionNode::GLOBALIDS);
  }

  vtkNew<vtkIdTypeArray> array;
  if (IdChannelArray)
  {
    array->SetName(IdChannelArray);
  }
  else
  {
    array->SetName("Extracted Point Ids");
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
  vtkPointData* particlePointData, char* IdChannelArray) const
{
  vtkDataArray* dataArray = nullptr;
  if (IdChannelArray)
  {
    dataArray = particlePointData->GetArray(IdChannelArray);
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
vtkExtractParticlesOverTime::~vtkExtractParticlesOverTime()
{
  delete[] this->IdChannelArray;
  this->IdChannelArray = nullptr;
}
//------------------------------------------------------------------------------
int vtkExtractParticlesOverTime::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}
//------------------------------------------------------------------------------
int vtkExtractParticlesOverTime::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataSet");

  return 1;
}

//------------------------------------------------------------------------------
int vtkExtractParticlesOverTime::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    this->Internals->SetNumberOfTimeSteps(
      inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS()));
  }
  else
  {
    this->Internals->SetNumberOfTimeSteps(0);
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

  os << indent << "IdChannelArray: " << (this->IdChannelArray ? this->IdChannelArray : "None")
     << std::endl;
}
