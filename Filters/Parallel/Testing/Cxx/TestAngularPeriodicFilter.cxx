/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDistancePolyDataFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "vtkActor.h"
#include "vtkAngularPeriodicFilter.h"
#include "vtkCamera.h"
#include "vtkGeometryFilter.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkLookupTable.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPointSource.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkStreamTracer.h"
#include "vtkTesting.h"
#include "vtkTriangleFilter.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLUnstructuredGridReader.h"


int TestAngularPeriodicFilter(int argc, char* argv[])
{
  vtkNew<vtkXMLUnstructuredGridReader> reader;

  vtkNew<vtkTesting> testHelper;
  testHelper->AddArguments(argc, argv);
  if (!testHelper->IsFlagSpecified("-D"))
  {
    std::cerr << "Error : -D /path/to/data was not specified.";
    return EXIT_FAILURE;
  }
  std::string dataRoot = testHelper->GetDataRoot();
  std::string inputFileName = dataRoot + "/Data/periodicPiece.vtu";
  reader->SetFileName(inputFileName.c_str());
  reader->Update();

  vtkNew<vtkMultiBlockDataSet> mb;
  mb->SetNumberOfBlocks(1);
  mb->SetBlock(0, reader->GetOutput());

  vtkNew<vtkAngularPeriodicFilter> angularPeriodicFilter;
  angularPeriodicFilter->SetInputData(mb.Get());
  angularPeriodicFilter->AddIndex(1);
  angularPeriodicFilter->SetIterationModeToMax();
  angularPeriodicFilter->SetRotationModeToDirectAngle();
  angularPeriodicFilter->SetRotationAngle(45.);
  angularPeriodicFilter->SetRotationAxisToZ();

  vtkNew<vtkGeometryFilter> geomFilter;
  geomFilter->SetInputData(mb.Get());

  vtkNew<vtkTriangleFilter> triangleFilter;
  triangleFilter->SetInputConnection(geomFilter->GetOutputPort());

  vtkNew<vtkPointSource> seed;
  seed->SetCenter(5.80752824733665, -3.46144284193073, -5.83410675177451);
  seed->SetNumberOfPoints(1);
  seed->SetRadius(2);

  vtkNew<vtkStreamTracer> streamTracer;
  streamTracer->SetInputConnection(angularPeriodicFilter->GetOutputPort());
  streamTracer->SetInputArrayToProcess(0, 0, 0, 0, "Result");
  streamTracer->SetInterpolatorType(0);
  streamTracer->SetIntegrationDirection(2);
  streamTracer->SetIntegratorType(2);
  streamTracer->SetIntegrationStepUnit(2);
  streamTracer->SetInitialIntegrationStep(0.2);
  streamTracer->SetMinimumIntegrationStep(0.01);
  streamTracer->SetMaximumIntegrationStep(0.5);
  streamTracer->SetMaximumNumberOfSteps(2000);
  streamTracer->SetMaximumPropagation(28.);
  streamTracer->SetTerminalSpeed(0.000000000001);
  streamTracer->SetMaximumError(0.000001);
  streamTracer->SetComputeVorticity(1);

  streamTracer->SetSourceConnection(seed->GetOutputPort());
  streamTracer->Update();

  vtkPolyData* pd = streamTracer->GetOutput();
  pd->GetPointData()->SetActiveScalars("RTData");

  vtkNew<vtkLookupTable> hueLut;
  hueLut->SetHueRange(0., 1.);
  hueLut->SetSaturationRange(1., 1.);
  hueLut->Build();

  vtkNew<vtkCompositePolyDataMapper> multiBlockMapper;
  multiBlockMapper->SetInputConnection(triangleFilter->GetOutputPort());
  multiBlockMapper->SetLookupTable(hueLut.Get());
  multiBlockMapper->SetScalarRange(131., 225.);
  multiBlockMapper->SetColorModeToMapScalars();
  multiBlockMapper->SetScalarModeToUsePointData();

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(streamTracer->GetOutputPort());
  mapper->SetLookupTable(hueLut.Get());
  mapper->SetScalarRange(131., 225.);
  mapper->SetColorModeToMapScalars();
  mapper->SetScalarModeToUsePointData();

  vtkNew<vtkActor> multiBlockActor;
  multiBlockActor->SetMapper(multiBlockMapper.Get());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper.Get());

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(multiBlockActor.GetPointer());
  renderer->AddActor(actor.GetPointer());
  renderer->GetActiveCamera()->SetPosition(3.97282457351685, -0.0373859405517578, -59.3025624847687);
  renderer->ResetCamera();
  renderer->SetBackground(1., 1., 1.);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer.GetPointer());
  renWin->SetMultiSamples(0);
  renWin->SetSize(300, 300);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  int retVal = vtkRegressionTestImage(renWin.GetPointer());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
