// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAlgorithm.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMultiTimeStepAlgorithm.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTemporalAlgorithm.h"

#include <algorithm>
#include <cstdlib>
#include <numeric>

#include <iostream>

namespace
{
/**
 * A base algorithm class to dispatch ProcessRequest into RequestXXX methods
 * as usual in VTK (and requried by vtkTemporalAlgorithm)
 */
//------------------------------------------------------------------------------
class TestAlgorithm : public vtkAlgorithm
{
public:
  static TestAlgorithm* New();
  vtkTypeMacro(TestAlgorithm, vtkAlgorithm);

  int NumRequestInformation = 0;
  int NumRequestData = 0;
  int NumRequestUpdateExtent = 0;

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
    return 1;
  }

protected:
  TestAlgorithm() = default;
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
  // unused but required by vtkTemporalAlgorithm
  virtual int RequestUpdateTime(vtkInformation*, vtkInformationVector**, vtkInformationVector*)
  {
    return 1;
  }
  // unused but required by vtkTemporalAlgorithm
  virtual int RequestTimeDependentInformation(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*)
  {
    return 1;
  }
};
vtkStandardNewMacro(TestAlgorithm);

//------------------------------------------------------------------------------
/**
 * Temporal source that creates a vtkImageData with a FieldData "Data"
 * containing only the current time value.
 * Available times are integers in [0, 9].
 */
class TestTimeSource : public TestAlgorithm
{
public:
  static TestTimeSource* New();
  vtkTypeMacro(TestTimeSource, TestAlgorithm);

  int RequestData(
    vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector) override
  {
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    vtkImageData* outImage = vtkImageData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
    double timeStep = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

    // add current time as FieldData
    vtkNew<vtkIntArray> currentTime;
    currentTime->SetName("Data");
    auto ts = std::lower_bound(this->TimeSteps.begin(), this->TimeSteps.end()--, timeStep);
    currentTime->InsertNextValue(*ts);
    outImage->GetFieldData()->AddArray(currentTime);

    // add current time to output metadata
    outImage->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), *ts);

    return 1;
  }

  int RequestInformation(
    vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector) override
  {
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    double range[2] = { this->TimeSteps.front(), this->TimeSteps.back() };
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), range, 2);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), this->TimeSteps.data(),
      static_cast<int>(this->TimeSteps.size()));
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_DEPENDENT_INFORMATION(), 1);
    return 1;
  }

  int FillOutputPortInformation(int, vtkInformation* info) override
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
    return 1;
  }

  TestTimeSource()
  {
    this->SetNumberOfInputPorts(0);
    this->SetNumberOfOutputPorts(1);
    this->TimeSteps.resize(10);
    std::iota(this->TimeSteps.begin(), this->TimeSteps.end(), 0);
  }

protected:
  std::vector<double> TimeSteps;

private:
  TestTimeSource(const TestTimeSource&) = delete;
  void operator=(const TestTimeSource&) = delete;
};
vtkStandardNewMacro(TestTimeSource);

/**
 * An In-situ variant of the TestTimeSource: only one timestep available,
 * and time value is increased at each update (starting at 4)
 */
//------------------------------------------------------------------------------
class TestInSituSource : public TestTimeSource
{
public:
  static TestInSituSource* New();
  vtkTypeMacro(TestInSituSource, TestTimeSource);

protected:
  // override to provide a single, incrementing, time step as it is in-situ.
  int RequestUpdateExtent(
    vtkInformation* request, vtkInformationVector** input, vtkInformationVector* output) override
  {
    this->TimeSteps[0] = this->TimeSteps[0] + 1;
    this->Superclass::RequestUpdateExtent(request, input, output);
    return 1;
  }

  TestInSituSource()
  {
    this->TimeSteps.clear();
    this->TimeSteps.push_back(4);
  }

private:
  TestInSituSource(const TestInSituSource&) = delete;
  void operator=(const TestInSituSource&) = delete;
};
vtkStandardNewMacro(TestInSituSource);

//------------------------------------------------------------------------------
/**
 * Temporal filter that adds a new field data containing cumulative sum of "Data"
 * from first available timestep until the requested one.
 * This naive version does everything "manually", specially CONTINUE_EXECUTING() management.
 */
class TestTimeFilter : public TestAlgorithm
{
public:
  vtkTypeMacro(TestTimeFilter, TestAlgorithm);
  static TestTimeFilter* New();

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

