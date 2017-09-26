/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLRenderTimerLog.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkOpenGLRenderTimerLog_h
#define vtkOpenGLRenderTimerLog_h

#include "vtkRenderTimerLog.h"
#include "vtkRenderingOpenGL2Module.h" // For export macros

#include <deque> // for deque!
#include <queue> // for queue!

class vtkOpenGLRenderTimer;

/**
 * @brief OpenGL2 override for vtkRenderTimerLog.
 */
class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLRenderTimerLog
    : public vtkRenderTimerLog
{
public:
  struct OGLEvent
  {
    OGLEvent() : Timer(nullptr) {}

    std::string Name;
    vtkOpenGLRenderTimer *Timer;
    std::vector<OGLEvent> Events;
  };

  struct OGLFrame
  {
    OGLFrame() : ChildCount(0) {}

    unsigned int ChildCount;
    std::vector<OGLEvent> Events;
  };

  static vtkOpenGLRenderTimerLog* New();
  vtkTypeMacro(vtkOpenGLRenderTimerLog, vtkRenderTimerLog)
  void PrintSelf(ostream &os, vtkIndent indent) override;

  bool IsSupported() override;

  /**
   * Overridden to do support check before returning.
   */
  bool GetLoggingEnabled() override { return this->DoLogging(); }

  void MarkFrame() override;

  void MarkStartEvent(const std::string &name) override;
  void MarkEndEvent() override;

  bool FrameReady() override;

  Frame PopFirstReadyFrame() override;

  /**
   * Releases any resources allocated on the graphics device.
   */
  void ReleaseGraphicsResources() override;

  /**
   * This implementations keeps a pool of vtkRenderTimers around, recycling them
   * to avoid constantly allocating/freeing them. The pool is sometimes trimmed
   * to free up memory if the number of timers in the pool is much greater the
   * the number of timers currently used. This setting controls the minimum
   * number of timers that will be kept. More may be kept if they are being
   * used, but the pool will never be trimmed below this amount.
   *
   * Default value is 32, but can be adjusted for specific use cases.
   */
  vtkSetMacro(MinTimerPoolSize, size_t)
  vtkGetMacro(MinTimerPoolSize, size_t)

protected:
  OGLFrame CurrentFrame;
  // We use a deque since they are iterable. convention is push back, pop front
  std::deque<OGLFrame> PendingFrames;
  std::queue<Frame> ReadyFrames;

  std::queue<vtkOpenGLRenderTimer*> TimerPool;

  size_t MinTimerPoolSize;

  vtkOpenGLRenderTimerLog();
  ~vtkOpenGLRenderTimerLog() override;

  bool DoLogging();

  Frame Convert(const OGLFrame &oglFrame);
  Event Convert(const OGLEvent &oglEvent);

  OGLEvent& NewEvent();
  OGLEvent* DeepestOpenEvent();
  OGLEvent& WalkOpenEvents(OGLEvent &event);

  vtkOpenGLRenderTimer* NewTimer();
  void ReleaseTimer(vtkOpenGLRenderTimer *timer);

  void ReleaseOGLFrame(OGLFrame &frame);
  void ReleaseOGLEvent(OGLEvent &event);

  void TrimTimerPool();

  void CheckPendingFrames();
  bool IsFrameReady(OGLFrame &frame);
  bool IsEventReady(OGLEvent &event);

  void ForceCloseFrame(OGLFrame &frame);
  void ForceCloseEvent(OGLEvent &event);

private:
  vtkOpenGLRenderTimerLog(const vtkOpenGLRenderTimerLog&) = delete;
  void operator=(const vtkOpenGLRenderTimerLog&) = delete;
};

#endif // vtkOpenGLRenderTimerLog_h
