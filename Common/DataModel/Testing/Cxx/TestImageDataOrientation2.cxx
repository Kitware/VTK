/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImageDataOrientation2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME Test orientation for image data
// .SECTION Description
// This program tests the location of an oriented Image Data by using a
// non-identity direction matrix and extracting points of the image data
// that fall within a sphere.

#include "vtkDebugLeaks.h"
#include "vtkGlyph3D.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSelectEnclosedPoints.h"
#include "vtkSphereSource.h"
#include "vtkThresholdPoints.h"

int TestImageDataOrientation2(int argc, char* argv[])
{
  // Standard rendering classes
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // Create an oriented image data
  double angle = -vtkMath::Pi() / 4;
  double direction[9] = { cos(angle), sin(angle), 0, -sin(angle), cos(angle), 0, 0, 0, 1 };
  vtkNew<vtkImageData> image;
  image->SetExtent(0, 6, 0, 10, 0, 10);
  image->SetOrigin(-0.4, 0.2, -0.6);
  image->SetSpacing(0.4, -0.25, 0.25);
  image->SetDirectionMatrix(direction);
  image->AllocateScalars(VTK_DOUBLE, 0);

  // Create a containing surface
  vtkNew<vtkSphereSource> ss;
  ss->SetPhiResolution(25);
  ss->SetThetaResolution(38);
  ss->SetCenter(0, 0, 0);
  ss->SetRadius(2.5);
  vtkNew<vtkPolyDataMapper> sphereMapper;
  sphereMapper->SetInputConnection(ss->GetOutputPort());
  vtkNew<vtkActor> sphereActor;
  sphereActor->SetMapper(sphereMapper);
  sphereActor->GetProperty()->SetRepresentationToWireframe();

  vtkNew<vtkSelectEnclosedPoints> select;
  select->SetInputData(image);
  select->SetSurfaceConnection(ss->GetOutputPort());

  // Now extract points
  vtkNew<vtkThresholdPoints> thresh;
  thresh->SetInputConnection(select->GetOutputPort());
  thresh->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "SelectedPoints");
  thresh->ThresholdByUpper(0.5);

  // Show points as glyphs
  vtkNew<vtkSphereSource> glyph;
  vtkNew<vtkGlyph3D> glypher;
  glypher->SetInputConnection(thresh->GetOutputPort());
  glypher->SetSourceConnection(glyph->GetOutputPort());
  glypher->SetScaleModeToDataScalingOff();
  glypher->SetScaleFactor(0.15);

  vtkNew<vtkPolyDataMapper> pointsMapper;
  pointsMapper->SetInputConnection(glypher->GetOutputPort());
  pointsMapper->ScalarVisibilityOff();

  vtkNew<vtkActor> pointsActor;
  pointsActor->SetMapper(pointsMapper);
  pointsActor->GetProperty()->SetColor(0, 0, 1);

  // Add actors
  //  renderer->AddActor(sphereActor);
  renderer->AddActor(pointsActor);

  // Standard testing code.
  renWin->SetSize(400, 400);
  renWin->Render();
  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
