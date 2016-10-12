/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImplicitCylinderWidget.cxx

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
#include "vtkImplicitCylinderWidget.h"
#include "vtkImplicitCylinderRepresentation.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkLODActor.h"
#include "vtkCylinder.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"

namespace {

const char eventLog[] =
  "# StreamVersion 1\n"
  "CharEvent 108 202 0 0 105 1 i\n"
  "MouseWheelBackwardEvent 161 106 0 0 0 0 i\n"
  "MouseWheelBackwardEvent 161 106 0 0 0 1 i\n"
  "MouseWheelBackwardEvent 161 106 0 0 0 0 i\n"
  "MouseWheelBackwardEvent 161 106 0 0 0 1 i\n"
  "MouseWheelBackwardEvent 161 106 0 0 0 0 i\n"
  "MouseWheelBackwardEvent 161 106 0 0 0 1 i\n"
  "MouseWheelBackwardEvent 161 106 0 0 0 0 i\n"
  "MouseWheelBackwardEvent 161 106 0 0 0 1 i\n"
  "LeftButtonPressEvent 174 264 0 0 0 0 i\n"
  "MouseMoveEvent 177 265 0 0 0 0 i\n"
  "MouseMoveEvent 194 266 0 0 0 0 i\n"
  "MouseMoveEvent 261 266 0 0 0 0 i\n"
  "MouseMoveEvent 313 268 0 0 0 0 i\n"
  "MouseMoveEvent 359 268 0 0 0 0 i\n"
  "MouseMoveEvent 395 266 0 0 0 0 i\n"
  "MouseMoveEvent 437 266 0 0 0 0 i\n"
  "MouseMoveEvent 475 262 0 0 0 0 i\n"
  "MouseMoveEvent 515 262 0 0 0 0 i\n"
  "MouseMoveEvent 549 259 0 0 0 0 i\n"
  "MouseMoveEvent 579 259 0 0 0 0 i\n"
  "MouseMoveEvent 599 259 0 0 0 0 i\n"
  "MouseMoveEvent 605 259 0 0 0 0 i\n"
  "MouseMoveEvent 611 259 0 0 0 0 i\n"
  "MouseMoveEvent 618 259 0 0 0 0 i\n"
  "MouseMoveEvent 622 255 0 0 0 0 i\n"
  "MouseMoveEvent 626 245 0 0 0 0 i\n"
  "MouseMoveEvent 632 227 0 0 0 0 i\n"
  "MouseMoveEvent 639 207 0 0 0 0 i\n"
  "MouseMoveEvent 646 190 0 0 0 0 i\n"
  "MouseMoveEvent 649 177 0 0 0 0 i\n"
  "MouseMoveEvent 652 168 0 0 0 0 i\n"
  "MouseMoveEvent 656 155 0 0 0 0 i\n"
  "MouseMoveEvent 658 144 0 0 0 0 i\n"
  "MouseMoveEvent 662 133 0 0 0 0 i\n"
  "MouseMoveEvent 664 118 0 0 0 0 i\n"
  "MouseMoveEvent 666 107 0 0 0 0 i\n"
  "MouseMoveEvent 668 97 0 0 0 0 i\n"
  "MouseMoveEvent 670 84 0 0 0 0 i\n"
  "MouseMoveEvent 671 73 0 0 0 0 i\n"
  "MouseMoveEvent 675 62 0 0 0 0 i\n"
  "MouseMoveEvent 677 53 0 0 0 0 i\n"
  "MouseMoveEvent 681 40 0 0 0 0 i\n"
  "MouseMoveEvent 686 12 0 0 0 0 i\n"
  "MouseMoveEvent 688 6 0 0 0 0 i\n"
  "MouseMoveEvent 689 -4 0 0 0 0 i\n"
  "MouseMoveEvent 690 -20 0 0 0 0 i\n"
  "MouseMoveEvent 692 -29 0 0 0 0 i\n"
  "MouseMoveEvent 688 -35 0 0 0 0 i\n"
  "MouseMoveEvent 661 -42 0 0 0 0 i\n"
  "MouseMoveEvent 617 -47 0 0 0 0 i\n"
  "MouseMoveEvent 577 -50 0 0 0 0 i\n"
  "MouseMoveEvent 554 -55 0 0 0 0 i\n"
  "MouseMoveEvent 523 -58 0 0 0 0 i\n"
  "MouseMoveEvent 497 -62 0 0 0 0 i\n"
  "MouseMoveEvent 468 -64 0 0 0 0 i\n"
  "MouseMoveEvent 438 -66 0 0 0 0 i\n"
  "MouseMoveEvent 412 -70 0 0 0 0 i\n"
  "MouseMoveEvent 388 -73 0 0 0 0 i\n"
  "MouseMoveEvent 350 -78 0 0 0 0 i\n"
  "MouseMoveEvent 317 -80 0 0 0 0 i\n"
  "MouseMoveEvent 270 -86 0 0 0 0 i\n"
  "MouseMoveEvent 223 -90 0 0 0 0 i\n"
  "MouseMoveEvent 182 -95 0 0 0 0 i\n"
  "MouseMoveEvent 149 -99 0 0 0 0 i\n"
  "MouseMoveEvent 109 -101 0 0 0 0 i\n"
  "MouseMoveEvent 75 -103 0 0 0 0 i\n"
  "MouseMoveEvent 39 -103 0 0 0 0 i\n"
  "MouseMoveEvent 1 -103 0 0 0 0 i\n"
  "MouseMoveEvent -8 -103 0 0 0 0 i\n"
  "MouseMoveEvent -13 -102 0 0 0 0 i\n"
  "MouseMoveEvent -20 -92 0 0 0 0 i\n"
  "MouseMoveEvent -34 -75 0 0 0 0 i\n"
  "MouseMoveEvent -43 -58 0 0 0 0 i\n"
  "MouseMoveEvent -59 -36 0 0 0 0 i\n"
  "MouseMoveEvent -69 -6 0 0 0 0 i\n"
  "MouseMoveEvent -81 20 0 0 0 0 i\n"
  "MouseMoveEvent -96 59 0 0 0 0 i\n"
  "MouseMoveEvent -104 89 0 0 0 0 i\n"
  "MouseMoveEvent -115 121 0 0 0 0 i\n"
  "MouseMoveEvent -129 154 0 0 0 0 i\n"
  "MouseMoveEvent -137 181 0 0 0 0 i\n"
  "MouseMoveEvent -145 209 0 0 0 0 i\n"
  "MouseMoveEvent -152 234 0 0 0 0 i\n"
  "MouseMoveEvent -155 253 0 0 0 0 i\n"
  "MouseMoveEvent -160 275 0 0 0 0 i\n"
  "MouseMoveEvent -162 298 0 0 0 0 i\n"
  "MouseMoveEvent -167 330 0 0 0 0 i\n"
  "MouseMoveEvent -169 346 0 0 0 0 i\n"
  "MouseMoveEvent -170 366 0 0 0 0 i\n"
  "MouseMoveEvent -174 387 0 0 0 0 i\n"
  "MouseMoveEvent -176 413 0 0 0 0 i\n"
  "MouseMoveEvent -177 439 0 0 0 0 i\n"
  "MouseMoveEvent -177 467 0 0 0 0 i\n"
  "MouseMoveEvent -177 492 0 0 0 0 i\n"
  "MouseMoveEvent -177 513 0 0 0 0 i\n"
  "MouseMoveEvent -177 520 0 0 0 0 i\n"
  "MouseMoveEvent -176 525 0 0 0 0 i\n"
  "MouseMoveEvent -161 529 0 0 0 0 i\n"
  "MouseMoveEvent -129 535 0 0 0 0 i\n"
  "MouseMoveEvent -90 535 0 0 0 0 i\n"
  "MouseMoveEvent -49 541 0 0 0 0 i\n"
  "MouseMoveEvent -2 541 0 0 0 0 i\n"
  "MouseMoveEvent 42 541 0 0 0 0 i\n"
  "MouseMoveEvent 85 544 0 0 0 0 i\n"
  "MouseMoveEvent 115 547 0 0 0 0 i\n"
  "MouseMoveEvent 145 547 0 0 0 0 i\n"
  "MouseMoveEvent 186 547 0 0 0 0 i\n"
  "MouseMoveEvent 234 547 0 0 0 0 i\n"
  "MouseMoveEvent 272 547 0 0 0 0 i\n"
  "MouseMoveEvent 299 547 0 0 0 0 i\n"
  "MouseMoveEvent 320 549 0 0 0 0 i\n"
  "MouseMoveEvent 345 549 0 0 0 0 i\n"
  "MouseMoveEvent 365 549 0 0 0 0 i\n"
  "MouseMoveEvent 382 549 0 0 0 0 i\n"
  "MouseMoveEvent 410 549 0 0 0 0 i\n"
  "MouseMoveEvent 439 549 0 0 0 0 i\n"
  "MouseMoveEvent 465 547 0 0 0 0 i\n"
  "MouseMoveEvent 482 545 0 0 0 0 i\n"
  "MouseMoveEvent 484 538 0 0 0 0 i\n"
  "MouseMoveEvent 484 529 0 0 0 0 i\n"
  "MouseMoveEvent 491 507 0 0 0 0 i\n"
  "MouseMoveEvent 492 498 0 0 0 0 i\n"
  "MouseMoveEvent 494 488 0 0 0 0 i\n"
  "MouseMoveEvent 496 480 0 0 0 0 i\n"
  "MouseMoveEvent 496 476 0 0 0 0 i\n"
  "MouseMoveEvent 497 468 0 0 0 0 i\n"
  "MouseMoveEvent 499 459 0 0 0 0 i\n"
  "MouseMoveEvent 500 450 0 0 0 0 i\n"
  "MouseMoveEvent 501 441 0 0 0 0 i\n"
  "MouseMoveEvent 502 433 0 0 0 0 i\n"
  "MouseMoveEvent 504 428 0 0 0 0 i\n"
  "MouseMoveEvent 505 420 0 0 0 0 i\n"
  "MouseMoveEvent 505 415 0 0 0 0 i\n"
  "MouseMoveEvent 506 409 0 0 0 0 i\n"
  "MouseMoveEvent 506 403 0 0 0 0 i\n"
  "MouseMoveEvent 506 395 0 0 0 0 i\n"
  "MouseMoveEvent 507 389 0 0 0 0 i\n"
  "MouseMoveEvent 507 383 0 0 0 0 i\n"
  "MouseMoveEvent 507 376 0 0 0 0 i\n"
  "MouseMoveEvent 507 368 0 0 0 0 i\n"
  "MouseMoveEvent 508 358 0 0 0 0 i\n"
  "MouseMoveEvent 509 350 0 0 0 0 i\n"
  "MouseMoveEvent 509 346 0 0 0 0 i\n"
  "MouseMoveEvent 509 341 0 0 0 0 i\n"
  "MouseMoveEvent 510 335 0 0 0 0 i\n"
  "MouseMoveEvent 510 333 0 0 0 0 i\n"
  "MouseMoveEvent 510 330 0 0 0 0 i\n"
  "MouseMoveEvent 510 325 0 0 0 0 i\n"
  "MouseMoveEvent 510 321 0 0 0 0 i\n"
  "MouseMoveEvent 510 318 0 0 0 0 i\n"
  "MouseMoveEvent 511 310 0 0 0 0 i\n"
  "MouseMoveEvent 512 304 0 0 0 0 i\n"
  "MouseMoveEvent 512 298 0 0 0 0 i\n"
  "MouseMoveEvent 514 290 0 0 0 0 i\n"
  "MouseMoveEvent 515 284 0 0 0 0 i\n"
  "MouseMoveEvent 515 278 0 0 0 0 i\n"
  "MouseMoveEvent 515 270 0 0 0 0 i\n"
  "MouseMoveEvent 516 260 0 0 0 0 i\n"
  "MouseMoveEvent 516 253 0 0 0 0 i\n"
  "MouseMoveEvent 517 242 0 0 0 0 i\n"
  "MouseMoveEvent 517 238 0 0 0 0 i\n"
  "MouseMoveEvent 519 234 0 0 0 0 i\n"
  "MouseMoveEvent 519 231 0 0 0 0 i\n"
  "MouseMoveEvent 519 229 0 0 0 0 i\n"
  "MouseMoveEvent 519 225 0 0 0 0 i\n"
  "MouseMoveEvent 519 221 0 0 0 0 i\n"
  "MouseMoveEvent 519 218 0 0 0 0 i\n"
  "MouseMoveEvent 520 211 0 0 0 0 i\n"
  "MouseMoveEvent 520 207 0 0 0 0 i\n"
  "MouseMoveEvent 521 203 0 0 0 0 i\n"
  "MouseMoveEvent 521 199 0 0 0 0 i\n"
  "MouseMoveEvent 520 197 0 0 0 0 i\n"
  "MouseMoveEvent 518 197 0 0 0 0 i\n"
  "MouseMoveEvent 515 197 0 0 0 0 i\n"
  "MouseMoveEvent 513 197 0 0 0 0 i\n"
  "MouseMoveEvent 504 197 0 0 0 0 i\n"
  "MouseMoveEvent 494 197 0 0 0 0 i\n"
  "MouseMoveEvent 485 197 0 0 0 0 i\n"
  "MouseMoveEvent 480 197 0 0 0 0 i\n"
  "MouseMoveEvent 472 197 0 0 0 0 i\n"
  "MouseMoveEvent 462 197 0 0 0 0 i\n"
  "MouseMoveEvent 455 197 0 0 0 0 i\n"
  "MouseMoveEvent 450 197 0 0 0 0 i\n"
  "MouseMoveEvent 444 197 0 0 0 0 i\n"
  "MouseMoveEvent 439 197 0 0 0 0 i\n"
  "MouseMoveEvent 433 197 0 0 0 0 i\n"
  "MouseMoveEvent 427 197 0 0 0 0 i\n"
  "MouseMoveEvent 419 197 0 0 0 0 i\n"
  "MouseMoveEvent 412 197 0 0 0 0 i\n"
  "MouseMoveEvent 405 197 0 0 0 0 i\n"
  "MouseMoveEvent 399 197 0 0 0 0 i\n"
  "MouseMoveEvent 393 197 0 0 0 0 i\n"
  "MouseMoveEvent 385 197 0 0 0 0 i\n"
  "MouseMoveEvent 377 197 0 0 0 0 i\n"
  "MouseMoveEvent 368 197 0 0 0 0 i\n"
  "MouseMoveEvent 362 197 0 0 0 0 i\n"
  "MouseMoveEvent 355 197 0 0 0 0 i\n"
  "MouseMoveEvent 347 197 0 0 0 0 i\n"
  "MouseMoveEvent 338 197 0 0 0 0 i\n"
  "MouseMoveEvent 329 197 0 0 0 0 i\n"
  "MouseMoveEvent 324 197 0 0 0 0 i\n"
  "MouseMoveEvent 319 197 0 0 0 0 i\n"
  "MouseMoveEvent 313 197 0 0 0 0 i\n"
  "MouseMoveEvent 308 197 0 0 0 0 i\n"
  "MouseMoveEvent 304 197 0 0 0 0 i\n"
  "MouseMoveEvent 300 197 0 0 0 0 i\n"
  "MouseMoveEvent 297 198 0 0 0 0 i\n"
  "MouseMoveEvent 297 202 0 0 0 0 i\n"
  "MouseMoveEvent 297 206 0 0 0 0 i\n"
  "MouseMoveEvent 297 211 0 0 0 0 i\n"
  "MouseMoveEvent 299 215 0 0 0 0 i\n"
  "MouseMoveEvent 299 220 0 0 0 0 i\n"
  "MouseMoveEvent 300 222 0 0 0 0 i\n"
  "MouseMoveEvent 300 225 0 0 0 0 i\n"
  "MouseMoveEvent 300 227 0 0 0 0 i\n"
  "MouseMoveEvent 300 234 0 0 0 0 i\n"
  "MouseMoveEvent 300 236 0 0 0 0 i\n"
  "MouseMoveEvent 300 239 0 0 0 0 i\n"
  "MouseMoveEvent 300 241 0 0 0 0 i\n"
  "MouseMoveEvent 300 244 0 0 0 0 i\n"
  "MouseMoveEvent 300 246 0 0 0 0 i\n"
  "MouseMoveEvent 299 249 0 0 0 0 i\n"
  "MouseMoveEvent 299 251 0 0 0 0 i\n"
  "MouseMoveEvent 299 254 0 0 0 0 i\n"
  "MouseMoveEvent 299 257 0 0 0 0 i\n"
  "MouseMoveEvent 299 260 0 0 0 0 i\n"
  "MouseMoveEvent 299 265 0 0 0 0 i\n"
  "MouseMoveEvent 299 267 0 0 0 0 i\n"
  "MouseMoveEvent 299 270 0 0 0 0 i\n"
  "MouseMoveEvent 299 272 0 0 0 0 i\n"
  "MouseMoveEvent 299 275 0 0 0 0 i\n"
  "MouseMoveEvent 298 275 0 0 0 0 i\n"
  "LeftButtonReleaseEvent 298 275 0 0 0 0 i\n"
  ;

// This does the actual work: updates the vtkCylinder implicit function.
// This in turn causes the pipeline to update and clip the object.
// Callback for the interaction
class vtkTICWCallback : public vtkCommand
{
public:
  static vtkTICWCallback *New()
  {
    return new vtkTICWCallback;
  }

