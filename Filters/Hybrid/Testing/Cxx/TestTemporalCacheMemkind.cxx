/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTemporalCacheMemkind.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// This test verifies that the extended memory feature of vtkTemporalDatasetCache,
// that is that it can manage cache contents in the extended memory space, works.

#include <vtkCommand.h>
#include <vtkFeatures.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkTemporalDataSetCache.h>
#include <vtkTemporalInterpolator.h>

#include <cassert>
#include <vector>

namespace
{

// todo: unify with infrastructure from the other temporalcache tests
class vtkTemporalSphereSource2 : public vtkSphereSource
{

public:
  static vtkTemporalSphereSource2* New();
  vtkTypeMacro(vtkTemporalSphereSource2, vtkSphereSource);

  vtkSetMacro(TimeStep, int);
  vtkGetMacro(TimeStep, int);
  vtkGetVector2Macro(TimeStepRange, int);

protected:
  vtkTemporalSphereSource2();
  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

public:
  int TimeStepRange[2];
  int TimeStep;
  int ActualTimeStep;
  std::vector<double> TimeStepValues;
};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkTemporalSphereSource2);

//------------------------------------------------------------------------------
vtkTemporalSphereSource2::vtkTemporalSphereSource2()
{
  this->TimeStepRange[0] = 0;
  this->TimeStepRange[1] = 0;
  this->TimeStep = 0;
  this->ActualTimeStep = 0;
}

//------------------------------------------------------------------------------
int vtkTemporalSphereSource2::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (!this->Superclass::RequestInformation(request, inputVector, outputVector))
  {
    return 0;
  }
  this->TimeStepRange[0] = 0;
  this->TimeStepRange[1] = 9;
  this->TimeStepValues.resize(this->TimeStepRange[1] - this->TimeStepRange[0] + 1);
  for (int i = 0; i <= this->TimeStepRange[1]; ++i)
  {
    this->TimeStepValues[i] = i;
  }

  outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), this->TimeStepValues.data(),
    static_cast<int>(this->TimeStepValues.size()));
  double timeRange[2];
  timeRange[0] = this->TimeStepValues.front();
  timeRange[1] = this->TimeStepValues.back();
  outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);

  return 1;
}

//------------------------------------------------------------------------------
bool vtkTestTemporalCacheSimpleWithinTolerance2(double a, double b)
{
  return (fabs(a - b) <= (a * 1E-6));
}

//------------------------------------------------------------------------------
int vtkTemporalSphereSource2::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataObject* doOutput = outInfo->Get(vtkDataObject::DATA_OBJECT());

  this->ActualTimeStep = this->TimeStep;
  if (this->TimeStep == 0 && outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    double requestedTimeValue = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    this->ActualTimeStep =
      std::find_if(this->TimeStepValues.begin(), this->TimeStepValues.end(),
        [requestedTimeValue](double const& v) {
          return vtkTestTemporalCacheSimpleWithinTolerance2(v, requestedTimeValue);
        }) -
      this->TimeStepValues.begin();
    this->ActualTimeStep = this->ActualTimeStep + this->TimeStepRange[0];
  }
  else
  {
    double timevalue;
    timevalue = this->TimeStepValues[this->ActualTimeStep - this->TimeStepRange[0]];
    vtkDebugMacro(<< "Using manually set t= " << timevalue << " Step : " << this->ActualTimeStep);
    doOutput->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), timevalue);
  }

  cout << "this->ActualTimeStep : " << this->ActualTimeStep << endl;
  return Superclass::RequestData(request, inputVector, outputVector);
}

class vtkTestTemporalCacheSimpleExecuteCallback2 : public vtkCommand
{
public:
  static vtkTestTemporalCacheSimpleExecuteCallback2* New()
  {
    return new vtkTestTemporalCacheSimpleExecuteCallback2;
  }
  void Execute(vtkObject* caller, unsigned long, void*) override
  {
    // count the number of timesteps requested
    vtkTemporalSphereSource2* sph = vtkTemporalSphereSource2::SafeDownCast(caller);
    vtkInformation* info = sph->GetExecutive()->GetOutputInformation(0);
    int Length = info->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()) ? 1 : 0;
    this->Count += Length;
  }
  unsigned int Count;
};

} // namespace

int TestTemporalCacheMemkind(int ac, char* av[])
{
  std::string home = ".";
  for (int a = 0; a < ac - 1; ++a)
  {
    if (!strcmp(av[a], "-home") && a < ac - 1)
    {
      home = std::string(av[a + 1]);
    }
  }
  cout << "Extended memory is backed by " << home << endl;
  vtkObjectBase::SetMemkindDirectory(home.c_str());

  cout << "******************** Test cachefilter ********************" << endl;
  // create temporal data
  vtkSmartPointer<vtkTemporalSphereSource2> sphere =
    vtkSmartPointer<vtkTemporalSphereSource2>::New();
  vtkTestTemporalCacheSimpleExecuteCallback2* executecb =
    vtkTestTemporalCacheSimpleExecuteCallback2::New();
  executecb->Count = 0;
  sphere->AddObserver(vtkCommand::StartEvent, executecb);
  executecb->Delete();

  // cache the data to prevent regenerating some of it
  vtkSmartPointer<vtkTemporalDataSetCache> tdsc1 = vtkSmartPointer<vtkTemporalDataSetCache>::New();
  tdsc1->CacheInMemkindOn();
  tdsc1->SetInputConnection(sphere->GetOutputPort());
  tdsc1->SetCacheSize(10);

  // a second cache to stress the shallow copy within memkind behavior
  vtkSmartPointer<vtkTemporalDataSetCache> tdsc2 = vtkSmartPointer<vtkTemporalDataSetCache>::New();
  tdsc2->CacheInMemkindOn();
  tdsc2->SetInputConnection(tdsc1->GetOutputPort());
  tdsc2->SetCacheSize(10);

  vtkSmartPointer<vtkTemporalInterpolator> interp = vtkSmartPointer<vtkTemporalInterpolator>::New();
  interp->SetInputConnection(tdsc2->GetOutputPort());

  // ask for some specific data points
  vtkInformation* info = interp->GetOutputInformation(0);
  interp->UpdateInformation();
  double time = 0;
  int i;
  int j;

  bool expect_extended = false;
#ifdef VTK_USE_MEMKIND
  expect_extended = true;
#endif
  cout << "Expecting extended in this run ? " << (expect_extended ? "YES" : "NO")
       << endl; // reference to avoid comp warns
  for (j = 0; j < 5; ++j)
  {
    for (i = 0; i < 9; ++i)
    {
      time = i + 0.5;
      info->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), time);
      interp->Update();
      assert(sphere->GetOutput()->GetIsInMemkind() == false); // upstream pipeline can be anything
      assert(tdsc1->GetOutputDataObject(0)->GetIsInMemkind() ==
        expect_extended); // when enabled, output is extended because of temporalcache's
                          // CacheInMemkindOn()
      assert(tdsc2->GetOutputDataObject(0)->GetIsInMemkind() == expect_extended); // ditto
      assert(interp->GetOutputDataObject(0)->GetIsInMemkind() ==
        false); // downstream pipeline can be anything
    }
  }

  if (executecb->Count == 11)
  {
    cout << "Executed expected number of times." << endl;
    return 0;
  }
  else
  {
    cerr << "Upstream executed the wrong number of times " << executecb->Count << " instead of 11."
         << endl;
  }

  return 1;
}
