// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// This test verifies that information keys are copied up & down the
// pipeline properly and NeedToExecute/StoreMetaData functions as expected.

#include "vtkErrorCode.h"
#include "vtkExecutive.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPolyDataAlgorithm.h"
#include "vtkTestErrorObserver.h"

#define TEST_SUCCESS 0
#define TEST_FAILURE 1

class TestErrorCode_MySource : public vtkPolyDataAlgorithm
{
public:
  static TestErrorCode_MySource* New();
  vtkTypeMacro(vtkPolyDataAlgorithm, vtkAlgorithm);

protected:
  TestErrorCode_MySource()
  {
    this->SetNumberOfInputPorts(0);
    this->SetNumberOfOutputPorts(1);
  }

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override
  {
    this->SetErrorCode(vtkErrorCode::UnknownError);
    return 0;
  }
};

vtkStandardNewMacro(TestErrorCode_MySource);

int TestErrorCode(int, char*[])
{
  vtkNew<TestErrorCode_MySource> mySource;
  vtkNew<vtkTest::ErrorObserver> errorObserver;
  mySource->GetExecutive()->AddObserver(vtkCommand::ErrorEvent, errorObserver);

  if (mySource->Update())
  {
    std::cerr << "Unexpected success on Update()" << std::endl;
    return TEST_FAILURE;
  }

  errorObserver->CheckErrorMessage("returned failure for request");

  if (mySource->GetErrorCode() != vtkErrorCode::UnknownError)
  {
    std::cerr << "Unexpected error code after Update()" << std::endl;
    return TEST_FAILURE;
  }
  return TEST_SUCCESS;
}
