// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include <vtkAffineArray.h>
#include <vtkAttributeDataToTableFilter.h>
#include <vtkCellData.h>
#include <vtkDoubleArray.h>
#include <vtkHyperTreeGrid.h>
#include <vtkHyperTreeGridPreConfiguredSource.h>
#include <vtkImageData.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkLogger.h>
#include <vtkMathUtilities.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkRTAnalyticSource.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkTable.h>
#include <vtkTemporalArrayOperatorFilter.h>

namespace
{
const double TIME_RANGE[2] = { 0, 5 };
const double TIME_VALUES[6] = { 0, 0.1, 0.2, 0.3, 0.4, 0.5 };
const std::string ARRAY_NAME = "timeData";

/**
 * Subclass the wavelet to add a time-dependent data array.
 */
class vtkTemporalRTAnalyticSource : public vtkRTAnalyticSource
{
public:
  static vtkTemporalRTAnalyticSource* New();
  vtkTypeMacro(vtkTemporalRTAnalyticSource, vtkRTAnalyticSource);

protected:
  vtkTemporalRTAnalyticSource() = default;

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override
  {
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), ::TIME_VALUES, 6);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), ::TIME_RANGE, 2);

    vtkRTAnalyticSource::RequestInformation(request, inputVector, outputVector);
    return 1;
  }

  /**
   * Data Array value is [Point index + time] for each point.
   */
  void ExecuteDataWithInformation(vtkDataObject* output, vtkInformation* outInfo) override
  {
    this->Superclass::ExecuteDataWithInformation(output, outInfo);

    // Split the update extent further based on piece request.
    vtkImageData* data = vtkImageData::GetData(outInfo);
    vtkIdType size = data->GetNumberOfPoints();

    double time = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    vtkNew<vtkAffineArray<double>> affine;
    affine->SetNumberOfTuples(size);
    affine->ConstructBackend(1, time);
    affine->SetName(ARRAY_NAME.c_str());
    data->GetPointData()->SetScalars(affine);

    double range[2];
    affine->GetRange(range);
  }

private:
  vtkTemporalRTAnalyticSource(const vtkTemporalRTAnalyticSource&) = delete;
  void operator=(const vtkTemporalRTAnalyticSource&) = delete;
};
vtkStandardNewMacro(vtkTemporalRTAnalyticSource);

/**
 * Subclass an HTG source to add a data array depending on time request
 */
class vtkTemporalHTGSource : public vtkHyperTreeGridPreConfiguredSource
{
public:
  static vtkTemporalHTGSource* New();
  vtkTypeMacro(vtkTemporalHTGSource, vtkHyperTreeGridPreConfiguredSource);

protected:
  vtkTemporalHTGSource() = default;

  /**
   * Declare timesteps [0, 5]
   */
  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override
  {
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), ::TIME_VALUES, 6);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), ::TIME_RANGE, 2);
    this->Superclass::RequestInformation(request, inputVector, outputVector);
    return 1;
  }

  /**
   *  Data array is index + time
   */
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override
  {
    this->Superclass::RequestData(request, inputVector, outputVector);
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    double time = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

    vtkHyperTreeGrid* htg = vtkHyperTreeGrid::GetData(outInfo);
    vtkIdType size = htg->GetNumberOfCells();

    vtkNew<vtkAffineArray<double>> affine;
    affine->SetNumberOfTuples(size);
    affine->ConstructBackend(1., time);
    affine->SetName(ARRAY_NAME.c_str());
    htg->GetCellData()->AddArray(affine);

    return 1;
  }

private:
  vtkTemporalHTGSource(const vtkTemporalHTGSource&) = delete;
  void operator=(const vtkTemporalHTGSource) = delete;
};
vtkStandardNewMacro(vtkTemporalHTGSource);

