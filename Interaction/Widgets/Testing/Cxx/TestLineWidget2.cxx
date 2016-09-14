/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLineWidget2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSmartPointer.h"

#include "vtkActor.h"
#include "vtkCommand.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkLineWidget2.h"
#include "vtkLineRepresentation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiBlockPLOT3DReader.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRibbonFilter.h"
#include "vtkRungeKutta4.h"
#include "vtkStreamTracer.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredGridOutlineFilter.h"

#include "vtkTestUtilities.h"

#include "TestLineWidget2EventLog.h"
#include <string>


// This does the actual work: updates the probe.
// Callback for the interaction
class vtkLW2Callback : public vtkCommand
{
public:
  static vtkLW2Callback *New()
  { return new vtkLW2Callback; }
  void Execute(vtkObject *caller, unsigned long, void*) VTK_OVERRIDE
  {
    vtkLineWidget2 *lineWidget = reinterpret_cast<vtkLineWidget2*>(caller);
    vtkLineRepresentation *rep =
      reinterpret_cast<vtkLineRepresentation*>(lineWidget->GetRepresentation());
    rep->GetPolyData(this->PolyData);
    this->Actor->VisibilityOn();
  }
  vtkLW2Callback():PolyData(0),Actor(0) {}
  vtkPolyData *PolyData;
  vtkActor *Actor;
};

int TestLineWidget2( int argc, char *argv[] )
{
  char* fname =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/combxyz.bin");
  char* fname2 =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/combq.bin");

  // Start by loading some data.
  //
  vtkSmartPointer<vtkMultiBlockPLOT3DReader> pl3d =
    vtkSmartPointer<vtkMultiBlockPLOT3DReader>::New();
  pl3d->SetXYZFileName(fname);
  pl3d->SetQFileName(fname2);
  pl3d->SetScalarFunctionNumber(100);
  pl3d->SetVectorFunctionNumber(202);
  pl3d->Update();
  vtkDataSet* pl3d_block0 = vtkDataSet::SafeDownCast(pl3d->GetOutput()->GetBlock(0));

  delete [] fname;
  delete [] fname2;

  vtkSmartPointer<vtkPolyData> seeds =
    vtkSmartPointer<vtkPolyData>::New();

  // Create streamtues
  vtkSmartPointer<vtkRungeKutta4> rk4 =
    vtkSmartPointer<vtkRungeKutta4>::New();

  vtkSmartPointer<vtkStreamTracer> streamer =
    vtkSmartPointer<vtkStreamTracer>::New();
  streamer->SetInputData(pl3d_block0);
  streamer->SetSourceData(seeds);
  streamer->SetMaximumPropagation(100);
  streamer->SetInitialIntegrationStep(.2);
  streamer->SetIntegrationDirectionToForward();
  streamer->SetComputeVorticity(1);
  streamer->SetIntegrator(rk4);

  vtkSmartPointer<vtkRibbonFilter> rf =
    vtkSmartPointer<vtkRibbonFilter>::New();
  rf->SetInputConnection(streamer->GetOutputPort());
  rf->SetInputArrayToProcess(1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "Normals");
  rf->SetWidth(0.1);
  rf->SetWidthFactor(5);

  vtkSmartPointer<vtkPolyDataMapper> streamMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  streamMapper->SetInputConnection(rf->GetOutputPort());
  double tmp[2];
  pl3d_block0->GetScalarRange(tmp);
  streamMapper->SetScalarRange(tmp[0], tmp[1]);

  vtkSmartPointer<vtkActor> streamline =
    vtkSmartPointer<vtkActor>::New();
  streamline->SetMapper(streamMapper);
  streamline->VisibilityOff();

  // An outline is shown for context.
  vtkSmartPointer<vtkStructuredGridOutlineFilter> outline =
    vtkSmartPointer<vtkStructuredGridOutlineFilter>::New();
  outline->SetInputData(pl3d_block0);

  vtkSmartPointer<vtkPolyDataMapper> outlineMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  outlineMapper->SetInputConnection(outline->GetOutputPort());

  vtkSmartPointer<vtkActor> outlineActor =
    vtkSmartPointer<vtkActor>::New();
  outlineActor->SetMapper(outlineMapper);

  // Create the RenderWindow, Renderer and both Actors
  //
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
  vtkSmartPointer<vtkLW2Callback> myCallback =
    vtkSmartPointer<vtkLW2Callback>::New();
  myCallback->PolyData = seeds;
  myCallback->Actor = streamline;

  // The line widget is used probe the dataset.
  //
  double p[3];
  vtkSmartPointer<vtkLineRepresentation> rep =
    vtkSmartPointer<vtkLineRepresentation>::New();
  p[0] = 0.0; p[1] = -1.0; p[2] = 0.0;
  rep->SetPoint1WorldPosition(p);
  p[0] = 0.0; p[1] =  1.0; p[2] = 0.0;
  rep->SetPoint2WorldPosition(p);
  rep->PlaceWidget(pl3d_block0->GetBounds());
  rep->GetPolyData(seeds);
  rep->DistanceAnnotationVisibilityOn();

  vtkSmartPointer<vtkLineWidget2> lineWidget =
    vtkSmartPointer<vtkLineWidget2>::New();
  lineWidget->SetInteractor(iren);
  lineWidget->SetRepresentation(rep);
  lineWidget->AddObserver(vtkCommand::InteractionEvent,myCallback);

  ren1->AddActor(streamline);
  ren1->AddActor(outlineActor);

  // Add the actors to the renderer, set the background and size
  //
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // record events
  vtkSmartPointer<vtkInteractorEventRecorder> recorder =
    vtkSmartPointer<vtkInteractorEventRecorder>::New();
  recorder->SetInteractor(iren);
//  recorder->SetFileName("c:/record.log");
//  recorder->Record();
  recorder->ReadFromInputStringOn();
  std::string TestLineWidget2EventLog(TestLineWidget2EventLog_p1);
  TestLineWidget2EventLog += TestLineWidget2EventLog_p2;
  TestLineWidget2EventLog += TestLineWidget2EventLog_p3;
  recorder->SetInputString(TestLineWidget2EventLog.c_str());

  // render the image
  //
  iren->Initialize();
  renWin->Render();
  recorder->Play();

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();

  iren->Start();

  return EXIT_SUCCESS;
}
