/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSeedWidget2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the vtkSeedWidget

// First include the required header files for the VTK classes we are using.
#include "vtkSeedWidget.h"
#include "vtkSeedRepresentation.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCommand.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkRegressionTestImage.h"
#include "vtkDebugLeaks.h"
#include "vtkCoordinate.h"
#include "vtkMath.h"
#include "vtkHandleWidget.h"
#include "vtkPointHandleRepresentation2D.h"
#include "vtkAxisActor2D.h"
#include "vtkProperty2D.h"
#include "vtkVolume16Reader.h"
#include "vtkImageMapToColors.h"
#include "vtkImageActor.h"
#include "vtkLookupTable.h"
#include "vtkTestUtilities.h"

// This callback is responsible for setting the seed label.
class vtkSeedCallback : public vtkCommand
{
public:
  static vtkSeedCallback *New() 
    { return new vtkSeedCallback; }
  virtual void Execute(vtkObject*, unsigned long eid, void*)
    {
      if ( eid == vtkCommand::CursorChangedEvent )
        {
        cout << "cursor changed\n";
        }
      else
        {
        cout << "point placed\n";
        }
    }
};


// The actual test function
int TestSeedWidget2( int argc, char *argv[] )
{
  // Create the RenderWindow, Renderer and both Actors
  //
  vtkRenderer *ren1 = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren1);

  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  // Create a test pipeline
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");

  // Start by creatin a black/white lookup table.
  vtkLookupTable *bwLut = vtkLookupTable::New();
    bwLut->SetTableRange (0, 2000);
    bwLut->SetSaturationRange (0, 0);
    bwLut->SetHueRange (0, 0);
    bwLut->SetValueRange (0, 1);
    bwLut->Build(); //effective built
  vtkVolume16Reader *v16 = vtkVolume16Reader::New();
    v16->SetDataDimensions(64,64);
    v16->SetDataByteOrderToLittleEndian();
    v16->SetFilePrefix(fname);
    v16->SetImageRange(1, 93);
    v16->SetDataSpacing (3.2, 3.2, 1.5);
  delete[] fname;
  vtkImageMapToColors *saggitalColors = vtkImageMapToColors::New();
    saggitalColors->SetInputConnection(v16->GetOutputPort());
    saggitalColors->SetLookupTable(bwLut);
  vtkImageActor *saggital = vtkImageActor::New();
    saggital->SetInput(saggitalColors->GetOutput());
    saggital->SetDisplayExtent(32,32, 0,63, 0,92);

  // Create the widget and its representation
  vtkPointHandleRepresentation2D *handle = vtkPointHandleRepresentation2D::New();
  handle->GetProperty()->SetColor(1,0,0);
  vtkSeedRepresentation *rep = vtkSeedRepresentation::New();
  rep->SetHandleRepresentation(handle);

  vtkSeedWidget *widget = vtkSeedWidget::New();
  widget->SetInteractor(iren);
  widget->SetRepresentation(rep);

  vtkSeedCallback *mcbk = vtkSeedCallback::New();
  widget->AddObserver(vtkCommand::PlacePointEvent,mcbk);
  widget->AddObserver(vtkCommand::CursorChangedEvent,mcbk);

  // Add the actors to the renderer, set the background and size
  //
  ren1->AddActor(saggital);
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // record events
  vtkInteractorEventRecorder *recorder = vtkInteractorEventRecorder::New();
  recorder->SetInteractor(iren);
  recorder->SetFileName("c:/record.log");
//  recorder->Record();
//  recorder->ReadFromInputStringOn();
//  recorder->SetInputString(eventLog);

  // render the image
  //
  iren->Initialize();
  renWin->Render();
  widget->On();
//  recorder->Play();

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }


  bwLut->Delete();
  v16->Delete();
  saggitalColors->Delete();
  saggital->Delete();
  handle->Delete();
  rep->Delete();
  widget->RemoveObserver(mcbk);
  mcbk->Delete();
  widget->Off();
  widget->Delete();
  iren->Delete();
  renWin->Delete();
  ren1->Delete();
  recorder->Delete();
  
  return !retVal;
}
