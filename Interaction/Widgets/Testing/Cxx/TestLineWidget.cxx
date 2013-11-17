/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLineWidget.cxx

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
#include "vtkLineWidget.h"
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
#include "vtkStreamLine.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredGridOutlineFilter.h"

#include "vtkTestUtilities.h"

const char TestLineWidgetEventLog[] =
  "# StreamVersion 1\n"
  "CharEvent 185 179 0 0 105 1 i\n"
  "KeyReleaseEvent 185 179 0 0 105 1 i\n"
  "MouseMoveEvent 185 178 0 0 0 0 i\n"
  "MouseMoveEvent 184 178 0 0 0 0 i\n"
  "MouseMoveEvent 183 178 0 0 0 0 i\n"
  "MouseMoveEvent 182 177 0 0 0 0 i\n"
  "MouseMoveEvent 179 174 0 0 0 0 i\n"
  "MouseMoveEvent 178 172 0 0 0 0 i\n"
  "MouseMoveEvent 175 171 0 0 0 0 i\n"
  "MouseMoveEvent 173 169 0 0 0 0 i\n"
  "MouseMoveEvent 170 166 0 0 0 0 i\n"
  "MouseMoveEvent 167 165 0 0 0 0 i\n"
  "MouseMoveEvent 161 162 0 0 0 0 i\n"
  "MouseMoveEvent 159 160 0 0 0 0 i\n"
  "MouseMoveEvent 157 159 0 0 0 0 i\n"
  "MouseMoveEvent 156 158 0 0 0 0 i\n"
  "MouseMoveEvent 156 157 0 0 0 0 i\n"
  "MouseMoveEvent 156 156 0 0 0 0 i\n"
  "MouseMoveEvent 156 155 0 0 0 0 i\n"
  "MouseMoveEvent 155 155 0 0 0 0 i\n"
  "MouseMoveEvent 155 154 0 0 0 0 i\n"
  "MouseMoveEvent 155 153 0 0 0 0 i\n"
  "MouseMoveEvent 154 153 0 0 0 0 i\n"
  "MouseMoveEvent 152 152 0 0 0 0 i\n"
  "MouseMoveEvent 151 150 0 0 0 0 i\n"
  "MouseMoveEvent 148 150 0 0 0 0 i\n"
  "MouseMoveEvent 148 149 0 0 0 0 i\n"
  "MouseMoveEvent 148 150 0 0 0 0 i\n"
  "LeftButtonPressEvent 148 150 0 0 0 0 i\n"
  "MouseMoveEvent 148 151 0 0 0 0 i\n"
  "MouseMoveEvent 146 151 0 0 0 0 i\n"
  "MouseMoveEvent 143 151 0 0 0 0 i\n"
  "MouseMoveEvent 142 151 0 0 0 0 i\n"
  "MouseMoveEvent 142 152 0 0 0 0 i\n"
  "MouseMoveEvent 141 151 0 0 0 0 i\n"
  "MouseMoveEvent 136 149 0 0 0 0 i\n"
  "MouseMoveEvent 134 149 0 0 0 0 i\n"
  "MouseMoveEvent 133 149 0 0 0 0 i\n"
  "MouseMoveEvent 132 149 0 0 0 0 i\n"
  "MouseMoveEvent 131 149 0 0 0 0 i\n"
  "MouseMoveEvent 131 150 0 0 0 0 i\n"
  "MouseMoveEvent 130 150 0 0 0 0 i\n"
  "MouseMoveEvent 131 150 0 0 0 0 i\n"
  "MouseMoveEvent 136 150 0 0 0 0 i\n"
  "LeftButtonReleaseEvent 136 150 0 0 0 0 i\n"
  "MouseMoveEvent 136 150 0 0 0 0 i\n"
  "MouseMoveEvent 136 151 0 0 0 0 i\n"
  "MouseMoveEvent 137 151 0 0 0 0 i\n"
  "MouseMoveEvent 138 151 0 0 0 0 i\n"
  "MouseMoveEvent 138 152 0 0 0 0 i\n"
  "MouseMoveEvent 139 152 0 0 0 0 i\n"
  "RightButtonPressEvent 139 152 0 0 0 0 i\n"
  "MouseMoveEvent 138 152 0 0 0 0 i\n"
  "MouseMoveEvent 137 152 0 0 0 0 i\n"
  "MouseMoveEvent 137 153 0 0 0 0 i\n"
  "MouseMoveEvent 136 154 0 0 0 0 i\n"
  "MouseMoveEvent 136 156 0 0 0 0 i\n"
  "MouseMoveEvent 136 157 0 0 0 0 i\n"
  "MouseMoveEvent 136 158 0 0 0 0 i\n"
  "MouseMoveEvent 137 159 0 0 0 0 i\n"
  "MouseMoveEvent 136 159 0 0 0 0 i\n"
  "MouseMoveEvent 136 160 0 0 0 0 i\n"
  "MouseMoveEvent 136 161 0 0 0 0 i\n"
  "MouseMoveEvent 136 160 0 0 0 0 i\n"
  "MouseMoveEvent 136 158 0 0 0 0 i\n"
  "MouseMoveEvent 136 157 0 0 0 0 i\n"
  "MouseMoveEvent 136 155 0 0 0 0 i\n"
  "MouseMoveEvent 135 152 0 0 0 0 i\n"
  "MouseMoveEvent 135 149 0 0 0 0 i\n"
  "MouseMoveEvent 136 147 0 0 0 0 i\n"
  "MouseMoveEvent 136 146 0 0 0 0 i\n"
  "MouseMoveEvent 136 144 0 0 0 0 i\n"
  "MouseMoveEvent 136 143 0 0 0 0 i\n"
  "MouseMoveEvent 136 141 0 0 0 0 i\n"
  "MouseMoveEvent 136 140 0 0 0 0 i\n"
  "MouseMoveEvent 136 139 0 0 0 0 i\n"
  "MouseMoveEvent 135 139 0 0 0 0 i\n"
  "MouseMoveEvent 135 137 0 0 0 0 i\n"
  "MouseMoveEvent 135 136 0 0 0 0 i\n"
  "MouseMoveEvent 134 136 0 0 0 0 i\n"
  "MouseMoveEvent 135 136 0 0 0 0 i\n"
  "RightButtonReleaseEvent 135 136 0 0 0 0 i\n"
  "MouseMoveEvent 135 136 0 0 0 0 i\n"
  "MouseMoveEvent 135 137 0 0 0 0 i\n"
  "MouseMoveEvent 135 138 0 0 0 0 i\n"
  "MouseMoveEvent 135 137 0 0 0 0 i\n"
  "MouseMoveEvent 134 136 0 0 0 0 i\n"
  "MouseMoveEvent 134 135 0 0 0 0 i\n"
  "MouseMoveEvent 134 134 0 0 0 0 i\n"
  "MouseMoveEvent 134 133 0 0 0 0 i\n"
  "MouseMoveEvent 134 132 0 0 0 0 i\n"
  "MouseMoveEvent 134 131 0 0 0 0 i\n"
  "MouseMoveEvent 134 130 0 0 0 0 i\n"
  "MouseMoveEvent 135 130 0 0 0 0 i\n"
  "MouseMoveEvent 135 129 0 0 0 0 i\n"
  "MouseMoveEvent 136 129 0 0 0 0 i\n"
  "MouseMoveEvent 137 129 0 0 0 0 i\n"
  "MouseMoveEvent 137 128 0 0 0 0 i\n"
  "MouseMoveEvent 138 128 0 0 0 0 i\n"
  "MouseMoveEvent 138 129 0 0 0 0 i\n"
  "MouseMoveEvent 137 130 0 0 0 0 i\n"
  "MouseMoveEvent 137 131 0 0 0 0 i\n"
  "MouseMoveEvent 137 132 0 0 0 0 i\n"
  "MouseMoveEvent 137 131 0 0 0 0 i\n"
  "LeftButtonPressEvent 137 131 0 0 0 0 i\n"
  "MouseMoveEvent 137 130 0 0 0 0 i\n"
  "MouseMoveEvent 137 128 0 0 0 0 i\n"
  "MouseMoveEvent 137 127 0 0 0 0 i\n"
  "MouseMoveEvent 138 127 0 0 0 0 i\n"
  "MouseMoveEvent 140 127 0 0 0 0 i\n"
  "MouseMoveEvent 141 126 0 0 0 0 i\n"
  "MouseMoveEvent 143 126 0 0 0 0 i\n"
  "MouseMoveEvent 148 123 0 0 0 0 i\n"
  "MouseMoveEvent 149 123 0 0 0 0 i\n"
  "MouseMoveEvent 149 122 0 0 0 0 i\n"
  "MouseMoveEvent 150 122 0 0 0 0 i\n"
  "MouseMoveEvent 151 122 0 0 0 0 i\n"
  "MouseMoveEvent 152 122 0 0 0 0 i\n"
  "MouseMoveEvent 153 121 0 0 0 0 i\n"
  "MouseMoveEvent 155 121 0 0 0 0 i\n"
  "LeftButtonReleaseEvent 155 121 0 0 0 0 i\n"
  "MouseMoveEvent 155 121 0 0 0 0 i\n"
  "MouseMoveEvent 155 122 0 0 0 0 i\n"
  "MouseMoveEvent 155 123 0 0 0 0 i\n"
  "MouseMoveEvent 155 124 0 0 0 0 i\n"
  "MouseMoveEvent 155 126 0 0 0 0 i\n"
  "MouseMoveEvent 155 127 0 0 0 0 i\n"
  "MouseMoveEvent 155 129 0 0 0 0 i\n"
  "MouseMoveEvent 153 132 0 0 0 0 i\n"
  "MouseMoveEvent 153 135 0 0 0 0 i\n"
  "MouseMoveEvent 152 139 0 0 0 0 i\n"
  "MouseMoveEvent 152 140 0 0 0 0 i\n"
  "MouseMoveEvent 151 142 0 0 0 0 i\n"
  "MouseMoveEvent 151 143 0 0 0 0 i\n"
  "MouseMoveEvent 151 145 0 0 0 0 i\n"
  "MouseMoveEvent 150 146 0 0 0 0 i\n"
  "MouseMoveEvent 149 147 0 0 0 0 i\n"
  "MouseMoveEvent 149 149 0 0 0 0 i\n"
  "MouseMoveEvent 148 151 0 0 0 0 i\n"
  "MouseMoveEvent 146 152 0 0 0 0 i\n"
  "MouseMoveEvent 143 154 0 0 0 0 i\n"
  "MouseMoveEvent 142 155 0 0 0 0 i\n"
  "MouseMoveEvent 141 155 0 0 0 0 i\n"
  "MouseMoveEvent 141 156 0 0 0 0 i\n"
  "MouseMoveEvent 141 157 0 0 0 0 i\n"
  "MouseMoveEvent 141 158 0 0 0 0 i\n"
  "MouseMoveEvent 141 159 0 0 0 0 i\n"
  "MouseMoveEvent 142 160 0 0 0 0 i\n"
  "MouseMoveEvent 142 161 0 0 0 0 i\n"
  "MouseMoveEvent 142 162 0 0 0 0 i\n"
  "MouseMoveEvent 141 162 0 0 0 0 i\n"
  "MouseMoveEvent 140 162 0 0 0 0 i\n"
  "MouseMoveEvent 138 162 0 0 0 0 i\n"
  "MouseMoveEvent 137 163 0 0 0 0 i\n"
  "MouseMoveEvent 137 164 0 0 0 0 i\n"
  "MouseMoveEvent 137 165 0 0 0 0 i\n"
  "MouseMoveEvent 137 166 0 0 0 0 i\n"
  "MouseMoveEvent 137 167 0 0 0 0 i\n"
  "LeftButtonPressEvent 137 167 0 0 0 0 i\n"
  "MouseMoveEvent 138 167 0 0 0 0 i\n"
  "MouseMoveEvent 139 168 0 0 0 0 i\n"
  "MouseMoveEvent 140 168 0 0 0 0 i\n"
  "MouseMoveEvent 141 168 0 0 0 0 i\n"
  "MouseMoveEvent 142 168 0 0 0 0 i\n"
  "MouseMoveEvent 143 168 0 0 0 0 i\n"
  "MouseMoveEvent 144 169 0 0 0 0 i\n"
  "MouseMoveEvent 145 169 0 0 0 0 i\n"
  "MouseMoveEvent 146 169 0 0 0 0 i\n"
  "MouseMoveEvent 146 170 0 0 0 0 i\n"
  "MouseMoveEvent 146 171 0 0 0 0 i\n"
  "MouseMoveEvent 147 171 0 0 0 0 i\n"
  "MouseMoveEvent 148 171 0 0 0 0 i\n"
  "MouseMoveEvent 149 172 0 0 0 0 i\n"
  "MouseMoveEvent 150 172 0 0 0 0 i\n"
  "MouseMoveEvent 151 172 0 0 0 0 i\n"
  "MouseMoveEvent 152 172 0 0 0 0 i\n"
  "MouseMoveEvent 154 172 0 0 0 0 i\n"
  "MouseMoveEvent 155 172 0 0 0 0 i\n"
  "MouseMoveEvent 156 172 0 0 0 0 i\n"
  "MouseMoveEvent 157 172 0 0 0 0 i\n"
  "MouseMoveEvent 158 171 0 0 0 0 i\n"
  "MouseMoveEvent 160 171 0 0 0 0 i\n"
  "MouseMoveEvent 162 170 0 0 0 0 i\n"
  "MouseMoveEvent 163 170 0 0 0 0 i\n"
  "MouseMoveEvent 164 170 0 0 0 0 i\n"
  "MouseMoveEvent 165 170 0 0 0 0 i\n"
  "LeftButtonReleaseEvent 165 170 0 0 0 0 i\n"
  "MouseMoveEvent 165 170 0 0 0 0 i\n"
  ;