  /**
   * This will be called when downstream requests update: initialize internal informations.
   */
  int RequestInformation(
    vtkInformation*, vtkInformationVector** inVector, vtkInformationVector*) override
  {
    vtkInformation* inInfo = inVector[0]->GetInformationObject(0);
    if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
    {
      const int nbOfTimeSteps = inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
      const double* inTimes = inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
      for (int ts = 0; ts < nbOfTimeSteps; ts++)
      {
        this->InputTimes.push_back(inTimes[ts]);
      }
    }
    this->UpdateIteration = 0;

    return 1;
  }

  /**
   * This will be called in loop while CONTINUE_EXECUTING is on.
   * Update requested time accordingly.
   */
  int RequestUpdateExtent(
    vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector*) override
  {
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    double time = this->InputTimes[this->UpdateIteration];
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), time);
    return 1;
  }

  /**
   * This will be called in loop while CONTINUE_EXECUTING() is on.
   * While current timestep is not the last (see InputTimes)
   * - Set CONTINUE_EXECUTING() to ON
   * - Aggregate data
   * On last time step:
   * - Remove CONTINUE_EXECUTING()
   * - Generate output data.
   */
  int RequestData(vtkInformation* request, vtkInformationVector** inVector,
    vtkInformationVector* outVector) override
  {
    vtkInformation* outInfo = outVector->GetInformationObject(0);

    vtkDataObject* input = vtkDataObject::GetData(inVector[0], 0);
    const double inputTime = input->GetInformation()->Get(vtkDataObject::DATA_TIME_STEP());
    const double requestedTime = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

    auto inputEnd =
      std::lower_bound(this->InputTimes.begin(), this->InputTimes.end(), requestedTime);

    auto inputArray = input->GetFieldData()->GetArray("Data");
    this->Value += inputArray->GetTuple1(0);

    if (inputTime == *inputEnd)
    {
      request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());

      vtkDataObject* output = vtkDataObject::GetData(outVector, 0);
      output->ShallowCopy(input);
      vtkNew<vtkDoubleArray> cumulativeData;
      cumulativeData->SetName("CumulativeData");
      cumulativeData->InsertNextValue(this->Value);

      output->GetFieldData()->AddArray(cumulativeData);
    }
    else
    {
      this->UpdateIteration++;
      request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
    }

    return 1;
  }

private:
  TestTimeFilter()
  {
    this->SetNumberOfInputPorts(1);
    this->SetNumberOfOutputPorts(1);
  }
  std::vector<double> InputTimes;
  double Value = 0;
  int UpdateIteration = 0;
  TestTimeFilter(const TestTimeFilter&) = delete;
  void operator=(const TestTimeFilter&) = delete;
};
vtkStandardNewMacro(TestTimeFilter);

/**
 * Same process as TestTimeFilter: cumulative sum of "Data"
 * This version uses vtkMultiTimeSteps algorithm
 */
class TestMultiTsAlgo : public vtkMultiTimeStepAlgorithm
{
public:
  vtkTypeMacro(TestMultiTsAlgo, TestAlgorithm);
  static TestMultiTsAlgo* New();

  int NumRequestUpdateExtent = 0;
  int NumRequestInformation = 0;
  int NumExecute = 0;

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

  /**
   * This will be called when downstream requests update: initialize internal informations.
   */
  int RequestInformation(
    vtkInformation*, vtkInformationVector** inVector, vtkInformationVector*) override
  {
    this->NumRequestInformation++;
    vtkInformation* inInfo = inVector[0]->GetInformationObject(0);
    if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
    {
      const int nbOfTimeSteps = inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
      const double* inTimes = inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
      for (int ts = 0; ts < nbOfTimeSteps; ts++)
      {
        this->InputTimes.push_back(inTimes[ts]);
      }
    }

    return 1;
  }

  /**
   * This will be called in loop while CONTINUE_EXECUTING() is on.
   * Setup Superclass list of timesteps to iterate on.
   * Called only once per output update request.
   */
  int RequestUpdateExtent(
    vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector) override
  {
    this->NumRequestUpdateExtent++;
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    const double requestedTime = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    const auto max =
      std::upper_bound(this->InputTimes.begin(), this->InputTimes.end(), requestedTime);
    std::vector<double> times(this->InputTimes.begin(), max);
    this->SetTimeSteps(times);
    return 1;
  }

