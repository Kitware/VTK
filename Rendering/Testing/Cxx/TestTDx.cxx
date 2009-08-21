/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTDx.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers the 3DConnexion device interface.
// 
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#include "vtkRenderWindowInteractor.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkActor.h"

#include "vtkPolyDataMapper.h"
#include "vtkCamera.h"

#include "vtkConeSource.h"
#include "vtkProperty.h"
#include "vtkTDxMotionEventInfo.h"
#include "vtkCommand.h"

class myCommand : public vtkCommand
{
public:
  myCommand()
    {
      this->Camera=0;
    }
  void Execute(vtkObject *caller, unsigned long eventId, 
               void *callData)
    {
      cout << "myCommand::Execute()" << endl;
      if(eventId==vtkCommand::TDxMotionEvent)
        {
        vtkTDxMotionEventInfo *i=
          static_cast<vtkTDxMotionEventInfo *>(callData);
        
        cout << "x=" << i->X << " y=" << i->Y << " z=" << i->Z
             << " a=" << i->A << " b=" << i->B << " c=" << i->C << endl;
        
        const double sensitivity=0.02;
        
        this->Camera->Roll(i->B*sensitivity);
        this->RenderWindowInteractor->Render();
        }
      else
        {
        cout << "unexpected VTK event" << endl;
        }
    }
  void SetCamera(vtkCamera *c)
    {
      this->Camera=c;
    }
  
  void SetRenderWindowInteractor(vtkRenderWindowInteractor *i)
    {
      this->RenderWindowInteractor=i;
    }
  
protected:
  vtkCamera *Camera;
  vtkRenderWindowInteractor *RenderWindowInteractor;
  
};

int TestTDx(int argc, char* argv[])
{
  vtkRenderWindowInteractor *iren=vtkRenderWindowInteractor::New();
  iren->SetUseTDx(true);
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->SetMultiSamples(0);
  
  renWin->SetAlphaBitPlanes(1);
  iren->SetRenderWindow(renWin);
  renWin->Delete();
  
  vtkRenderer *renderer = vtkRenderer::New();
  renWin->AddRenderer(renderer);
  renderer->Delete();

  vtkConeSource *coneSource1=vtkConeSource::New();
  vtkPolyDataMapper *coneMapper1=vtkPolyDataMapper::New();
  coneMapper1->SetInputConnection(coneSource1->GetOutputPort());
  coneSource1->Delete();
  vtkActor *coneActor1=vtkActor::New();
  coneActor1->SetMapper(coneMapper1);
  coneMapper1->Delete();
  coneActor1->SetPosition(-2.0,0.0,0.0);
  renderer->AddActor(coneActor1);
  coneActor1->Delete();
  
  vtkConeSource *coneSource2=vtkConeSource::New();
  vtkPolyDataMapper *coneMapper2=vtkPolyDataMapper::New();
  coneMapper2->SetInputConnection(coneSource2->GetOutputPort());
  coneSource2->Delete();
  vtkActor *coneActor2=vtkActor::New();
  coneActor2->SetMapper(coneMapper2);
  coneMapper2->Delete();
  coneActor2->SetPosition(0.0,0.0,0.0);
  coneActor2->GetProperty()->SetLighting(false);
  renderer->AddActor(coneActor2);
  coneActor2->Delete();
  
  vtkConeSource *coneSource3=vtkConeSource::New();
  vtkPolyDataMapper *coneMapper3=vtkPolyDataMapper::New();
  coneMapper3->SetInputConnection(coneSource3->GetOutputPort());
  coneSource3->Delete();
  vtkActor *coneActor3=vtkActor::New();
  coneActor3->SetMapper(coneMapper3);
  coneMapper3->Delete();
  coneActor3->SetPosition(2.0,0.0,0.0);
  renderer->AddActor(coneActor3);
  coneActor3->Delete();
  
  renderer->SetBackground(0.1,0.3,0.0);
  renWin->SetSize(200,200);
  
  renWin->Render();
  
  vtkCamera *camera=renderer->GetActiveCamera();
  camera->Azimuth(-40.0);
  camera->Elevation(20.0);
  renderer->ResetCamera();
  renWin->Render();
  
  myCommand *c=new myCommand;
  c->Register(0);
  c->SetCamera(camera);
  c->SetRenderWindowInteractor(iren);
  
  iren->AddObserver(vtkCommand::TDxMotionEvent,c,0);
  
  
  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }
  iren->Delete();
  
  c->Delete();
  c->Delete();
  
  return !retVal;
}