  void Execute(vtkObject *caller, unsigned long, void*) VTK_OVERRIDE
  {
    vtkImplicitCylinderWidget *cylWidget =
      reinterpret_cast<vtkImplicitCylinderWidget*>(caller);
    vtkImplicitCylinderRepresentation *rep =
      reinterpret_cast<vtkImplicitCylinderRepresentation*>(cylWidget->GetRepresentation());
    rep->GetCylinder(this->Cylinder);
    this->Actor->VisibilityOn();
  }

  vtkTICWCallback() : Cylinder(0), Actor(0) {}

  vtkCylinder *Cylinder;
  vtkActor *Actor;
};

} // anonymous namespace

int TestImplicitCylinderWidget2(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
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

  // This portion of the code clips the mace with the vtkCylinders
  // implicit function. The clipped region is colored green.
  vtkSmartPointer<vtkCylinder> cylinder =
    vtkSmartPointer<vtkCylinder>::New();
  vtkSmartPointer<vtkClipPolyData> clipper =
    vtkSmartPointer<vtkClipPolyData>::New();
  clipper->SetInputConnection(apd->GetOutputPort());
  clipper->SetClipFunction(cylinder);
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
  vtkSmartPointer<vtkTICWCallback> myCallback =
    vtkSmartPointer<vtkTICWCallback>::New();
  myCallback->Cylinder = cylinder;
  myCallback->Actor = selectActor;

  vtkSmartPointer<vtkImplicitCylinderRepresentation> rep =
    vtkSmartPointer<vtkImplicitCylinderRepresentation>::New();
  rep->SetPlaceFactor(1.25);
  rep->PlaceWidget(glyph->GetOutput()->GetBounds());

  vtkSmartPointer<vtkImplicitCylinderWidget> cylWidget =
    vtkSmartPointer<vtkImplicitCylinderWidget>::New();
  cylWidget->SetInteractor(iren);
  cylWidget->SetRepresentation(rep);
  cylWidget->AddObserver(vtkCommand::InteractionEvent,myCallback);

  ren1->AddActor(maceActor);
  ren1->AddActor(selectActor);

  // Add the actors to the renderer, set the background and size
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);
  renWin->SetMultiSamples(0);

  // Tests
  double wbounds[6];
  double center[3], center1[3], center2[3];
  cylWidget->SetEnabled(1);
  rep->GetCenter(center);

  // #1: With ConstrainCenter on, center SHOULD NOT be settable outside widget bounds
  rep->ConstrainToWidgetBoundsOn();
  rep->GetWidgetBounds(wbounds);
  rep->SetCenter(wbounds[1] + 1.0, wbounds[3] + 1.0, wbounds[5] + 1.0);
  rep->GetCenter(center1);
  if (center1[0] > wbounds[1] || center1[1] > wbounds[3] || center1[2] > wbounds[5])
  {
    std::cerr << "center ("
              << center1[0] << "," << center1[1] << "," << center1[2]
              << ") outside widget bounds ("
              << wbounds[0] << "-" << wbounds[1] << ","
              << wbounds[2] << "-" << wbounds[3] << ","
              << wbounds[4] << "-" << wbounds[5] << std::endl;
    return EXIT_FAILURE;
  }

  // #2: With ConstrainCenter off, center SHOULD be settable outside current widget bounds.
  rep->ConstrainToWidgetBoundsOff();
  center1[0] = wbounds[1] + 1.0;
  center1[1] = wbounds[3] + 1.0;
  center1[2] = wbounds[5] + 1.0;
  rep->SetCenter(center1);
  rep->GetCenter(center2);
  if (center1[0] != center2[0] || center1[1] != center2[1] || center1[2] != center2[2])
  {
    std::cerr << "center not set correctly. expected ("
              << center1[0] << "," << center1[1] << "," << center1[2]
              << "), got: ("
              << center2[0] << "," << center2[1] << "," << center2[2]
              << ")" << std::endl;
    return EXIT_FAILURE;
  }

  rep->SetCenter(center);
  cylWidget->SetEnabled(0);

  // #3: With ConstrainCenter on and OutsideBounds off, the translation of the
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
  recorder->SetInputString(eventLog);

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
