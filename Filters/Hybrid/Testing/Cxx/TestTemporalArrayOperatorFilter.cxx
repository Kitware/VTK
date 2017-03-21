/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTemporalArrayOperatorFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <vtkDoubleArray.h>
#include <vtkImageData.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkMathUtilities.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkRTAnalyticSource.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkTemporalArrayOperatorFilter.h>

class vtkTemporalRTAnalyticSource : public vtkRTAnalyticSource
{
public:
  static vtkTemporalRTAnalyticSource* New();
  vtkTypeMacro(vtkTemporalRTAnalyticSource, vtkRTAnalyticSource);

protected:
  vtkTemporalRTAnalyticSource() {}

  int RequestInformation(vtkInformation* request,
                         vtkInformationVector** inputVector,
                         vtkInformationVector* outputVector) VTK_OVERRIDE
  {
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    double range[2] = { 0, 5 };
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), range, 2);
    double outTimes[6] = { 0, 1, 2, 3, 4, 5 };
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), outTimes, 6);
    vtkRTAnalyticSource::RequestInformation(request, inputVector, outputVector);
    return 1;
  }

  void ExecuteDataWithInformation(vtkDataObject* output, vtkInformation* outInfo) VTK_OVERRIDE
  {
    Superclass::ExecuteDataWithInformation(output, outInfo);

    // Split the update extent further based on piece request.
    vtkImageData* data = vtkImageData::GetData(outInfo);
    int* outExt = data->GetExtent();

    // find the region to loop over
    int maxX = (outExt[1] - outExt[0]) + 1;
    int maxY = (outExt[3] - outExt[2]) + 1;
    int maxZ = (outExt[5] - outExt[4]) + 1;

    vtkNew<vtkDoubleArray> timeArray;
    timeArray->SetName("timeData");
    timeArray->SetNumberOfValues(maxX * maxY * maxZ);
    data->GetPointData()->SetScalars(timeArray.Get());

    double t = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    vtkIdType cnt = 0;
    for (int idxZ = 0; idxZ < maxZ; idxZ++)
    {
      for (int idxY = 0; idxY < maxY; idxY++)
      {
        for (int idxX = 0; idxX < maxX; idxX++, cnt++)
        {
          timeArray->SetValue(cnt, (1 + t) * idxX + t);
        }
      }
    }
  }

private:
  vtkTemporalRTAnalyticSource(const vtkTemporalRTAnalyticSource&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTemporalRTAnalyticSource&) VTK_DELETE_FUNCTION;
};

vtkStandardNewMacro(vtkTemporalRTAnalyticSource);

//------------------------------------------------------------------------------
// Program main
int TestTemporalArrayOperatorFilter(int, char*[])
{
  vtkNew<vtkTemporalRTAnalyticSource> wavelet;

  // Test ADD operation and default suffix name
  vtkNew<vtkTemporalArrayOperatorFilter> operatorFilter;
  operatorFilter->SetInputConnection(wavelet->GetOutputPort());

  operatorFilter->SetFirstTimeStepIndex(3);
  operatorFilter->SetSecondTimeStepIndex(0);
  operatorFilter->SetInputArrayToProcess(0, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, "timeData");
  operatorFilter->SetOperator(vtkTemporalArrayOperatorFilter::ADD);

  operatorFilter->UpdateInformation();
  operatorFilter->GetOutputInformation(0)->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), 2);
  operatorFilter->Update();

  vtkDataSet* diff =
    vtkDataSet::SafeDownCast(operatorFilter->GetOutputDataObject(0));

  double range[2];
  diff->GetPointData()->GetArray("timeData")->GetRange(range);
  if (range[0] != 0 && range[1] != 20)
  {
    std::cerr << "Bad initial range:" << range[0] << ";" << range[1] << endl;
    return EXIT_FAILURE;
  }

  if (!diff->GetPointData()->GetArray("timeData_add"))
  {
    std::cerr << "Missing 'add' output array!" << endl;
    return EXIT_FAILURE;
  }
  diff->GetPointData()->GetArray("timeData_add")->GetRange(range);
  if (range[0] != 3 || range[1] != 103)
  {
    std::cerr << "Bad 'sub' result range:" << range[0] << ";" << range[1] << endl;
    return EXIT_FAILURE;
  }

  // Test SUB operation and suffix name
  operatorFilter->SetOperator(vtkTemporalArrayOperatorFilter::SUB);
  operatorFilter->SetOutputArrayNameSuffix("_diff");
  operatorFilter->Update();
  diff = vtkDataSet::SafeDownCast(operatorFilter->GetOutputDataObject(0));

  if (!diff->GetPointData()->GetArray("timeData_diff"))
  {
    std::cerr << "Missing 'sub' output array!" << endl;
    return EXIT_FAILURE;
  }
  diff->GetPointData()->GetArray("timeData_diff")->GetRange(range);
  if (range[0] != 3 || range[1] != 63)
  {
    std::cerr << "Bad 'sub' result range:" << range[0] << ";" << range[1] << endl;
    return EXIT_FAILURE;
  }

  // Test MUL operation and suffix name
  operatorFilter->SetOperator(vtkTemporalArrayOperatorFilter::MUL);
  operatorFilter->SetOutputArrayNameSuffix("_mul");
  operatorFilter->Update();
  diff = vtkDataSet::SafeDownCast(operatorFilter->GetOutputDataObject(0));

  if (!diff->GetPointData()->GetArray("timeData_mul"))
  {
    std::cerr << "Missing 'mul' output array!" << endl;
    return EXIT_FAILURE;
  }
  diff->GetPointData()->GetArray("timeData_mul")->GetRange(range);
  if (range[0] != 0 || range[1] != 1660)
  {
    std::cerr << "Bad 'mul' result range:" << range[0] << ";" << range[1] << endl;
    return EXIT_FAILURE;
  }

  // Test DIV operation and default suffix name
  operatorFilter->SetFirstTimeStepIndex(0);
  operatorFilter->SetSecondTimeStepIndex(4);
  operatorFilter->SetOperator(vtkTemporalArrayOperatorFilter::DIV);
  operatorFilter->SetOutputArrayNameSuffix("");
  operatorFilter->Update();
  diff = vtkDataSet::SafeDownCast(operatorFilter->GetOutputDataObject(0));

  if (!diff->GetPointData()->GetArray("timeData_div"))
  {
    std::cerr << "Missing 'div' output array!" << endl;
    return EXIT_FAILURE;
  }
  diff->GetPointData()->GetArray("timeData_div")->GetRange(range);
  if (range[0] != 0 || vtkMathUtilities::FuzzyCompare(range[1], 0.192308))
  {
    std::cerr << "Bad 'div' result range:" << range[0] << ";" << range[1] << endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
