// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDataSet.h"
#include "vtkFieldData.h"
#include "vtkHDFReader.h"
#include "vtkInformation.h"
#include "vtkMergeBlocks.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTecplotReader.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtksys/SystemTools.hxx"

#include <iostream>
#include <string>

int TestTecplotReader3(int argc, char* argv[])
{
  char* dataRoot = vtkTestUtilities::GetDataRoot(argc, argv);
  const std::string tecplotDir = std::string(dataRoot) + "/Data/TecPlot/";

  if (argc < 2)
  {
    return EXIT_SUCCESS;
  }

  const char* filename = argv[1];

  vtkNew<vtkTecplotReader> tecplotReader;
  tecplotReader->SetFileName((tecplotDir + filename).c_str());
  tecplotReader->UpdateInformation();

  vtkInformation* outInfo = tecplotReader->GetOutputInformation(0);
  int numTimeSteps = outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  std::vector<double> timeSteps(numTimeSteps);
  outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), timeSteps.data());

  vtkMultiBlockDataSet* ds = tecplotReader->GetOutput();
  if (ds == nullptr)
  {
    std::cerr << "Failed to read data set from " << filename << std::endl;
    return EXIT_FAILURE;
  }

  const std::string fileNameHdf =
    vtksys::SystemTools::GetFilenameWithoutExtension(filename) + ".vtkhdf";
  vtkNew<vtkHDFReader> hdfReader;
  hdfReader->SetFileName((tecplotDir + fileNameHdf).c_str());
  hdfReader->Update();

  // Needed because there are issues writing multi block with multiple time steps (see
  // https://gitlab.kitware.com/vtk/vtk/-/work_items/20172)
  vtkNew<vtkMergeBlocks> mergeBlocks;
  mergeBlocks->SetInputConnection(tecplotReader->GetOutputPort());
  mergeBlocks->SetOutputDataSetType(VTK_UNSTRUCTURED_GRID);
  mergeBlocks->SetMergePoints(false);
  mergeBlocks->SetMergePartitionsOnly(false);

  for (double timeStep : timeSteps)
  {
    tecplotReader->UpdateTimeStep(timeStep);
    mergeBlocks->UpdateTimeStep(timeStep);

    vtkUnstructuredGrid* mergeBlockOutput =
      vtkUnstructuredGrid::SafeDownCast(mergeBlocks->GetOutput());

    hdfReader->UpdateTimeStep(timeStep);

    // Since there are multiple time steps, we need to remove the "Time" field data
    vtkDataSet* hdfOutput = vtkDataSet::SafeDownCast(hdfReader->GetOutput());
    if (hdfOutput && hdfOutput->GetFieldData())
    {
      hdfOutput->GetFieldData()->RemoveArray("Time");
    }

    if (!vtkTestUtilities::CompareDataObjects(hdfOutput, mergeBlockOutput))
    {
      delete[] dataRoot;
      return EXIT_FAILURE;
    }
  }

  delete[] dataRoot;

  return EXIT_SUCCESS;
}