//------------------------------------------------------------------------------
bool TestDefault()
{
  vtkNew<vtkTemporalRTAnalyticSource> wavelet;

  vtkNew<vtkTemporalArrayOperatorFilter> operatorFilter;
  operatorFilter->SetInputConnection(wavelet->GetOutputPort());
  operatorFilter->SetInputArrayToProcess(
    ARRAY_NAME.c_str(), vtkDataObject::FIELD_ASSOCIATION_POINTS);

  const int firstTimeStep = 3;
  const int secondTimeStep = 0;
  operatorFilter->SetFirstTimeStepIndex(firstTimeStep);
  operatorFilter->SetSecondTimeStepIndex(secondTimeStep);

  operatorFilter->UpdateInformation();
  // requested time value is not taken into account because RelativeMode is off
  const int requestTimeStep = 2;
  operatorFilter->UpdateTimeStep(::TIME_VALUES[requestTimeStep]);

  vtkDataSet* outputData = vtkDataSet::SafeDownCast(operatorFilter->GetOutputDataObject(0));
  const vtkIdType size = outputData->GetNumberOfPoints();

  double range[2];
  outputData->GetPointData()->GetArray(ARRAY_NAME.c_str())->GetRange(range);
  if (range[0] != ::TIME_VALUES[firstTimeStep] || range[1] != range[0] + size - 1)
  {
    vtkLog(ERROR, << "Bad initial range:" << range[0] << ";" << range[1]);
    return false;
  }

  const std::string outArrayName = ARRAY_NAME + "_add";
  vtkDataArray* outArray = outputData->GetPointData()->GetArray(outArrayName.c_str());
  if (!outArray)
  {
    vtkLog(ERROR, << "Missing 'add' output array!");
    return false;
  }
  outArray->GetRange(range);
  if (range[0] != ::TIME_VALUES[firstTimeStep] + ::TIME_VALUES[secondTimeStep] ||
    range[1] != range[0] + 2 * (size - 1))
  {
    vtkLog(ERROR, << "Bad 'add' result range:" << range[0] << ";" << range[1]);
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool TestOperatorSub()
{
  vtkNew<vtkTemporalRTAnalyticSource> wavelet;

  vtkNew<vtkTemporalArrayOperatorFilter> operatorFilter;
  operatorFilter->SetInputConnection(wavelet->GetOutputPort());
  operatorFilter->SetInputArrayToProcess(
    ARRAY_NAME.c_str(), vtkDataObject::FIELD_ASSOCIATION_POINTS);
  operatorFilter->SetOperator(vtkTemporalArrayOperatorFilter::SUB);
  operatorFilter->SetOutputArrayNameSuffix("_diff");
  const int firstTimeStep = 3;
  const int secondTimeStep = 0;
  operatorFilter->SetFirstTimeStepIndex(firstTimeStep);
  operatorFilter->SetSecondTimeStepIndex(secondTimeStep);

  const int requestTimeStep = 2;
  operatorFilter->UpdateTimeStep(::TIME_VALUES[requestTimeStep]);

  vtkDataSet* outputData = vtkDataSet::SafeDownCast(operatorFilter->GetOutputDataObject(0));

  const std::string outArrayName = ARRAY_NAME + "_diff";
  vtkDataArray* outArray = outputData->GetPointData()->GetArray(outArrayName.c_str());
  if (!outArray)
  {
    vtkLog(ERROR, << "Missing 'sub' output array!");
    return false;
  }

  double range[2];
  outArray->GetRange(range);
  if (!vtkMathUtilities::NearlyEqual(
        range[0], (::TIME_VALUES[firstTimeStep] - ::TIME_VALUES[secondTimeStep]), 1e-6) ||
    !vtkMathUtilities::NearlyEqual(range[1], range[0], 1e-6))
  {
    vtkLog(ERROR, << "Bad 'sub' result range: " << range[0] << ";" << range[1]);
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool TestRelativeMode()
{
  vtkNew<vtkTemporalRTAnalyticSource> wavelet;

  vtkNew<vtkTemporalArrayOperatorFilter> operatorFilter;
  operatorFilter->SetInputConnection(wavelet->GetOutputPort());
  operatorFilter->SetInputArrayToProcess(
    ARRAY_NAME.c_str(), vtkDataObject::FIELD_ASSOCIATION_POINTS);
  operatorFilter->SetOperator(vtkTemporalArrayOperatorFilter::MUL);
  const int requestTimeStep = 3;
  operatorFilter->SetRelativeMode(true);
  operatorFilter->SetTimeStepShift(-requestTimeStep);

  operatorFilter->UpdateTimeStep(::TIME_VALUES[requestTimeStep]);
  vtkDataSet* outputData = vtkDataSet::SafeDownCast(operatorFilter->GetOutputDataObject(0));
  const vtkIdType size = outputData->GetNumberOfPoints();

  const std::string outArrayName = ARRAY_NAME + "_mul";
  vtkDataArray* outArray = outputData->GetPointData()->GetArray(outArrayName.c_str());
  if (!outArray)
  {
    vtkLog(ERROR, << "Missing 'mul' output array!");
    return false;
  }

  double range[2];
  outArray->GetRange(range);
  if (range[0] != ::TIME_VALUES[requestTimeStep] * ::TIME_VALUES[0] ||
    range[1] != (::TIME_VALUES[requestTimeStep] + size - 1) * (::TIME_VALUES[0] + size - 1))
  {
    vtkLog(ERROR, << "Bad 'mul' result range:" << range[0] << ";" << range[1]);
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool TestTable()
{
  vtkNew<vtkTemporalRTAnalyticSource> wavelet;
  vtkNew<vtkAttributeDataToTableFilter> toTable;
  toTable->SetInputConnection(wavelet->GetOutputPort());

  vtkNew<vtkTemporalArrayOperatorFilter> operatorFilter;
  operatorFilter->SetInputConnection(toTable->GetOutputPort());
  operatorFilter->SetRelativeMode(true);
  operatorFilter->SetInputArrayToProcess(ARRAY_NAME.c_str(), vtkDataObject::FIELD_ASSOCIATION_ROWS);
  operatorFilter->SetOperator(vtkTemporalArrayOperatorFilter::SUB);

  const int shift = operatorFilter->GetTimeStepShift();
  const int currentTimeStep = 1;
  operatorFilter->UpdateTimeStep(::TIME_VALUES[currentTimeStep]);

  vtkTable* diff = vtkTable::SafeDownCast(operatorFilter->GetOutputDataObject(0));
  vtkIdType size = diff->GetNumberOfRows();

  double range[2];
  diff->GetRowData()->GetArray(ARRAY_NAME.c_str())->GetRange(range);
  if (range[0] != ::TIME_VALUES[currentTimeStep] &&
    range[1] != ::TIME_VALUES[currentTimeStep] + size - 1)
  {
    vtkLog(ERROR, << "Bad initial range:" << range[0] << ";" << range[1]);
    return false;
  }

  diff->GetRowData()->GetArray("timeData_sub")->GetRange(range);
  if (!vtkMathUtilities::NearlyEqual(range[0],
        (::TIME_VALUES[currentTimeStep] - ::TIME_VALUES[currentTimeStep + shift]), 1e-6) ||
    !vtkMathUtilities::NearlyEqual(range[1], range[0], 1e-6))
  {
    vtkLog(ERROR, << "Bad initial range:" << range[0] << ";" << range[1]);
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool TestHTG()
{
  vtkNew<vtkTemporalHTGSource> source;

  vtkNew<vtkTemporalArrayOperatorFilter> operatorFilter;
  operatorFilter->SetInputConnection(source->GetOutputPort());
  operatorFilter->SetRelativeMode(true);
  operatorFilter->SetInputArrayToProcess(
    ARRAY_NAME.c_str(), vtkDataObject::FIELD_ASSOCIATION_CELLS);
  operatorFilter->SetOperator(vtkTemporalArrayOperatorFilter::ADD);

  int shift = operatorFilter->GetTimeStepShift();
  int currentTimeStep = 1;
  operatorFilter->UpdateTimeStep(::TIME_VALUES[currentTimeStep]);

  vtkHyperTreeGrid* sum = vtkHyperTreeGrid::SafeDownCast(operatorFilter->GetOutputDataObject(0));
  vtkIdType size = sum->GetNumberOfCells();
  vtkLog(INFO, << size);

  double range[2];
  sum->GetCellData()->GetArray("timeData_add")->GetRange(range);
  double min = ::TIME_VALUES[currentTimeStep] + ::TIME_VALUES[currentTimeStep + shift];
  if (range[0] != min && range[1] != min + 2 * size - 2)
  {
    vtkLog(ERROR, << "Bad _add range:" << range[0] << ";" << range[1]);
    return false;
  }

  return true;
}
};

//------------------------------------------------------------------------------
int TestTemporalArrayOperatorFilter(int, char*[])
{
  if (!::TestDefault())
  {
    vtkLog(ERROR, << "TestDefault failed");
    return EXIT_FAILURE;
  }
  else if (!::TestOperatorSub())
  {
    vtkLog(ERROR, << "TestOperatorSub failed");
    return EXIT_FAILURE;
  }
  else if (!::TestRelativeMode())
  {
    vtkLog(ERROR, << "TestRelativeMode failed");
    return EXIT_FAILURE;
  }
  else if (!::TestTable())
  {
    vtkLog(ERROR, << "TestTable failed");
    return EXIT_FAILURE;
  }
  else if (!::TestHTG())
  {
    vtkLog(ERROR, << "TestHTG failed");
    return false;
  }

  return EXIT_SUCCESS;
}
