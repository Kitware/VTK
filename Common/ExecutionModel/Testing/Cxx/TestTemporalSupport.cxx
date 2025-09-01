// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAlgorithm.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <cstdlib>

#define CHECK(b)                                                                                   \
  do                                                                                               \
  {                                                                                                \
    if (!(b))                                                                                      \
    {                                                                                              \
      cerr << "Error on Line " << __LINE__ << ":" << endl;                                         \
      return false;                                                                                \
    }                                                                                              \
  } while (false)

namespace
{
//------------------------------------------------------------------------------
class TestAlgorithm : public vtkAlgorithm
{
public:
  static TestAlgorithm* New();
  vtkTypeMacro(TestAlgorithm, vtkAlgorithm);

  vtkGetMacro(NumRequestInformation, int);
  vtkGetMacro(NumRequestData, int);
  vtkGetMacro(NumRequestUpdateExtent, int);
  vtkGetMacro(NumRequestUpdateTime, int);
  vtkGetMacro(NumRequestTimeDependentInformation, int);

  vtkTypeBool ProcessRequest(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override
  {
    if (request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
      this->NumRequestInformation++;
      return this->RequestInformation(request, inputVector, outputVector);
    }
    if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
    {
      this->NumRequestUpdateExtent++;
      return this->RequestUpdateExtent(request, inputVector, outputVector);
    }
    if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_DATA()))
    {
      this->NumRequestData++;
      return this->RequestData(request, inputVector, outputVector);
    }
    if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_TIME()))
    {
      this->NumRequestUpdateTime++;
      return this->RequestUpdateTime(request, inputVector, outputVector);
    }
    if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_TIME_DEPENDENT_INFORMATION()))
    {
      this->NumRequestTimeDependentInformation++;
      return this->RequestTimeDependentInformation(request, inputVector, outputVector);
    }
    return 1;
  }

protected:
  TestAlgorithm()
  {
    this->NumRequestInformation = 0;
    this->NumRequestData = 0;
    this->NumRequestUpdateExtent = 0;
    this->NumRequestUpdateTime = 0;
    this->NumRequestTimeDependentInformation = 0;
  }
  virtual int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*)
  {
    return 1;
  }
  virtual int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*)
  {
    return 1;
  }
  virtual int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*)
  {
    return 1;
  }
  virtual int RequestUpdateTime(vtkInformation*, vtkInformationVector**, vtkInformationVector*)
  {
    return 1;
  }
  virtual int RequestTimeDependentInformation(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*)
  {
    return 1;
  }

  int NumRequestInformation;
  int NumRequestData;
  int NumRequestUpdateExtent;
  int NumRequestUpdateTime;
  int NumRequestTimeDependentInformation;
};
vtkStandardNewMacro(TestAlgorithm);

//------------------------------------------------------------------------------
class TestTimeSource : public TestAlgorithm
{
public:
  static TestTimeSource* New();
  vtkTypeMacro(TestTimeSource, TestAlgorithm);
  vtkSetMacro(HasTimeDependentData, bool);

  TestTimeSource()
  {
    this->SetNumberOfInputPorts(0);
    this->SetNumberOfOutputPorts(1);
    for (int i = 0; i < 10; i++)
    {
      this->TimeSteps.push_back(i);
    }
    this->HasTimeDependentData = false;
  }

  int RequestData(
    vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector) override
  {
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    vtkImageData* outImage = vtkImageData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
    double timeStep = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

    outImage->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), timeStep);
    int scalarType = vtkImageData::GetScalarType(outInfo);
    int numComponents = vtkImageData::GetNumberOfScalarComponents(outInfo);
    outImage->AllocateScalars(scalarType, numComponents);
    return 1;
  }

  int RequestInformation(
    vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector) override
  {
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    double range[2] = { 0, 9 };
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), range, 2);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), TimeSteps.data(),
      static_cast<int>(TimeSteps.size()));
    if (this->HasTimeDependentData)
    {
      outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_DEPENDENT_INFORMATION(), 1);
    }
    return 1;
  }

  int FillOutputPortInformation(int, vtkInformation* info) override
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
    return 1;
  }

