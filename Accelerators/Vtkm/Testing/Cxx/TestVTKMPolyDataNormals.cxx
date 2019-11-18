/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestVTKMExtractVOI.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkmPolyDataNormals.h"

#include "vtkActor.h"
#include "vtkArrowSource.h"
#include "vtkCamera.h"
#include "vtkCellCenters.h"
#include "vtkCellData.h"
#include "vtkCleanPolyData.h"
#include "vtkCylinderSource.h"
#include "vtkGlyph3D.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkTriangleFilter.h"

namespace
{

void MakeInputDataSet(vtkPolyData* ds)
{
  vtkNew<vtkCylinderSource> cylinder;
  cylinder->SetRadius(1.0);
  cylinder->SetResolution(8);
  cylinder->CappingOn();

  vtkNew<vtkTriangleFilter> triangle;
  triangle->SetInputConnection(cylinder->GetOutputPort());

  vtkNew<vtkCleanPolyData> clean;
  clean->SetInputConnection(triangle->GetOutputPort());

  clean->Update();

  ds->ShallowCopy(clean->GetOutput());
  ds->GetPointData()->Initialize();
  ds->GetCellData()->Initialize();
}

}

int TestVTKMPolyDataNormals(int argc, char* argv[])
{
  vtkNew<vtkPolyData> input;
  MakeInputDataSet(input);

  vtkNew<vtkmPolyDataNormals> normals;
  normals->SetInputData(input);
  normals->ComputePointNormalsOn();
  normals->ComputeCellNormalsOn();
  normals->AutoOrientNormalsOn();
  normals->FlipNormalsOn();
  normals->ConsistencyOn();

  // cylinder mapper and actor
  vtkNew<vtkPolyDataMapper> cylinderMapper;
  cylinderMapper->SetInputData(input);

  vtkNew<vtkActor> cylinderActor;
  cylinderActor->SetMapper(cylinderMapper);
  vtkSmartPointer<vtkProperty> cylinderProperty;
  cylinderProperty.TakeReference(cylinderActor->MakeProperty());
  cylinderProperty->SetRepresentationToWireframe();
  cylinderProperty->SetColor(0.3, 0.3, 0.3);
  cylinderActor->SetProperty(cylinderProperty);

  vtkNew<vtkArrowSource> arrow;

  // point normals
  vtkNew<vtkGlyph3D> pnGlyphs;
  pnGlyphs->SetInputConnection(normals->GetOutputPort());
  pnGlyphs->SetSourceConnection(arrow->GetOutputPort());
  pnGlyphs->SetScaleFactor(0.5);
  pnGlyphs->OrientOn();
  pnGlyphs->SetVectorModeToUseNormal();

  vtkNew<vtkPolyDataMapper> pnMapper;
  pnMapper->SetInputConnection(pnGlyphs->GetOutputPort());

  vtkNew<vtkActor> pnActor;
  pnActor->SetMapper(pnMapper);

  vtkNew<vtkRenderer> pnRenderer;
  pnRenderer->AddActor(cylinderActor);
  pnRenderer->AddActor(pnActor);
  pnRenderer->ResetCamera();
  pnRenderer->GetActiveCamera()->SetPosition(0.0, 4.5, 7.5);
  pnRenderer->ResetCameraClippingRange();

  // cell normals
  vtkNew<vtkCellCenters> cells;
  cells->SetInputConnection(normals->GetOutputPort());

  vtkNew<vtkGlyph3D> cnGlyphs;
  cnGlyphs->SetInputConnection(cells->GetOutputPort());
  cnGlyphs->SetSourceConnection(arrow->GetOutputPort());
  cnGlyphs->SetScaleFactor(0.5);
  cnGlyphs->OrientOn();
  cnGlyphs->SetVectorModeToUseNormal();

  vtkNew<vtkPolyDataMapper> cnMapper;
  cnMapper->SetInputConnection(cnGlyphs->GetOutputPort());

  vtkNew<vtkActor> cnActor;
  cnActor->SetMapper(cnMapper);

  vtkNew<vtkRenderer> cnRenderer;
  cnRenderer->AddActor(cylinderActor);
  cnRenderer->AddActor(cnActor);
  cnRenderer->ResetCamera();
  cnRenderer->GetActiveCamera()->SetPosition(0.0, 8.0, 0.1);
  cnRenderer->ResetCameraClippingRange();

  // render
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(600, 300);
  pnRenderer->SetViewport(0.0, 0.0, 0.5, 1.0);
  renWin->AddRenderer(pnRenderer);
  cnRenderer->SetViewport(0.5, 0.0, 1.0, 1.0);
  renWin->AddRenderer(cnRenderer);

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
