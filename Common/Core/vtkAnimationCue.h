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
/**
 * @class   vtkAnimationCue
 * @brief   a seqin an animation.
 *
 * vtkAnimationCue and vtkAnimationScene provide the framework to support
 * animations in VTK. vtkAnimationCue represents an entity that changes/
 * animates with time, while vtkAnimationScene represents scene or setup
 * for the animation, which consists on individual cues or other scenes.
 *
 * A cue has three states: UNINITIALIZED, ACTIVE and INACTIVE.
 * UNINITIALIZED represents an point in time before the start time of the cue.
 * The cue is in ACTIVE state at a point in time between start time and end time
 * for the cue. While, beyond the end time, it is in INACTIVE state.
 * When the cue enters the ACTIVE state, StartAnimationCueEvent is fired. This
 * event may be handled to initialize the entity to be animated.
 * When the cue leaves the ACTIVE state, EndAnimationCueEvent is fired, which
 * can be handled to cleanup after having run the animation.
 * For every request to render during the ACTIVE state, AnimationCueTickEvent is
 * fired, which must be handled to perform the actual animation.
 * @sa
 * vtkAnimationScene
*/

#ifndef vtkAnimationCue_h
#define vtkAnimationCue_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"

class VTKCOMMONCORE_EXPORT vtkAnimationCue: public vtkObject
{
public:
  vtkTypeMacro(vtkAnimationCue,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkAnimationCue* New();

  // Structure passed on every event invocation.
  // Depending upon the cue time mode, these times are either
  // normalized [0,1] or relative to the scene that contains the cue.
  // All this information is also available by asking the cue
  // directly for it within the handler. Thus, this information can
  // be accessed in wrapped languages.
  class AnimationCueInfo
  {
  public:
    double StartTime;
    double EndTime;
    double AnimationTime;// valid only in AnimationCueTickEvent handler
    double DeltaTime;   // valid only in AnimationCueTickEvent handler
    double ClockTime;   // valid only in AnimationCueTickEvent handler
  };

  //@{
  /**
   * Get/Set the time mode. In Normalized mode, the start and end
   * times of the cue are normalized [0,1] with respect to the start and
   * end times of the container scene. In Relative mode the start and end
   * time of the cue are specified in offset seconds relative to the
   * start time of the container scene.
   */
  virtual void SetTimeMode(int mode);
  vtkGetMacro(TimeMode, int);
  void SetTimeModeToRelative()
    { this->SetTimeMode(TIMEMODE_RELATIVE); }
  void SetTimeModeToNormalized()
    { this->SetTimeMode(TIMEMODE_NORMALIZED); }
  //@}

  //@{
  /**
   * Get/Set the Start time for this cue.
   * When the current time is >= StartTime, the Cue is in
   * ACTIVE state. if Current time i < StartTime, the Cue is in
   * UNINITIALIZED state. Whenever the cue enters the ACTIVE state from
   * an INACTIVE state, it triggers the StartEvent.
   * The Start time is in seconds relative to the start of the
   * container Scene (when in Relative time mode) or is normalized
   * over the span of the container Scene (when in Normalized time mode).
   */
  vtkSetMacro(StartTime, double);
  vtkGetMacro(StartTime, double);
  //@}

  //@{
  /**
   * Get/Set the End time for this cue.
   * When the current time is > EndTime, the Cue is in
   * INACTIVE state. Whenever the cue leaves an ACTIVE state to enter
   * INACTIVE state, the EndEvent is triggered.
   * The End time is in seconds relative to the start of the
   * container Scene (when in Relative time mode) or is normalized
   * over the span of the container Scene (when in Normalized time mode).
   */
  vtkSetMacro(EndTime, double);
  vtkGetMacro(EndTime, double);
  //@}

  /**
   * Indicates a tick or point in time in the animation.
   * Triggers a Tick event if currenttime >= StartTime and
   * currenttime <= EndTime.
   * Whenever the state of the cue changes,
   * either StartEvent or EndEvent is triggered depending upon
   * whether the cue entered Active state or quit active state respectively.
   * The current time is relative to the start of the container Scene
   * (when in Relative time mode) or is normalized
   * over the span of the container Scene (when in Normalized time mode).
   * deltatime is the time since last call to Tick. deltatime also can be in seconds
   * relative to the start of the container Scene or normalized depending upon the
   * cue's Time mode.
   * clocktime is the time from the scene i.e. it does not depend on the time
   * mode for the cue.
   * For the first call to Tick
   * after a call to Initialize(), the deltatime is 0;
   */
  virtual void Tick(double currenttime, double deltatime, double clocktime);

  /**
   * Called when the playing of the scene begins.
   * This will set the Cue to UNINITIALIZED state.
   */
  virtual void Initialize();

  /**
   * Called when the scene reaches the end.
   * If the cue state is ACTIVE when this method is called, this will
   * trigger a EndAnimationCueEvent.
   */
  virtual void Finalize();

  //@{
  /**
   * This is valid only in a AnimationCueTickEvent handler.
   * Before firing the event the animation cue sets the AnimationTime to
   * the time of the tick.
   */
  vtkGetMacro(AnimationTime, double);
  //@}

  //@{
  /**
   * This is valid only in a AnimationCueTickEvent handler.
   * Before firing the event the animation cue sets the DeltaTime
   * to the difference in time between the current tick and the last tick.
   */
  vtkGetMacro(DeltaTime, double);
  //@}

  //@{
  /**
   * This is valid only in a AnimationCueTickEvent handler.
   * Before firing the event the animation cue sets the ClockTime to
   * the time of the tick. ClockTime is directly the time from the animation
   * scene neither normalized nor offsetted to the start of the scene.
   */
  vtkGetMacro(ClockTime, double);
  //@}

  enum TimeCodes
  {
    TIMEMODE_NORMALIZED=0,
    TIMEMODE_RELATIVE=1
  };

protected:
  vtkAnimationCue();
  ~vtkAnimationCue() override;

  enum {
    UNINITIALIZED=0,
    INACTIVE,
    ACTIVE
  };

  double StartTime;
  double EndTime;
  int TimeMode;

  // These are set when the AnimationCueTickEvent event
  // is fired. Thus giving access to the information in
  // the AnimationCueInfo struct in wrapped languages.
  double AnimationTime;
  double DeltaTime;
  double ClockTime;

  /**
   * Current state of the Cue.
   */
  int CueState;

  //@{
  /**
   * These are the internal methods that actually trigger they
   * corresponding events. Subclasses can override these to
   * do extra processing at start/end or on tick.
   */
  virtual void StartCueInternal();
  virtual void TickInternal(double currenttime, double deltatime,
    double clocktime);
  virtual void EndCueInternal();
  //@}

private:
  vtkAnimationCue(const vtkAnimationCue&) = delete;
  void operator=(const vtkAnimationCue&) = delete;
};

#endif



