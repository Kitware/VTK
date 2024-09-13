// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This tests multiple interactor timers simultaneously.

#include "vtkCommand.h"
#include "vtkNew.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkTesting.h"

namespace
{
constexpr int REALLY_FAST_TIME = 3;
constexpr int FAST_TIME = 25;
constexpr int RENDER_TIME = 100;
constexpr int SLOW_TIME = 1500;
constexpr int END_TIME = 4000;

bool CheckCount(int fullTime, int shortTime, int count)
{
  int expected = fullTime / shortTime;
  int tolerance = expected / 5; // 20% tolerance
  return (count < expected - tolerance || count > expected + tolerance) ? false : true;
}
}

class vtkTimerCallback : public vtkCommand
{
public:
  static vtkTimerCallback* New()
  {
    vtkTimerCallback* cb = new vtkTimerCallback;
    cb->ReallyFastTimerId = 0;
    cb->ReallyFastTimerCount = 0;
    cb->FastTimerId = 0;
    cb->FastTimerCount = 0;
    cb->RenderTimerId = 0;
    cb->RenderTimerCount = 0;
    cb->SlowTimerId = 0;
    cb->SlowTimerCount = 0;
    cb->OneShotTimerId = 0;
    cb->QuitOnOneShotTimer = 1;
    return cb;
  }

  void Execute(vtkObject* caller, unsigned long eventId, void* callData) override
  {
    if (vtkCommand::TimerEvent == eventId)
    {
      int tid = *static_cast<int*>(callData);

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

        vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::SafeDownCast(caller);
        if (iren && iren->GetRenderWindow() && iren->GetRenderWindow()->GetRenderers())
        {
          int n = this->RenderTimerCount % 20;
          if (n > 10)
          {
            n = 20 - n;
          }

          double f = static_cast<double>(n) / 10.0;

          vtkRenderer* renderer = iren->GetRenderWindow()->GetRenderers()->GetFirstRenderer();
          if (renderer)
          {
            renderer->SetBackground(f, f, f);
          }

          iren->Render();
        }
      }
      else if (tid == this->SlowTimerId)
      {
        ++this->SlowTimerCount;
      }
      else if (tid == this->OneShotTimerId)
      {
        this->Report();

        if (this->QuitOnOneShotTimer)
        {
          cout << "QuitOnOneShotTimer is true." << endl;

          vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::SafeDownCast(caller);
          if (iren)
          {
            iren->DestroyTimer(this->ReallyFastTimerId);
            iren->DestroyTimer(this->FastTimerId);
            iren->DestroyTimer(this->RenderTimerId);
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

  void SetSlowTimerId(int tid)
  {
    this->SlowTimerId = tid;
    this->SlowTimerCount = 0;
  }

  void SetOneShotTimerId(int tid) { this->OneShotTimerId = tid; }

  void SetQuitOnOneShotTimer(int quit) { this->QuitOnOneShotTimer = quit; }

  void Report()
  {
    cout << "vtkTimerCallback::Report" << endl;
    cout << "  ReallyFastTimerId: " << this->ReallyFastTimerId << endl;
    cout << "  ReallyFastTimerCount: " << this->ReallyFastTimerCount << endl;
    cout << "  FastTimerId: " << this->FastTimerId << endl;
    cout << "  FastTimerCount: " << this->FastTimerCount << endl;
    cout << "  RenderTimerId: " << this->RenderTimerId << endl;
    cout << "  RenderTimerCount: " << this->RenderTimerCount << endl;
    cout << "  SlowTimerId: " << this->RenderTimerId << endl;
    cout << "  SlowTimerCount: " << this->SlowTimerCount << endl;
    cout << "  OneShotTimerId: " << this->OneShotTimerId << endl;
    cout << "  QuitOnOneShotTimer: " << this->QuitOnOneShotTimer << endl;
  }

  bool CheckTimerCount()
  {
    // Really fast timer can't be tested reliably as it may heavilly impacted by
    // the CPU charge on some systems.
    if (!::CheckCount(::END_TIME, ::REALLY_FAST_TIME, this->ReallyFastTimerCount))
    {
      std::cout << "Unexpected really fast timer count: " << this->ReallyFastTimerCount
                << std::endl;
      std::cout << "This does not count as an error" << std::endl;
    }

    bool ret = true;
    if (!::CheckCount(::END_TIME, ::FAST_TIME, this->FastTimerCount))
    {
      std::cerr << "Unexpected fast timer count:" << this->FastTimerCount << std::endl;
      ret = false;
    }
    if (!::CheckCount(::END_TIME, ::RENDER_TIME, this->RenderTimerCount))
    {
      std::cerr << "Unexpected render timer count:" << this->RenderTimerCount << std::endl;
      ret = false;
    }
    if (!::CheckCount(::END_TIME, ::SLOW_TIME, this->SlowTimerCount))
    {
      std::cerr << "Unexpected slow timer count:" << this->SlowTimerCount << std::endl;
      ret = false;
    }
    return ret;
  }

private:
  int ReallyFastTimerId;
  int ReallyFastTimerCount;
  int FastTimerId;
  int FastTimerCount;
  int RenderTimerId;
  int RenderTimerCount;
  int SlowTimerId;
  int SlowTimerCount;
  int OneShotTimerId;
  int QuitOnOneShotTimer;
};

int TestInteractorTimers(int argc, char* argv[])
{
  int i;

  vtkNew<vtkTesting> testing;
  for (i = 0; i < argc; ++i)
  {
    testing->AddArgument(argv[i]);
  }

  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renWin;
  // only run unit test with X11 window.
  if (!renWin || !renWin->IsA("vtkXOpenGLRenderWindow"))
  {
    return VTK_SKIP_RETURN_CODE;
  }
  renWin->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // Initialize must be called prior to creating timer events.
  //
  cout << "Calling iren->Initialize()..." << endl;
  iren->Initialize();

  // Sign up to receive TimerEvent:
  //
  vtkNew<vtkTimerCallback> cb;
  iren->AddObserver(vtkCommand::TimerEvent, cb);

  // Create two relatively fast repeating timers:
  //
  int tid;
  tid = iren->CreateRepeatingTimer(::REALLY_FAST_TIME);
  cb->SetReallyFastTimerId(tid);

  tid = iren->CreateRepeatingTimer(::FAST_TIME);
  cb->SetFastTimerId(tid);

  // Create a slower repeating timer to trigger Render calls.
  // (This fires at the rate of approximately 10 frames per second.)
  //
  tid = iren->CreateRepeatingTimer(::RENDER_TIME);
  cb->SetRenderTimerId(tid);

  // Create a very slow repeating timer.
  // (This fires at the rate of approximately once every 1.5s.)
  //
  tid = iren->CreateRepeatingTimer(::SLOW_TIME);
  cb->SetSlowTimerId(tid);

  // And create a one shot timer to quit after 4 seconds.
  //
  tid = iren->CreateOneShotTimer(::END_TIME);
  cb->SetOneShotTimerId(tid);
  cb->SetQuitOnOneShotTimer(!testing->IsInteractiveModeSpecified());

  // Run event loop until the one shot timer fires:
  //
  cout << "Calling iren->Start()..." << endl;
  iren->Start();

  bool ret = cb->CheckTimerCount();

  return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
