/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTemporalXdmfReaderWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Description:
// This tests temporal reading and writing of static meshes using
// vtkXdmfReader and vtkXdmfWriter.

#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTestUtilities.h"
#include "vtkThreshold.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXdmfReader.h"
#include "vtkXdmfWriter.h"
#include "vtksys/SystemTools.hxx"

#define ASSERT_TEST(_cond_, _msg_) \
  if (!(_cond_)) { std::cerr << _msg_ << std::endl; return VTK_ERROR; }

int TestStaticMesh(vtkXdmfReader* reader)
{
  reader->UpdateInformation();

  vtkInformation* outInfo = reader->GetExecutive()->GetOutputInformation(0);

  int steps = (outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS())) ?
    outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS()) : 0;
  ASSERT_TEST(steps == 3, "Read data does not have 3 time steps as expected!");
  double* timeSteps =
      outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

  vtkPoints* geometryAtT0 = 0;
  vtkCellArray* topologyAtT0 = 0;
  for (int i = 0; i < steps; i++)
  {
    double updateTime = timeSteps[i];
    outInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(),
      updateTime);
    reader->Update();
    vtkMultiBlockDataSet* mb =
      vtkMultiBlockDataSet::SafeDownCast(reader->GetOutputDataObject(0));
    ASSERT_TEST(mb, "Root data is not a multiblock data set as expected!");
    ASSERT_TEST(mb->GetNumberOfBlocks() == 2, "Root multiblock data is supposed to have 2 blocks!");
    vtkUnstructuredGrid* grid =
      vtkUnstructuredGrid::SafeDownCast(mb->GetBlock(0));
    ASSERT_TEST(grid, "Block 0 is not an unstructured grid as expected!");
    if (i == 0)
    {
      geometryAtT0 = grid->GetPoints();
      topologyAtT0 = grid->GetCells();
    }

    ASSERT_TEST(grid->GetPoints() == geometryAtT0, "Geometry is not static over time as expected!");
    ASSERT_TEST(grid->GetCells() == topologyAtT0, "Topology is not static over time as expected!");
  }
  return 0;
}

int TestTemporalXdmfReaderWriter(int argc, char *argv[])
{
  // Read the input data file
  char *filePath = vtkTestUtilities::ExpandDataFileName(argc, argv,
    "Data/XDMF/temporalStaticMeshes.xmf");
  vtkNew<vtkXdmfReader> reader;
  reader->SetFileName(filePath);
  if (TestStaticMesh(reader.Get()) == VTK_ERROR)
  {
    std::cerr << "Error while reading " << reader->GetFileName() << std::endl;
    return VTK_ERROR;
  }

  // Write the input data to a new Xdmf file
  std::string outFilePath = "temporalStaticMeshesTest.xmf";
  vtkNew<vtkXdmfWriter> writer;
  writer->SetFileName(outFilePath.c_str());
  writer->WriteAllTimeStepsOn();
  writer->MeshStaticOverTimeOn();
  writer->SetInputConnection(reader->GetOutputPort());
  writer->Write();

  // Test written file
  vtkNew<vtkXdmfReader> reader2;
  reader2->SetFileName(outFilePath.c_str());
  if (TestStaticMesh(reader2.Get()) == VTK_ERROR)
  {
      std::cerr << "Error while reading " << reader2->GetFileName() << std::endl;
    return VTK_ERROR;
  }

  return 0;
}
