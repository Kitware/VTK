/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkElevationFilter.h"
#include "vtkGlyph3DMapper.h"
#include "vtkNew.h"
#include "vtkPlaneSource.h"
#include "vtkPointData.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"

int TestGlyph3DMapperCulling(int argc, char* argv[])
{
  int res = 10;
  vtkNew<vtkPlaneSource> plane;
  plane->SetResolution(res, res);

  vtkNew<vtkSphereSource> squad;
  squad->SetPhiResolution(10);
  squad->SetThetaResolution(10);
  squad->SetRadius(0.05);

  vtkNew<vtkGlyph3DMapper> glypher;
  glypher->SetInputConnection(plane->GetOutputPort());
  glypher->SetSourceConnection(squad->GetOutputPort());
  glypher->SetCullingAndLOD(true);
  glypher->SetNumberOfLOD(2);
  glypher->SetLODDistanceAndTargetReduction(0, 18.0, 0.2);
  glypher->SetLODDistanceAndTargetReduction(1, 20.0, 1.0);
  glypher->SetLODColoring(true);

  vtkNew<vtkActor> glyphActor;
  glyphActor->SetMapper(glypher);

  // Standard rendering classes
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);

  renWin->SetMultiSamples(0);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // set up the view
  renderer->SetBackground(0.5, 0.5, 0.5);
  renWin->SetSize(300, 300);

  renderer->AddActor(glyphActor);

  renderer->GetActiveCamera()->Azimuth(45.0);
  renderer->GetActiveCamera()->Roll(20.0);
  renderer->ResetCamera();

  renWin->Render();

  vtkIdType maxLOD = glypher->GetMaxNumberOfLOD();
  if (maxLOD < 2)
  {
    cout << "This feature cannot be tested, this GPU only supports " << maxLOD << " LODs.\n";
    return EXIT_SUCCESS;
  }

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
