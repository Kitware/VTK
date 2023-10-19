// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**Description
 * Test for closed surface clipping with multiple planes
 */

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkClipClosedSurface.h"
#include "vtkNew.h"
#include "vtkPlane.h"
#include "vtkPlaneCollection.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSuperquadricSource.h"
#include "vtkTransform.h"

int TestClipClosedSurface2(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  // Initialize the render window
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetSize(301, 300); // Intentional NPOT size

  vtkNew<vtkRenderer> ren;
  ren->SetBackground(0.3, 0.3, 0.32);
  renWin->AddRenderer(ren);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkSuperquadricSource> source;
  source->SetPhiResolution(24);
  source->SetPhiRoundness(0.5);
  source->SetThetaResolution(24);
  source->SetThetaRoundness(0.5);

  // Define the clipping planes
  vtkNew<vtkPlane> clipPlane1;
  clipPlane1->SetOrigin(0.27, -0.16, 0.46);
  clipPlane1->SetNormal(0.48, -0.29, 0.83);
  vtkNew<vtkPlane> clipPlane2;
  clipPlane2->SetOrigin(-0.39, 0.22, 0.30);
  clipPlane2->SetNormal(-0.8, -0.31, 0.5);
  vtkNew<vtkPlane> clipPlane3;
  clipPlane3->SetOrigin(0.16, -0.19, 0.42);
  clipPlane3->SetNormal(0.0, -0.95, 0.31);

  // Group the planes into a collection
  vtkNew<vtkPlaneCollection> planes;
  planes->AddItem(clipPlane1);
  planes->AddItem(clipPlane2);
  planes->AddItem(clipPlane3);

  // Create a clip
  vtkNew<vtkClipClosedSurface> clip;
  clip->SetClippingPlanes(planes);
  clip->SetInputConnection(source->GetOutputPort());
  clip->SetBaseColor(0.44, 0.31, 0.31);
  clip->SetClipColor(0.87, 0.63, 0.87);
  clip->SetScalarModeToColors();
  clip->GenerateClipFaceOutputOn();
  clip->GenerateFacesOn();
  clip->GenerateOutlineOff();
  clip->InsideOutOn();

  // Add the clip actor to the viewport
  vtkNew<vtkPolyDataMapper> clipMapper;
  clipMapper->SetInputConnection(clip->GetOutputPort());
  vtkNew<vtkActor> clipActor;
  clipActor->SetMapper(clipMapper);
  clipActor->GetProperty()->SetDiffuse(0.5);
  clipActor->GetProperty()->SetAmbient(0.5);
  ren->AddActor(clipActor);

  // Add the triangulated clip face
  double n[3];
  clipPlane3->GetNormal(n);
  vtkMath::MultiplyScalar(n, 0.5);
  vtkNew<vtkTransform> t;
  t->Translate(n);
  vtkNew<vtkPolyDataMapper> clipFaceMapper;
  clipFaceMapper->SetInputConnection(clip->GetOutputPort(1));
  vtkNew<vtkActor> clipFaceActor;
  clipFaceActor->SetMapper(clipFaceMapper);
  clipFaceActor->SetUserTransform(t);
  clipFaceActor->GetProperty()->SetColor(0.18, 0.54, 0.34);
  clipFaceActor->GetProperty()->SetDiffuse(0.5);
  clipFaceActor->GetProperty()->SetAmbient(0.5);
  ren->AddActor(clipFaceActor);

  ren->GetActiveCamera()->SetViewUp(0, -1, 0);
  ren->ResetCamera();
  renWin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
