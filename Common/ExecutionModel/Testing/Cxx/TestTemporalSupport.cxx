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
#include "vtkNew.h"
#include <assert.h>

#define CHECK(b, errors) if(!(b)){ errors++; cerr<<"Error on Line "<<__LINE__<<":"<<endl;}

using namespace std;
class TestAlgorithm: public vtkAlgorithm
{
public:
  static TestAlgorithm *New();
  vtkTypeMacro(TestAlgorithm,vtkAlgorithm);

  vtkGetMacro(NumRequestInformation , int);
  vtkGetMacro(NumRequestData , int);
  vtkGetMacro(NumRequestUpdateExtent, int);
  vtkGetMacro(NumRequestUpdateTime, int);
  vtkGetMacro(NumRequestTimeDependentInformation, int);

  virtual int ProcessRequest(vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
  {
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    this->NumRequestInformation++;
    return this->RequestInformation(request, inputVector, outputVector);
    }
  if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
    {
    this->NumRequestUpdateExtent++;
    return this->RequestUpdateExtent(request, inputVector, outputVector);
    }
  if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_DATA()))
    {
    this->NumRequestData++;
    return this->RequestData(request, inputVector, outputVector);
    }
  if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_TIME()))
    {
    this->NumRequestUpdateTime++;
    return this->RequestUpdateTime(request, inputVector, outputVector);
    }
  if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_TIME_DEPENDENT_INFORMATION()))
    {
    this->NumRequestTimeDependentInformation++;
    return this->RequestTimeDependentInformation(request, inputVector, outputVector);
    }
  return 1;
  }
protected:
  TestAlgorithm()
  {
    this->NumRequestInformation =0;
    this->NumRequestData =0;
    this->NumRequestUpdateExtent=0;
    this->NumRequestUpdateTime=0;
    this->NumRequestTimeDependentInformation=0;
  }
  virtual int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*){ return 1;}
  virtual int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) {return 1;};
  virtual int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) { return 1; };
  virtual int RequestUpdateTime(vtkInformation*, vtkInformationVector**, vtkInformationVector*) { return 1; };
  virtual int RequestTimeDependentInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) { return 1; };

  int NumRequestInformation;
  int NumRequestData;
  int NumRequestUpdateExtent;
  int NumRequestUpdateTime;
  int NumRequestTimeDependentInformation;
};
vtkStandardNewMacro(TestAlgorithm);

class TestTimeSource : public TestAlgorithm
{
public:
  static TestTimeSource *New();
  vtkTypeMacro(TestTimeSource,TestAlgorithm);
  vtkSetMacro(HasTimeDependentData,bool);

  TestTimeSource()
  {
    this->SetNumberOfInputPorts(0);
    this->SetNumberOfOutputPorts(1);
    for(int i=0; i<10 ;i++)
      {
      this->TimeSteps.push_back(i);
      }
    this->HasTimeDependentData=false;
  }

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
  {
    vtkInformation *outInfo = outputVector->GetInformationObject(0);
    vtkImageData *outImage = vtkImageData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
    double timeStep = outInfo->Get( vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

    outImage->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(),timeStep);
    int scalarType = vtkImageData::GetScalarType(outInfo);
    int numComponents = vtkImageData::GetNumberOfScalarComponents(outInfo);
    outImage->AllocateScalars(scalarType, numComponents);
    return 1;
  }

  virtual int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
  {
    vtkInformation *outInfo = outputVector->GetInformationObject(0);
    double range[2]= {0,9};
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), range,2);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
                 &TimeSteps[0], static_cast<int>(TimeSteps.size()));
    if(this->HasTimeDependentData)
      {
      outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_DEPENDENT_INFORMATION(),1);
      }
    return 1;
  }

  int FillOutputPortInformation(int, vtkInformation *info)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
    return 1;
  }
private:
  vector<double> TimeSteps;
  bool HasTimeDependentData;
  TestTimeSource(const TestTimeSource&);  // Not implemented.
  void operator=(const TestTimeSource&);  // Not implemented.
};
vtkStandardNewMacro(TestTimeSource);


class TestTimeFilter: public TestAlgorithm
{
public:
  vtkTypeMacro(TestTimeFilter,TestAlgorithm);
  static TestTimeFilter *New();

  vtkSetMacro(StartTime, double);
  vtkSetMacro(TimeIterations, int);

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

  int RequestData(vtkInformation* request, vtkInformationVector**in, vtkInformationVector* )
  {
    cout<<"Has TD: "<<
      in[0]->GetInformationObject(0)->Get(vtkStreamingDemandDrivenPipeline::TIME_DEPENDENT_INFORMATION())<<endl;
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
    return 1;
  }

  int RequestUpdateExtent(vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector*)
  {
    vtkInformation *inInfo  = inputVector[0]->GetInformationObject(0);
    double timeStep=  this->StartTime + (double)this->TimeIndex;
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
  TestTimeFilter(const TestTimeFilter&);  // Not implemented.
  void operator=(const TestTimeFilter&);  // Not implemented.
};
vtkStandardNewMacro(TestTimeFilter);


int TestTimeDependentInformationExecution()
{
  int numErrors(0);
  for(int i=1; i<2; i++)
    {
    bool hasTemporalMeta = i==0? false : true;
    vtkNew<TestTimeSource> imageSource;
    imageSource->SetHasTimeDependentData(hasTemporalMeta);

    vtkNew<TestTimeFilter> filter;
    filter->SetTimeIterations(1);
    filter->SetInputConnection(imageSource->GetOutputPort());

    filter->SetStartTime(2.0);
    filter->Update();

    CHECK(imageSource->GetNumRequestData()==1, numErrors);
    CHECK(imageSource->GetNumRequestInformation()==1, numErrors);
    CHECK(imageSource->GetNumRequestUpdateExtent()==1, numErrors);
    if(hasTemporalMeta)
      {
      CHECK(imageSource->GetNumRequestTimeDependentInformation()==1, numErrors);
      CHECK(filter->GetNumRequestUpdateTime()==1, numErrors);
      }
    else
      {
      CHECK(imageSource->GetNumRequestTimeDependentInformation()==0, numErrors);
      CHECK(filter->GetNumRequestUpdateTime()==0, numErrors);
      }

    filter->SetStartTime(3.0);
    filter->Update(0);
    double dataTime = imageSource->GetOutputDataObject(0)->GetInformation()->Get(vtkDataObject::DATA_TIME_STEP());
    CHECK(dataTime==3.0, numErrors);
    }

   return numErrors;
}

int TestContinueExecution()
{
  int numErrors(0);
  vtkSmartPointer<TestTimeSource> imageSource = vtkSmartPointer<TestTimeSource>::New();
  vtkSmartPointer<TestTimeFilter> filter = vtkSmartPointer<TestTimeFilter>::New();
  filter->SetInputConnection(imageSource->GetOutputPort());

  int numSteps=3;
  for(int t = 0; t<numSteps; t++ )
    {
    filter->SetStartTime(t);
    filter->Update();
    }
  CHECK(imageSource->GetNumRequestData()==numSteps+1,numErrors);
  return numErrors;
}

int TestTemporalSupport(int, char*[])
{
  int totalErrors(0);
  int errors(0);
  if((errors=TestTimeDependentInformationExecution())!=0)
    {
    totalErrors+=errors;
    cerr<<errors<<" errors in TestTimeDependentInformationExecution"<<endl;
    }
  if((errors=TestContinueExecution())!=0)
    {
    totalErrors+=errors;
    cerr<<errors<<" errors in TestContinueExecution"<<endl;
    }
  return totalErrors;
}
