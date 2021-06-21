/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestButtonWidgetPlacement.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkActor.h"
#include "vtkAppendPolyData.h"
#include "vtkBMPReader.h"
#include "vtkButtonWidget.h"
#include "vtkCamera.h"
#include "vtkConeSource.h"
#include "vtkEllipticalButtonSource.h"
#include "vtkGlyph3D.h"
#include "vtkNew.h"
#include "vtkPNGReader.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTIFFReader.h"
#include "vtkTestUtilities.h"
#include "vtkTexturedButtonRepresentation.h"

#include <string>
#include <vector>

int TestButtonWidgetPlacement(int argc, char* argv[])
{
  std::vector<std::string> fnames{ "Data/beach.tif", "Data/fran_cut.png", "Data/hearts8bit.png",
    "Data/masonry.bmp" };
  std::vector<std::string> fullFnames(4);
  std::vector<vtkSmartPointer<vtkImageReader2>> readers(4);

  // Create images for buttons
  for (int i = 0; i < 4; ++i)
  {
    if (i == 0)
    {
      readers[i] = vtkSmartPointer<vtkTIFFReader>::New();
      vtkTIFFReader::SafeDownCast(readers[i])->SetOrientationType(4);
    }
    else if (i == 3)
    {
      readers[i] = vtkSmartPointer<vtkBMPReader>::New();
    }
    else
    {
      readers[i] = vtkSmartPointer<vtkPNGReader>::New();
    }
    std::cout << fullFnames[i] << "\n";
    fullFnames[i] = vtkTestUtilities::ExpandDataFileName(argc, argv, fnames[i].c_str());
    readers[i]->SetFileName(fullFnames[i].c_str());
    readers[i]->Update();
  }

  // Create the RenderWindow, Renderer
  vtkNew<vtkRenderer> ren;
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(ren);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // The SetInteractor method is how 3D widgets are associated with the render
  // window interactor. Internally, SetInteractor sets up a bunch of callbacks
  // using the Command/Observer mechanism (AddObserver()).
  double scale = 1.;
  std::vector<std::vector<double>> corners;
  for (int x = -1; x < 2; x += 2)
  {
    for (int y = -1; y < 2; y += 2)
    {
      corners.push_back({ x * 1., y * 1., 0. });
    }
  }
  double normal[3] = { 0., 0., 1. };

  vtkNew<vtkEllipticalButtonSource> geometries[4];
  vtkNew<vtkTexturedButtonRepresentation> reps[4];
  vtkNew<vtkButtonWidget> buttons[4];

  for (int i = 0; i < 4; ++i)
  {
    const auto& geometry = geometries[i];
    const auto& rep = reps[i];
    const auto& button = buttons[i];

    geometry->TwoSidedOn();
    geometry->SetCircumferentialResolution(24);
    geometry->SetShoulderResolution(24);
    geometry->SetTextureResolution(24);
    geometry->SetTextureStyleToFitImage();
    rep->SetNumberOfStates(1);
    rep->SetButtonTexture(0, readers[i]->GetOutput());
    rep->SetPlaceFactor(1);
    rep->PlaceWidget(scale, corners[i].data(), normal);
    rep->FollowCameraOn();
    rep->SetButtonGeometryConnection(geometry->GetOutputPort());
    button->SetInteractor(iren);
    button->SetRepresentation(rep);
  }

  // Add the actors to the renderer, set the background and size
  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkConeSource> cone;
  vtkNew<vtkGlyph3D> glyph;
  glyph->SetInputConnection(sphere->GetOutputPort());
  glyph->SetSourceConnection(cone->GetOutputPort());
  glyph->SetVectorModeToUseNormal();
  glyph->SetScaleModeToScaleByVector();
  glyph->SetScaleFactor(1.0);
  glyph->Update();

  // Appending just makes things simpler to manage.
  vtkNew<vtkAppendPolyData> apd;
  apd->AddInputConnection(glyph->GetOutputPort());
  apd->AddInputConnection(sphere->GetOutputPort());

  vtkNew<vtkPolyDataMapper> maceMapper;
  maceMapper->SetInputConnection(apd->GetOutputPort());

  vtkNew<vtkActor> maceActor;
  maceActor->SetMapper(maceMapper);
  maceActor->VisibilityOn();
  ren->AddActor(maceActor);
  ren->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // render the image
  iren->Initialize();
  renWin->Render();

  for (const auto& button : buttons)
  {
    button->EnabledOn();
  }

  iren->Start();

  return EXIT_SUCCESS;
}
