// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**Description
 * Test for closed surface clipping with inside out flag and the second output
 */

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkClipClosedSurface.h"
#include "vtkFlyingEdges3D.h"
#include "vtkImageGaussianSmooth.h"
#include "vtkNew.h"
#include "vtkPlane.h"
#include "vtkPlaneCollection.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkTransform.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkVolume16Reader.h"

int TestClipClosedSurface1(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  // Initialize the render window
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetSize(301, 300); // Intentional NPOT size

  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");
  vtkNew<vtkVolume16Reader> v16;
  v16->SetDataDimensions(64, 64);
  v16->SetDataOrigin(0, 0, 0);
  v16->SetDataByteOrderToLittleEndian();
  v16->SetFilePrefix(fname);
  v16->SetImageRange(1, 93);
  v16->SetDataSpacing(3.2, 3.2, 1.5);
  v16->Update();
  delete[] fname;

  vtkNew<vtkImageGaussianSmooth> smooth;
  smooth->SetDimensionality(3);
  smooth->SetInputConnection(v16->GetOutputPort());
  smooth->SetStandardDeviations(1.75, 1.75, 0);
  smooth->SetRadiusFactor(3);

  vtkNew<vtkFlyingEdges3D> iso;
  iso->SetInputConnection(smooth->GetOutputPort());
  iso->SetValue(0, 1150);
  double planeNormal[3] = { 0.88, 0.47, -0.1 };
  vtkNew<vtkPlane> clipPlane;
  clipPlane->SetNormal(planeNormal);
  clipPlane->SetOrigin(105, 125, 60);
  vtkNew<vtkPlaneCollection> capPlanes;
  capPlanes->AddItem(clipPlane);
  vtkNew<vtkClipClosedSurface> clip;
  clip->SetClippingPlanes(capPlanes);
  clip->SetInputConnection(iso->GetOutputPort());
  clip->SetBaseColor(0.9804, 0.9216, 0.8431);
  clip->SetClipColor(1.0, 1.0, 1.0);
  clip->SetScalarModeToColors();
  clip->GenerateFacesOn();
  clip->GenerateClipFaceOutputOn();
  clip->GenerateOutlineOn();

  vtkNew<vtkPolyDataMapper> clipMapper;
  clipMapper->SetInputConnection(clip->GetOutputPort());
  vtkNew<vtkActor> clipActor;
  clipActor->SetMapper(clipMapper);
  ren->AddActor(clipActor);

  // Now add an inside-out clip surface
  vtkNew<vtkClipClosedSurface> clipI;
  clipI->SetClippingPlanes(capPlanes);
  clipI->SetInputConnection(iso->GetOutputPort());
  clipI->SetBaseColor(0.9804, 0.9216, 0.8431);
  clipI->SetClipColor(1.0, 1.0, 1.0);
  clipI->SetScalarModeToColors();
  clipI->GenerateFacesOn();
  clipI->InsideOutOn();

  // Translate the inverse clipped volume to create a gap
  double n[3] = { planeNormal[0], planeNormal[1], planeNormal[2] };
  vtkMath::MultiplyScalar(n, -50);
  vtkNew<vtkTransform> t;
  t->Translate(n);

  vtkNew<vtkPolyDataMapper> clipIMapper;
  clipIMapper->SetInputConnection(clipI->GetOutputPort());
  vtkNew<vtkActor> clipIActor;
  clipIActor->SetUserTransform(t);
  clipIActor->SetMapper(clipIMapper);
  ren->AddActor(clipIActor);

  double n1[3] = { planeNormal[0], planeNormal[1], planeNormal[2] };
  vtkMath::MultiplyScalar(n1, -25);
  vtkNew<vtkTransform> t1;
  t1->Translate(n1);

  vtkNew<vtkPolyDataMapper> clipFaceMapper;
  clipFaceMapper->SetInputConnection(clip->GetOutputPort(1));
  vtkNew<vtkActor> clipFaceActor;
  clipFaceActor->SetUserTransform(t1);
  clipFaceActor->SetMapper(clipFaceMapper);
  clipFaceActor->GetProperty()->SetColor(0.18, 0.54, 0.34);
  ren->AddActor(clipFaceActor);

  ren->GetActiveCamera()->SetPosition(-244.6, 367.4, 102.54);
  ren->GetActiveCamera()->SetFocalPoint(78.55, 85.95, 71.5);
  ren->GetActiveCamera()->SetViewUp(0, 0, -1);
  ren->ResetCamera();

  renWin->Render();

  iren->Start();

  return EXIT_SUCCESS;
}
