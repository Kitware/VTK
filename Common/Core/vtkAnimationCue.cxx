/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAnimationCue.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAnimationCue.h"
#include "vtkObjectFactory.h"
#include "vtkCommand.h"

vtkStandardNewMacro(vtkAnimationCue);

//----------------------------------------------------------------------------
vtkAnimationCue::vtkAnimationCue()
{
  this->StartTime = this->EndTime = 0.0;
  this->CueState = vtkAnimationCue::UNINITIALIZED;
  this->TimeMode = TIMEMODE_RELATIVE;
  this->AnimationTime = 0;
  this->DeltaTime = 0;
  this->ClockTime = 0;
}

//----------------------------------------------------------------------------
vtkAnimationCue::~vtkAnimationCue()
{
}

//----------------------------------------------------------------------------
void vtkAnimationCue::StartCueInternal()
{
  vtkAnimationCue::AnimationCueInfo info;
  info.StartTime = this->StartTime;
  info.EndTime = this->EndTime;
  info.AnimationTime = 0.0;
  info.DeltaTime = 0.0;
  info.ClockTime = 0.0;
  this->InvokeEvent(vtkCommand::StartAnimationCueEvent, &info);
}

//----------------------------------------------------------------------------
void vtkAnimationCue::EndCueInternal()
{
  vtkAnimationCue::AnimationCueInfo info;
  info.StartTime = this->StartTime;
  info.EndTime = this->EndTime;
  info.AnimationTime = this->EndTime;
  info.DeltaTime = 0.0;
  info.ClockTime = 0.0;
  this->InvokeEvent(vtkCommand::EndAnimationCueEvent, &info);
}

//----------------------------------------------------------------------------
void vtkAnimationCue::TickInternal(
  double currenttime, double deltatime, double clocktime)
{
  vtkAnimationCue::AnimationCueInfo info;
  info.StartTime = this->StartTime;
  info.EndTime = this->EndTime;
  info.DeltaTime = deltatime;
  info.AnimationTime = currenttime;
  info.ClockTime =clocktime;

  this->AnimationTime = currenttime;
  this->DeltaTime = deltatime;
  this->ClockTime = clocktime;

  this->InvokeEvent(vtkCommand::AnimationCueTickEvent, &info);

  this->AnimationTime = 0;
  this->DeltaTime = 0;
  this->ClockTime = 0;
}


//----------------------------------------------------------------------------
void vtkAnimationCue::Tick(double currenttime, double deltatime, double clocktime)
{
  // Check to see if we have crossed the Cue start.
  if (currenttime >= this->StartTime &&
    this->CueState == vtkAnimationCue::UNINITIALIZED)
    {
    this->CueState = vtkAnimationCue::ACTIVE;
    this->StartCueInternal();
    }

  // Note that Tick event is sent for both start time and
  // end time.
  if (this->CueState == vtkAnimationCue::ACTIVE)
    {
    if (currenttime <= this->EndTime)
      {
      this->TickInternal(currenttime, deltatime, clocktime);
      }
    if (currenttime >= this->EndTime)
      {
      this->EndCueInternal();
      this->CueState = vtkAnimationCue::INACTIVE;
      }
    }
}

//----------------------------------------------------------------------------
void vtkAnimationCue::SetTimeMode(int mode)
{
  this->TimeMode = mode;
}

//----------------------------------------------------------------------------
void vtkAnimationCue::Initialize()
{
  this->CueState = vtkAnimationCue::UNINITIALIZED;
}

//----------------------------------------------------------------------------
void vtkAnimationCue::Finalize()
{
  if (this->CueState == vtkAnimationCue::ACTIVE)
    {
    this->EndCueInternal();
    }
  this->CueState = vtkAnimationCue::INACTIVE;
}

//----------------------------------------------------------------------------
void vtkAnimationCue::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "StartTime: " << this->StartTime << endl;
  os << indent << "EndTime: " << this->EndTime << endl;
  os << indent << "CueState: " << this->CueState << endl;
  os << indent << "TimeMode: " << this->TimeMode << endl;
  os << indent << "AnimationTime: " << this->AnimationTime << endl;
  os << indent << "DeltaTime: " << this->DeltaTime << endl;
  os << indent << "ClockTime: " << this->ClockTime << endl;
}
