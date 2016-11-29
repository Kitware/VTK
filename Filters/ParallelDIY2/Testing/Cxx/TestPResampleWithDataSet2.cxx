/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestResampleWithDataset.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPResampleWithDataSet.h"

#include "vtkActor.h"
#include "vtkBoundingBox.h"
#include "vtkCompositeDataGeometryFilter.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkCompositeRenderManager.h"
#include "vtkCylinder.h"
#include "vtkExtentTranslator.h"
#include "vtkImageData.h"
#include "vtkMPIController.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRTAnalyticSource.h"
#include "vtkSphere.h"
#include "vtkTableBasedClipDataSet.h"
#include "vtkThreshold.h"
#include "vtkTransform.h"
#include "vtkTransformFilter.h"

#include <algorithm>
#include <cmath>


namespace {

void CreateSourceDataSet(vtkMultiBlockDataSet* dataset, int rank, int numberOfProcs,
                        int blocksPerProc)
{
  int numPieces = blocksPerProc * numberOfProcs;
  dataset->SetNumberOfBlocks(numPieces);

  vtkNew<vtkExtentTranslator> extentTranslator;
  extentTranslator->SetWholeExtent(-16, 16, -16, 16, -16, 16);
  extentTranslator->SetNumberOfPieces(numPieces);
  extentTranslator->SetSplitModeToBlock();

  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-16, 16, -16, 16, -16, 16);
  wavelet->SetCenter(0, 0, 0);

  vtkNew<vtkCylinder> cylinder;
  cylinder->SetCenter(0, 0, 0);
  cylinder->SetRadius(15);
  cylinder->SetAxis(0, 1, 0);
  vtkNew<vtkTableBasedClipDataSet> clipCyl;
  clipCyl->SetClipFunction(cylinder.GetPointer());
  clipCyl->InsideOutOn();

  vtkNew<vtkSphere> sphere;
  sphere->SetCenter(0, 0, 4);
  sphere->SetRadius(12);
  vtkNew<vtkTableBasedClipDataSet> clipSphr;
  clipSphr->SetInputConnection(clipCyl->GetOutputPort());
  clipSphr->SetClipFunction(sphere.GetPointer());

  vtkNew<vtkTransform> transform;
  transform->RotateZ(45);
  vtkNew<vtkTransformFilter> transFilter;
  transFilter->SetInputConnection(clipSphr->GetOutputPort());
  transFilter->SetTransform(transform.GetPointer());

  for (int i = 0; i < blocksPerProc; ++i)
  {
    int piece = (rank * blocksPerProc) + i;

    int blockExtent[6];
    extentTranslator->SetPiece(piece);
    extentTranslator->PieceToExtent();
    extentTranslator->GetExtent(blockExtent);

    wavelet->UpdateExtent(blockExtent);
    clipCyl->SetInputData(wavelet->GetOutputDataObject(0));
    transFilter->Update();

    vtkDataObject *block = transFilter->GetOutputDataObject(0)->NewInstance();
    block->DeepCopy(transFilter->GetOutputDataObject(0));
    dataset->SetBlock(piece, block);
    block->Delete();
  }
}

void CreateInputDataSet(vtkMultiBlockDataSet* dataset, const double bounds[6],
                        int rank, int numberOfProcs, int numberOfBlocks)
{
  static const int dims[] = { 96, 32, 64 };
  dataset->SetNumberOfBlocks(numberOfBlocks);

  double size[3] = { bounds[1] - bounds[0], bounds[3] - bounds[2],
                     (bounds[5] - bounds[4])/static_cast<double>(numberOfBlocks) };
  for (int i = 0; i < numberOfBlocks; ++i)
  {
    double origin[3] = { bounds[0], bounds[2], bounds[4] + static_cast<double>(i)*size[2] };
    double spacing = (*std::max_element(size, size+3))/static_cast<double>(dims[i%3]);

    int extent[6];
    extent[0] = 0;
    extent[1] = static_cast<int>(size[0]/spacing) - 1;
    extent[2] = rank * (static_cast<int>(size[1]/spacing)/numberOfProcs);
    extent[3] = extent[2] + (static_cast<int>(size[1]/spacing)/numberOfProcs);
    extent[4] = 0;
    extent[5] = static_cast<int>(std::ceil(size[2]/spacing));

    vtkNew<vtkImageData> img;
    img->SetExtent(extent);
    img->SetOrigin(origin);
    img->SetSpacing(spacing, spacing, spacing);
    dataset->SetBlock(i, img.GetPointer());
  }
}

