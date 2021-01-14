/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Test3DLinearGridPlaneCutterCellData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtk3DLinearGridPlaneCutter.h>
#include <vtkActor.h>
#include <vtkCellData.h>
#include <vtkIdFilter.h>
#include <vtkNew.h>
#include <vtkPlane.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTestUtilities.h>
#include <vtkXMLUnstructuredGridReader.h>

int Test3DLinearGridPlaneCutterCellData(int argc, char* argv[])
{
  const auto cellArrayName = "CellIds";

  vtkNew<vtkXMLUnstructuredGridReader> reader;
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/slightlyRotated.vtu");
  reader->SetFileName(fname);
  delete[] fname;

  // add simple cell data
  vtkNew<vtkIdFilter> computeIds;
  computeIds->SetInputConnection(reader->GetOutputPort());
  computeIds->SetPointIds(false);
  computeIds->SetCellIds(true);
  computeIds->SetCellIdsArrayName(cellArrayName);
  computeIds->Update();

  vtkNew<vtkPlane> plane;
  plane->SetOrigin(0.0, 0.0, 0.0);
  plane->SetNormal(0, 1.0, 0.5);

  vtkNew<vtk3DLinearGridPlaneCutter> slicer;
  slicer->SetInputConnection(computeIds->GetOutputPort());
  slicer->SetPlane(plane);
  slicer->SetInterpolateAttributes(true);
  slicer->SetMergePoints(false);
  slicer->Update();

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(slicer->GetOutputPort());
  mapper->SetScalarModeToUseCellData();
  mapper->SetColorModeToMapScalars();
  mapper->ScalarVisibilityOn();
  mapper->SelectColorArray(cellArrayName);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> ren;
  ren->AddActor(actor);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(ren);

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
