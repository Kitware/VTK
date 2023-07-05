// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkRenderTimerLog
 * @brief   Asynchronously measures GPU execution times for a series of events.
 *
 * This class measures the time it takes for events to occur on the GPU by
 * posting timing events into the rendering command stream. This can be used
 * to compute the time spent doing work on the GPU without stalling the
 * CPU.
 *
 * To aid asynchronous usage, this class uses the concepts "Event" and "Frame",
 * where a Frame is a logical collection of Events. The timer log can manage
 * multiple Frames at a time:
 * - The current Frame, where new Events are created.
 * - Pending Frames, for which all Events have been marked, but the results are
 *   not available (the timer requests are still waiting to be processed by the
 *   graphics device).
 * - Ready Frames, which have been completed by the graphics device and may be
 *   retrieved.
 *
 * Call MarkFrame() to begin a new Frame. This pushes the current Frame to the
 * collection of pending Frames, and creates a new one to store future Events.
 *
 * Call MarkStartEvent() and MarkEndEvent() to mark the beginning and end of
 * an Event. These Events may be nested, but all child Events must have their
 * end marked before the parent Event ends.
 *
 * Use FrameReady() and PopFirstReadyFrame() to check for completed Frames and
 * retrieve results.
 *
 * This is currently only implemented for the OpenGL2 backend. The IsSupported()
 * method can be used to detect if there is a valid implementation available.
 */

#ifndef vtkRenderTimerLog_h
#define vtkRenderTimerLog_h

#include "vtkObject.h"
#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkType.h"                // For vtkTypeUint64, etc
#include <sstream>                  // for std::ostringstream
#include <string>                   // for std::string
#include <vector>                   // for std::vector

/**
 * Creates a ScopedEventLogger on @a timer with the given @a name. @a name is
 * passed into a stream and may be constructed using the << operator.
 */
#define VTK_SCOPED_RENDER_EVENT(eventName, timer) VTK_SCOPED_RENDER_EVENT2(eventName, timer, _event)

/**
 * Creates a ScopedEventLogger on @a timer with the given @a name. @a name is
 * passed into a stream and may be constructed using the << operator. The logger
 * will be created with the provided @a identifier.
 */
#define VTK_SCOPED_RENDER_EVENT2(eventName, timer, identifier)                                     \
  vtkRenderTimerLog::ScopedEventLogger identifier;                                                 \
  do                                                                                               \
  {                                                                                                \
    std::ostringstream _eventNameStream;                                                           \
    _eventNameStream << eventName;                                                                 \
    identifier = timer->StartScopedEvent(_eventNameStream.str());                                  \
    (void)identifier; /* Prevent set-but-not-used var warnings */                                  \
  } while (false)     /* Do-while loop prevents duplicate semicolon warnings */

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGCORE_EXPORT vtkRenderTimerLog : public vtkObject
{
public:
  struct Frame;

  /** Container for a single timed event. */
  struct VTKRENDERINGCORE_EXPORT Event
  {
    /** Event name. */
    std::string Name;

    /** Times are in nanoseconds. @{ */
    vtkTypeUInt64 StartTime;
    vtkTypeUInt64 EndTime;
    /**@}*/

    /** Convenience methods to compute times */
    float ElapsedTimeSeconds() const { return this->ElapsedTimeNanoseconds() * 1e-9f; }
    float ElapsedTimeMilliseconds() const { return this->ElapsedTimeNanoseconds() * 1e-6f; }
    vtkTypeUInt64 ElapsedTimeNanoseconds() const { return this->EndTime - this->StartTime; }

    /** Child events that occurred while this event was running. */
    std::vector<Event> Events;

    /** Print details of the event to a stream.
     * @param os The stream.
     * @param threshMs Only print events with a time > threshMs milliseconds.
     * @param indent Starting indentation for the first event.
     */
    void Print(std::ostream& os, float threshMs = 0.f, vtkIndent indent = vtkIndent())
    {
      this->Print(os, 0.f, threshMs, indent);
    }

    friend struct vtkRenderTimerLog::Frame;

  protected:
    void Print(std::ostream& os, float parentTime, float threshMs, vtkIndent indent);
  };

  /** Container for a frame's events. */
  struct VTKRENDERINGCORE_EXPORT Frame
  {
    std::vector<Event> Events;

    /** Print details of all events in this frame to a stream.
     * @param os The stream.
     * @param threshMs Only print events with a time > threshMs milliseconds.
     */
    void Print(std::ostream& os, float threshMs = 0.f);
  };

  /**
   * RAII struct for logging events. Such events start when
   * vtkRenderTimerLog::StartScopedEvent(name) is called, and end when the
   * returned object is destroyed, or ScopedEventLogger::Stop() is called.
   */
  struct VTKRENDERINGCORE_EXPORT ScopedEventLogger
  {
    ScopedEventLogger()
      : Log(nullptr)
    {
    }
    ScopedEventLogger(ScopedEventLogger&& other) noexcept;
    ScopedEventLogger& operator=(ScopedEventLogger&& other) noexcept;
    ~ScopedEventLogger() { this->Stop(); }
    void Stop();
    friend class vtkRenderTimerLog;

  protected:
    ScopedEventLogger(vtkRenderTimerLog* log)
      : Log(log)
    {
    }

  private:
    void operator=(const ScopedEventLogger&) = delete;
    ScopedEventLogger(const ScopedEventLogger& other) = delete;
    vtkRenderTimerLog* Log;
  };

  static vtkRenderTimerLog* New();
  vtkTypeMacro(vtkRenderTimerLog, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns true if stream timings are implemented for the current graphics
   * backend.
   */
  virtual bool IsSupported() VTK_FUTURE_CONST;

  /**
   * Call to mark the start of a new frame, or the end of an old one. Does
   * nothing if no events have been recorded in the current frame.
   */
  virtual void MarkFrame();

  /**
   * Create a RAII scoped event. See ScopedEventLogger for details.
   */
  ScopedEventLogger StartScopedEvent(const std::string& name);

  /**
   * Mark the beginning or end of an event. @{
   */
  virtual void MarkStartEvent(const std::string& name);
  virtual void MarkEndEvent();
  /**@}*/

  /**
   * Returns true if there are any frames ready with complete timing info.
   */
  virtual bool FrameReady();

  /**
   * Retrieve the first available frame's timing info. The returned frame is
   * removed from this log.
   */
  virtual Frame PopFirstReadyFrame();

  /** If false, no events are recorded. Default is false. @{ */
  vtkSetMacro(LoggingEnabled, bool);
  vtkGetMacro(LoggingEnabled, bool);
  vtkBooleanMacro(LoggingEnabled, bool);
  /**@}*/

  /**
   * If there are more than FrameLimit frames pending/ready, drop the old ones
   * until we are under this limit. Prevents things from backing up.
   * Default is 32. Set to 0 to disable. @{
   */
  vtkSetMacro(FrameLimit, unsigned int);
  vtkGetMacro(FrameLimit, unsigned int);
  /**@}*/

  /**
   * Releases any resources allocated on the graphics device.
   */
  virtual void ReleaseGraphicsResources();

protected:
  vtkRenderTimerLog();
  ~vtkRenderTimerLog() override;

  mutable bool LoggingEnabled;
  unsigned int FrameLimit;

private:
  vtkRenderTimerLog(const vtkRenderTimerLog&) = delete;
  void operator=(const vtkRenderTimerLog&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkRenderTimerLog_h
