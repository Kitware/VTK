/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestInteractorTimers.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests multiple interactor timers simultaneously.

#include "vtkCommand.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTesting.h"

class vtkTimerCallback : public vtkCommand
{
public:
  static vtkTimerCallback *New()
    {
    vtkTimerCallback *cb = new vtkTimerCallback;
    cb->ReallyFastTimerId = 0;
    cb->ReallyFastTimerCount = 0;
    cb->FastTimerId = 0;
    cb->FastTimerCount = 0;
    cb->RenderTimerId = 0;
    cb->RenderTimerCount = 0;
    cb->OneShotTimerId = 0;
    cb->QuitOnOneShotTimer = 1;
    return cb;
    }

  virtual void Execute(vtkObject *caller, unsigned long eventId,
    void *callData)
    {
    if (vtkCommand::TimerEvent == eventId)
      {
      int tid = * static_cast<int *>(callData);

      if (tid == this->ReallyFastTimerId)
        {
        ++this->ReallyFastTimerCount;
        }
      else if (tid == this->FastTimerId)
        {
        ++this->FastTimerCount;
        }
      else if (tid == this->RenderTimerId)
        {
        ++this->RenderTimerCount;

        vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::SafeDownCast(caller);
        if (iren && iren->GetRenderWindow() && iren->GetRenderWindow()->GetRenderers())
          {
          int n = this->RenderTimerCount % 20;
          if (n>10)
            {
            n = 20 - n;
            }

          double f = static_cast<double>(n) / 10.0;

          vtkRenderer *renderer = iren->GetRenderWindow()->GetRenderers()->GetFirstRenderer();
          if (renderer)
            {
            renderer->SetBackground(f, f, f);
            }

          iren->Render();
          }
        }
      else if (tid == this->OneShotTimerId)
        {
        this->Report();

        if (this->QuitOnOneShotTimer)
          {
          cout << "QuitOnOneShotTimer is true." << endl;

          vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::SafeDownCast(caller);
          if (iren)
            {
            cout << "Calling iren->ExitCallback()..." << endl;
            iren->ExitCallback();
            }
          }
        else
          {
          cout << "QuitOnOneShotTimer is false." << endl;
          cout << "Remaining interactive..." << endl;
          }
        }
      }
    }

  void SetReallyFastTimerId(int tid)
    {
    this->ReallyFastTimerId = tid;
    this->ReallyFastTimerCount = 0;
    }

  void SetFastTimerId(int tid)
    {
    this->FastTimerId = tid;
    this->FastTimerCount = 0;
    }

  void SetRenderTimerId(int tid)
    {
    this->RenderTimerId = tid;
    this->RenderTimerCount = 0;
    }

  void SetOneShotTimerId(int tid)
    {
    this->OneShotTimerId = tid;
    }

  void SetQuitOnOneShotTimer(int quit)
    {
    this->QuitOnOneShotTimer = quit;
    }

  void Report()
    {
    cout << "vtkTimerCallback::Report" << endl;
    cout << "  ReallyFastTimerId: " << this->ReallyFastTimerId << endl;
    cout << "  ReallyFastTimerCount: " << this->ReallyFastTimerCount << endl;
    cout << "  FastTimerId: " << this->FastTimerId << endl;
    cout << "  FastTimerCount: " << this->FastTimerCount << endl;
    cout << "  RenderTimerId: " << this->RenderTimerId << endl;
    cout << "  RenderTimerCount: " << this->RenderTimerCount << endl;
    cout << "  OneShotTimerId: " << this->OneShotTimerId << endl;
    cout << "  QuitOnOneShotTimer: " << this->QuitOnOneShotTimer << endl;
    }

private:
  int ReallyFastTimerId;
  int ReallyFastTimerCount;
  int FastTimerId;
  int FastTimerCount;
  int RenderTimerId;
  int RenderTimerCount;
  int OneShotTimerId;
  int QuitOnOneShotTimer;
};

int TestInteractorTimers(int argc, char* argv[])
{
  int i;

  vtkTesting * testing = vtkTesting::New();
  for (i = 0; i < argc; ++i)
    {
    testing->AddArgument(argv[i]);
    }

  vtkRenderer *renderer = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(renderer);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  // Initialize must be called prior to creating timer events.
  //
  cout << "Calling iren->Initialize()..." << endl;
  iren->Initialize();

  // Sign up to receive TimerEvent:
  //
  vtkTimerCallback *cb = vtkTimerCallback::New();
  iren->AddObserver(vtkCommand::TimerEvent, cb);

  // Create two relatively fast repeating timers:
  //
  int tid;
  tid = iren->CreateRepeatingTimer(3);
  cb->SetReallyFastTimerId(tid);

  tid = iren->CreateRepeatingTimer(25);
  cb->SetFastTimerId(tid);

  // Create a slower repeating timer to trigger Render calls.
  // (This fires at the rate of approximately 10 frames per second.)
  //
  tid = iren->CreateRepeatingTimer(100);
  cb->SetRenderTimerId(tid);

  // And create a one shot timer to quit after 10 seconds.
  //
  tid = iren->CreateOneShotTimer(10000);
  cb->SetOneShotTimerId(tid);
  cb->SetQuitOnOneShotTimer(!testing->IsInteractiveModeSpecified());

  // Run event loop until the one shot timer fires:
  //
  cout << "Calling iren->Start()..." << endl;
  iren->Start();

  // Clean up:
  //
  cb->Delete();
  renderer->Delete();
  renWin->Delete();
  iren->Delete();
  testing->Delete();

  return 0;
}