  /**
   * This will be called once with all cached input in the vector, to generate the final data.
   */
  int Execute(vtkInformation*, const std::vector<vtkSmartPointer<vtkDataObject>>& inputs,
    vtkInformationVector* outVector) override
  {
    this->NumExecute++;
    double value = 0;
    for (const auto& input : inputs)
    {
      auto inputArray = input->GetFieldData()->GetArray("Data");
      value += inputArray->GetTuple1(0);
    }

    vtkDataObject* output = vtkDataObject::GetData(outVector, 0);
    vtkNew<vtkDoubleArray> cumulativeData;
    cumulativeData->SetName("CumulativeData");
    cumulativeData->InsertNextValue(value);
    output->GetFieldData()->AddArray(cumulativeData);

    return 1;
  }

private:
  TestMultiTsAlgo()
  {
    this->SetNumberOfInputPorts(1);
    this->SetNumberOfOutputPorts(1);
  }
  std::vector<double> InputTimes;
  TestMultiTsAlgo(const TestMultiTsAlgo&) = delete;
  void operator=(const TestMultiTsAlgo&) = delete;
};
vtkStandardNewMacro(TestMultiTsAlgo);

/**
 * Same process as TestTimeFilter: cumulative sum of "Data"
 * This version uses vtkTemporalAlgorithm that relies on the
 *  NO_PRIOR_TEMPORAL_ACCESS() key to handle Insitu source.
 */
class TestTemporalAlgorithm : public vtkTemporalAlgorithm<TestAlgorithm>
{
public:
  vtkTypeMacro(TestTemporalAlgorithm, vtkTemporalAlgorithm<TestAlgorithm>);
  static TestTemporalAlgorithm* New();

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

