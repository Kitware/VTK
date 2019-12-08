/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestRandomHyperTreeGridSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkRandomHyperTreeGridSource.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkHyperTreeGridGeometry.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"

#include <sstream>

namespace
{

double colors[8][3] = { { 1.0, 1.0, 1.0 }, { 0.0, 1.0, 1.0 }, { 1.0, 0.0, 1.0 }, { 1.0, 1.0, 0.0 },
  { 1.0, 0.0, 0.0 }, { 0.0, 1.0, 0.0 }, { 0.0, 0.0, 1.0 }, { 0.7, 0.3, 0.3 } };

void ConstructScene(vtkRenderer* renderer, int numPieces)
{
  for (int i = 0; i < numPieces; ++i)
  {
    vtkNew<vtkRandomHyperTreeGridSource> source;
    source->SetDimensions(5, 5, 2); // GridCell 4, 4, 1
    source->SetSeed(3713971);
    source->SetSplitFraction(0.75);

    vtkNew<vtkHyperTreeGridGeometry> geom;
    geom->SetInputConnection(source->GetOutputPort());

    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(geom->GetOutputPort());
    mapper->SetPiece(i);
    mapper->SetNumberOfPieces(numPieces);

    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);
    actor->GetProperty()->SetRepresentationToSurface();
    actor->GetProperty()->EdgeVisibilityOn();
    actor->GetProperty()->SetColor(colors[i]);

    renderer->AddActor(actor);
  }

  std::ostringstream labelStr;
  labelStr << "NumPieces: " << numPieces;

  vtkNew<vtkTextActor> label;
  label->SetInput(labelStr.str().c_str());
  label->GetTextProperty()->SetVerticalJustificationToBottom();
  label->GetTextProperty()->SetJustificationToCentered();
  label->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
  label->GetPositionCoordinate()->SetValue(0.5, 0.);
  renderer->AddActor(label);

  renderer->ResetCamera();
  renderer->GetActiveCamera()->Zoom(1.3);
}

} // end anon namespace

int TestRandomHyperTreeGridSource(int, char*[])
{
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetSize(500, 500);

  {
    vtkNew<vtkRenderer> renderer;
    renderer->SetViewport(0.0, 0.5, 0.5, 1.0);
    ConstructScene(renderer, 1);
    renWin->AddRenderer(renderer.Get());
  }

  {
    vtkNew<vtkRenderer> renderer;
    renderer->SetViewport(0.5, 0.5, 1.0, 1.0);
    ConstructScene(renderer, 2);
    renWin->AddRenderer(renderer.Get());
  }

  {
    vtkNew<vtkRenderer> renderer;
    renderer->SetViewport(0.0, 0.0, 0.5, 0.5);
    ConstructScene(renderer, 4);
    renWin->AddRenderer(renderer.Get());
  }

  {
    vtkNew<vtkRenderer> renderer;
    renderer->SetViewport(0.5, 0.0, 1.0, 0.5);
    ConstructScene(renderer, 8);
    renWin->AddRenderer(renderer.Get());
  }

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.Get());

  renWin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
