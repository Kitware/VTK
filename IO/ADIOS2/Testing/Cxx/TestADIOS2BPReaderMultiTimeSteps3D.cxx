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
// This tests reading of a simple ADIOS2 bp file.

#include "vtkADIOS2CoreImageReader.h"

#include "vtkActor.h"
#include "vtkAlgorithm.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkColorTransferFunction.h"
#include "vtkCompositeRenderManager.h"
#include "vtkDataArray.h"
#include "vtkDataSetMapper.h"
#include "vtkExecutive.h"
#include "vtkImageData.h"
#include "vtkImageDataToPointSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiBlockVolumeMapper.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedIntArray.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkXMLPMultiBlockDataWriter.h"

#include "vtkNew.h"
#include "vtkTestUtilities.h"

#include <sstream> // istringstream

int TestADIOS2BPReaderMultiTimeSteps3D(int argc, char* argv[])
{
  vtkNew<vtkADIOS2CoreImageReader> reader;

  // Read the input data file
  char* filePath =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/ADIOS2/3D_7-point_24-step/gs.bp");

  if (!reader->CanReadFile(filePath))
  {
    std::cerr << "Cannot read file " << reader->GetFileName() << std::endl;
    return 0;
  }
  reader->SetFileName(filePath);
  delete[] filePath;

  reader->UpdateInformation();
  auto& availVars = reader->GetAvilableVariables();
  assert(availVars.size() == 3);
  // Get the dimension
  std::string varName = availVars.begin()->first;

  // Enable multi time stesp
  reader->SetTimeStepArray("step");
  reader->SetDimensionArray("U");
  reader->SetArrayStatus("step", false);

  reader->SetActiveScalar(std::make_pair("U", vtkADIOS2CoreImageReader::VarType::CellData));
  reader->Update();

  vtkSmartPointer<vtkMultiBlockDataSet> output =
    vtkMultiBlockDataSet::SafeDownCast(reader->GetOutput());
  assert(output->GetNumberOfBlocks() == 1);
  vtkSmartPointer<vtkMultiPieceDataSet> mpds =
    vtkMultiPieceDataSet::SafeDownCast(output->GetBlock(0));
  assert(mpds->GetNumberOfPieces() == 6);
  vtkSmartPointer<vtkImageData> image0 = vtkImageData::SafeDownCast(mpds->GetPiece(0));
  vtkSmartPointer<vtkImageData> image1 = vtkImageData::SafeDownCast(mpds->GetPiece(1));

  // Use vtkXMLPMultiBlockDataWriter + vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()
  // to write out the data if needed

  vtkNew<vtkImageDataToPointSet> imageToPointset;

  assert(image0->GetCellData()->GetNumberOfArrays() == 2);
  image0->GetCellData()->SetActiveScalars("U");
  imageToPointset->SetInputData(image0);

  imageToPointset->Update();

  // Since I fail to find a proper mapper to render two vtkImageDatas inside
  // a vtkMultiPieceDataSet in a vtkMultiBlockDataSet, I render the image directly here
  vtkNew<vtkDataSetMapper> mapper;
  mapper->SetInputDataObject(imageToPointset->GetOutput());
  mapper->ScalarVisibilityOn();
  mapper->SetScalarRange(0, 2000);
  mapper->SetScalarModeToUseCellData();
  mapper->ColorByArrayComponent("U", 0);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->EdgeVisibilityOn();

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->SetBackground(0.5, 0.5, 0.5);
  renderer->GetActiveCamera()->Elevation(300);
  renderer->GetActiveCamera()->Yaw(60);
  renderer->ResetCamera();

  vtkNew<vtkRenderWindow> rendWin;
  rendWin->SetSize(600, 300);
  rendWin->AddRenderer(renderer);

  rendWin->Render();

  // Do the test comparsion
  int retval = vtkRegressionTestImage(rendWin);
  if (retval == vtkRegressionTester::DO_INTERACTOR)
  {
    vtkNew<vtkRenderWindowInteractor> iren;
    iren->SetRenderWindow(rendWin);
    iren->Initialize();
    iren->Start();
    retval = vtkRegressionTester::PASSED;
  }

  return !retval;
}
