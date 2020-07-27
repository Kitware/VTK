//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================
#include "vtkmExternalFaces.h"

#include "vtkActor.h"
#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkCylinder.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRandomAttributeGenerator.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphere.h"
#include "vtkTableBasedClipDataSet.h"
#include "vtkTransform.h"
#include "vtkTransformFilter.h"
#include "vtkUnstructuredGrid.h"

#include "vtkDataArray.h"
#include "vtkPointData.h"

namespace
{

bool Convert2DUnstructuredGridToPolyData(vtkUnstructuredGrid* in, vtkPolyData* out)
{
  vtkIdType numCells = in->GetNumberOfCells();
  out->AllocateEstimate(numCells, 1);
  out->SetPoints(in->GetPoints());

  for (vtkIdType i = 0; i < numCells; ++i)
  {
    vtkCell* cell = in->GetCell(i);
    if (cell->GetCellType() != VTK_TRIANGLE && cell->GetCellType() != VTK_QUAD)
    {
      std::cout << "Error: Unexpected cell type: " << cell->GetCellType() << "\n";
      return false;
    }
    out->InsertNextCell(cell->GetCellType(), cell->GetPointIds());
  }

  out->GetPointData()->PassData(in->GetPointData());
  return true;
}

} // anonymous namespace

int TestVTKMExternalFaces(int argc, char* argv[])
{
  // create pipeline
  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-16, 16, -16, 16, -16, 16);
  wavelet->SetCenter(0, 0, 0);

  vtkNew<vtkCylinder> cylinder;
  cylinder->SetCenter(0, 0, 0);
  cylinder->SetRadius(15);
  cylinder->SetAxis(0, 1, 0);
  vtkNew<vtkTableBasedClipDataSet> clipCyl;
  clipCyl->SetInputConnection(wavelet->GetOutputPort());
  clipCyl->SetClipFunction(cylinder);
  clipCyl->InsideOutOn();

  vtkNew<vtkSphere> sphere;
  sphere->SetCenter(0, 0, 4);
  sphere->SetRadius(12);
  vtkNew<vtkTableBasedClipDataSet> clipSphr;
  clipSphr->SetInputConnection(clipCyl->GetOutputPort());
  clipSphr->SetClipFunction(sphere);

  vtkNew<vtkTransform> transform;
  transform->RotateZ(45);
  vtkNew<vtkTransformFilter> transFilter;
  transFilter->SetInputConnection(clipSphr->GetOutputPort());
  transFilter->SetTransform(transform);

  vtkNew<vtkRandomAttributeGenerator> cellDataAdder;
  cellDataAdder->SetInputConnection(transFilter->GetOutputPort());
  cellDataAdder->SetDataTypeToFloat();
  cellDataAdder->GenerateCellVectorsOn();

  vtkNew<vtkmExternalFaces> externalFaces;
  externalFaces->SetInputConnection(cellDataAdder->GetOutputPort());

  // execute pipeline
  externalFaces->Update();
  vtkUnstructuredGrid* result = externalFaces->GetOutput();

  vtkIdType numInputPoints = result->GetNumberOfPoints();

  externalFaces->CompactPointsOn();
  externalFaces->Update();
  result = externalFaces->GetOutput();

  if (result->GetNumberOfPoints() >= numInputPoints)
  {
    std::cout << "Expecting the number of points in the output to be less "
              << "than the input (" << result->GetNumberOfPoints() << ">=" << numInputPoints
              << ")\n";
    return 1;
  }

  if (result->GetCellData()->GetArray("RandomCellVectors")->GetNumberOfTuples() !=
    result->GetNumberOfCells())
  {
    std::cout << "Expecting a cell field with number of entries equal to "
              << "the number of cells";
    return 1;
  }

  vtkNew<vtkPolyData> polydata;
  if (!Convert2DUnstructuredGridToPolyData(result, polydata))
  {
    std::cout << "Error converting result to polydata\n";
    return 1;
  }

  // render results
  double scalarRange[2];
  polydata->GetPointData()->GetArray("RTData")->GetRange(scalarRange);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputData(polydata);
  mapper->SetScalarRange(scalarRange);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->ResetCamera();

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  iren->Initialize();

  renWin->Render();
  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
