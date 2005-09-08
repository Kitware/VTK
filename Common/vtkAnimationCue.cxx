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

vtkCxxRevisionMacro(vtkAnimationCue, "1.4");
vtkStandardNewMacro(vtkAnimationCue);

//----------------------------------------------------------------------------
vtkAnimationCue::vtkAnimationCue()
{
  this->StartTime = this->EndTime = 0.0;
  this->CueState = vtkAnimationCue::UNINITIALIZED;
  this->TimeMode = TIMEMODE_RELATIVE;
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
  this->InvokeEvent(vtkCommand::EndAnimationCueEvent, &info);
}

//----------------------------------------------------------------------------
void vtkAnimationCue::TickInternal(double currenttime, double deltatime)
{
  vtkAnimationCue::AnimationCueInfo info;
  info.StartTime = this->StartTime;
  info.EndTime = this->EndTime;
  info.DeltaTime = deltatime; 
  info.AnimationTime = currenttime;

  this->InvokeEvent(vtkCommand::AnimationCueTickEvent, &info);
}


//----------------------------------------------------------------------------
void vtkAnimationCue::Tick(double currenttime, double deltatime)
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
      this->TickInternal(currenttime, deltatime);
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
}
