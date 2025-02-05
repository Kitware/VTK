// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * Test the vtkGeometryFilter on degenerated cells.
 *
 * By degenerated cell we mean here a "cell that is defined using a same point several time".
 * This was found as a community workaround to store tetrahedron as hexahedron.
 * https://discourse.paraview.org/t/paraview-versions-greater-5-11-fail-to-display-all-mesh-elements/15810
 *
 * While this is not supported in VTK, the vtkGeometryFilter used to provide
 * an acceptable output when computing the external surface, as for Rendering purpose:
 * external faces are correctly extracted (but a lot of *inner* faces too).
 *
 * This test uses a dataset made of 2 tetrahedron stored as hexahedron.
 * They are rendered with white faces and red backfaces: a missing
 * face should make some backface visible. Also test the number of produced faces.
 *
 * See more discussion on https://gitlab.kitware.com/vtk/vtk/-/issues/19600
 */

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkGeometryFilter.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkXMLUnstructuredGridReader.h"

int TestGeometryFilterDegeneratedCells(int argc, char* argv[])
{
  char* filename =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/degenerated-hexahedrons.vtu");

  vtkNew<vtkXMLUnstructuredGridReader> reader;
  reader->SetFileName(filename);
  delete[] filename;

  vtkNew<vtkGeometryFilter> geomFilter;
  geomFilter->SetInputConnection(reader->GetOutputPort());
  geomFilter->Update();

  auto out = geomFilter->GetOutput();
  if (out->GetNumberOfCells() != 12)
  {
    vtkLog(
      ERROR, "wrong number of output cells. Has " << out->GetNumberOfCells() << " but expects 12");
  }

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(geomFilter->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  // red backfaces to detect missing external face.
  vtkNew<vtkProperty> backfaceProp;
  backfaceProp->SetColor(255, 0, 0);
  actor->SetBackfaceProperty(backfaceProp);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);

  vtkNew<vtkRenderWindow> win;
  win->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(win);
  win->Render();

  // orient to catch the regression from 4a46c5dd
  vtkCamera* cam = renderer->GetActiveCamera();
  cam->Azimuth(-90);

  interactor->Start();

  return EXIT_SUCCESS;
}
