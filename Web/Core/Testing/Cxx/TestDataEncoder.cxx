/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDataEncoder.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <vtkDataEncoder.h>
#include <vtkImageCast.h>
#include <vtkImageData.h>
#include <vtkImageMandelbrotSource.h>
#include <vtkLogger.h>
#include <vtkNew.h>
#include <vtkSmartPointer.h>

#include <chrono>
#include <thread>
#include <vector>

vtkSmartPointer<vtkImageData> GetData()
{
  vtkNew<vtkImageMandelbrotSource> source;
  source->SetWholeExtent(0, 256, 0, 256, 0, 0);

  vtkNew<vtkImageCast> caster;
  caster->SetInputConnection(source->GetOutputPort());
  caster->SetOutputScalarTypeToUnsignedChar();
  caster->Update();
  return caster->GetOutput();
}

bool TestCreate()
{
  vtkLogScopeFunction(INFO);
  //--------------------------------------------------------------
  // Create a bunch of instances and ensure it doesn't cause issues
  // #18344
  for (int cc = 0; cc < 100; cc++)
  {
    vtkNew<vtkDataEncoder> encoder;
  }

  std::vector<vtkSmartPointer<vtkDataEncoder>> encoders;
  encoders.reserve(100);
  for (int cc = 0; cc < 100; cc++)
  {
    encoders.push_back(vtk::TakeSmartPointer(vtkDataEncoder::New()));
  }
  return true;
}

bool TestFlush()
{
  vtkLogScopeFunction(INFO);
  constexpr int KEY = 1020;

  vtkNew<vtkDataEncoder> encoder;
  encoder->SetMaxThreads(5);
  encoder->Initialize();

  // call flush without pushing any data.
  encoder->Flush(KEY);

  // push some data and then call flush.
  for (int cc = 0; cc < 10; cc++)
  {
    encoder->Push(KEY, GetData(), 50);
  }

  encoder->Flush(KEY);

  // call flush again.
  encoder->Flush(KEY);

  // push some data and then call flush.
  for (int cc = 0; cc < 10; cc++)
  {
    encoder->Push(KEY, GetData(), 50);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  encoder->Flush(KEY);

  return true;
}

bool TestLatestOutput()
{
  vtkLogScopeFunction(INFO);
  constexpr int KEY = 1020;

  vtkNew<vtkDataEncoder> encoder;

  vtkSmartPointer<vtkUnsignedCharArray> result;
  if (encoder->GetLatestOutput(KEY, result))
  {
    vtkLogF(ERROR, "no output expected!");
    return false;
  }

  // push some data and then call flush.
  for (int cc = 0; cc < 10; cc++)
  {
    encoder->Push(KEY, GetData(), 50);
  }

  encoder->Flush(KEY);
  if (!encoder->GetLatestOutput(KEY, result))
  {
    vtkLogF(ERROR, "latest output expected!");
    return false;
  }

  return true;
}

int TestDataEncoder(int /*argc*/, char* /*argv*/[])
{
  TestCreate();
  TestFlush();
  TestLatestOutput();
  return EXIT_SUCCESS;
}
