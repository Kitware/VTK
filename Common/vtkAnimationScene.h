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
// for the animation, which consists on individual cues or other scenes.
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

#define VTK_ANIMATION_SCENE_PLAYMODE_SEQUENCE 0
#define VTK_ANIMATION_SCENE_PLAYMODE_REALTIME 1

class VTK_COMMON_EXPORT vtkAnimationScene: public vtkAnimationCue
{
public:
  vtkTypeRevisionMacro(vtkAnimationScene, vtkAnimationCue);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkAnimationScene* New();

  // Description:
  // Get/Set the PlayMode for running/playing the animation scene.
  // In the Sequence mode, all the frames are generated one after the other.
  // The time reported to each Tick of the constituent cues (during Play) is
  // incremented by 1/frame rate, irrespective of the current time.
  // In the RealTime mode, time indicates the instance in time. 
  vtkSetMacro(PlayMode, int);
  void SetModeToSequence() { this->SetPlayMode(VTK_ANIMATION_SCENE_PLAYMODE_SEQUENCE); }
  void SetModeToRealTime() { this->SetPlayMode(VTK_ANIMATION_SCENE_PLAYMODE_REALTIME); }
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
  
  // Description:
  // Starts playing the animation scene.
  void Play();

  // Description:
  // Stops the animation scene that is running.
  void Stop();

  // Description:
  // Enable/Disable animation loop.
  vtkSetMacro(Loop, int);
  vtkGetMacro(Loop, int);

  // Description:
  // Makes the state of the scene same as the given time.
  void SetCurrentTime(double time);

  // Description:
  // Overridden to allow change to Normalized mode only
  // if none of the constituent cues is in Relative time mode.
  virtual void SetTimeMode(int mode);

  // Description:
  // Called when the playing of the scene begins.
  // Triggers Initialize on all contained cues.
  virtual void Initialize();

  
  // Description:
  // Called when the scene reaches the end.
  // Triggers Finalize on all contained cues.
  virtual void Finalize();

  // Description:
  // Returns if the animation is being played.
  int IsInPlay() { return this->InPlay; } 
protected:
  vtkAnimationScene();
  ~vtkAnimationScene();

  // Description:
  // Called on every valid tick.
  // Calls ticks on all the contained cues.
  virtual void TickInternal(double currenttime, double deltatime);

  
  int PlayMode;
  double FrameRate;
  int Loop;
  int InPlay;
  int StopPlay;
  double CurrentTime;

  vtkCollection* AnimationCues;
  vtkCollectionIterator* AnimationCuesIterator;
  vtkTimerLog* AnimationTimer;
  
private:
  vtkAnimationScene(const vtkAnimationScene&); // Not implemented.
  void operator=(const vtkAnimationScene&); // Not implemented.
};

#endif

