/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLRenderTimerLog.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLRenderTimerLog.h"

#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderTimer.h"

#include <algorithm>
#include <cassert>
#include <utility>

vtkStandardNewMacro(vtkOpenGLRenderTimerLog)

//------------------------------------------------------------------------------
void vtkOpenGLRenderTimerLog::PrintSelf(std::ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "CurrentFrame: "
     << this->CurrentFrame.ChildCount << " events logged\n"
     << indent << "PendingFrames: " << this->PendingFrames.size() << " frames\n"
     << indent << "ReadyFrames: " << this->ReadyFrames.size() << " frames\n"
     << indent << "TimerPool: " << this->TimerPool.size() << " free timers\n";
}

//------------------------------------------------------------------------------
bool vtkOpenGLRenderTimerLog::IsSupported()
{
  return vtkOpenGLRenderTimer::IsSupported();
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderTimerLog::MarkFrame()
{
  if (!this->DoLogging())
  {
    return;
  }

  // Do nothing if no events on current frame:
  if (this->CurrentFrame.ChildCount == 0)
  {
    return;
  }

  // Stop any running timers (otherwise the PendingQueue will get clogged, since
  // such timers would never be ready)
  this->ForceCloseFrame(this->CurrentFrame);

  this->PendingFrames.push_back(this->CurrentFrame);
  this->CurrentFrame.ChildCount = 0;
  this->CurrentFrame.Events.clear();
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderTimerLog::MarkStartEvent(const std::string &name)
{
  if (!this->DoLogging())
  {
    return;
  }

  OGLEvent &event = this->NewEvent();
  event.Name = name;
  event.Timer = this->NewTimer();
  event.Timer->Start();
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderTimerLog::MarkEndEvent()
{
  if (!this->DoLogging())
  {
    return;
  }

  OGLEvent *event = this->DeepestOpenEvent();
  if (event)
  {
    event->Timer->Stop();
  }
  else
  {
    vtkWarningMacro("No open events to end.");
  }
}

//------------------------------------------------------------------------------
bool vtkOpenGLRenderTimerLog::FrameReady()
{
  if (!this->DoLogging())
  {
    return false;
  }

  this->CheckPendingFrames();
  return !this->ReadyFrames.empty();
}

//------------------------------------------------------------------------------
vtkRenderTimerLog::Frame vtkOpenGLRenderTimerLog::PopFirstReadyFrame()
{
  if (!this->DoLogging() || this->ReadyFrames.empty())
  {
    return Frame();
  }

  Frame frame = this->ReadyFrames.front();
  this->ReadyFrames.pop();
  return frame;
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderTimerLog::ReleaseGraphicsResources()
{
  this->ReleaseOGLFrame(this->CurrentFrame);
  this->CurrentFrame.ChildCount = 0;
  this->CurrentFrame.Events.clear();
  while (!this->PendingFrames.empty())
  {
    this->ReleaseOGLFrame(this->PendingFrames.front());
    this->PendingFrames.pop_front();
  }
}

//------------------------------------------------------------------------------
vtkOpenGLRenderTimerLog::vtkOpenGLRenderTimerLog()
  : MinTimerPoolSize(32)
{

}

//------------------------------------------------------------------------------
vtkOpenGLRenderTimerLog::~vtkOpenGLRenderTimerLog()
{
  this->ReleaseGraphicsResources();
  while (!this->TimerPool.empty())
  {
    delete this->TimerPool.front();
    this->TimerPool.pop();
  }
}

//------------------------------------------------------------------------------
bool vtkOpenGLRenderTimerLog::DoLogging()
{
  if (!this->LoggingEnabled)
  {
    return false;
  }

  if (this->IsSupported())
  {
    return true;
  }

  // Disable logging if not supported.
  this->LoggingEnabled = false;
  return false;
}

//------------------------------------------------------------------------------
vtkRenderTimerLog::Frame
vtkOpenGLRenderTimerLog::Convert(const OGLFrame &oglFrame)
{
  Frame frame;
  frame.Events.reserve(oglFrame.Events.size());
  for (auto event : oglFrame.Events)
  {
    frame.Events.push_back(this->Convert(event));
  }

  return frame;
}

//------------------------------------------------------------------------------
vtkRenderTimerLog::Event
vtkOpenGLRenderTimerLog::Convert(const OGLEvent &oglEvent)
{
  Event event;
  event.Name = oglEvent.Name;
  event.StartTime = oglEvent.Timer->GetStartTime();
  event.EndTime = oglEvent.Timer->GetStopTime();

  event.Events.reserve(oglEvent.Events.size());
  for (auto subOGLEvent : oglEvent.Events)
  {
    event.Events.push_back(this->Convert(subOGLEvent));
  }

  return event;
}

//------------------------------------------------------------------------------
vtkOpenGLRenderTimerLog::OGLEvent& vtkOpenGLRenderTimerLog::NewEvent()
{
  ++this->CurrentFrame.ChildCount;
  OGLEvent *openEvent = this->DeepestOpenEvent();

  if (openEvent)
  {
    openEvent->Events.push_back(OGLEvent());
    return openEvent->Events.back();
  }
  else
  {
    this->CurrentFrame.Events.push_back(OGLEvent());
    return this->CurrentFrame.Events.back();
  }
}

//------------------------------------------------------------------------------
vtkOpenGLRenderTimerLog::OGLEvent* vtkOpenGLRenderTimerLog::DeepestOpenEvent()
{
  OGLEvent *openEvent = nullptr;
  if (!this->CurrentFrame.Events.empty())
  {
    OGLEvent &lastTopEvent = this->CurrentFrame.Events.back();
    if (!lastTopEvent.Timer->Stopped())
    {
      openEvent = &this->WalkOpenEvents(lastTopEvent);
    }
  }
  return openEvent;
}

//------------------------------------------------------------------------------
vtkOpenGLRenderTimerLog::OGLEvent &
vtkOpenGLRenderTimerLog::WalkOpenEvents(OGLEvent &event)
{
  assert(event.Timer->Started() && !event.Timer->Stopped());
  if (event.Events.empty())
  {
    return event;
  }

  OGLEvent &lastChild = event.Events.back();
  if (lastChild.Timer->Stopped())
  {
    return event;
  }

  return this->WalkOpenEvents(lastChild);
}

//------------------------------------------------------------------------------
vtkOpenGLRenderTimer *vtkOpenGLRenderTimerLog::NewTimer()
{
  if (this->TimerPool.empty())
  {
    return new vtkOpenGLRenderTimer();
  }
  else
  {
    vtkOpenGLRenderTimer *result = this->TimerPool.front();
    this->TimerPool.pop();
    return result;
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderTimerLog::ReleaseTimer(vtkOpenGLRenderTimer *timer)
{
  timer->Reset();
  this->TimerPool.push(timer);
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderTimerLog::ReleaseOGLFrame(OGLFrame &frame)
{
  for (auto event : frame.Events)
  {
    this->ReleaseOGLEvent(event);
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderTimerLog::ReleaseOGLEvent(OGLEvent &event)
{
  this->ReleaseTimer(event.Timer);
  event.Timer = nullptr;
  for (auto subEvent : event.Events)
  {
    this->ReleaseOGLEvent(subEvent);
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderTimerLog::TrimTimerPool()
{
  // Try not to keep too many timers around in the pool. If there are 2x as
  // many as we currently need, free the extras.
  size_t needed = this->CurrentFrame.ChildCount;
  for (auto frame : this->PendingFrames)
  {
    needed += frame.ChildCount;
  }

  needed = std::max(needed * 2, this->MinTimerPoolSize);

  while (this->TimerPool.size() > needed)
  {
    delete this->TimerPool.front();
    this->TimerPool.pop();
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderTimerLog::CheckPendingFrames()
{
  while (!this->PendingFrames.empty())
  {
    OGLFrame &frame = this->PendingFrames.front();
    if (this->IsFrameReady(frame))
    {
      this->ReadyFrames.push(this->Convert(frame));
      this->ReleaseOGLFrame(frame);
      this->PendingFrames.pop_front();
    }
    else
    { // If this frame isn't done, later frames won't be either.
      break;
    }
  }

  while (this->FrameLimit > 0 &&
         this->PendingFrames.size() + this->ReadyFrames.size() >
         this->FrameLimit)
  {
    if (!this->ReadyFrames.empty())
    {
      this->ReadyFrames.pop();
    }
    else if (!this->PendingFrames.empty())
    {
      this->ReleaseOGLFrame(this->PendingFrames.front());
      this->PendingFrames.pop_front();
    }
    else
    { // Shouldn't happen, but eh. Cheap insurance.
      break;
    }
  }
}

//------------------------------------------------------------------------------
bool vtkOpenGLRenderTimerLog::IsFrameReady(OGLFrame &frame)
{
  for (auto event : frame.Events)
  {
    if (!this->IsEventReady(event))
    {
      return false;
    }
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkOpenGLRenderTimerLog::IsEventReady(OGLEvent &event)
{
  if (!event.Timer->Ready())
  {
    return false;
  }

  for (auto subEvent : event.Events)
  {
    if (!this->IsEventReady(subEvent))
    {
      return false;
    }
  }

  return true;
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderTimerLog::ForceCloseFrame(OGLFrame &frame)
{
  for (auto event: frame.Events)
  {
    this->ForceCloseEvent(event);
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderTimerLog::ForceCloseEvent(OGLEvent &event)
{
  if (!event.Timer->Started())
  {
    vtkWarningMacro("Timer for event '" << event.Name << "' was never started? "
                    "This is an internal error. Timing results will be "
                    "unreliable.");

    // If this somehow happens, start the timer so it will not clog the pending
    // queue:
    event.Timer->Start();
  }

  if (!event.Timer->Stopped())
  {
    vtkWarningMacro("Timer for event '" << event.Name << "' was never stopped. "
                    "Ensure that all events have an end mark (the issue may "
                    "be with a different event). Timing results will be "
                    "unreliable.");
    event.Timer->Stop();
  }

  for (auto subEvent : event.Events)
  {
    this->ForceCloseEvent(subEvent);
  }
}
