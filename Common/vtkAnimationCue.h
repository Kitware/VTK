/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAnimationCue.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAnimationCue - a seqin an animation.
// .SECTION Description
// vtkAnimationCue and vtkAnimationScene provide the framework to support
// animations in VTK. vtkAnimationCue represents an entity that changes/
// animates with time, while vtkAnimationScene represents scene or setup 
// for the animation, which consists on individual cues or other scenes.
//
// A cue has three states: UNINITIALIZED, ACTIVE and INACTIVE.
// UNINITIALIZED represents an point in time before the start time of the cue.
// The cue is in ACTIVE state at a point in time between start time and end time
// for the cue. While, beyond the end time, it is in INACTIVE state.
// When the cue enters the ACTIVE state, StartAnimationCueEvent is fired. This 
// event may be handled to initialize the entity to be animated.
// When the cue leaves the ACTIVE state, EndAnimationCueEvent is fired, which 
// can be handled to cleanup after having run the animation.
// For every request to render during the ACTIVE state, AnimationCueTickEvent is
// fired, which must be handled to perform the actual animation. 
// .SECTION See Also
// vtkAnimationScene

#ifndef __vtkAnimationCue_h
#define __vtkAnimationCue_h

#include "vtkObject.h"

class VTK_COMMON_EXPORT vtkAnimationCue: public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkAnimationCue,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkAnimationCue* New();

  //BTX
  // Structure passed on every event invocation.
  // Depending upon the cue time mode, these times are either
  // normalized [0,1] or relative to the scene that contains the cue.
  class AnimationCueInfo
    {
  public:
    double StartTime;
    double EndTime;
    double AnimationTime;// valid only on AnimationCueTickEvent
    double DeltaTime;   // valid only on AnimationCueTickEvent
    };
  //ETX
  
  // Description:
  // Get/Set the time mode. In Normalized mode, the start and end 
  // times of the cue are normalized [0,1] with respect to the start and 
  // end times of the container scene. In Relative mode the start and end
  // time of the cue are specified in offset seconds relative to the 
  // start time of the container scene.
  virtual void SetTimeMode(int mode);
  vtkGetMacro(TimeMode, int);
  void SetTimeModeToRelative() 
    { this->SetTimeMode(TIMEMODE_RELATIVE); }
  void SetTimeModeToNormalized() 
    { this->SetTimeMode(TIMEMODE_NORMALIZED); }

  // Description:
  // Get/Set the Start time for this cue.
  // When the current time is >= StartTime, the Cue is in
  // ACTIVE state. if Current time i < StartTime, the Cue is in
  // UNINITIALIZED state. Whenever the cue enters the ACTIVE state from
  // an INACTIVE state, it triggers the StartEvent.
  // The Start time is in seconds relative to the start of the 
  // container Scene (when in Relative time mode) or is normalized
  // over the span of the container Scene (when in Normalized time mode).
  vtkSetMacro(StartTime, double);
  vtkGetMacro(StartTime, double);

  // Description:
  // Get/Set the End time for this cue.
  // When the current time is > EndTime, the Cue is in
  // INACTIVE state. Whenever the cue leaves an ACTIVE state to enter 
  // INACTIVE state, the EndEvent is triggered.
  // The End time is in seconds relative to the start of the 
  // container Scene (when in Relative time mode) or is normalized
  // over the span of the container Scene (when in Normalized time mode).
  vtkSetMacro(EndTime, double);
  vtkGetMacro(EndTime, double);
 
  // Description:
  // Indicates a tick or point in time in the animation.
  // Triggers a Tick event if currenttime >= StartTime and
  // currenttime <= EndTime.
  // Whenever the state of the cue changes,
  // either StartEvent or EndEvent is triggerred depending upon 
  // whether the cue entered Active state or quit active state respectively.
  // The current time is relative to the start of the container Scene 
  // (when in Relative time mode) or is normalized
  // over the span of the container Scene (when in Normalized time mode).
  // deltatime is the time since last call to Tick. deltatime also can be in seconds
  // relative to the start of the container Scene or normalized depending upon the
  // cue's Time mode.
  // For the first call to Tick
  // after a call to Initialize(), the deltatime is 0;
  virtual void Tick(double currenttime, double deltatime);

  // Description:
  // Called when the playing of the scene begins.
  // This will set the Cue to UNINITIALIZED state.
  virtual void Initialize();

  // Description:
  // Called when the scene reaches the end.
  // If the cue state is ACTIVE when this method is called, this will
  // trigger a EndAnimationCueEvent.
  virtual void Finalize();

//BTX
  enum TimeCodes
  {
    TIMEMODE_NORMALIZED=0,
    TIMEMODE_RELATIVE=1
  };
//ETX
protected:
  vtkAnimationCue();
  ~vtkAnimationCue();
//BTX
  enum {
    UNINITIALIZED=0,
    INACTIVE,
    ACTIVE
  };
//ETX
  double StartTime;
  double EndTime;
  int TimeMode;
  
  // Description:
  // Current state of the Cue.
  int CueState;

  // Description:
  // These are the internal methods that actually trigger they
  // corresponding events. Subclasses can override these to
  // do extra processing at start/end or on tick.
  virtual void StartCueInternal();
  virtual void TickInternal(double currenttime, double deltatime);
  virtual void EndCueInternal();
 
private:
  vtkAnimationCue(const vtkAnimationCue&);  // Not implemented.
  void operator=(const vtkAnimationCue&);  // Not implemented.
};

#endif



