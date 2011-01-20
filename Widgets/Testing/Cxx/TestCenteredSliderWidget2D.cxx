/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCenteredSliderWidget2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the vtkSliderWidget with a 2D representation.

// First include the required header files for the VTK classes we are using.
#include "vtkSmartPointer.h"

#include "vtkCenteredSliderWidget.h"
#include "vtkSliderRepresentation2D.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkAppendPolyData.h"
#include "vtkClipPolyData.h"
#include "vtkConeSource.h"
#include "vtkGlyph3D.h"
#include "vtkActor.h"
#include "vtkLODActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCommand.h"
#include "vtkSphere.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkWidgetEventTranslator.h"
#include "vtkWidgetEvent.h"
#include "vtkSmartPointer.h"

// This does the actual work: updates the probe.
// Callback for the interaction
class vtkCenteredSlider2DCallback : public vtkCommand
{
public:
  static vtkCenteredSlider2DCallback *New()
  { return new vtkCenteredSlider2DCallback; }
  virtual void Execute(vtkObject *caller, unsigned long, void*)
  {
    vtkCenteredSliderWidget *sliderWidget =
      reinterpret_cast<vtkCenteredSliderWidget*>(caller);
    double widgetValue = sliderWidget->GetValue();
    this->Glyph->SetScaleFactor(this->Glyph->GetScaleFactor()*widgetValue);
  }
  vtkCenteredSlider2DCallback():Glyph(0) {}
  vtkGlyph3D *Glyph;
};

int TestCenteredSliderWidget2D(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  // Create a mace out of filters.
  vtkSmartPointer<vtkSphereSource> sphereSource =
    vtkSmartPointer<vtkSphereSource>::New();
  vtkSmartPointer<vtkConeSource> cone =
    vtkSmartPointer<vtkConeSource>::New();
  vtkSmartPointer<vtkGlyph3D> glyph =
    vtkSmartPointer<vtkGlyph3D>::New();
  glyph->SetInput(sphereSource->GetOutput());
  glyph->SetSource(cone->GetOutput());
  glyph->SetVectorModeToUseNormal();
  glyph->SetScaleModeToScaleByVector();
  glyph->SetScaleFactor(0.25);

  // The sphere and spikes are appended into a single polydata.
  // This just makes things simpler to manage.
  vtkSmartPointer<vtkAppendPolyData> apd =
    vtkSmartPointer<vtkAppendPolyData>::New();
  apd->AddInput(glyph->GetOutput());
  apd->AddInput(sphereSource->GetOutput());

  vtkSmartPointer<vtkPolyDataMapper> maceMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  maceMapper->SetInput(apd->GetOutput());

  vtkSmartPointer<vtkLODActor> maceActor =
    vtkSmartPointer<vtkLODActor>::New();
  maceActor->SetMapper(maceMapper);
  maceActor->VisibilityOn();
  maceActor->SetPosition(1,1,1);

  // Create the RenderWindow, Renderer and both Actors
  vtkSmartPointer<vtkRenderer> ren1 =
    vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer(ren1);

  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  // VTK widgets consist of two parts: the widget part that handles event
  // processing; and the widget representation that defines how the widget
  // appears in the scene (i.e., matters pertaining to geometry).
  vtkSmartPointer<vtkSliderRepresentation2D> sliderRep =
    vtkSmartPointer<vtkSliderRepresentation2D>::New();
  sliderRep->SetMinimumValue(0.7);
  sliderRep->SetMaximumValue(1.3);
  sliderRep->SetValue(1.0);
  sliderRep->SetTitleText("Spike Size");
  sliderRep->GetPoint1Coordinate()->SetCoordinateSystemToNormalizedDisplay();
  sliderRep->GetPoint1Coordinate()->SetValue(0.2,0.1);
  sliderRep->GetPoint2Coordinate()->SetCoordinateSystemToNormalizedDisplay();
  sliderRep->GetPoint2Coordinate()->SetValue(0.8,0.1);
  sliderRep->SetSliderLength(0.02);
  sliderRep->SetSliderWidth(0.03);
  sliderRep->SetEndCapLength(0.03);
  sliderRep->SetEndCapWidth(0.03);
  sliderRep->SetTubeWidth(0.005);

  vtkSmartPointer<vtkCenteredSliderWidget> sliderWidget =
    vtkSmartPointer<vtkCenteredSliderWidget>::New();
  sliderWidget->SetInteractor(iren);
  sliderWidget->SetRepresentation(sliderRep);

  vtkSmartPointer<vtkCenteredSlider2DCallback> callback =
    vtkSmartPointer<vtkCenteredSlider2DCallback>::New();
  callback->Glyph = glyph;
  sliderWidget->AddObserver(vtkCommand::InteractionEvent,callback);

  ren1->AddActor(maceActor);

  // Add the actors to the renderer, set the background and size
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // record events
  vtkSmartPointer<vtkInteractorEventRecorder> recorder =
    vtkSmartPointer<vtkInteractorEventRecorder>::New();
  recorder->SetInteractor(iren);
  recorder->SetFileName("c:/record.log");
//  recorder->Record();
//  recorder->ReadFromInputStringOn();
//  recorder->SetInputString(eventLog);

  // render the image
  iren->Initialize();
  renWin->Render();
//  recorder->Play();

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();

  iren->Start();

  return EXIT_SUCCESS;
}
