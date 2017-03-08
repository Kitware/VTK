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
#include "vtkCylinder.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRTAnalyticSource.h"
#include "vtkSphere.h"
#include "vtkTableBasedClipDataSet.h"
#include "vtkTransform.h"
#include "vtkTransformFilter.h"
#include "vtkUnstructuredGrid.h"

#include "vtkPointData.h"
#include "vtkDataArray.h"


namespace {

bool Convert2DUnstructuredGridToPolyData(vtkUnstructuredGrid *in,
                                         vtkPolyData *out)
{
  out->Allocate();
  out->SetPoints(in->GetPoints());

  vtkIdType numCells = in->GetNumberOfCells();
  for (vtkIdType i = 0; i < numCells; ++i)
  {
    vtkCell *cell = in->GetCell(i);
    if (cell->GetCellType() != VTK_TRIANGLE && cell->GetCellType() != VTK_QUAD)
    {
      std::cout << "Error: Unexpected cell type: " << cell->GetCellType()
                << "\n";
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

  vtkNew<vtkmExternalFaces> externalFaces;
  externalFaces->SetInputConnection(transFilter->GetOutputPort());

  // execute pipeline
  externalFaces->Update();

  vtkIdType numInputPoints = externalFaces->GetOutput()->GetNumberOfPoints();

  externalFaces->CompactPointsOn();
  externalFaces->Update();

  if (externalFaces->GetOutput()->GetNumberOfPoints() >= numInputPoints)
  {
    std::cout << "Expecting the number of points in the output to be less "
              << "than the input ("
              << externalFaces->GetOutput()->GetNumberOfPoints() << ">="
              << numInputPoints << ")\n";
    return 1;
  }


  vtkNew<vtkPolyData> polydata;
  if (!Convert2DUnstructuredGridToPolyData(externalFaces->GetOutput(),
                                           polydata.GetPointer()))
  {
    std::cout << "Error converting result to polydata\n";
    return 1;
  }


  // render results
  double scalarRange[2];
  polydata->GetPointData()->GetArray("RTData")->GetRange(scalarRange);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputData(polydata.GetPointer());
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
