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

  for (int i = 0; i < numberOfBlocks; ++i)
  {
    int blockExtent[6];
    extentTranslator->SetPiece(i);
    extentTranslator->PieceToExtent();
    extentTranslator->GetExtent(blockExtent);

    wavelet->UpdateExtent(blockExtent);

    vtkDataObject *block = wavelet->GetOutputDataObject(0)->NewInstance();
    block->DeepCopy(wavelet->GetOutputDataObject(0));
    dataset->SetBlock(i, block);
    block->Delete();
  }
}

} // anonymous namespace


int TestResampleWithDataSet(int argc, char *argv[])
{
  // create input dataset
  vtkNew<vtkMultiBlockDataSet> input;
  CreateInputDataSet(input.GetPointer(), 3);

  vtkNew<vtkMultiBlockDataSet> source;
  CreateSourceDataSet(source.GetPointer(), 5);

  vtkNew<vtkResampleWithDataSet> resample;
  resample->SetInputData(input.GetPointer());
  resample->SetSourceData(source.GetPointer());

  // test default output
  resample->Update();
  vtkMultiBlockDataSet *result = static_cast<vtkMultiBlockDataSet*>(resample->GetOutput());
  vtkDataSet *block = static_cast<vtkDataSet*>(result->GetBlock(0));
  if (block->GetFieldData()->GetNumberOfArrays() != 1 ||
      block->GetCellData()->GetNumberOfArrays() != 1 ||
      block->GetPointData()->GetNumberOfArrays() != 3)
  {
    std::cout << "Unexpected number of arrays in default output" << std::endl;
    return !vtkTesting::FAILED;
  }

  // pass point and cell arrays
  resample->PassCellArraysOn();
  resample->PassPointArraysOn();
  resample->Update();
  result = static_cast<vtkMultiBlockDataSet*>(resample->GetOutput());
  block = static_cast<vtkDataSet*>(result->GetBlock(0));
  if (block->GetFieldData()->GetNumberOfArrays() != 1 ||
      block->GetCellData()->GetNumberOfArrays() != 6 ||
      block->GetPointData()->GetNumberOfArrays() != 8)
  {
    std::cout << "Unexpected number of arrays in output with pass cell and point arrays" << std::endl;
    return !vtkTesting::FAILED;
  }

  // dont pass field arrays
  resample->PassFieldArraysOff();
  resample->Update();
  result = static_cast<vtkMultiBlockDataSet*>(resample->GetOutput());
  block = static_cast<vtkDataSet*>(result->GetBlock(0));
  if (block->GetFieldData()->GetNumberOfArrays() != 0 ||
      block->GetCellData()->GetNumberOfArrays() != 6 ||
      block->GetPointData()->GetNumberOfArrays() != 8)
  {
    std::cout << "Unexpected number of arrays in output with pass field arrays off" << std::endl;
    return !vtkTesting::FAILED;
  }

  // Render
  vtkNew<vtkCompositeDataGeometryFilter> toPoly;
  toPoly->SetInputData(resample->GetOutputDataObject(0));

  double range[2];
  toPoly->Update();
  toPoly->GetOutput()->GetPointData()->GetArray("RTData")->GetRange(range);

  vtkNew<vtkCompositePolyDataMapper> mapper;
  mapper->SetInputConnection(toPoly->GetOutputPort());
  mapper->SetScalarRange(range);

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
