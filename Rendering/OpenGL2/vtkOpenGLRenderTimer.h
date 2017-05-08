/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLRenderTimer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkOpenGLRenderTimer
 * @brief   Asynchronously measures GPU execution time.
 *
 *
 * This class posts events to the OpenGL server to measure execution times
 * of GPU processes. The queries are asynchronous and multiple
 * vtkOpenGLRenderTimers may overlap / be nested.
 *
 * This uses GL_TIMESTAMP rather than GL_ELAPSED_TIME, since only one
 * GL_ELAPSED_TIME query may be active at a time. Since GL_TIMESTAMP is not
 * available on OpenGL ES2
 */

#ifndef vtkOpenGLRenderTimer_h
#define vtkOpenGLRenderTimer_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkType.h" // For vtkTypeUint64, etc

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLRenderTimer
{
public:
  vtkOpenGLRenderTimer();
  ~vtkOpenGLRenderTimer();

  /**
   * Clear out any previous results and prepare for a new query.
   */
  void Reset();

  /**
   * Mark the start of a timed event.
   */
  void Start();

  /**
   * Mark the end of a timed event.
   */
  void Stop();

  /**
   * Returns true if the timer has been started. The query may not be ready yet.
   */
  bool Started();

  /**
   * Returns true if the timer has been stopped. The query may not be ready yet.
   */
  bool Stopped();

  /**
   * Returns true when the timing results are available.
   */
  bool Ready();

  //@{
  /**
   * If Ready() returns true, get the elapsed time in the requested units.
   */
  float GetElapsedSeconds();
  float GetElapsedMilliseconds();
  vtkTypeUInt64 GetElapsedNanoseconds();
  //@}

  //@{
  /**
   * This class can also be used in a reusable manner where the start and stop
   * events stay in flight until they are both completed. Calling ReusableStart
   * while they are in flight is ignored. The Elapsed time is always the result
   * from the most recently completed flight. Typical usage is
   *
   * render loop
   *   timer->ReusableStart();
   *   // do some rendering
   *   timer->ReusableStop();
   *   time = timer->GetReusableElapsedSeconds();
   *
   * the elapsed seconds will return zero until a flight has completed.
   *
   * The idea being that with OpenGL render commands are
   * asynchronous. You might render multiple times before the first
   * render on the GPU is completed. These reusable methods provide
   * a method for provinding a constant measure of the time required
   * for a command with the efficiency of only having one timing in
   * process/flight at a time. Making this a lightweight timer
   * in terms of OpenGL API calls.
   *
   * These reusable methods are not meant to be mixed with other methods
   * in this class.
   */
  void ReusableStart();
  void ReusableStop();
  float GetReusableElapsedSeconds();
  //@}

  /**
   * Simply calls Reset() to ensure that query ids are freed. All stored timing
   * information will be lost.
   */
  void ReleaseGraphicsResources();

protected:
  bool StartReady;
  bool EndReady;

  vtkTypeUInt32 StartQuery;
  vtkTypeUInt32 EndQuery;

  vtkTypeUInt64 StartTime;
  vtkTypeUInt64 EndTime;

  bool ReusableStarted;
  bool ReusableEnded;

private:
  vtkOpenGLRenderTimer(const vtkOpenGLRenderTimer&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenGLRenderTimer&) VTK_DELETE_FUNCTION;
};

#endif // vtkOpenGLRenderTimer_h

// VTK-HeaderTest-Exclude: vtkOpenGLRenderTimer.h
