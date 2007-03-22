/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestHandleWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the vtkSliderWidget.

#include "vtkConeSource.h"
#include "vtkGlyph3D.h"
#include "vtkSphereSource.h"
#include "vtkAppendPolyData.h"
#include "vtkHandleWidget.h"
#include "vtkPointHandleRepresentation3D.h"
#include "vtkPointHandleRepresentation2D.h"
#include "vtkCoordinate.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCommand.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkRegressionTestImage.h"
#include "vtkDebugLeaks.h"
#include "vtkOutlineFilter.h"
#include "vtkImplicitPlaneWidget2.h"
#include "vtkImplicitPlaneRepresentation.h"
#include "vtkBoundedPlanePointPlacer.h"
#include "vtkCutter.h"
#include "vtkLODActor.h"
#include "vtkPlane.h"
#include "vtkProperty.h"
#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

// This does the actual work: updates the vtkPline implicit function.
// This in turn causes the pipeline to update and clip the object.
// Callback for the interaction
class vtkTIPW3Callback : public vtkCommand
{
public:
  static vtkTIPW3Callback *New() 
    { return new vtkTIPW3Callback; }
  virtual void Execute(vtkObject *caller, unsigned long, void*)
    {
      vtkImplicitPlaneWidget2 *planeWidget = 
        reinterpret_cast<vtkImplicitPlaneWidget2*>(caller);
      vtkImplicitPlaneRepresentation *rep = 
        reinterpret_cast<vtkImplicitPlaneRepresentation*>(planeWidget->GetRepresentation());
      rep->GetPlane(this->Plane);
      this->Actor->VisibilityOn();
    }
  vtkTIPW3Callback():Plane(0),Actor(0) {}
  vtkPlane *Plane;
  vtkActor *Actor;

};

