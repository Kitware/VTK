/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImplicitPlaneWidget2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSmartPointer.h"

#include "vtkActor.h"
#include "vtkAppendPolyData.h"
#include "vtkClipPolyData.h"
#include "vtkCommand.h"
#include "vtkConeSource.h"
#include "vtkGlyph3D.h"
#include "vtkImplicitPlaneWidget2.h"
#include "vtkImplicitPlaneRepresentation.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkLODActor.h"
#include "vtkPlane.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"

const char eventLog3[] =
  "# StreamVersion 1\n"
  "CharEvent 108 202 0 0 105 1 i\n"
  "MouseWheelBackwardEvent 147 151 0 0 0 0 i\n"
  "MouseWheelBackwardEvent 147 151 0 0 0 0 i\n"
  "MouseWheelBackwardEvent 147 151 0 0 0 0 i\n"
  "MouseWheelBackwardEvent 147 151 0 0 0 1 i\n"
  "MouseWheelBackwardEvent 147 151 0 0 0 0 i\n"
  "MouseWheelBackwardEvent 147 151 0 0 0 0 i\n"
  "MouseWheelBackwardEvent 147 151 0 0 0 1 i\n"
  "MouseWheelBackwardEvent 147 151 0 0 0 0 i\n"
  "MouseWheelBackwardEvent 147 151 0 0 0 1 i\n"
  "MouseWheelBackwardEvent 147 151 0 0 0 0 i\n"
  "MouseWheelBackwardEvent 147 151 0 0 0 0 i\n"
  "LeftButtonPressEvent 196 93 0 0 0 0 i\n"
  "MouseMoveEvent 196 92 0 0 0 0 i\n"
  "MouseMoveEvent 246 84 0 0 0 0 i\n"
  "MouseMoveEvent 297 76 0 0 0 0 i\n"
  "MouseMoveEvent 308 76 0 0 0 0 i\n"
  "MouseMoveEvent 314 76 0 0 0 0 i\n"
  "MouseMoveEvent 325 73 0 0 0 0 i\n"
  "MouseMoveEvent 331 71 0 0 0 0 i\n"
  "MouseMoveEvent 336 69 0 0 0 0 i\n"
  "MouseMoveEvent 344 67 0 0 0 0 i\n"
  "MouseMoveEvent 348 67 0 0 0 0 i\n"
  "MouseMoveEvent 351 67 0 0 0 0 i\n"
  "MouseMoveEvent 356 66 0 0 0 0 i\n"
  "MouseMoveEvent 356 60 0 0 0 0 i\n"
  "MouseMoveEvent 359 49 0 0 0 0 i\n"
  "MouseMoveEvent 361 34 0 0 0 0 i\n"
  "MouseMoveEvent 364 22 0 0 0 0 i\n"
  "MouseMoveEvent 367 -1 0 0 0 0 i\n"
  "MouseMoveEvent 373 -23 0 0 0 0 i\n"
  "MouseMoveEvent 375 -41 0 0 0 0 i\n"
  "MouseMoveEvent 376 -53 0 0 0 0 i\n"
  "MouseMoveEvent 378 -65 0 0 0 0 i\n"
  "MouseMoveEvent 380 -77 0 0 0 0 i\n"
  "MouseMoveEvent 381 -87 0 0 0 0 i\n"
  "MouseMoveEvent 383 -94 0 0 0 0 i\n"
  "MouseMoveEvent 382 -98 0 0 0 0 i\n"
  "MouseMoveEvent 374 -102 0 0 0 0 i\n"
  "MouseMoveEvent 357 -105 0 0 0 0 i\n"
  "MouseMoveEvent 337 -109 0 0 0 0 i\n"
  "MouseMoveEvent 322 -112 0 0 0 0 i\n"
  "MouseMoveEvent 298 -114 0 0 0 0 i\n"
  "MouseMoveEvent 277 -117 0 0 0 0 i\n"
  "MouseMoveEvent 250 -121 0 0 0 0 i\n"
  "MouseMoveEvent 220 -124 0 0 0 0 i\n"
  "MouseMoveEvent 191 -129 0 0 0 0 i\n"
  "MouseMoveEvent 154 -132 0 0 0 0 i\n"
  "MouseMoveEvent 134 -137 0 0 0 0 i\n"
  "MouseMoveEvent 116 -139 0 0 0 0 i\n"
  "MouseMoveEvent 96 -140 0 0 0 0 i\n"
  "MouseMoveEvent 83 -143 0 0 0 0 i\n"
  "MouseMoveEvent 69 -145 0 0 0 0 i\n"
  "MouseMoveEvent 48 -147 0 0 0 0 i\n"
  "MouseMoveEvent 28 -149 0 0 0 0 i\n"
  "MouseMoveEvent 18 -150 0 0 0 0 i\n"
  "MouseMoveEvent 15 -148 0 0 0 0 i\n"
  "MouseMoveEvent 10 -136 0 0 0 0 i\n"
  "MouseMoveEvent 1 -109 0 0 0 0 i\n"
  "MouseMoveEvent -6 -82 0 0 0 0 i\n"
  "MouseMoveEvent -14 -59 0 0 0 0 i\n"
  "MouseMoveEvent -19 -32 0 0 0 0 i\n"
  "MouseMoveEvent -26 -3 0 0 0 0 i\n"
  "MouseMoveEvent -37 35 0 0 0 0 i\n"
  "MouseMoveEvent -40 66 0 0 0 0 i\n"
  "MouseMoveEvent -44 95 0 0 0 0 i\n"
  "MouseMoveEvent -50 125 0 0 0 0 i\n"
  "MouseMoveEvent -56 149 0 0 0 0 i\n"
  "MouseMoveEvent -61 172 0 0 0 0 i\n"
  "MouseMoveEvent -65 201 0 0 0 0 i\n"
  "MouseMoveEvent -69 216 0 0 0 0 i\n"
  "MouseMoveEvent -72 227 0 0 0 0 i\n"
  "MouseMoveEvent -74 235 0 0 0 0 i\n"
  "MouseMoveEvent -56 236 0 0 0 0 i\n"
  "MouseMoveEvent -41 237 0 0 0 0 i\n"
  "MouseMoveEvent -19 237 0 0 0 0 i\n"
  "MouseMoveEvent -1 237 0 0 0 0 i\n"
  "MouseMoveEvent 20 237 0 0 0 0 i\n"
  "MouseMoveEvent 48 237 0 0 0 0 i\n"
  "MouseMoveEvent 133 237 0 0 0 0 i\n"
  "MouseMoveEvent 215 237 0 0 0 0 i\n"
  "MouseMoveEvent 277 237 0 0 0 0 i\n"
  "MouseMoveEvent 307 237 0 0 0 0 i\n"
  "MouseMoveEvent 338 237 0 0 0 0 i\n"
  "MouseMoveEvent 369 237 0 0 0 0 i\n"
  "MouseMoveEvent 399 237 0 0 0 0 i\n"
  "MouseMoveEvent 417 237 0 0 0 0 i\n"
  "MouseMoveEvent 430 237 0 0 0 0 i\n"
  "MouseMoveEvent 432 237 0 0 0 0 i\n"
  "MouseMoveEvent 435 234 0 0 0 0 i\n"
  "MouseMoveEvent 438 227 0 0 0 0 i\n"
  "MouseMoveEvent 440 222 0 0 0 0 i\n"
  "MouseMoveEvent 442 216 0 0 0 0 i\n"
  "MouseMoveEvent 445 209 0 0 0 0 i\n"
  "MouseMoveEvent 447 198 0 0 0 0 i\n"
  "MouseMoveEvent 452 186 0 0 0 0 i\n"
  "MouseMoveEvent 453 176 0 0 0 0 i\n"
  "MouseMoveEvent 457 164 0 0 0 0 i\n"
  "MouseMoveEvent 461 150 0 0 0 0 i\n"
  "MouseMoveEvent 463 138 0 0 0 0 i\n"
  "MouseMoveEvent 465 128 0 0 0 0 i\n"
  "MouseMoveEvent 465 122 0 0 0 0 i\n"
  "MouseMoveEvent 466 112 0 0 0 0 i\n"
  "MouseMoveEvent 467 102 0 0 0 0 i\n"
  "MouseMoveEvent 467 92 0 0 0 0 i\n"
  "MouseMoveEvent 467 83 0 0 0 0 i\n"
  "LeftButtonReleaseEvent 467 83 0 0 0 0 i\n"
  ;

