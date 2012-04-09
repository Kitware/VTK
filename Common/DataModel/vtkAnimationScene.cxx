/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAnimationScene.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAnimationScene.h"

#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkTimerLog.h"

vtkStandardNewMacro(vtkAnimationScene);

//----------------------------------------------------------------------------
vtkAnimationScene::vtkAnimationScene()
{
  this->PlayMode = PLAYMODE_SEQUENCE;
  this->FrameRate = 10.0;
  this->Loop = 0;
  this->InPlay = 0;
  this->StopPlay = 0;
  this->AnimationTime = 0.0;

  this->AnimationCues = vtkCollection::New();
  this->AnimationCuesIterator = this->AnimationCues->NewIterator();
  this->AnimationTimer = vtkTimerLog::New();
}

//----------------------------------------------------------------------------
vtkAnimationScene::~vtkAnimationScene()
{
  if (this->InPlay)
    {
    this->Stop();
    }
  this->AnimationCues->Delete();
  this->AnimationCuesIterator->Delete();
  this->AnimationTimer->Delete();
}

//----------------------------------------------------------------------------
void vtkAnimationScene::AddCue(vtkAnimationCue* cue)
{
  if (this->AnimationCues->IsItemPresent(cue))
    {
    vtkErrorMacro("Animation cue already present in the scene");
    return;
    }
  if (this->TimeMode == vtkAnimationCue::TIMEMODE_NORMALIZED &&
    cue->GetTimeMode() != vtkAnimationCue::TIMEMODE_NORMALIZED)
    {
    vtkErrorMacro("A cue with relative time mode cannot be added to a scene "
      "with normalized time mode.");
    return;
    }
  this->AnimationCues->AddItem(cue);
}

//----------------------------------------------------------------------------
void vtkAnimationScene::RemoveCue(vtkAnimationCue* cue)
{
  this->AnimationCues->RemoveItem(cue);
}

//----------------------------------------------------------------------------
void vtkAnimationScene::RemoveAllCues()
{
  this->AnimationCues->RemoveAllItems();
}
//----------------------------------------------------------------------------
int vtkAnimationScene::GetNumberOfCues()
{
  return this->AnimationCues->GetNumberOfItems();
}
//----------------------------------------------------------------------------
void vtkAnimationScene::SetTimeMode(int mode)
{
  if (mode == vtkAnimationCue::TIMEMODE_NORMALIZED)
    {
    // If noralized time mode is being set on the scene,
    // ensure that none of the contained cues need relative times.
    vtkCollectionIterator *it = this->AnimationCuesIterator;
    for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextItem())
      {
      vtkAnimationCue* cue =
        vtkAnimationCue::SafeDownCast(it->GetCurrentObject());
      if (cue && cue->GetTimeMode() != vtkAnimationCue::TIMEMODE_NORMALIZED)
        {
        vtkErrorMacro("Scene contains a cue in relative mode. It must be removed "
          "or chaged to normalized mode before changing the scene time mode");
        return;
        }
      }
    }
  this->Superclass::SetTimeMode(mode);
}

//----------------------------------------------------------------------------
void vtkAnimationScene::InitializeChildren()
{
  // run thr all the cues and init them.
  vtkCollectionIterator *it = this->AnimationCuesIterator;
  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextItem())
    {
    vtkAnimationCue* cue =
      vtkAnimationCue::SafeDownCast(it->GetCurrentObject());
    if (cue)
      {
      cue->Initialize();
      }
    }
}

//----------------------------------------------------------------------------
void vtkAnimationScene::FinalizeChildren()
{
  vtkCollectionIterator *it = this->AnimationCuesIterator;
  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextItem())
    {
    vtkAnimationCue* cue =
      vtkAnimationCue::SafeDownCast(it->GetCurrentObject());
    if (cue)
      {
      cue->Finalize();
      }
    }
}