  int NumInit = 0;
  int NumExec = 0;
  int NumFinal = 0;

protected:
  /**
   * Called only once per output update request in post processing,
   * or only when NO_PRIOR_TEMPORAL_ACCESS() is set to RESET() for InSitu.
   */
  int Initialize(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override
  {
    this->NumInit++;
    this->Value = 0;
    return 1;
  }

  /**
   * Called when time input time is updated:
   * - once per input time step in post processing
   * - once per pipeline update in In situ.
   */
  int Execute(vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector*) override
  {
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    vtkDataObject* input = vtkDataObject::GetData(inInfo);
    auto inputArray = input->GetFieldData()->GetArray("Data");
    this->NumExec++;
    this->Value += inputArray->GetTuple1(0);
    return 1;
  }

  /**
   * Called to generate an output.
   * - the whole input data has been processed in PostProcesssing
   * - current request has been processed in InSitu (partial result)
   */
  int Finalize(vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector) override
  {
    this->NumFinal++;
    vtkDataObject* output = vtkDataObject::GetData(outputVector, 0);
    vtkNew<vtkDoubleArray> cumulativeData;
    cumulativeData->SetName("CumulativeData");
    cumulativeData->InsertNextValue(this->Value);
    output->GetFieldData()->AddArray(cumulativeData);
    return 1;
  }

private:
  TestTemporalAlgorithm()
  {
    this->SetNumberOfInputPorts(1);
    this->SetNumberOfOutputPorts(1);
  }
  ~TestTemporalAlgorithm() override = default;
  TestTemporalAlgorithm(const TestTemporalAlgorithm&) = delete;
  void operator=(const TestTemporalAlgorithm&) = delete;

  double Value = 0;
};
vtkStandardNewMacro(TestTemporalAlgorithm);

//------------------------------------------------------------------------------
bool TestNaiveImplementation()
{
  vtkNew<TestTimeSource> source;
  vtkNew<TestTimeFilter> filter;
  filter->SetInputConnection(source->GetOutputPort(0));
  filter->UpdateTimeStep(3.);

  if (source->NumRequestData != 4 || source->NumRequestData != filter->NumRequestData)
  {
    std::cerr << "Unexpected number of RequestData calls.\n";
    return false;
  }

  vtkDataObject* out = filter->GetOutputDataObject(0);
  vtkDataArray* data = out->GetFieldData()->GetArray("CumulativeData");
  if (data->GetTuple1(0) != 6)
  {
    std::cerr << "Error !\n";
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool TestMultiTimeStep()
{
  vtkNew<TestTimeSource> source;
  vtkNew<TestMultiTsAlgo> filter;
  filter->SetInputConnection(source->GetOutputPort(0));
  filter->UpdateTimeStep(3.);

  if (source->NumRequestData != 4)
  {
    std::cerr << "Unexpected number of RequestData calls.\n";
    return false;
  }

  if (source->NumRequestInformation != 1 ||
    source->NumRequestInformation != filter->NumRequestInformation)
  {
    std::cerr << "Unexpected number of RequestInformation calls.\n";
    return false;
  }

  if (filter->NumExecute != 1)
  {
    std::cerr << "Unexpected number of Execute calls\n";
    return false;
  }

  vtkDataObject* out = filter->GetOutputDataObject(0);
  vtkDataArray* data = out->GetFieldData()->GetArray("CumulativeData");
  const double first = data->GetTuple1(0);
  if (first != 6)
  {
    std::cerr << "Wrong first value: " << first << " instead of 6\n";
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool TestTemporalAlgo()
{
  vtkNew<TestTimeSource> source;
  vtkNew<TestTemporalAlgorithm> filter;
  filter->SetInputConnection(source->GetOutputPort(0));
  filter->UpdateTimeStep(3.);

  if (filter->NumInit != source->NumRequestInformation || filter->NumInit != 1)
  {
    std::cerr << "Wrong number of call to vtkTemporalAlgorithm::Initialize \n";
    return false;
  }
  if (filter->NumExec != source->NumRequestData || filter->NumExec != 4)
  {
    std::cerr << "Wrong number of call to vtkTemporalAlgorithm::Execute \n";
    return false;
  }
  if (filter->NumFinal != filter->NumInit)
  {
    std::cerr << "Wrong number of call to vtkTemporalAlgorithm::Finalize \n";
    return false;
  }

  vtkDataObject* out = filter->GetOutputDataObject(0);
  vtkDataArray* dataArray = out->GetFieldData()->GetArray("CumulativeData");
  const double data = dataArray->GetTuple1(0);
  if (data != 6)
  {
    std::cerr << "Wrong first value: " << data << " instead of 6\n";
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool TestTemporalAlgoInSitu()
{
  vtkNew<TestInSituSource> insituSource;
  insituSource->SetNoPriorTemporalAccessInformationKey();
  insituSource->UpdateInformation();
  vtkInformation* insituSourceInfo = insituSource->GetOutputInformation(0);
  double* times = insituSourceInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  int nbTimes = insituSourceInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

  if (nbTimes != 1)
  {
    std::cerr << "InSitu source should have 1 timestep, but has " << nbTimes << "\n";
    return false;
  }

  vtkNew<TestTemporalAlgorithm> insituAlgo;
  insituAlgo->SetInputConnection(insituSource->GetOutputPort(0));
  insituAlgo->UpdateTimeStep(times[0]);

  insituSource->Modified();
  insituSource->UpdateInformation();
  insituSourceInfo = insituSource->GetOutputInformation(0);
  times = insituSourceInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  insituAlgo->UpdateTimeStep(times[0]);

  if (insituAlgo->NumInit != 1)
  {
    std::cerr << "Wrong number of call to vtkTemporalAlgorithm::Initialize. \n";
    return false;
  }
  if (insituAlgo->NumExec != insituSource->NumRequestData || insituAlgo->NumExec != 2)
  {
    std::cerr << "Wrong number of call to vtkTemporalAlgorithm::Execute. \n";
    return false;
  }
  if (insituAlgo->NumFinal != insituAlgo->NumExec)
  {
    std::cerr << "Wrong number of call to vtkTemporalAlgorithm::Finalize. \n";
    return false;
  }

  vtkDataObject* out = insituAlgo->GetOutputDataObject(0);
  assert(out);
  vtkDataArray* dataArray = out->GetFieldData()->GetArray("CumulativeData");
  assert(dataArray);
  const double partialData = dataArray->GetTuple1(0);
  if (partialData != 11)
  {
    std::cerr << "Wrong partial value: " << partialData << " instead of 11\n";
    return false;
  }

  return true;
}
}

//------------------------------------------------------------------------------
int TestTemporalHelpers(int, char*[])
{
  if (!::TestNaiveImplementation())
  {
    std::cerr << "Errors in TestNaiveImplementation" << std::endl;
    return EXIT_FAILURE;
  }
  if (!::TestMultiTimeStep())
  {
    std::cerr << "Errors in TestMultiTimeStep" << std::endl;
    return EXIT_FAILURE;
  }
  if (!::TestTemporalAlgo())
  {
    std::cerr << "Errors in TestTemporalAlgo" << std::endl;
    return EXIT_FAILURE;
  }
  if (!::TestTemporalAlgoInSitu())
  {
    std::cerr << "Errors in TestTemporalAlgoInSitu" << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
