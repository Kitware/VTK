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
/**
 * @class   vtkAnimationScene
 * @brief   the animation scene manager.
 *
 * vtkAnimationCue and vtkAnimationScene provide the framework to support
 * animations in VTK. vtkAnimationCue represents an entity that changes/
 * animates with time, while vtkAnimationScene represents scene or setup
 * for the animation, which consists of individual cues or other scenes.
 *
 * A scene can be played in real time mode, or as a sequence of frames
 * 1/frame rate apart in time.
 * @sa
 * vtkAnimationCue
*/

#ifndef vtkAnimationScene_h
#define vtkAnimationScene_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkAnimationCue.h"

class vtkAnimationCue;
class vtkCollection;
class vtkCollectionIterator;
class vtkTimerLog;

class VTKCOMMONDATAMODEL_EXPORT vtkAnimationScene: public vtkAnimationCue
{
public:
  vtkTypeMacro(vtkAnimationScene, vtkAnimationCue);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkAnimationScene* New();

  //@{
  /**
   * Get/Set the PlayMode for running/playing the animation scene.
   * In the Sequence mode, all the frames are generated one after the other.
   * The time reported to each Tick of the constituent cues (during Play) is
   * incremented by 1/frame rate, irrespective of the current time.
   * In the RealTime mode, time indicates the instance in time.
   */
  vtkSetMacro(PlayMode, int);
  void SetModeToSequence() { this->SetPlayMode(PLAYMODE_SEQUENCE); }
  void SetModeToRealTime() { this->SetPlayMode(PLAYMODE_REALTIME); }
  vtkGetMacro(PlayMode, int);
  //@}

  //@{
  /**
   * Get/Set the frame rate (in frames per second).
   * This parameter affects only in the Sequence mode. The time interval
   * indicated to each cue on every tick is progressed by 1/frame-rate seconds.
   */
  vtkSetMacro(FrameRate, double);
  vtkGetMacro(FrameRate, double);
  //@}

  //@{
  /**
   * Add/Remove an AnimationCue to/from the Scene.
   * It's an error to add a cue twice to the Scene.
   */
  void AddCue(vtkAnimationCue* cue);
  void RemoveCue(vtkAnimationCue* cue);
  void RemoveAllCues();
  int  GetNumberOfCues();
  //@}

  /**
   * Starts playing the animation scene. Fires a vtkCommand::StartEvent
   * before play beings and vtkCommand::EndEvent after play ends.
   */
  virtual void Play();

  /**
   * Stops the animation scene that is running.
   */
  void Stop();

  //@{
  /**
   * Enable/Disable animation loop.
   */
  vtkSetMacro(Loop, int);
  vtkGetMacro(Loop, int);
  //@}

  /**
   * Makes the state of the scene same as the given time.
   */
  void SetAnimationTime(double time);

  /**
   * Overridden to allow change to Normalized mode only
   * if none of the constituent cues is in Relative time mode.
   */
  void SetTimeMode(int mode) override;

  /**
   * Returns if the animation is being played.
   */
  int IsInPlay() { return this->InPlay; }

  enum PlayModes
  {
    PLAYMODE_SEQUENCE=0,
    PLAYMODE_REALTIME=1
  };

protected:
  vtkAnimationScene();
  ~vtkAnimationScene() override;

  //@{
  /**
   * Called on every valid tick.
   * Calls ticks on all the contained cues.
   */
  void TickInternal(double currenttime, double deltatime, double clocktime) override;
  void StartCueInternal() override;
  void EndCueInternal() override;
  //@}

  void InitializeChildren();
  void FinalizeChildren();

  int PlayMode;
  double FrameRate;
  int Loop;
  int InPlay;
  int StopPlay;

  vtkCollection* AnimationCues;
  vtkCollectionIterator* AnimationCuesIterator;
  vtkTimerLog* AnimationTimer;

private:
  vtkAnimationScene(const vtkAnimationScene&) = delete;
  void operator=(const vtkAnimationScene&) = delete;
};

#endif
