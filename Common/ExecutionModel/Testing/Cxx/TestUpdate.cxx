// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// This test verifies that Update* method returns expected values

#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkExecutive.h"
#include "vtkPolyDataAlgorithm.h"

#include <iostream>

namespace
{
class MySource : public vtkPolyDataAlgorithm
{
public:
  static MySource* New();
  vtkTypeMacro(vtkPolyDataAlgorithm, vtkAlgorithm);

  vtkSetMacro(Fail, bool);

protected:
  MySource()
  {
    this->SetNumberOfInputPorts(0);
    this->SetNumberOfOutputPorts(1);
  }

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override
  {
    return this->Fail ? 0 : 1;
  }

private:
  bool Fail = false;
};

vtkStandardNewMacro(MySource);
}

int TestUpdate(int, char*[])
{
  vtkNew<::MySource> source;

  if (!source->UpdateInformation())
  {
    std::cerr << "Unexpected failure on UpdateInformation()" << std::endl;
    return EXIT_FAILURE;
  }

  if (!source->UpdateDataObject())
  {
    std::cerr << "Unexpected failure on UpdateDataObject()" << std::endl;
    return EXIT_FAILURE;
  }

  if (!source->UpdateWholeExtent())
  {
    std::cerr << "Unexpected failure on UpdateWholeExtent()" << std::endl;
    return EXIT_FAILURE;
  }

  if (!source->Update())
  {
    std::cerr << "Unexpected failure on Update()" << std::endl;
    return EXIT_FAILURE;
  }

  vtkNew<vtkCallbackCommand> nullEventCallback;
  source->GetExecutive()->AddObserver(vtkCommand::ErrorEvent, nullEventCallback);
  source->SetFail(true);
  if (source->Update())
  {
    std::cerr << "Unexpected success on Update()" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
