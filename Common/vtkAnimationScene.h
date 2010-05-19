/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAnimationScene.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAnimationScene - the animation scene manager.
// .SECTION Description
// vtkAnimationCue and vtkAnimationScene provide the framework to support
// animations in VTK. vtkAnimationCue represents an entity that changes/
// animates with time, while vtkAnimationScene represents scene or setup 
// for the animation, which consists of individual cues or other scenes.
//
// A scene can be played in real time mode, or as a seqence of frames
// 1/frame rate apart in time.
// .SECTION See Also
// vtkAnimationCue

#ifndef __vtkAnimationScene_h
#define __vtkAnimationScene_h

#include "vtkAnimationCue.h"

class vtkAnimationCue;
class vtkCollection;
class vtkCollectionIterator;
class vtkTimerLog;

class VTK_COMMON_EXPORT vtkAnimationScene: public vtkAnimationCue
{
public:
  vtkTypeMacro(vtkAnimationScene, vtkAnimationCue);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkAnimationScene* New();

  // Description:
  // Get/Set the PlayMode for running/playing the animation scene.
  // In the Sequence mode, all the frames are generated one after the other.
  // The time reported to each Tick of the constituent cues (during Play) is
  // incremented by 1/frame rate, irrespective of the current time.
  // In the RealTime mode, time indicates the instance in time. 
  vtkSetMacro(PlayMode, int);
  void SetModeToSequence() { this->SetPlayMode(PLAYMODE_SEQUENCE); }
  void SetModeToRealTime() { this->SetPlayMode(PLAYMODE_REALTIME); }
  vtkGetMacro(PlayMode, int);

  // Description:
  // Get/Set the frame rate (in frames per second).
  // This parameter affects only in the Sequence mode. The time interval
  // indicated to each cue on every tick is progressed by 1/frame-rate seconds.
  vtkSetMacro(FrameRate, double);
  vtkGetMacro(FrameRate, double);
  
  // Description:
  // Add/Remove an AnimationCue to/from the Scene.
  // It's an error to add a cue twice to the Scene.
  void AddCue(vtkAnimationCue* cue);
  void RemoveCue(vtkAnimationCue* cue);
  void RemoveAllCues();
  int  GetNumberOfCues();
  
  // Description:
  // Starts playing the animation scene. Fires a vtkCommand::StartEvent
  // before play beings and vtkCommand::EndEvent after play ends.
  virtual void Play();

  // Description:
  // Stops the animation scene that is running.
  void Stop();

  // Description:
  // Enable/Disable animation loop.
  vtkSetMacro(Loop, int);
  vtkGetMacro(Loop, int);

  // Description:
  // Makes the state of the scene same as the given time.
  void SetAnimationTime(double time);
  vtkGetMacro(AnimationTime, double);

  // Description:
  // Overridden to allow change to Normalized mode only
  // if none of the constituent cues is in Relative time mode.
  virtual void SetTimeMode(int mode);

  // Description:
  // Returns if the animation is being played.
  int IsInPlay() { return this->InPlay; } 

//BTX
  enum PlayModes
  {
    PLAYMODE_SEQUENCE=0,
    PLAYMODE_REALTIME=1
  };
//ETX

protected:
  vtkAnimationScene();
  ~vtkAnimationScene();

  // Description:
  // Called on every valid tick.
  // Calls ticks on all the contained cues.
  virtual void TickInternal(double currenttime, double deltatime, double clocktime);
  virtual void StartCueInternal();
  virtual void EndCueInternal();

  void InitializeChildren();
  void FinalizeChildren();
  
  int PlayMode;
  double FrameRate;
  int Loop;
  int InPlay;
  int StopPlay;
  double AnimationTime;

  vtkCollection* AnimationCues;
  vtkCollectionIterator* AnimationCuesIterator;
  vtkTimerLog* AnimationTimer;
  
private:
  vtkAnimationScene(const vtkAnimationScene&); // Not implemented.
  void operator=(const vtkAnimationScene&); // Not implemented.
};

#endif
