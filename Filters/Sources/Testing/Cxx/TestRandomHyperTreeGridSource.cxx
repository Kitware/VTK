// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

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

bool ConstructScene(vtkRenderer* renderer, int numPieces)
{
  bool result = true;
  float maskedFraction = 1.0 / (2.0 * numPieces);
  for (int i = 0; i < numPieces; ++i)
  {
    vtkNew<vtkRandomHyperTreeGridSource> source;
    source->SetDimensions(5, 5, 2); // GridCell 4, 4, 1
    source->SetSeed(371399);
    source->SetSplitFraction(0.25);
    source->SetMaskedFraction(maskedFraction);
    source->Update();

    if (source->GetMaskedCellProportion() > maskedFraction)
    {
      std::cout << "The masked cell proportion is " << source->GetMaskedCellProportion()
                << " and it should be less or equal than " << maskedFraction << std::endl;
      result = false;
    }

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

  /*
   * Camera is a bit confusing since it shows a 2D plane
   * while the HTG is actually 3D.
   */
  renderer->ResetCamera();
  renderer->GetActiveCamera()->Zoom(1.3);
  return result;
}

} // end anon namespace

int TestRandomHyperTreeGridSource(int, char*[])
{
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetSize(500, 500);
  bool result = true;
  {
    vtkNew<vtkRenderer> renderer;
    renderer->SetViewport(0.0, 0.5, 0.5, 1.0);
    result &= ConstructScene(renderer, 1);
    renWin->AddRenderer(renderer.Get());
  }
  {
    vtkNew<vtkRenderer> renderer;
    renderer->SetViewport(0.5, 0.5, 1.0, 1.0);
    result &= ConstructScene(renderer, 2);
    renWin->AddRenderer(renderer.Get());
  }
  {
    vtkNew<vtkRenderer> renderer;
    renderer->SetViewport(0.0, 0.0, 0.5, 0.5);
    result &= ConstructScene(renderer, 4);
    renWin->AddRenderer(renderer.Get());
  }

  {
    vtkNew<vtkRenderer> renderer;
    renderer->SetViewport(0.5, 0.0, 1.0, 0.5);
    result &= ConstructScene(renderer, 8);
    renWin->AddRenderer(renderer.Get());
  }

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.Get());

  renWin->Render();
  iren->Start();
  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