char HandleWidgetLog[] = 
"# StreamVersion 1\n"
"ConfigureEvent 600 -1 0 0 0 0 0 i\n"
"ExposeEvent 300 299 0 0 0 0 0 i\n"
"EnterEvent 598 197 0 0 0 0 0 i\n"
"MouseMoveEvent 586 197 0 0 0 0 0 i\n"
"MouseMoveEvent 585 197 0 0 0 0 0 i\n"
"MouseMoveEvent 585 196 0 0 0 0 0 i\n"
"MouseMoveEvent 585 195 0 0 0 0 0 i\n"
"MouseMoveEvent 585 194 0 0 0 0 0 i\n"
"MouseMoveEvent 581 188 0 0 0 0 0 i\n"
"MouseMoveEvent 571 184 0 0 0 0 0 i\n"
"MouseMoveEvent 559 180 0 0 0 0 0 i\n"
"MouseMoveEvent 549 178 0 0 0 0 0 i\n"
"MouseMoveEvent 547 177 0 0 0 0 0 i\n"
"MouseMoveEvent 548 177 0 0 0 0 0 i\n"
"MouseMoveEvent 550 177 0 0 0 0 0 i\n"
"MouseMoveEvent 551 176 0 0 0 0 0 i\n"
"MouseMoveEvent 551 175 0 0 0 0 0 i\n"
"MouseMoveEvent 552 175 0 0 0 0 0 i\n"
"MouseMoveEvent 552 174 0 0 0 0 0 i\n"
"KeyPressEvent 552 174 0 0 116 1 t i\n"
"CharEvent 552 174 0 0 116 1 t i\n"
"KeyReleaseEvent 552 174 0 0 116 1 t i\n"
"MouseMoveEvent 546 176 0 0 0 0 t i\n"
"MouseMoveEvent 530 188 0 0 0 0 t i\n"
"MouseMoveEvent 504 218 0 0 0 0 t i\n"
"MouseMoveEvent 504 226 0 0 0 0 t i\n"
"MouseMoveEvent 518 214 0 0 0 0 t i\n"
"MouseMoveEvent 528 208 0 0 0 0 t i\n"
"MouseMoveEvent 534 206 0 0 0 0 t i\n"
"MouseMoveEvent 540 200 0 0 0 0 t i\n"
"MouseMoveEvent 548 194 0 0 0 0 t i\n"
"MouseMoveEvent 556 186 0 0 0 0 t i\n"
"MouseMoveEvent 566 178 0 0 0 0 t i\n"
"MouseMoveEvent 567 178 0 0 0 0 t i\n"
"MouseMoveEvent 568 179 0 0 0 0 t i\n"
"MouseMoveEvent 569 179 0 0 0 0 t i\n"
"MouseMoveEvent 569 180 0 0 0 0 t i\n"
"MouseMoveEvent 569 181 0 0 0 0 t i\n"
"MouseMoveEvent 570 181 0 0 0 0 t i\n"
"MouseMoveEvent 570 182 0 0 0 0 t i\n"
"LeftButtonPressEvent 570 182 0 0 0 0 t i\n"
"MouseMoveEvent 345 96 0 0 0 0 t i\n"
"MouseMoveEvent 347 106 0 0 0 0 t i\n"
"MouseMoveEvent 347 124 0 0 0 0 t i\n"
"MouseMoveEvent 349 148 0 0 0 0 t i\n"
"MouseMoveEvent 349 172 0 0 0 0 t i\n"
"MouseMoveEvent 349 198 0 0 0 0 t i\n"
"MouseMoveEvent 349 222 0 0 0 0 t i\n"
"MouseMoveEvent 347 244 0 0 0 0 t i\n"
"MouseMoveEvent 343 264 0 0 0 0 t i\n"
"MouseMoveEvent 341 276 0 0 0 0 t i\n"
"MouseMoveEvent 335 286 0 0 0 0 t i\n"
"MouseMoveEvent 333 292 0 0 0 0 t i\n"
"MouseMoveEvent 332 294 0 0 0 0 t i\n"
"MouseMoveEvent 331 295 0 0 0 0 t i\n"
"MouseMoveEvent 330 297 0 0 0 0 t i\n"
"MouseMoveEvent 329 299 0 0 0 0 t i\n"
"MouseMoveEvent 327 305 0 0 0 0 t i\n"
"MouseMoveEvent 321 313 0 0 0 0 t i\n"
"MouseMoveEvent 320 315 0 0 0 0 t i\n"
"MouseMoveEvent 314 319 0 0 0 0 t i\n"
"MouseMoveEvent 308 325 0 0 0 0 t i\n"
"MouseMoveEvent 304 329 0 0 0 0 t i\n"
"MouseMoveEvent 298 335 0 0 0 0 t i\n"
"MouseMoveEvent 292 339 0 0 0 0 t i\n"
"MouseMoveEvent 288 343 0 0 0 0 t i\n"
"MouseMoveEvent 282 345 0 0 0 0 t i\n"
"MouseMoveEvent 281 346 0 0 0 0 t i\n"
"MouseMoveEvent 279 346 0 0 0 0 t i\n"
"MouseMoveEvent 278 346 0 0 0 0 t i\n"
"MouseMoveEvent 278 348 0 0 0 0 t i\n"
"MouseMoveEvent 279 350 0 0 0 0 t i\n"
"MouseMoveEvent 279 352 0 0 0 0 t i\n"
"MouseMoveEvent 279 353 0 0 0 0 t i\n"
"MouseMoveEvent 279 355 0 0 0 0 t i\n"
"MouseMoveEvent 279 354 0 0 0 0 t i\n"
"MouseMoveEvent 279 353 0 0 0 0 t i\n"
"MouseMoveEvent 279 352 0 0 0 0 t i\n"
"MouseMoveEvent 279 350 0 0 0 0 t i\n"
"MouseMoveEvent 279 349 0 0 0 0 t i\n"
"MouseMoveEvent 280 348 0 0 0 0 t i\n"
"MouseMoveEvent 281 347 0 0 0 0 t i\n"
"MouseMoveEvent 281 346 0 0 0 0 t i\n"
"MouseMoveEvent 282 346 0 0 0 0 t i\n"
"LeftButtonPressEvent 282 346 0 0 0 0 t i\n"
"MouseMoveEvent 375 323 0 0 0 0 t i\n"
"MouseMoveEvent 369 321 0 0 0 0 t i\n"
"MouseMoveEvent 361 319 0 0 0 0 t i\n"
"MouseMoveEvent 351 317 0 0 0 0 t i\n"
"MouseMoveEvent 333 315 0 0 0 0 t i\n"
"MouseMoveEvent 317 315 0 0 0 0 t i\n"
"MouseMoveEvent 301 315 0 0 0 0 t i\n"
"MouseMoveEvent 287 317 0 0 0 0 t i\n"
"MouseMoveEvent 273 319 0 0 0 0 t i\n"
"MouseMoveEvent 265 319 0 0 0 0 t i\n"
"MouseMoveEvent 264 319 0 0 0 0 t i\n"
"MouseMoveEvent 263 319 0 0 0 0 t i\n"
"MouseMoveEvent 263 318 0 0 0 0 t i\n"
"MouseMoveEvent 264 318 0 0 0 0 t i\n"
"MouseMoveEvent 265 318 0 0 0 0 t i\n"
"MouseMoveEvent 266 318 0 0 0 0 t i\n"
"MouseMoveEvent 267 317 0 0 0 0 t i\n"
"MouseMoveEvent 275 315 0 0 0 0 t i\n"
"MouseMoveEvent 281 311 0 0 0 0 t i\n"
"MouseMoveEvent 287 309 0 0 0 0 t i\n"
"MouseMoveEvent 290 309 0 0 0 0 t i\n"
"MouseMoveEvent 291 309 0 0 0 0 t i\n"
"MouseMoveEvent 292 309 0 0 0 0 t i\n"
"LeftButtonPressEvent 292 309 0 0 0 0 t i\n"
"MouseMoveEvent 358 228 0 0 0 0 t i\n"
"MouseMoveEvent 358 238 0 0 0 0 t i\n"
"MouseMoveEvent 358 250 0 0 0 0 t i\n"
"MouseMoveEvent 358 258 0 0 0 0 t i\n"
"MouseMoveEvent 358 268 0 0 0 0 t i\n"
"MouseMoveEvent 358 271 0 0 0 0 t i\n"
"MouseMoveEvent 356 279 0 0 0 0 t i\n"
"MouseMoveEvent 354 285 0 0 0 0 t i\n"
"MouseMoveEvent 354 287 0 0 0 0 t i\n"
"MouseMoveEvent 352 293 0 0 0 0 t i\n"
"MouseMoveEvent 351 295 0 0 0 0 t i\n"
"MouseMoveEvent 350 297 0 0 0 0 t i\n"
"MouseMoveEvent 350 298 0 0 0 0 t i\n"
"MouseMoveEvent 350 299 0 0 0 0 t i\n"
"MouseMoveEvent 350 300 0 0 0 0 t i\n"
"MouseMoveEvent 350 301 0 0 0 0 t i\n"
"MouseMoveEvent 350 302 0 0 0 0 t i\n"
"MouseMoveEvent 350 303 0 0 0 0 t i\n"
"MouseMoveEvent 350 304 0 0 0 0 t i\n"
"MouseMoveEvent 351 305 0 0 0 0 t i\n"
"MouseMoveEvent 351 306 0 0 0 0 t i\n"
"MouseMoveEvent 351 307 0 0 0 0 t i\n"
"MouseMoveEvent 352 308 0 0 0 0 t i\n"
"MouseMoveEvent 352 309 0 0 0 0 t i\n"
"MouseMoveEvent 353 309 0 0 0 0 t i\n"
"MouseMoveEvent 353 310 0 0 0 0 t i\n"
"MouseMoveEvent 353 311 0 0 0 0 t i\n"
"MouseMoveEvent 353 312 0 0 0 0 t i\n"
"MouseMoveEvent 353 313 0 0 0 0 t i\n"
"MouseMoveEvent 353 314 0 0 0 0 t i\n"
"LeftButtonPressEvent 353 314 0 0 0 0 t i\n"
"MouseMoveEvent 338 439 0 0 0 0 t i\n"
"MouseMoveEvent 338 429 0 0 0 0 t i\n"
"MouseMoveEvent 344 413 0 0 0 0 t i\n"
"MouseMoveEvent 356 395 0 0 0 0 t i\n"
"MouseMoveEvent 378 381 0 0 0 0 t i\n"
"MouseMoveEvent 406 373 0 0 0 0 t i\n"
"MouseMoveEvent 440 365 0 0 0 0 t i\n"
"MouseMoveEvent 470 365 0 0 0 0 t i\n"
"MouseMoveEvent 490 365 0 0 0 0 t i\n"
"MouseMoveEvent 498 369 0 0 0 0 t i\n"
"MouseMoveEvent 499 371 0 0 0 0 t i\n"
"MouseMoveEvent 500 373 0 0 0 0 t i\n"
"MouseMoveEvent 500 374 0 0 0 0 t i\n"
"MouseMoveEvent 500 376 0 0 0 0 t i\n"
"MouseMoveEvent 500 378 0 0 0 0 t i\n"
"MouseMoveEvent 500 386 0 0 0 0 t i\n"
"MouseMoveEvent 500 394 0 0 0 0 t i\n"
"MouseMoveEvent 498 402 0 0 0 0 t i\n"
"MouseMoveEvent 498 412 0 0 0 0 t i\n"
"MouseMoveEvent 494 418 0 0 0 0 t i\n"
"MouseMoveEvent 492 426 0 0 0 0 t i\n"
"MouseMoveEvent 488 434 0 0 0 0 t i\n"
"MouseMoveEvent 482 440 0 0 0 0 t i\n"
"MouseMoveEvent 474 448 0 0 0 0 t i\n"
"MouseMoveEvent 472 454 0 0 0 0 t i\n"
"MouseMoveEvent 471 456 0 0 0 0 t i\n"
"MouseMoveEvent 470 456 0 0 0 0 t i\n"
"MouseMoveEvent 470 457 0 0 0 0 t i\n"
"MouseMoveEvent 470 459 0 0 0 0 t i\n"
"MouseMoveEvent 470 460 0 0 0 0 t i\n"
"LeftButtonPressEvent 470 460 0 0 0 0 t i\n"
"MouseMoveEvent 536 235 0 0 0 0 t i\n"
"MouseMoveEvent 530 241 0 0 0 0 t i\n"
"MouseMoveEvent 516 247 0 0 0 0 t i\n"
"MouseMoveEvent 500 255 0 0 0 0 t i\n"
"MouseMoveEvent 484 263 0 0 0 0 t i\n"
"MouseMoveEvent 472 269 0 0 0 0 t i\n"
"MouseMoveEvent 460 273 0 0 0 0 t i\n"
"MouseMoveEvent 452 279 0 0 0 0 t i\n"
"MouseMoveEvent 444 281 0 0 0 0 t i\n"
"MouseMoveEvent 443 281 0 0 0 0 t i\n"
"MouseMoveEvent 443 282 0 0 0 0 t i\n"
"MouseMoveEvent 441 282 0 0 0 0 t i\n"
"MouseMoveEvent 439 283 0 0 0 0 t i\n"
"MouseMoveEvent 433 285 0 0 0 0 t i\n"
"MouseMoveEvent 423 285 0 0 0 0 t i\n"
"MouseMoveEvent 413 287 0 0 0 0 t i\n"
"MouseMoveEvent 410 287 0 0 0 0 t i\n"
"MouseMoveEvent 408 287 0 0 0 0 t i\n"
"MouseMoveEvent 407 287 0 0 0 0 t i\n"
"MouseMoveEvent 407 288 0 0 0 0 t i\n"
"MouseMoveEvent 407 289 0 0 0 0 t i\n"
"MouseMoveEvent 406 289 0 0 0 0 t i\n"
"MouseMoveEvent 406 290 0 0 0 0 t i\n"
"LeftButtonPressEvent 406 290 0 0 0 0 t i\n"
"MouseMoveEvent 403 413 0 0 0 0 t i\n"
"MouseMoveEvent 395 409 0 0 0 0 t i\n"
"MouseMoveEvent 387 401 0 0 0 0 t i\n"
"MouseMoveEvent 377 393 0 0 0 0 t i\n"
"MouseMoveEvent 369 385 0 0 0 0 t i\n"
"MouseMoveEvent 363 381 0 0 0 0 t i\n"
"MouseMoveEvent 362 379 0 0 0 0 t i\n"
"MouseMoveEvent 361 378 0 0 0 0 t i\n"
"MouseMoveEvent 361 377 0 0 0 0 t i\n"
"MouseMoveEvent 362 377 0 0 0 0 t i\n"
"MouseMoveEvent 366 381 0 0 0 0 t i\n"
"MouseMoveEvent 367 381 0 0 0 0 t i\n"
"MouseMoveEvent 368 382 0 0 0 0 t i\n"
"MouseMoveEvent 368 383 0 0 0 0 t i\n"
"MouseMoveEvent 369 383 0 0 0 0 t i\n"
"LeftButtonPressEvent 369 383 0 0 0 0 t i\n"
"MouseMoveEvent 455 388 0 0 0 0 t i\n"
"MouseMoveEvent 447 388 0 0 0 0 t i\n"
"MouseMoveEvent 439 386 0 0 0 0 t i\n"
"MouseMoveEvent 431 386 0 0 0 0 t i\n"
"MouseMoveEvent 428 386 0 0 0 0 t i\n"
"MouseMoveEvent 426 386 0 0 0 0 t i\n"
"MouseMoveEvent 425 386 0 0 0 0 t i\n"
"MouseMoveEvent 424 386 0 0 0 0 t i\n"
"MouseMoveEvent 423 386 0 0 0 0 t i\n"
"MouseMoveEvent 423 387 0 0 0 0 t i\n"
"MouseMoveEvent 423 388 0 0 0 0 t i\n"
"MouseMoveEvent 422 389 0 0 0 0 t i\n"
"MouseMoveEvent 422 390 0 0 0 0 t i\n"
"MouseMoveEvent 421 390 0 0 0 0 t i\n"
"MouseMoveEvent 421 391 0 0 0 0 t i\n"
"MouseMoveEvent 420 392 0 0 0 0 t i\n"
"MouseMoveEvent 420 393 0 0 0 0 t i\n"
"MouseMoveEvent 420 394 0 0 0 0 t i\n"
"MouseMoveEvent 419 394 0 0 0 0 t i\n"
"LeftButtonPressEvent 419 394 0 0 0 0 t i\n"
"MouseMoveEvent 395 280 0 0 0 0 t i\n"
"MouseMoveEvent 395 279 0 0 0 0 t i\n"
"MouseMoveEvent 397 273 0 0 0 0 t i\n"
"MouseMoveEvent 401 267 0 0 0 0 t i\n"
"MouseMoveEvent 405 261 0 0 0 0 t i\n"
"MouseMoveEvent 411 253 0 0 0 0 t i\n"
"MouseMoveEvent 415 245 0 0 0 0 t i\n"
"MouseMoveEvent 421 237 0 0 0 0 t i\n"
"MouseMoveEvent 429 229 0 0 0 0 t i\n"
"MouseMoveEvent 437 219 0 0 0 0 t i\n"
"MouseMoveEvent 445 211 0 0 0 0 t i\n"
"MouseMoveEvent 451 205 0 0 0 0 t i\n"
"MouseMoveEvent 452 203 0 0 0 0 t i\n"
"MouseMoveEvent 453 202 0 0 0 0 t i\n"
"MouseMoveEvent 454 202 0 0 0 0 t i\n"
"MouseMoveEvent 454 201 0 0 0 0 t i\n"
"MouseMoveEvent 454 199 0 0 0 0 t i\n"
"MouseMoveEvent 454 198 0 0 0 0 t i\n"
"MouseMoveEvent 454 195 0 0 0 0 t i\n"
"MouseMoveEvent 454 194 0 0 0 0 t i\n"
"MouseMoveEvent 454 193 0 0 0 0 t i\n"
"MouseMoveEvent 454 190 0 0 0 0 t i\n"
"MouseMoveEvent 454 187 0 0 0 0 t i\n"
"MouseMoveEvent 456 181 0 0 0 0 t i\n"
"MouseMoveEvent 456 179 0 0 0 0 t i\n"
"MouseMoveEvent 456 178 0 0 0 0 t i\n"
"MouseMoveEvent 456 177 0 0 0 0 t i\n"
"MouseMoveEvent 457 176 0 0 0 0 t i\n"
"KeyPressEvent 457 176 0 0 113 1 q i\n"
"CharEvent 457 176 0 0 113 1 q i\n"
"ExitEvent 457 176 0 0 113 1 q i\n"
;

