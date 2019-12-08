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
#include "vtkCompositePolyDataMapper.h"
#include "vtkDataArray.h"
#include "vtkDataSetMapper.h"
#include "vtkImageData.h"
#include "vtkImageDataToPointSet.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkUnsignedIntArray.h"

#include "vtkNew.h"
#include "vtkTestUtilities.h"

#include <sstream> // istringstream

int TestADIOS2BPReaderSingleTimeStep(int argc, char* argv[])
{
  vtkNew<vtkADIOS2CoreImageReader> reader;

  // Read the input data file
  char* filePath =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/ADIOS2/HeatMap3D/HeatMap3D.bp");

  if (!reader->CanReadFile(filePath))
  {
    std::cerr << "Cannot read file " << reader->GetFileName() << std::endl;
    return 0;
  }
  reader->SetFileName(filePath);
  delete[] filePath;

  reader->UpdateInformation();
  auto& availVars = reader->GetAvilableVariables();
  assert(availVars.size() == 2);
  // Get the dimension
  std::string varName = availVars.begin()->first;

  reader->SetOrigin(0.0, 0.0, 0.0);
  reader->SetSpacing(1.0, 1.0, 1.0);
  reader->SetDimensionArray("temperature");
  reader->SetActiveScalar({ "temperature", vtkADIOS2CoreImageReader::VarType::CellData });

  reader->Update();

  vtkSmartPointer<vtkMultiBlockDataSet> output =
    vtkMultiBlockDataSet::SafeDownCast(reader->GetOutput());
  assert(output->GetNumberOfBlocks() == 1);
  vtkSmartPointer<vtkMultiPieceDataSet> mpds =
    vtkMultiPieceDataSet::SafeDownCast(output->GetBlock(0));
  assert(mpds->GetNumberOfPieces() == 2);
  vtkSmartPointer<vtkImageData> image0 = vtkImageData::SafeDownCast(mpds->GetPiece(0));

  // Use vtkXMLPMultiBlockDataWriter if you want to dump the data

  // Render the first image for testing
  vtkNew<vtkDataSetMapper> mapper;
  mapper->SetInputDataObject(image0);
  mapper->ScalarVisibilityOn();
  mapper->SetScalarRange(0, 2000);
  mapper->SetScalarModeToUseCellData();
  mapper->ColorByArrayComponent("temperature", 0);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->EdgeVisibilityOn();

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->SetBackground(0.5, 0.5, 0.5);
  renderer->ResetCamera();
  renderer->GetActiveCamera()->Elevation(2000);

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
