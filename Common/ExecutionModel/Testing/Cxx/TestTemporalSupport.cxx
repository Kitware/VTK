/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTemporalSupport.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkImageData.h"
#include "vtkAlgorithm.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkDataArray.h"
#include "vtkAbstractArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include <assert.h>

#define ExpectEq(a,b) if(a!=b){ cerr<<a<<"!="<<b<<endl; return 0;}

using namespace std;
class TestTimeSource : public vtkAlgorithm
{
public:
  static TestTimeSource *New();
  vtkTypeMacro(TestTimeSource,vtkAlgorithm);
  vtkGetMacro(NumRequestData,int);

  TestTimeSource()
  {
    this->SetNumberOfInputPorts(0);
    this->SetNumberOfOutputPorts(1);
    for(int i=0; i<10 ;i++)
      {
      this->TimeSteps.push_back(i);
      }
    this->NumRequestData =0;
  }

  int ProcessRequest(vtkInformation* request,
                     vtkInformationVector** inputVector,
                     vtkInformationVector* outputVector)
  {
    // generate the data
    if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
      {
      this->NumRequestData++;
      vtkInformation *outInfo = outputVector->GetInformationObject(0);
      vtkImageData *outImage = vtkImageData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
      double timeStep = outInfo->Get( vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

      outInfo->Set(vtkDataObject::DATA_TIME_STEP(),timeStep);
      int scalarType = vtkImageData::GetScalarType(outInfo);
      int numComponents = vtkImageData::GetNumberOfScalarComponents(outInfo);
      outImage->AllocateScalars(scalarType, numComponents);
      return 1;
      }

    // execute information
    if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
      {
      vtkInformation *outInfo = outputVector->GetInformationObject(0);
      double range[2]= {0,9};
      outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), range,2);
      outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &TimeSteps[0], TimeSteps.size());
      return 1;
      }

    return this->Superclass::ProcessRequest(request, inputVector, outputVector);
  }
  int FillOutputPortInformation(int, vtkInformation *info)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
    return 1;
  }
private:
  vector<double> TimeSteps;
  int NumRequestData;
};
vtkStandardNewMacro(TestTimeSource);

class TestTimeFilter: public vtkAlgorithm
{
public:
  vtkTypeMacro(TestTimeFilter,vtkAlgorithm);
  static TestTimeFilter *New();

  vtkSetMacro(StartTime, double);
  void PrintSelf(ostream&, vtkIndent){}

  int FillInputPortInformation(int, vtkInformation *info)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
    return 1;
  }

  int FillOutputPortInformation(int, vtkInformation *info)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
    return 1;
  }

  int ProcessRequest(vtkInformation* request,
                     vtkInformationVector** inputVector,
                     vtkInformationVector* outputVector)
  {
    if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
      {
      this->TimeIndex++;
      if(this->TimeIndex<this->TimeIterations)
        {
        request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
        }
      else
        {
        this->TimeIndex=0;
        request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
        }
      }

    if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
      {
      vtkInformation *inInfo  = inputVector[0]->GetInformationObject(0);
      double timeStep=  this->StartTime + (double)this->TimeIndex;
      inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), timeStep);
      return 1;
      }

    return this->Superclass::ProcessRequest(request, inputVector, outputVector);
  }

private:
  double StartTime;
  int TimeIndex;
  int TimeIterations;
  TestTimeFilter()
  {
    this->SetNumberOfInputPorts(1);
    this->SetNumberOfOutputPorts(1);
    this->StartTime = 0;
    this->TimeIndex = 0;
    this->TimeIterations = 2;
  }
};
vtkStandardNewMacro(TestTimeFilter);

int TestTemporalSupport(int, char**)
{
  vtkSmartPointer<TestTimeSource> imageSource = vtkSmartPointer<TestTimeSource>::New();
  vtkSmartPointer<TestTimeFilter> filter = vtkSmartPointer<TestTimeFilter>::New();
  filter->SetInputConnection(imageSource->GetOutputPort());

  int numSteps=3;
  for(int t = 0; t<numSteps; t++ )
    {
    filter->SetStartTime(t);
    filter->Update();
    }
  ExpectEq(imageSource->GetNumRequestData(),numSteps+1);
  return 0;
}
