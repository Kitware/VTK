/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestResampleWithDataset3.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkResampleWithDataSet.h"

#include "vtkActor.h"
#include "vtkCellData.h"
#include "vtkCompositeDataGeometryFilter.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkCylinder.h"
#include "vtkExtentTranslator.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkRandomAttributeGenerator.h"
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


namespace {

void CreateInputDataSet(vtkMultiBlockDataSet* dataset, int numberOfBlocks)
{
  dataset->SetNumberOfBlocks(numberOfBlocks);

  vtkNew<vtkExtentTranslator> extentTranslator;
  extentTranslator->SetWholeExtent(-16, 16, -16, 16, -16, 16);
  extentTranslator->SetNumberOfPieces(numberOfBlocks);
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

  vtkNew<vtkRandomAttributeGenerator> randomAttrs;
  randomAttrs->SetInputConnection(transFilter->GetOutputPort());
  randomAttrs->GenerateAllPointDataOn();
  randomAttrs->GenerateAllCellDataOn();
  randomAttrs->GenerateFieldArrayOn();
  randomAttrs->SetNumberOfTuples(100);

  for (int i = 0; i < numberOfBlocks; ++i)
  {
    int blockExtent[6];
    extentTranslator->SetPiece(i);
    extentTranslator->PieceToExtent();
    extentTranslator->GetExtent(blockExtent);

    wavelet->UpdateExtent(blockExtent);
    clipCyl->SetInputData(wavelet->GetOutputDataObject(0));
    randomAttrs->Update();

    vtkDataObject *block = randomAttrs->GetOutputDataObject(0)->NewInstance();
    block->DeepCopy(randomAttrs->GetOutputDataObject(0));
    dataset->SetBlock(i, block);
    block->Delete();
  }
}

void CreateSourceDataSet(vtkMultiBlockDataSet* dataset, int numberOfBlocks)
{
  dataset->SetNumberOfBlocks(numberOfBlocks);

  vtkNew<vtkExtentTranslator> extentTranslator;
  extentTranslator->SetWholeExtent(-22, 22, -22, 22, -16, 16);
  extentTranslator->SetNumberOfPieces(numberOfBlocks);
  extentTranslator->SetSplitModeToBlock();

  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-22, 22, -22, 22, -16, 16);
  wavelet->SetCenter(0, 0, 0);

  vtkNew<vtkThreshold> threshold;
  threshold->SetInputConnection(wavelet->GetOutputPort());
  threshold->ThresholdByLower(185.0);

  for (int i = 0; i < numberOfBlocks; ++i)
  {
    int blockExtent[6];
    extentTranslator->SetPiece(i);
    extentTranslator->PieceToExtent();
    extentTranslator->GetExtent(blockExtent);

    wavelet->UpdateExtent(blockExtent);
    threshold->Update();

    vtkDataObject *block = threshold->GetOutputDataObject(0)->NewInstance();
    block->DeepCopy(threshold->GetOutputDataObject(0));
    dataset->SetBlock(i, block);
    block->Delete();
  }
}

} // anonymous namespace


int TestResampleWithDataSet3(int argc, char *argv[])
{
  // create input dataset
  vtkNew<vtkMultiBlockDataSet> input;
  CreateInputDataSet(input.GetPointer(), 3);

  vtkNew<vtkMultiBlockDataSet> source;
  CreateSourceDataSet(source.GetPointer(), 5);

  vtkNew<vtkResampleWithDataSet> resample;
  resample->SetInputData(input.GetPointer());
  resample->SetSourceData(source.GetPointer());


  vtkMultiBlockDataSet *result;
  vtkDataSet *block0;

  // Test that ghost arrays are not generated
  resample->MarkBlankPointsAndCellsOff();
  resample->Update();
  result = vtkMultiBlockDataSet::SafeDownCast(resample->GetOutput());
  block0 = vtkDataSet::SafeDownCast(result->GetBlock(0));
  if (block0->GetPointGhostArray() || block0->GetCellGhostArray())
  {
    std::cout << "Error: ghost arrays were generated with MarkBlankPointsAndCellsOff()" << std::endl;
    return !vtkTesting::FAILED;
  }

  // Test that ghost arrays are generated
  resample->MarkBlankPointsAndCellsOn();
  resample->Update();
  result = vtkMultiBlockDataSet::SafeDownCast(resample->GetOutput());
  block0 = vtkDataSet::SafeDownCast(result->GetBlock(0));
  if (!block0->GetPointGhostArray() || !block0->GetCellGhostArray())
  {
    std::cout << "Error: no ghost arrays generated with MarkBlankPointsAndCellsOn()" << std::endl;
    return !vtkTesting::FAILED;
  }

  // Render
  double scalarRange[2];
  vtkNew<vtkCompositeDataGeometryFilter> toPoly;
  toPoly->SetInputConnection(resample->GetOutputPort());
  toPoly->Update();
  toPoly->GetOutput()->GetPointData()->GetArray("RTData")->GetRange(scalarRange);

  vtkNew<vtkCompositePolyDataMapper> mapper;
  mapper->SetInputConnection(toPoly->GetOutputPort());
  mapper->SetScalarRange(scalarRange);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper.GetPointer());

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor.GetPointer());
  renderer->ResetCamera();

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer.GetPointer());

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());
  iren->Initialize();

  renWin->Render();
  int retVal = vtkRegressionTestImage(renWin.GetPointer());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