int TestHandleWidget( int argc, char *argv[] )
{
  // Create a mace out of filters.
  //
  vtkSphereSource *sphere = vtkSphereSource::New();
  vtkConeSource *cone = vtkConeSource::New();
  vtkGlyph3D *glyph = vtkGlyph3D::New();
  glyph->SetInputConnection(sphere->GetOutputPort());
  glyph->SetSource(cone->GetOutput());
  glyph->SetVectorModeToUseNormal();
  glyph->SetScaleModeToScaleByVector();
  glyph->SetScaleFactor(0.25);

  // The sphere and spikes are appended into a single polydata. 
  // This just makes things simpler to manage.
  vtkAppendPolyData *apd = vtkAppendPolyData::New();
  apd->AddInput(glyph->GetOutput());
  apd->AddInput(sphere->GetOutput());

  // This portion of the code clips the mace with the vtkPlanes 
  // implicit function. The cut region is colored green.
  vtkPlane *plane = vtkPlane::New();
  vtkCutter *cutter = vtkCutter::New();
  cutter->SetInputConnection(apd->GetOutputPort());
  cutter->SetCutFunction(plane);

  vtkPolyDataMapper *selectMapper = vtkPolyDataMapper::New();
  selectMapper->SetInputConnection(cutter->GetOutputPort());

  vtkLODActor *selectActor = vtkLODActor::New();
  selectActor->SetMapper(selectMapper);
  selectActor->GetProperty()->SetColor(0,1,0);
  selectActor->VisibilityOff();
  selectActor->SetScale(1.01, 1.01, 1.01);  
  
  vtkOutlineFilter* outline = vtkOutlineFilter::New();
    outline->SetInputConnection(apd->GetOutputPort());
  vtkPolyDataMapper* outlineMapper = vtkPolyDataMapper::New();
    outlineMapper->SetInputConnection(outline->GetOutputPort());
  vtkActor* outlineActor =  vtkActor::New();
    outlineActor->SetMapper( outlineMapper);

  vtkImplicitPlaneRepresentation *rep = vtkImplicitPlaneRepresentation::New();
  rep->SetPlaceFactor(0.7);
  rep->GetPlaneProperty()->SetAmbientColor(0.0, 0.5, 0.5);
  rep->GetPlaneProperty()->SetOpacity(0.3);
  rep->PlaceWidget(outline->GetOutput()->GetBounds());  
  vtkImplicitPlaneWidget2 *planeWidget = vtkImplicitPlaneWidget2::New();
  planeWidget->SetRepresentation(rep);

  vtkTIPW3Callback *myCallback = vtkTIPW3Callback::New();
  myCallback->Plane = plane;
  myCallback->Actor = selectActor;

  planeWidget->AddObserver(vtkCommand::InteractionEvent,myCallback);
  

  // Create the RenderWindow, Renderer and both Actors
  //
  vtkRenderer *ren1 = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren1);

  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  // VTK widgets consist of two parts: the widget part that handles event processing;
  // and the widget representation that defines how the widget appears in the scene 
  // (i.e., matters pertaining to geometry).
  vtkPointHandleRepresentation3D *handleRep 
    = vtkPointHandleRepresentation3D::New();
  handleRep->SetPlaceFactor(2.5);
  handleRep->PlaceWidget(outlineActor->GetBounds());
  handleRep->SetHandleSize(10);

  vtkHandleWidget *handleWidget = vtkHandleWidget::New();
  handleWidget->SetInteractor(iren);
  planeWidget->SetInteractor(iren);
  handleWidget->SetRepresentation(handleRep);

  ren1->AddActor(selectActor);
  ren1->AddActor(outlineActor);

  // Add the actors to the renderer, set the background and size
  //

  // record events
  vtkInteractorEventRecorder *recorder = vtkInteractorEventRecorder::New();
  recorder->SetInteractor(iren);
  //recorder->SetFileName("/tmp/record.log");
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(HandleWidgetLog); 
  recorder->EnabledOn();
  
  // Should we constrain the handles to the oblique plane ?
  bool constrainHandlesToObliquePlane = false;
  for (int i = 0; i < argc; i++)
    {
    if (strcmp("-ConstrainHandlesToPlane", argv[i]) == 0)
      {
      constrainHandlesToObliquePlane = true;
      break;
      }
    }

  // Set some defaults.
  //
  rep->SetNormal(0.942174, 0.25322, 0.219519);
  double worldPos[3] = {-0.0417953, 0.202206, -0.0538641};
  handleRep->SetWorldPosition(worldPos);
  rep->GetPlane(myCallback->Plane);
  
  if (constrainHandlesToObliquePlane)
    {
    vtkBoundedPlanePointPlacer *placer = vtkBoundedPlanePointPlacer::New();

    // Define the the plane as the image plane widget's plane
    placer->SetProjectionNormalToOblique();
    placer->SetObliquePlane(myCallback->Plane);

    // Also add bounding planes for the bounds of the dataset.
    double bounds[6];
    outline->GetOutput()->GetBounds(bounds);
    plane = vtkPlane::New();
    plane->SetOrigin( bounds[0], bounds[2], bounds[4] );
    plane->SetNormal( 1.0, 0.0, 0.0 );
    placer->AddBoundingPlane( plane );
    plane->Delete();
    
    plane = vtkPlane::New();
    plane->SetOrigin( bounds[1], bounds[3], bounds[5] );
    plane->SetNormal( -1.0, 0.0, 0.0 );
    placer->AddBoundingPlane( plane );
    plane->Delete();
      
    plane = vtkPlane::New();
    plane->SetOrigin( bounds[0], bounds[2], bounds[4] );
    plane->SetNormal( 0.0, 1.0, 0.0 );
    placer->AddBoundingPlane( plane );
    plane->Delete();
    
    plane = vtkPlane::New();
    plane->SetOrigin( bounds[1], bounds[3], bounds[5] );
    plane->SetNormal( 0.0, -1.0, 0.0 );
    placer->AddBoundingPlane( plane );
    plane->Delete();
    
    plane = vtkPlane::New();
    plane->SetOrigin( bounds[0], bounds[2], bounds[4] );
    plane->SetNormal( 0.0, 0.0, 1.0 );
    placer->AddBoundingPlane( plane );
    plane->Delete();
    
    plane = vtkPlane::New();
    plane->SetOrigin( bounds[1], bounds[3], bounds[5] );
    plane->SetNormal( 0.0, 0.0, -1.0 );
    placer->AddBoundingPlane( plane );
    plane->Delete();

    handleRep->SetPointPlacer(placer);
    placer->Delete();

    }

  iren->Initialize();
  handleWidget->EnabledOn();
  planeWidget->EnabledOn();
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(600, 600);
  ren1->ResetCamera();
  ren1->ResetCameraClippingRange();
  renWin->Render();

  recorder->Play();
  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  apd->Delete();
  planeWidget->Delete();
  glyph->Delete();
  sphere->Delete();
  cone->Delete();
  rep->Delete();
  myCallback->Delete();
  selectMapper->Delete();
  selectActor->Delete();
  cutter->Delete();
  myCallback->Plane->Delete();
  outline->Delete();
  outlineMapper->Delete();
  outlineActor->Delete();
  handleWidget->Delete();
  handleRep->Delete();
  iren->Delete();
  renWin->Delete();
  ren1->Delete();
  recorder->Delete();

  return !retVal;

}

