// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include <vtkDataSet.h>
#include <vtkGenerateTimeSteps.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkNew.h>
#include <vtkPolyData.h>
#include <vtkSphereSource.h>
#include <vtkStreamingDemandDrivenPipeline.h>

//------------------------------------------------------------------------------
// Program main
int TestGenerateTimeSteps(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkGenerateTimeSteps> genTime;
  genTime->SetInputConnection(sphere->GetOutputPort());
  genTime->GenerateTimeStepValues(0, 10, 2);

  // Test all timestep API
  int numTimes = genTime->GetNumberOfTimeSteps();
  if (numTimes != 5)
  {
    std::cerr << "Unexpected number of TimeSteps:" << numTimes << std::endl;
    return EXIT_FAILURE;
  }

  std::vector<double> timesVec(numTimes);
  genTime->GetTimeStepValues(timesVec.data());
  for (int i = 0; i < numTimes; i++)
  {
    if (timesVec[i] != i * 2)
    {
      std::cerr << "Unexpected timestep for " << i << ": " << timesVec[i] << std::endl;
      return EXIT_FAILURE;
    }
  }

  timesVec.emplace_back(10);
  genTime->SetTimeStepValues(6, timesVec.data());
  genTime->AddTimeStepValue(12);
  genTime->UpdateInformation();

  vtkInformation* outInfo = genTime->GetOutputInformation(0);
  double* times = outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  numTimes = outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  double* range = outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_RANGE());

  if (numTimes != 7)
  {
    std::cerr << "Unexpected number of TimeSteps:" << numTimes << std::endl;
    return EXIT_FAILURE;
  }
  for (int i = 0; i < numTimes; i++)
  {
    if (times[i] != i * 2)
    {
      std::cerr << "Unexpected timestep for " << i << ": " << times[i] << std::endl;
      return EXIT_FAILURE;
    }
  }
  if (range[0] != 0 || range[1] != 12)
  {
    std::cerr << "Unexpected timestep range: " << range[0] << " " << range[1] << std::endl;
    return EXIT_FAILURE;
  }

  outInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), 2);
  genTime->Update();

  vtkDataSet* ds = vtkDataSet::SafeDownCast(genTime->GetOutput());
  if (ds->GetNumberOfPoints() != sphere->GetOutput()->GetNumberOfPoints())
  {
    std::cerr << "Unexpected output data" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
