/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMeasurementCubeHandleRepresentation3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSmartPointer.h"

#include "vtkConeSource.h"
#include "vtkCubeSource.h"
#include "vtkGlyph3D.h"
#include "vtkSphereSource.h"
#include "vtkAppendPolyData.h"
#include "vtkHandleWidget.h"
#include "vtkMeasurementCubeHandleRepresentation3D.h"
#include "vtkCoordinate.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkNew.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCommand.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkOutlineFilter.h"
#include "vtkImplicitPlaneWidget2.h"
#include "vtkImplicitPlaneRepresentation.h"
#include "vtkBoundedPlanePointPlacer.h"
#include "vtkCutter.h"
#include "vtkLODActor.h"
#include "vtkPlane.h"
#include "vtkProperty.h"
#include "vtkBillboardTextActor3D.h"
#include "vtkTextProperty.h"
#include "vtkTestUtilities.h"

int TestMeasurementCubeHandleRepresentation3D(int vtkNotUsed(argc),
                                              char* vtkNotUsed(argv)[])
{
  double bounds[6] = {-1.,1.,-1.,1.,-1.,1.};

  // Create the RenderWindow and Renderer
  //
  vtkSmartPointer<vtkRenderer> ren1 =
    vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(ren1);

  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  // VTK widgets consist of two parts: the widget part that handles event
  // processing, and the widget representation that defines how the widget
  // appears in the scene (i.e., matters pertaining to geometry).
  vtkSmartPointer<vtkHandleWidget> handleWidget =
    vtkSmartPointer<vtkHandleWidget>::New();
  handleWidget->SetInteractor(iren);

  // Use a vtkMeasurementCubeHandleRepresentation3D to represent the handle widget
  vtkSmartPointer<vtkMeasurementCubeHandleRepresentation3D> unitCubeRep =
    vtkSmartPointer<vtkMeasurementCubeHandleRepresentation3D>::New();
  unitCubeRep->PlaceWidget(bounds);
  unitCubeRep->SetHandleSize(30);
  handleWidget->SetRepresentation(unitCubeRep);
  double p[3] = {1.,0.,0.};
  unitCubeRep->SetWorldPosition(p);

  {
    //Create a sphere
    vtkSmartPointer<vtkSphereSource> sphereSource =
      vtkSmartPointer<vtkSphereSource>::New();
    sphereSource->Update();

    //Create a mapper and actor
    vtkSmartPointer<vtkPolyDataMapper> mapper =
      vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(sphereSource->GetOutputPort());
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);

    //Set the color of the sphere
    actor->GetProperty()->SetColor(1.0, 0.0, 0.0); //(R,G,B)

    //Add the actor to the scene
    ren1->AddActor(actor);
  }

  // Set some defaults.
  //
  iren->Initialize();
  renWin->Render();
  handleWidget->EnabledOn();

  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(400, 400);
  ren1->ResetCamera();
  ren1->ResetCameraClippingRange();
  renWin->Render();

  iren->Start();

  return EXIT_SUCCESS;
}