// This does the actual work: updates the probe.
// Callback for the interaction
class vtkLWCallback : public vtkCommand
{
public:
  static vtkLWCallback *New()
  { return new vtkLWCallback; }
  virtual void Execute(vtkObject *caller, unsigned long, void*)
  {
    vtkLineWidget *lineWidget = reinterpret_cast<vtkLineWidget*>(caller);
    lineWidget->GetPolyData(this->PolyData);
    this->Actor->VisibilityOn();
  }
  vtkLWCallback():PolyData(0),Actor(0) {}
  vtkPolyData *PolyData;
  vtkActor *Actor;
};

int TestLineWidget( int argc, char *argv[] )
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

  vtkSmartPointer<vtkStreamLine> streamer =
    vtkSmartPointer<vtkStreamLine>::New();
  streamer->SetInputData(pl3d_block0);
  streamer->SetSourceData(seeds);
  streamer->SetMaximumPropagationTime(100);
  streamer->SetIntegrationStepLength(.2);
  streamer->SetStepLength(.001);
  streamer->SetNumberOfThreads(1);
  streamer->SetIntegrationDirectionToForward();
  streamer->VorticityOn();
  streamer->SetIntegrator(rk4);

  vtkSmartPointer<vtkRibbonFilter> rf =
    vtkSmartPointer<vtkRibbonFilter>::New();
  rf->SetInputConnection(streamer->GetOutputPort());
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
  vtkSmartPointer<vtkLWCallback> myCallback =
    vtkSmartPointer<vtkLWCallback>::New();
  myCallback->PolyData = seeds;
  myCallback->Actor = streamline;

  // The plane widget is used probe the dataset.
  //
  vtkSmartPointer<vtkLineWidget> lineWidget =
    vtkSmartPointer<vtkLineWidget>::New();
  lineWidget->SetInteractor(iren);
  lineWidget->SetInputData(pl3d_block0);
  lineWidget->SetAlignToYAxis();
  lineWidget->PlaceWidget();
  lineWidget->GetPolyData(seeds);
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
  recorder->SetInputString(TestLineWidgetEventLog);

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
