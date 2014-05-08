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

// This test verifies that information keys are copied up & down the
// pipeline properly and NeedToExecute/StoreMetaData functions as expected.

#include "vtkInformation.h"
#include "vtkInformationDataObjectMetaDataKey.h"
#include "vtkInformationIntegerRequestKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPolyDataAlgorithm.h"
#include "vtkPolyDataNormals.h"

#define TEST_SUCCESS 0
#define TEST_FAILURE 1

class MySource : public vtkPolyDataAlgorithm
{
public:
  static MySource *New();
  vtkTypeMacro(vtkPolyDataAlgorithm,vtkAlgorithm);

  static vtkInformationDataObjectMetaDataKey* META_DATA();
  static vtkInformationIntegerRequestKey* REQUEST();
  static vtkInformationIntegerKey* DATA();

  bool Failed;
  unsigned int NumberOfExecutions;
  int Result;

protected:
  MySource()
  {
    this->SetNumberOfInputPorts(0);
    this->SetNumberOfOutputPorts(1);
    this->Failed = false;
    this->NumberOfExecutions = 0;
    this->Result = -1;
  }

  virtual int RequestInformation(vtkInformation*,
                                 vtkInformationVector**,
                                 vtkInformationVector* outputVector)
  {
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    vtkPolyData* pd = vtkPolyData::New();
    outInfo->Set(META_DATA(), pd);
    pd->Delete();
    return 1;
  }
  virtual int RequestData(vtkInformation*,
                          vtkInformationVector**,
                          vtkInformationVector* outputVector)
  {
    // Here we verify that a request set at the end of the pipeline
    // made it to here properly.
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    if (!outInfo->Has(REQUEST()) ||
        outInfo->Get(REQUEST()) != this->Result)
      {
      this->Failed = true;
      }
    this->NumberOfExecutions++;
    return 1;
  }
};

vtkStandardNewMacro(MySource);

vtkInformationKeyMacro(MySource, META_DATA, DataObjectMetaData);
vtkInformationKeyMacro(MySource, DATA, Integer);

class vtkInformationMyRequestKey : public vtkInformationIntegerRequestKey
{
public:
  vtkInformationMyRequestKey(const char* name, const char* location) :
    vtkInformationIntegerRequestKey(name, location)
    {
    this->DataKey = MySource::DATA();
    }
};
vtkInformationKeySubclassMacro(MySource, REQUEST, MyRequest, IntegerRequest);

int TestMetaData(int, char*[])
{
  vtkNew<MySource> mySource;
  vtkNew<vtkPolyDataNormals> filter;

  filter->SetInputConnection(mySource->GetOutputPort());

  filter->UpdateInformation();

  // Do we have the meta-data created by the reader at the end
  // of the pipeline?
  if (!filter->GetOutputInformation(0)->Has(MySource::META_DATA()))
    {
    return TEST_FAILURE;
    }

  filter->GetOutputInformation(0)->Set(MySource::REQUEST(), 2);
  mySource->Result = 2;

  filter->Update();
  // Nothing changed. This should not cause re-execution
  filter->Update();

  filter->GetOutputInformation(0)->Set(MySource::REQUEST(), 3);
  mySource->Result = 3;

  // Request changed. This should cause re-execution
  filter->Update();

  if (mySource->NumberOfExecutions != 2)
    {
    return TEST_FAILURE;
    }

  if (mySource->Failed)
    {
    return TEST_FAILURE;
    }
  return TEST_SUCCESS;
}