//----------------------------------------------------------------------------
void vtkAnimationScene::Play()
{
  if (this->InPlay)
    {
    return;
    }

  if (this->TimeMode == vtkAnimationCue::TIMEMODE_NORMALIZED)
    {
    vtkErrorMacro("Cannot play a scene with normalized time mode");
    return;
    }
  if (this->EndTime <= this->StartTime)
    {
    vtkErrorMacro("Scene start and end times are not suitable for playing");
    return;
    }

  this->InvokeEvent(vtkCommand::StartEvent);

  this->InPlay = 1;
  this->StopPlay = 0;
  this->FrameRate = (!this->FrameRate)? 1.0 : this->FrameRate;
  // the actual play loop, check for StopPlay flag.

  double currenttime = this->AnimationTime;
  // adjust currenttime to a valid time.
  currenttime = (currenttime < this->StartTime || currenttime >= this->EndTime)?
    this->StartTime : currenttime;

  double time_per_frame =
    (this->PlayMode == PLAYMODE_SEQUENCE)?  (1.0 / this->FrameRate) : 1;
  do
    {
    this->Initialize(); // Set the Scene in unintialized mode.
    this->AnimationTimer->StartTimer();
    double timer_start_time = currenttime;
    double deltatime = 0.0;
    do
      {
      this->Tick(currenttime, deltatime, currenttime);

      // needed to compute delta times.
      double previous_tick_time = currenttime;

      switch (this->PlayMode)
        {
      case PLAYMODE_REALTIME:
        this->AnimationTimer->StopTimer();
        currenttime = this->AnimationTimer->GetElapsedTime() +
          timer_start_time;
        break;

      case PLAYMODE_SEQUENCE:
        currenttime += time_per_frame;
        break;

      default:
        vtkErrorMacro("Invalid Play Mode");
        this->StopPlay = 1;
        }

      deltatime = currenttime - previous_tick_time;
      deltatime = (deltatime < 0)? -1*deltatime : deltatime;
      } while (!this->StopPlay && this->CueState != vtkAnimationCue::INACTIVE);
      // End of loop for 1 cycle.

    // restart the loop.
    currenttime = this->StartTime;
    } while (this->Loop && !this->StopPlay);

  this->StopPlay = 0;
  this->InPlay = 0;

  this->InvokeEvent(vtkCommand::EndEvent);
}

//----------------------------------------------------------------------------
void vtkAnimationScene::Stop()
{
  if (!this->InPlay)
    {
    return;
    }
  this->StopPlay = 1;
}

//----------------------------------------------------------------------------
void vtkAnimationScene::TickInternal(
  double currenttime, double deltatime, double clocktime)
{
  this->AnimationTime = currenttime;
  this->ClockTime = clocktime;

  vtkCollectionIterator* iter = this->AnimationCuesIterator;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkAnimationCue* cue = vtkAnimationCue::SafeDownCast(
      iter->GetCurrentObject());
    if (cue)
      {
      switch(cue->GetTimeMode())
        {
      case vtkAnimationCue::TIMEMODE_RELATIVE:
        cue->Tick(currenttime - this->StartTime, deltatime, clocktime);
        break;

      case vtkAnimationCue::TIMEMODE_NORMALIZED:
        cue->Tick( (currenttime - this->StartTime) / (this->EndTime - this->StartTime),
          deltatime / (this->EndTime - this->StartTime), clocktime);
        break;

      default:
        vtkErrorMacro("Invalid cue time mode");
        }
      }
    }

  this->Superclass::TickInternal(currenttime, deltatime, clocktime);
}

//----------------------------------------------------------------------------
void vtkAnimationScene::StartCueInternal()
{
  this->Superclass::StartCueInternal();
  this->InitializeChildren();
}

//----------------------------------------------------------------------------
void vtkAnimationScene::EndCueInternal()
{
  this->FinalizeChildren();
  this->Superclass::EndCueInternal();
}

//----------------------------------------------------------------------------
void vtkAnimationScene::SetAnimationTime(double currenttime)
{
  if (this->InPlay)
    {
    vtkErrorMacro("SetAnimationTime cannot be called while playing");
    return;
    }
  this->Initialize();
  this->Tick(currenttime, 0.0, currenttime);
  if (this->CueState == vtkAnimationCue::INACTIVE)
    {
    this->Finalize();
    }
}

//----------------------------------------------------------------------------
void vtkAnimationScene::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PlayMode: " << this->PlayMode << endl;
  os << indent << "FrameRate: " << this->FrameRate << endl;
  os << indent << "Loop: " << this->Loop << endl;
  os << indent << "InPlay: " << this->InPlay << endl;
  os << indent << "StopPlay: " << this->StopPlay << endl;
  os << indent << "AnimationTime: " << this->AnimationTime << endl;
}
