/*=========================================================================

  Program:   Visualization Toolkit
  Module:    BoxWidget.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAppendPolyData.h"
#include "vtkBoxWidget.h"
#include "vtkCommand.h"
#include "vtkConeSource.h"
#include "vtkGlyph3D.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTransform.h"

#include "vtkRegressionTestImage.h"
#include "vtkDebugLeaks.h"

// Callback for the interaction
class vtkMyCallback : public vtkCommand
{
public:
  static vtkMyCallback *New() 
    { return new vtkMyCallback; }
  virtual void Execute(vtkObject *caller, unsigned long, void*)
    {
      vtkBoxWidget *boxWidget = reinterpret_cast<vtkBoxWidget*>(caller);
      boxWidget->GetTransform(this->Transform);
      this->Actor->SetUserTransform(this->Transform);
    }
  vtkMyCallback():Transform(0),Actor(0) {}
  vtkTransform *Transform;
  vtkActor     *Actor;
};


int main( int argc, char *argv[] )
{
  vtkDebugLeaks::PromptUserOff();

  vtkRenderer *renderer = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(renderer);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);

  vtkBoxWidget *boxWidget = vtkBoxWidget::New();
    boxWidget->SetInteractor( iren );
    boxWidget->SetPlaceFactor( 1.25 );

  vtkConeSource *cone = vtkConeSource::New();
    cone->SetResolution(6);
  vtkSphereSource *sphere = vtkSphereSource::New();
    sphere->SetThetaResolution(8); sphere->SetPhiResolution(8);
  vtkGlyph3D *glyph = vtkGlyph3D::New();
    glyph->SetInput(sphere->GetOutput());
    glyph->SetSource(cone->GetOutput());
    glyph->SetVectorModeToUseNormal();
    glyph->SetScaleModeToScaleByVector();
    glyph->SetScaleFactor(0.25);
                                                        
  vtkAppendPolyData *append = vtkAppendPolyData::New();
    append->AddInput(glyph->GetOutput());
    append->AddInput(sphere->GetOutput());
  
  vtkPolyDataMapper *maceMapper = vtkPolyDataMapper::New();
    maceMapper->SetInput(append->GetOutput());

  vtkActor *maceActor = vtkActor::New();
    maceActor->SetMapper(maceMapper);

  renderer->AddActor(maceActor);
  renderer->SetBackground(0,0,0);
  renWin->SetSize(300,300);

  // Configure the box widget including callbacks
  vtkTransform *t = vtkTransform::New();
  boxWidget->SetProp3D(maceActor);
  boxWidget->PlaceWidget();

  vtkMyCallback *myCallback = vtkMyCallback::New();
  myCallback->Transform = t;
  myCallback->Actor = maceActor;
  boxWidget->AddObserver(vtkCommand::InteractionEvent,myCallback);

  // interact with data
  renWin->Render();
  iren->SetKeyCode('i');
  iren->InvokeEvent(vtkCommand::CharEvent,NULL);
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );

  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  // Clean up
  renderer->Delete();
  renWin->Delete();
  iren->Delete();
  sphere->Delete();
  cone->Delete();
  glyph->Delete();
  append->Delete();
  maceMapper->Delete();
  maceActor->Delete();
  boxWidget->Delete();
  t->Delete();
  myCallback->Delete();

  return !retVal;
}
