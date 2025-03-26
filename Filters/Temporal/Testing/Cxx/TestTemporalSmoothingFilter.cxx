#include "vtkAlgorithm.h"
#include "vtkDataObject.h"
#include "vtkDataSetAlgorithm.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataAlgorithm.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTemporalSmoothing.h"

#include <cstdlib>

class MockTemporalPointSource : public vtkPolyDataAlgorithm
{
public:
  static MockTemporalPointSource* New();

  std::vector<double> TimeSteps = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

  MockTemporalPointSource()
  {
    this->SetNumberOfInputPorts(0);
    this->SetNumberOfOutputPorts(1);
  }

  int RequestInformation(vtkInformation* vtkNotUsed(request),
    vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outVector) override
  {
    vtkInformation* outInfo = outVector->GetInformationObject(0);

    double timeRange[2] = { 0, 9 };
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), this->TimeSteps.data(),
      static_cast<int>(this->TimeSteps.size()));
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);

    return 1;
  }

  int RequestData(vtkInformation* vtkNotUsed(request),
    vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector) override
  {
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    vtkPolyData* outData = vtkPolyData::GetData(outInfo);

    double requestedTimeStep = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

    vtkNew<vtkPoints> points;
    points->SetNumberOfPoints(3);
    points->SetPoint(0, requestedTimeStep, requestedTimeStep, requestedTimeStep);
    points->SetPoint(1, requestedTimeStep + 1, requestedTimeStep + 1, requestedTimeStep + 1);
    points->SetPoint(2, requestedTimeStep + 2, requestedTimeStep + 2, requestedTimeStep + 2);

    outData->SetPoints(points);

    vtkNew<vtkFloatArray> array;
    array->SetNumberOfValues(3);
    array->SetValue(0, requestedTimeStep);
    array->SetValue(1, requestedTimeStep + 1);
    array->SetValue(2, requestedTimeStep + 2);

    outData->GetPointData()->AddArray(array);

    return 1;
  }

  void SetNumTimeSteps(int numTimeSteps)
  {
    this->TimeSteps.clear();
    this->TimeSteps.reserve(numTimeSteps);
    for (int i = 0; i < numTimeSteps; i++)
    {
      this->TimeSteps.push_back(i);
    }
  }
};
vtkStandardNewMacro(MockTemporalPointSource);

int TestRequestOutOfBoundsTimeStep()
{
  vtkNew<MockTemporalPointSource> source;
  source->SetNumTimeSteps(30);

  vtkNew<vtkTemporalSmoothing> temporalSmoothing;
  temporalSmoothing->SetTemporalWindowHalfWidth(5);
  temporalSmoothing->SetInputConnection(source->GetOutputPort());

  float clampedValue;
  {
    temporalSmoothing->UpdateTimeStep(1);

    vtkPolyData* result = vtkPolyData::SafeDownCast(temporalSmoothing->GetOutput());
    vtkFloatArray* array = vtkFloatArray::SafeDownCast(result->GetPointData()->GetArray(0));

    clampedValue = array->GetValue(0);
  }

  float originalValue;
  {
    temporalSmoothing->UpdateTimeStep(5);

    vtkPolyData* result = vtkPolyData::SafeDownCast(temporalSmoothing->GetOutput());
    vtkFloatArray* array = vtkFloatArray::SafeDownCast(result->GetPointData()->GetArray(0));

    originalValue = array->GetValue(0);
  }

  // We expect the filter to clamp out-of-bounds timesteps requests to the first available one
  if (originalValue != clampedValue)
  {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int TestUniformSmoothing()
{
  // Half-width = 1
  {
    vtkNew<MockTemporalPointSource> source;

    vtkNew<vtkTemporalSmoothing> temporalSmoothing;
    temporalSmoothing->SetTemporalWindowHalfWidth(1);
    temporalSmoothing->SetInputConnection(source->GetOutputPort());

    temporalSmoothing->UpdateTimeStep(1);

    vtkPolyData* result = vtkPolyData::SafeDownCast(temporalSmoothing->GetOutput());
    vtkFloatArray* array = vtkFloatArray::SafeDownCast(result->GetPointData()->GetArray(0));

    float v0 = array->GetValue(0);
    float v1 = array->GetValue(1);
    float v2 = array->GetValue(2);

    if (v0 != 1 || v1 != 2 || v2 != 3)
    {
      return EXIT_FAILURE;
    }
  }

  // Half-width = 5
  {
    vtkNew<MockTemporalPointSource> source;
    source->SetNumTimeSteps(30);

    vtkNew<vtkTemporalSmoothing> temporalSmoothing;
    temporalSmoothing->SetTemporalWindowHalfWidth(5);
    temporalSmoothing->SetInputConnection(source->GetOutputPort());

    temporalSmoothing->UpdateTimeStep(15.);

    vtkPolyData* result = vtkPolyData::SafeDownCast(temporalSmoothing->GetOutput());
    vtkFloatArray* array = vtkFloatArray::SafeDownCast(result->GetPointData()->GetArray(0));

    float v0 = array->GetValue(0);
    float v1 = array->GetValue(1);
    float v2 = array->GetValue(2);

    if (v0 != 15 || v1 != 16 || v2 != 17)
    {
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}

int TestTemporalSmoothingFilter(int, char*[])
{
  if (TestUniformSmoothing())
  {
    vtkErrorWithObjectMacro(nullptr, "Test failed: \n Wrong smoothing filter output.");
    return EXIT_FAILURE;
  }

  if (TestRequestOutOfBoundsTimeStep())
  {
    vtkErrorWithObjectMacro(nullptr, "Test failed: \n Wrong behavior on out-of-bound time step.");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
