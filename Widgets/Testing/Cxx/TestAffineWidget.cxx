/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAffineWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the vtkAffineWidget.

// First include the required header files for the VTK classes we are using.
#include "vtkAffineWidget.h"
#include "vtkAffineRepresentation2D.h"
#include "vtkImageActor.h"
#include "vtkImageData.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkInteractorStyleImage.h"
#include "vtkVolume16Reader.h"
#include "vtkImageShiftScale.h"
#include "vtkImageActor.h"
#include "vtkCommand.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkRegressionTestImage.h"
#include "vtkDebugLeaks.h"
#include "vtkTestUtilities.h"
#include "vtkTransform.h"

// This callback is responsible for adjusting the point position.
// It looks in the region around the point and finds the maximum or
// minimum value.
class vtkAffineCallback : public vtkCommand
{
public:
  static vtkAffineCallback *New() 
    { return new vtkAffineCallback; }
  virtual void Execute(vtkObject *caller, unsigned long, void*);
  vtkAffineCallback():ImageActor(0),AffineRep(0) 
    {
      this->Transform = vtkTransform::New();
    }
  ~vtkAffineCallback()
    {
      this->Transform->Delete();
    }
  vtkImageActor *ImageActor;
  vtkAffineRepresentation2D *AffineRep;
  vtkTransform *Transform;
};

// Method re-positions the points using random perturbation
void vtkAffineCallback::Execute(vtkObject*, unsigned long, void*)
{
  this->AffineRep->GetTransform(this->Transform);
  this->ImageActor->SetUserTransform(this->Transform);
}


int TestAffineWidget( int argc, char *argv[] )
{
  // Create the pipeline
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");
   
  vtkVolume16Reader* v16 = vtkVolume16Reader::New();
  v16->SetDataDimensions(64, 64);
  v16->SetDataByteOrderToLittleEndian();
  v16->SetImageRange(1, 93);
  v16->SetDataSpacing(3.2, 3.2, 1.5);
  v16->SetFilePrefix(fname);
  v16->ReleaseDataFlagOn();
  v16->SetDataMask(0x7fff);
  v16->Update();
  delete[] fname;

  double range[2];
  v16->GetOutput()->GetScalarRange(range);

  vtkImageShiftScale* shifter = vtkImageShiftScale::New();
  shifter->SetShift(-1.0*range[0]);
  shifter->SetScale(255.0/(range[1]-range[0]));
  shifter->SetOutputScalarTypeToUnsignedChar();
  shifter->SetInputConnection(v16->GetOutputPort());
  shifter->ReleaseDataFlagOff();
  shifter->Update();
  
  vtkImageActor* imageActor = vtkImageActor::New();
  imageActor->SetInput(shifter->GetOutput());
  imageActor->VisibilityOn();
  imageActor->SetDisplayExtent(0, 63, 0, 63, 46, 46);
  imageActor->InterpolateOn();
    
  double bounds[6];
  imageActor->GetBounds(bounds);
    
  // Create the RenderWindow, Renderer and both Actors
  //
  vtkRenderer *ren1 = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren1);

  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  vtkInteractorStyleImage *style = vtkInteractorStyleImage::New();
  iren->SetInteractorStyle(style);

  // VTK widgets consist of two parts: the widget part that handles event processing;
  // and the widget representation that defines how the widget appears in the scene 
  // (i.e., matters pertaining to geometry).
  vtkAffineRepresentation2D *rep = vtkAffineRepresentation2D::New();
  rep->SetBoxWidth(100);
  rep->SetCircleWidth(75);
  rep->SetAxesWidth(60);
  rep->DisplayTextOn();
  rep->PlaceWidget(bounds);

  vtkAffineWidget *widget = vtkAffineWidget::New();
  widget->SetInteractor(iren);
  widget->SetRepresentation(rep);

  vtkAffineCallback *acbk = vtkAffineCallback::New();
  acbk->AffineRep = rep;
  acbk->ImageActor = imageActor;
  widget->AddObserver(vtkCommand::InteractionEvent,acbk);
  widget->AddObserver(vtkCommand::EndInteractionEvent,acbk);

  // Add the actors to the renderer, set the background and size
  ren1->AddActor(imageActor);
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
//  recorder->Play();

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();

  int retVal = vtkRegressionTestImage( renWin );
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  v16->Delete();
  shifter->Delete();
  imageActor->Delete();
  widget->Off();
  widget->RemoveObserver(acbk);
  widget->Delete();
  acbk->Delete();
  rep->Delete();
  style->Delete();
  iren->Delete();
  renWin->Delete();
  ren1->Delete();
  recorder->Delete();
  
  return !retVal;

}