// This does the actual work: updates the vtkPlane implicit function.
// This in turn causes the pipeline to update and clip the object.
// Callback for the interaction
class vtkTIPW2Callback : public vtkCommand
{
public:
  static vtkTIPW2Callback *New()
  { return new vtkTIPW2Callback; }
  void Execute(vtkObject *caller, unsigned long, void*) VTK_OVERRIDE
  {
    vtkImplicitPlaneWidget2 *planeWidget =
      reinterpret_cast<vtkImplicitPlaneWidget2*>(caller);
    vtkImplicitPlaneRepresentation *rep =
      reinterpret_cast<vtkImplicitPlaneRepresentation*>(planeWidget->GetRepresentation());
    rep->GetPlane(this->Plane);
    this->Actor->VisibilityOn();
  }
  vtkTIPW2Callback():Plane(0),Actor(0) {}
  vtkPlane *Plane;
  vtkActor *Actor;

};

int TestImplicitPlaneWidget3(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  // Create a mace out of filters.
  vtkSmartPointer<vtkSphereSource> sphere =
    vtkSmartPointer<vtkSphereSource>::New();
  vtkSmartPointer<vtkConeSource> cone =
    vtkSmartPointer<vtkConeSource>::New();
  vtkSmartPointer<vtkGlyph3D> glyph =
    vtkSmartPointer<vtkGlyph3D>::New();
  glyph->SetInputConnection(sphere->GetOutputPort());
  glyph->SetSourceConnection(cone->GetOutputPort());
  glyph->SetVectorModeToUseNormal();
  glyph->SetScaleModeToScaleByVector();
  glyph->SetScaleFactor(0.25);
  glyph->Update();

  // The sphere and spikes are appended into a single polydata.
  // This just makes things simpler to manage.
  vtkSmartPointer<vtkAppendPolyData> apd =
    vtkSmartPointer<vtkAppendPolyData>::New();
  apd->AddInputConnection(glyph->GetOutputPort());
  apd->AddInputConnection(sphere->GetOutputPort());

  vtkSmartPointer<vtkPolyDataMapper> maceMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  maceMapper->SetInputConnection(apd->GetOutputPort());

  vtkSmartPointer<vtkLODActor> maceActor =
    vtkSmartPointer<vtkLODActor>::New();
  maceActor->SetMapper(maceMapper);
  maceActor->VisibilityOn();

  // This portion of the code clips the mace with the vtkPlanes
  // implicit function. The clipped region is colored green.
  vtkSmartPointer<vtkPlane> plane =
    vtkSmartPointer<vtkPlane>::New();
  vtkSmartPointer<vtkClipPolyData> clipper =
    vtkSmartPointer<vtkClipPolyData>::New();
  clipper->SetInputConnection(apd->GetOutputPort());
  clipper->SetClipFunction(plane);
  clipper->InsideOutOn();

  vtkSmartPointer<vtkPolyDataMapper> selectMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  selectMapper->SetInputConnection(clipper->GetOutputPort());

  vtkSmartPointer<vtkLODActor> selectActor =
    vtkSmartPointer<vtkLODActor>::New();
  selectActor->SetMapper(selectMapper);
  selectActor->GetProperty()->SetColor(0,1,0);
  selectActor->VisibilityOff();
  selectActor->SetScale(1.01, 1.01, 1.01);

  // Create the RenderWindow, Renderer and both Actors
  vtkSmartPointer<vtkRenderer> ren1 =
    vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer(ren1);

  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  // The SetInteractor method is how 3D widgets are associated with the render
  // window interactor. Internally, SetInteractor sets up a bunch of callbacks
  // using the Command/Observer mechanism (AddObserver()).
  vtkSmartPointer<vtkTIPW2Callback> myCallback =
    vtkSmartPointer<vtkTIPW2Callback>::New();
  myCallback->Plane = plane;
  myCallback->Actor = selectActor;

  vtkSmartPointer<vtkImplicitPlaneRepresentation> rep =
    vtkSmartPointer<vtkImplicitPlaneRepresentation>::New();
  rep->SetPlaceFactor(1.25);
  rep->PlaceWidget(glyph->GetOutput()->GetBounds());

  vtkSmartPointer<vtkImplicitPlaneWidget2> planeWidget =
    vtkSmartPointer<vtkImplicitPlaneWidget2>::New();
  planeWidget->SetInteractor(iren);
  planeWidget->SetRepresentation(rep);
  planeWidget->AddObserver(vtkCommand::InteractionEvent,myCallback);

  ren1->AddActor(maceActor);
  ren1->AddActor(selectActor);

  // Add the actors to the renderer, set the background and size
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);
  renWin->SetMultiSamples(0);

  // Tests
  double wbounds[6];
  double origin[3], origin1[3], origin2[3];
  planeWidget->SetEnabled(1);
  rep->GetOrigin(origin);

  // #1: With ConstrainOrigin on, origin SHOULD NOT be settable outside widget bounds
  rep->ConstrainToWidgetBoundsOn();
  rep->GetWidgetBounds(wbounds);
  rep->SetOrigin(wbounds[1] + 1.0, wbounds[3] + 1.0, wbounds[5] + 1.0);
  rep->GetOrigin(origin1);
  if (origin1[0] > wbounds[1] || origin1[1] > wbounds[3] || origin1[2] > wbounds[5])
  {
    std::cerr << "origin ("
              << origin1[0] << "," << origin1[1] << "," << origin1[2]
              << ") outside widget bounds ("
              << wbounds[0] << "-" << wbounds[1] << ","
              << wbounds[2] << "-" << wbounds[3] << ","
              << wbounds[4] << "-" << wbounds[5] << std::endl;
    return EXIT_FAILURE;
  }

  // #2: With ConstrainOrigin off, origin SHOULD be settable outside current widget bounds.
  rep->ConstrainToWidgetBoundsOff();
  origin1[0] = wbounds[1] + 1.0;
  origin1[1] = wbounds[3] + 1.0;
  origin1[2] = wbounds[5] + 1.0;
  rep->SetOrigin(origin1);
  rep->GetOrigin(origin2);
  if (origin1[0] != origin2[0] || origin1[1] != origin2[1] || origin1[2] != origin2[2])
  {
    std::cerr << "origin not set correctly. expected ("
              << origin1[0] << "," << origin1[1] << "," << origin1[2]
              << "), got: ("
              << origin2[0] << "," << origin2[1] << "," << origin2[2]
              << ")" << std::endl;
    return EXIT_FAILURE;
  }

  rep->SetOrigin(origin);
  planeWidget->SetEnabled(0);

  // #3: With ConstrainOrigin on and OutsideBounds off, the translation of the
  // widget should be limited
  rep->OutsideBoundsOff();
  rep->ConstrainToWidgetBoundsOn();

  vtkSmartPointer<vtkInteractorEventRecorder> recorder =
    vtkSmartPointer<vtkInteractorEventRecorder>::New();
  recorder->SetInteractor(iren);
#if 0 // uncomment if recording
  recorder->SetFileName("record.log");
  recorder->Record();

  iren->Initialize();
  renWin->Render();
  iren->Start();

  recorder->Off();
#else
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(eventLog3);

  // render the image
  iren->Initialize();
  renWin->Render();
  recorder->Play();

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();

  iren->Start();
#endif

  return EXIT_SUCCESS;
}
