// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This tests that the X event loop does not wait for the display connection
// when events are already in the Xlib queue.
//
// A timer callback can read events from the display connection into the Xlib
// queue (a render does this). If the event loop then waits for the
// connection to become readable, those events are processed only when
// something else wakes up the loop (a timer or a new event). With pointer
// motion hints this blocks an interaction: the X server does not send another
// motion event until the queued one is processed.

#include "vtkCommand.h"
#include "vtkNew.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTesting.h"
#include "vtkTimerLog.h"
#include "vtkX11Functions.h"

#include <cstring>
#include <iostream>

namespace
{
constexpr int QUEUE_EVENT_TIME = 100;
// wakes up the event loop if the queued event is not processed
constexpr int WAKE_UP_TIME = 3000;
constexpr double MAX_DELAY = 1.0;

class vtkQueuedEventCallback : public vtkCommand
{
public:
  static vtkQueuedEventCallback* New() { return new vtkQueuedEventCallback; }

  /**
   * Handles the three events of the test:
   * - the QueueEventTimerId timer puts an event in the Xlib queue,
   * - ExitEvent, invoked when the event loop processes that event, stores
   *   how long the event waited in the queue (Delay) and ends the event loop,
   * - the WakeUpTimerId timer ends the event loop if the queued event was
   *   not processed. Without it, an event loop that ignores the Xlib queue
   *   would wait forever.
   */
  void Execute(vtkObject* caller, unsigned long eventId, void* callData) override
  {
    auto iren = vtkRenderWindowInteractor::SafeDownCast(caller);
    if (eventId == vtkCommand::TimerEvent)
    {
      int tid = *static_cast<int*>(callData);
      if (tid == this->QueueEventTimerId)
      {
        this->QueueDeleteWindowEvent(iren->GetRenderWindow());
      }
      else if (tid == this->WakeUpTimerId)
      {
        iren->TerminateApp();
      }
    }
    else if (eventId == vtkCommand::ExitEvent)
    {
      this->Delay = vtkTimerLog::GetUniversalTime() - this->QueueTime;
      iren->TerminateApp();
    }
  }

  /**
   * Sends a WM_DELETE_WINDOW message (the interactor invokes ExitEvent for
   * it) and reads it into the Xlib queue, leaving the connection empty.
   */
  void QueueDeleteWindowEvent(vtkRenderWindow* renWin)
  {
    auto display = static_cast<Display*>(renWin->GetGenericDisplayId());
    auto window = reinterpret_cast<Window>(renWin->GetGenericWindowId());
    XClientMessageEvent client;
    memset(&client, 0, sizeof(client));
    client.type = ClientMessage;
    client.display = display;
    client.window = window;
    client.message_type = vtkXInternAtom(display, "WM_PROTOCOLS", False);
    client.format = 32;
    client.data.l[0] = vtkXInternAtom(display, "WM_DELETE_WINDOW", False);
    this->QueueTime = vtkTimerLog::GetUniversalTime();
    vtkXSendEvent(display, window, False, NoEventMask, reinterpret_cast<XEvent*>(&client));
    vtkXSync(display, False);
  }

  int QueueEventTimerId = 0;
  int WakeUpTimerId = 0;
  double QueueTime = 0.0;
  double Delay = -1.0;
};
}

int TestXInteractorQueuedEvents(int, char*[])
{
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
  iren->Initialize();
  renWin->Render();

  vtkNew<vtkQueuedEventCallback> cb;
  iren->AddObserver(vtkCommand::TimerEvent, cb);
  iren->AddObserver(vtkCommand::ExitEvent, cb);
  cb->QueueEventTimerId = iren->CreateOneShotTimer(::QUEUE_EVENT_TIME);
  cb->WakeUpTimerId = iren->CreateOneShotTimer(::WAKE_UP_TIME);
  iren->Start();

  if (cb->Delay < 0)
  {
    std::cerr << "The queued event was not processed." << std::endl;
    return EXIT_FAILURE;
  }
  std::cout << "The queued event was processed after " << cb->Delay << " s." << std::endl;
  if (cb->Delay > ::MAX_DELAY)
  {
    std::cerr << "The event loop waited for the display connection with an event in the queue."
              << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