private:
  std::vector<double> TimeSteps;
  bool HasTimeDependentData;
  TestTimeSource(const TestTimeSource&) = delete;
  void operator=(const TestTimeSource&) = delete;
};
vtkStandardNewMacro(TestTimeSource);

//------------------------------------------------------------------------------
class TestTimeFilter : public TestAlgorithm
{
public:
  vtkTypeMacro(TestTimeFilter, TestAlgorithm);
  static TestTimeFilter* New();

  vtkSetMacro(StartTime, double);
  vtkSetMacro(TimeIterations, int);

  void PrintSelf(ostream&, vtkIndent) override {}

  int FillInputPortInformation(int, vtkInformation* info) override
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
    return 1;
  }

  int FillOutputPortInformation(int, vtkInformation* info) override
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
    return 1;
  }

  int RequestData(
    vtkInformation* request, vtkInformationVector** in, vtkInformationVector*) override
  {
    cout << "Has TD: "
         << in[0]->GetInformationObject(0)->Get(
              vtkStreamingDemandDrivenPipeline::TIME_DEPENDENT_INFORMATION())
         << endl;
    this->TimeIndex++;
    if (this->TimeIndex < this->TimeIterations)
    {
      request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
    }
    else
    {
      this->TimeIndex = 0;
      request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
    }
    return 1;
  }

  int RequestUpdateExtent(
    vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector*) override
  {
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    double timeStep = this->StartTime + (double)this->TimeIndex;
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), timeStep);
    return 1;
  }

private:
  TestTimeFilter()
  {
    this->SetNumberOfInputPorts(1);
    this->SetNumberOfOutputPorts(1);
    this->StartTime = 0;
    this->TimeIndex = 0;
    this->TimeIterations = 2;
  }
  double StartTime;
  int TimeIndex;
  int TimeIterations;
  TestTimeFilter(const TestTimeFilter&) = delete;
  void operator=(const TestTimeFilter&) = delete;
};
vtkStandardNewMacro(TestTimeFilter);

//------------------------------------------------------------------------------
bool TestTimeDependentInformationExecution()
{
  for (int i = 1; i < 2; i++)
  {
    bool hasTemporalMeta = i != 0;
    vtkNew<TestTimeSource> imageSource;
    imageSource->SetHasTimeDependentData(hasTemporalMeta);

    vtkNew<TestTimeFilter> filter;
    filter->SetTimeIterations(1);
    filter->SetInputConnection(imageSource->GetOutputPort());

    filter->SetStartTime(2.0);
    filter->Update();

    CHECK(imageSource->GetNumRequestData() == 1);
    CHECK(imageSource->GetNumRequestInformation() == 1);
    CHECK(imageSource->GetNumRequestUpdateExtent() == 1);
    if (hasTemporalMeta)
    {
      CHECK(imageSource->GetNumRequestTimeDependentInformation() == 1);
      CHECK(filter->GetNumRequestUpdateTime() == 1);
    }
    else
    {
      CHECK(imageSource->GetNumRequestTimeDependentInformation() == 0);
      CHECK(filter->GetNumRequestUpdateTime() == 0);
    }

    filter->SetStartTime(3.0);
    filter->Update(0);
    double dataTime =
      imageSource->GetOutputDataObject(0)->GetInformation()->Get(vtkDataObject::DATA_TIME_STEP());
    CHECK(dataTime == 3.0);
  }

  return true;
}

//------------------------------------------------------------------------------
int TestContinueExecution()
{
  vtkNew<TestTimeSource> imageSource;
  vtkNew<TestTimeFilter> filter;
  filter->SetInputConnection(imageSource->GetOutputPort());

  int numSteps = 3;
  for (int t = 0; t < numSteps; t++)
  {
    filter->SetStartTime(t);
    filter->Update();
  }
  CHECK(imageSource->GetNumRequestData() == numSteps + 1);
  return true;
}
}

//------------------------------------------------------------------------------
int TestTemporalSupport(int, char*[])
{
  if (!::TestTimeDependentInformationExecution())
  {
    cerr << "Errors in TestTimeDependentInformationExecution" << endl;
    return EXIT_FAILURE;
  }
  if (!::TestContinueExecution())
  {
    cerr << "Errors in TestContinueExecution" << endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
