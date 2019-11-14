/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestColorByStringArrayDefaultLookupTable2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

#include <vtkActor2D.h>
#include <vtkCellData.h>
#include <vtkDiskSource.h>
#include <vtkNew.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper2D.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkStdString.h>
#include <vtkStringArray.h>

int TestColorByStringArrayDefaultLookupTable2D(int argc, char* argv[])
{
  vtkNew<vtkDiskSource> disk;
  disk->SetInnerRadius(0.0);
  disk->SetCircumferentialResolution(32);
  disk->Update();

  vtkNew<vtkPolyData> polydata;
  polydata->ShallowCopy(disk->GetOutput());

  // Set up string array associated with cells
  vtkNew<vtkStringArray> sArray;
  char arrayName[] = "string type";
  sArray->SetName(arrayName);
  sArray->SetNumberOfComponents(1);
  sArray->SetNumberOfTuples(polydata->GetNumberOfCells());

  vtkVariant strings[5];
  strings[0] = "violin";
  strings[1] = "viola";
  strings[2] = "cello";
  strings[3] = "bass";
  strings[4] = "double bass";

  // Round-robin assignment of string strings
  for (int i = 0; i < polydata->GetNumberOfCells(); ++i)
  {
    sArray->SetValue(i, strings[i % 5].ToString());
  }

  vtkCellData* cd = polydata->GetCellData();
  cd->AddArray(sArray);

  vtkNew<vtkCoordinate> pCoord;
  pCoord->SetCoordinateSystemToWorld();

  vtkNew<vtkCoordinate> coord;
  coord->SetCoordinateSystemToNormalizedViewport();
  coord->SetReferenceCoordinate(pCoord);

  vtkNew<vtkPolyDataMapper2D> mapper;
  mapper->SetInputDataObject(polydata);
  mapper->ScalarVisibilityOn();
  mapper->SetColorModeToMapScalars();
  mapper->SetScalarModeToUseCellFieldData();

  mapper->ColorByArrayComponent(arrayName, -1);
  mapper->SetTransformCoordinate(coord);

  vtkNew<vtkActor2D> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  renderWindow->Render();
  renderer->ResetCamera();
  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
