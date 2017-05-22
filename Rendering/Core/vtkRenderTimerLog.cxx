/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderTimerLog.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkRenderTimerLog.h"

#include "vtkObjectFactory.h"

#include <iomanip>
#include <utility>

vtkObjectFactoryNewMacro(vtkRenderTimerLog)

//------------------------------------------------------------------------------
vtkRenderTimerLog::vtkRenderTimerLog()
  : LoggingEnabled(false),
    FrameLimit(32)
{
}

//------------------------------------------------------------------------------
vtkRenderTimerLog::~vtkRenderTimerLog()
{
}

//------------------------------------------------------------------------------
void vtkRenderTimerLog::PrintSelf(std::ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
bool vtkRenderTimerLog::IsSupported()
{
  return false;
}

//------------------------------------------------------------------------------
void vtkRenderTimerLog::MarkFrame()
{
}

//------------------------------------------------------------------------------
vtkRenderTimerLog::ScopedEventLogger
vtkRenderTimerLog::StartScopedEvent(const std::string &name)
{
  this->MarkStartEvent(name);
  return ScopedEventLogger(this);
}

//------------------------------------------------------------------------------
void vtkRenderTimerLog::MarkStartEvent(const std::string &)
{
}

//------------------------------------------------------------------------------
void vtkRenderTimerLog::MarkEndEvent()
{
}

//------------------------------------------------------------------------------
bool vtkRenderTimerLog::FrameReady()
{
  vtkWarningMacro("vtkRenderTimerLog unsupported for the current rendering "
                  "backend.");
  return false;
}

//------------------------------------------------------------------------------
vtkRenderTimerLog::Frame vtkRenderTimerLog::PopFirstReadyFrame()
{
  return Frame();
}

//------------------------------------------------------------------------------
void vtkRenderTimerLog::ReleaseGraphicsResources()
{
}

//------------------------------------------------------------------------------
vtkRenderTimerLog::ScopedEventLogger::ScopedEventLogger(ScopedEventLogger &&o)
  : Log(nullptr)
{
  std::swap(o.Log, this->Log);
}

//------------------------------------------------------------------------------
vtkRenderTimerLog::ScopedEventLogger &
vtkRenderTimerLog::ScopedEventLogger::operator=(ScopedEventLogger &&o)
{
  std::swap(o.Log, this->Log);
  return *this;
}

//------------------------------------------------------------------------------
void vtkRenderTimerLog::ScopedEventLogger::Stop()
{
  if (this->Log)
  {
    this->Log->MarkEndEvent();
    this->Log = nullptr;
  }
}

//------------------------------------------------------------------------------
void vtkRenderTimerLog::Frame::Print(std::ostream &os, float threshMs)
{
  vtkIndent indent;
  for (auto event : this->Events)
  {
    event.Print(os, 0.f, threshMs, indent);
  }
}

//------------------------------------------------------------------------------
void vtkRenderTimerLog::Event::Print(std::ostream &os, float parentTime,
                                     float threshMs, vtkIndent indent)
{
  float thisTime = this->ElapsedTimeMilliseconds();
  if (thisTime < threshMs)
  {
    return;
  }

  float parentPercent = 100.f;
  if (parentTime > 0.f)
  {
    parentPercent = thisTime / parentTime * 100.f;
  }

  os << indent << "- "
     << std::fixed << std::setw(5) << std::setprecision(1) << parentPercent
     << std::setw(0) << "% "
     << std::setw(8) << std::setprecision(3) << thisTime
     << std::setw(0) << " ms \""
     << this->Name << "\"\n";

  vtkIndent nextIndent = indent.GetNextIndent();
  for (auto event : this->Events)
  {
    event.Print(os, thisTime, threshMs, nextIndent);
  }
}