void ComputeGlobalBounds(vtkMultiBlockDataSet* dataset,
                         vtkMultiProcessController *controller,
                         double bounds[6])
{
  vtkBoundingBox bb;
  for (unsigned i = 0; i < dataset->GetNumberOfBlocks(); ++i)
  {
    vtkDataSet *block = vtkDataSet::SafeDownCast(dataset->GetBlock(i));
    if (block)
    {
      bb.AddBounds(block->GetBounds());
    }
  }

  double lbmin[3], lbmax[3];
  bb.GetBounds(lbmin[0], lbmax[0], lbmin[1], lbmax[1], lbmin[2], lbmax[2]);

  double gbmin[3], gbmax[3];
  controller->AllReduce(lbmin, gbmin, 3, vtkCommunicator::MIN_OP);
  controller->AllReduce(lbmax, gbmax, 3, vtkCommunicator::MAX_OP);

  for (int i = 0; i < 3; ++i)
  {
    bounds[2*i] = gbmin[i];
    bounds[2*i + 1] = gbmax[i];
  }
}

} // anonymous namespace


int TestPResampleWithDataSet2(int argc, char *argv[])
{
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv);

  int numProcs = controller->GetNumberOfProcesses();
  int rank = controller->GetLocalProcessId();

  // create source and input datasets
  vtkNew<vtkMultiBlockDataSet> source;
  CreateSourceDataSet(source.GetPointer(), rank, numProcs, 5);

  // compute full bounds of source dataset
  double bounds[6];
  ComputeGlobalBounds(source.GetPointer(), controller.GetPointer(), bounds);

  vtkNew<vtkMultiBlockDataSet> input;
  CreateInputDataSet(input.GetPointer(), bounds, rank, numProcs, 3);


  vtkNew<vtkPResampleWithDataSet> resample;
  resample->SetController(controller.GetPointer());
  resample->SetInputData(input.GetPointer());
  resample->SetSourceData(source.GetPointer());
  resample->Update();

  // Render
  vtkNew<vtkThreshold> threshold;
  threshold->SetInputConnection(resample->GetOutputPort());
  threshold->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS,
                                    "vtkValidPointMask");
  threshold->ThresholdByUpper(1);

  vtkNew<vtkCompositeDataGeometryFilter> toPoly;
  toPoly->SetInputConnection(threshold->GetOutputPort());

  double range[2];
  toPoly->Update();
  toPoly->GetOutput()->GetPointData()->GetArray("RTData")->GetRange(range);

  vtkNew<vtkCompositePolyDataMapper> mapper;
  mapper->SetInputConnection(toPoly->GetOutputPort());
  mapper->SetScalarRange(range);


  // Setup parallel rendering
  vtkNew<vtkCompositeRenderManager> prm;
  vtkSmartPointer<vtkRenderer> renderer =
    vtkSmartPointer<vtkRenderer>::Take(prm->MakeRenderer());
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::Take(prm->MakeRenderWindow());
  renWin->AddRenderer(renderer.GetPointer());
  renWin->DoubleBufferOn();
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  prm->SetRenderWindow(renWin.GetPointer());
  prm->SetController(controller.GetPointer());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper.GetPointer());
  renderer->AddActor(actor.GetPointer());

  int retVal;
  if (rank == 0)
  {
    prm->ResetAllCameras();
    renWin->Render();
    retVal = vtkRegressionTester::Test(argc, argv, renWin.GetPointer(), 20);
    if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
      prm->StartInteractor();
    }
    controller->TriggerBreakRMIs();
  }
  else
  {
    prm->StartServices();
  }
  controller->Barrier();

  controller->Broadcast(&retVal, 1, 0);
  controller->Finalize();

  return !retVal;
}
