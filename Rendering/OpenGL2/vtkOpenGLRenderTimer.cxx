// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkOpenGLRenderTimer.h"

#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderer.h" // For query allocation bug check

#include "vtk_glad.h"

#if defined(__EMSCRIPTEN__)
#include "webgl/webgl2_ext.h" // for EXT_disjoint_timer_query_webgl2
#if defined(EMSCRIPTEN_GL_EXT_disjoint_timer_query_webgl2)
// WebGL 2 exposes GPU timing through EXT_disjoint_timer_query_webgl2, but the
// browsers that implement it report zero GL_QUERY_COUNTER_BITS_EXT for
// GL_TIMESTAMP_EXT (the counter is disabled as a timing side channel
// mitigation), which the extension spec explicitly allows. queryCounterEXT()
// therefore raises GL_INVALID_OPERATION and its queries never become
// available, so the timestamp based implementation used everywhere else cannot
// work here.
//
// Fall back to a single GL_TIME_ELAPSED_EXT query spanning the
// whole interval. The consequences of this are documented in the header.
#define VTK_RENDER_TIMER_USE_TIME_ELAPSED
// gl2ext.h keeps its prototypes behind GL_GLEXT_PROTOTYPES.
#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2ext.h>           // for glGetQueryObjectui64vEXT
#include <emscripten/html5_webgl.h> // for emscripten_webgl_enable_extension
#endif                              // defined(EMSCRIPTEN_GL_EXT_disjoint_timer_query_webgl2)
#else
// glQueryCounter unavailable in OpenGL ES:
#ifdef GL_ES_VERSION_3_0
#define VTK_RENDER_TIMER_NO_TIMESTAMP_QUERIES
#endif // GL_ES_VERSION_3_0
#endif // defined(__EMSCRIPTEN__)

namespace
{
#if !defined(VTK_RENDER_TIMER_NO_TIMESTAMP_QUERIES)

//------------------------------------------------------------------------------
// Returns true once the result of `query` can be read back without stalling.
bool QueryResultAvailable(vtkTypeUInt32 query)
{
  if (query == 0)
  {
    return false;
  }
#ifdef GL_ES_VERSION_3_0
  // OpenGL ES / WebGL 2 only provide the unsigned flavor of this entry point.
  GLuint ready = 0;
  glGetQueryObjectuiv(static_cast<GLuint>(query), GL_QUERY_RESULT_AVAILABLE, &ready);
#else
  GLint ready = 0;
  glGetQueryObjectiv(static_cast<GLuint>(query), GL_QUERY_RESULT_AVAILABLE, &ready);
#endif
  return ready != 0;
}

//------------------------------------------------------------------------------
// Reads the (nanosecond) result of `query`. Only call once QueryResultAvailable
// returned true, otherwise this stalls until the GPU catches up.
vtkTypeUInt64 QueryResultNanoseconds(vtkTypeUInt32 query)
{
  GLuint64 result = 0;
#ifdef VTK_RENDER_TIMER_USE_TIME_ELAPSED
  glGetQueryObjectui64vEXT(static_cast<GLuint>(query), GL_QUERY_RESULT, &result);
#else
  glGetQueryObjectui64v(static_cast<GLuint>(query), GL_QUERY_RESULT, &result);
#endif
  return result;
}

#endif // VTK_RENDER_TIMER_NO_TIMESTAMP_QUERIES

#ifdef VTK_RENDER_TIMER_USE_TIME_ELAPSED
//------------------------------------------------------------------------------
// True when the GPU signalled a disjoint operation (context switch, power
// event, ...). Outstanding timers are invalid and must not be reprorted.
bool GpuDisjointOccurred()
{
  GLint disjoint = 0;
  glGetIntegerv(GL_GPU_DISJOINT_EXT, &disjoint);
  return disjoint != 0;
}

// The GL_TIME_ELAPSED_EXT query currently being recorded, or 0 when the target
// is idle. Only one such query may be active per context.
GLuint ActiveElapsedQuery = 0;

//------------------------------------------------------------------------------
// The browser may not implement the extension at all (WebKit, for one). Cache
// the answer, but only once a context actually exists.
bool TimerQueryExtensionAvailable()
{
  static int available = -1;
  if (available < 0)
  {
    const EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context = emscripten_webgl_get_current_context();
    if (context == 0)
    {
      return false; // no context yet, ask again later.
    }
    // Firefox exposes the WebGL 1 spelling on WebGL 2 contexts, so accept either.
    available = (emscripten_webgl_enable_extension(context, "EXT_disjoint_timer_query_webgl2") ||
                  emscripten_webgl_enable_extension(context, "EXT_disjoint_timer_query"))
      ? 1
      : 0;
  }
  return available == 1;
}
#endif // VTK_RENDER_TIMER_USE_TIME_ELAPSED

} // end anonymous namespace

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkOpenGLRenderTimer::vtkOpenGLRenderTimer()
  : StartReady(false)
  , EndReady(false)
  , StartQuery(0)
  , EndQuery(0)
  , StartTime(0)
  , EndTime(0)
  , ReusableStarted(false)
  , ReusableEnded(false)
{
}

//------------------------------------------------------------------------------
vtkOpenGLRenderTimer::~vtkOpenGLRenderTimer()
{
  if (this->StartQuery != 0 || this->EndQuery != 0)
  {
    this->Reset();
  }
}

//------------------------------------------------------------------------------
bool vtkOpenGLRenderTimer::IsSupported()
{
#ifdef VTK_RENDER_TIMER_NO_TIMESTAMP_QUERIES
  return false;
#elif defined(VTK_RENDER_TIMER_USE_TIME_ELAPSED)
  return TimerQueryExtensionAvailable();
#else
  static const bool s = !vtkOpenGLRenderer::HaveAppleQueryAllocationBug();
  return s;
#endif
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderTimer::Reset()
{
#ifndef VTK_RENDER_TIMER_NO_TIMESTAMP_QUERIES
  if (this->StartQuery == 0 && this->EndQuery == 0)
  {
    // short-circuit to avoid checking if queries weren't initialized at all.
    // this is necessary since `IsSupported` may make OpenGL calls on APPLE
    // through `HaveAppleQueryAllocationBug` invocation and that may be not be
    // correct when timers are being destroyed.
    return;
  }

  if (!this->IsSupported())
  {
    return;
  }

#ifdef VTK_RENDER_TIMER_USE_TIME_ELAPSED
  // StartQuery and EndQuery alias the same object here, so close the target if
  // it is still recording and delete the object exactly once.
  if (this->StartQuery != 0 && ActiveElapsedQuery == static_cast<GLuint>(this->StartQuery))
  {
    glEndQuery(GL_TIME_ELAPSED_EXT);
    ActiveElapsedQuery = 0;
  }
  if (this->StartQuery != 0)
  {
    glDeleteQueries(1, static_cast<GLuint*>(&this->StartQuery));
  }
  this->StartQuery = 0;
  this->EndQuery = 0;
#else
  if (this->StartQuery != 0)
  {
    glDeleteQueries(1, static_cast<GLuint*>(&this->StartQuery));
    this->StartQuery = 0;
  }

  if (this->EndQuery != 0)
  {
    glDeleteQueries(1, static_cast<GLuint*>(&this->EndQuery));
    this->EndQuery = 0;
  }
#endif

  this->StartReady = false;
  this->EndReady = false;
  this->StartTime = 0;
  this->EndTime = 0;
  this->ReusableStarted = false;
  this->ReusableEnded = false;
#endif // VTK_RENDER_TIMER_NO_TIMESTAMP_QUERIES
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderTimer::Start()
{
  if (!this->IsSupported())
  {
    return;
  }

  this->Reset();

#ifdef VTK_RENDER_TIMER_USE_TIME_ELAPSED
  if (ActiveElapsedQuery != 0)
  {
    vtkGenericWarningMacro("vtkOpenGLRenderTimer::Start called while another "
                           "GL_TIME_ELAPSED query is in flight. Timers cannot be nested on this "
                           "platform, no result will be reported.");
    return;
  }
  glGenQueries(1, static_cast<GLuint*>(&this->StartQuery));
  glBeginQuery(GL_TIME_ELAPSED_EXT, static_cast<GLuint>(this->StartQuery));
  ActiveElapsedQuery = static_cast<GLuint>(this->StartQuery);
#elif !defined(VTK_RENDER_TIMER_NO_TIMESTAMP_QUERIES)
  glGenQueries(1, static_cast<GLuint*>(&this->StartQuery));
  glQueryCounter(static_cast<GLuint>(this->StartQuery), GL_TIMESTAMP);
#endif
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderTimer::Stop()
{
#ifndef VTK_RENDER_TIMER_NO_TIMESTAMP_QUERIES
  if (!this->IsSupported())
  {
    return;
  }

  if (this->EndQuery != 0)
  {
    vtkGenericWarningMacro("vtkOpenGLRenderTimer::Stop called before "
                           "resetting. Ignoring.");
    return;
  }

  if (this->StartQuery == 0)
  {
    vtkGenericWarningMacro("vtkOpenGLRenderTimer::Stop called before "
                           "vtkOpenGLRenderTimer::Start. Ignoring.");
    return;
  }

#ifdef VTK_RENDER_TIMER_USE_TIME_ELAPSED
  glEndQuery(GL_TIME_ELAPSED_EXT);
  ActiveElapsedQuery = 0;
  // The single elapsed query carries both endpoints.
  this->EndQuery = this->StartQuery;
#else
  glGenQueries(1, static_cast<GLuint*>(&this->EndQuery));
  glQueryCounter(static_cast<GLuint>(this->EndQuery), GL_TIMESTAMP);
#endif
#endif // VTK_RENDER_TIMER_NO_TIMESTAMP_QUERIES
}

//------------------------------------------------------------------------------
bool vtkOpenGLRenderTimer::Started()
{
#ifndef VTK_RENDER_TIMER_NO_TIMESTAMP_QUERIES
  return this->StartQuery != 0;
#else  // VTK_RENDER_TIMER_NO_TIMESTAMP_QUERIES
  return false;
#endif // VTK_RENDER_TIMER_NO_TIMESTAMP_QUERIES
}

//------------------------------------------------------------------------------
bool vtkOpenGLRenderTimer::Stopped()
{
#ifndef VTK_RENDER_TIMER_NO_TIMESTAMP_QUERIES
  return this->EndQuery != 0;
#else  // VTK_RENDER_TIMER_NO_TIMESTAMP_QUERIES
  return false;
#endif // VTK_RENDER_TIMER_NO_TIMESTAMP_QUERIES
}

//------------------------------------------------------------------------------
bool vtkOpenGLRenderTimer::Ready()
{
#ifndef VTK_RENDER_TIMER_NO_TIMESTAMP_QUERIES
  if (!this->IsSupported())
  {
    return false;
  }

  // querying 0 is invalid.
  if (this->StartQuery == 0 || this->EndQuery == 0)
  {
    return false;
  }

#ifdef VTK_RENDER_TIMER_USE_TIME_ELAPSED
  if (!this->EndReady)
  {
    if (!QueryResultAvailable(this->StartQuery))
    {
      return false;
    }

    if (GpuDisjointOccurred())
    {
      // The measurement is meaningless, drop it instead of reporting a spike.
      this->Reset();
      return false;
    }

    // Absolute timestamps are unavailable, report the interval as [0, elapsed].
    this->StartTime = 0;
    this->EndTime = QueryResultNanoseconds(this->StartQuery);
    this->StartReady = true;
    this->EndReady = true;
  }
#else
  if (!this->StartReady)
  {
    if (!QueryResultAvailable(this->StartQuery))
    {
      return false;
    }

    this->StartReady = true;
    this->StartTime = QueryResultNanoseconds(this->StartQuery);
  }

  if (!this->EndReady)
  {
    if (!QueryResultAvailable(this->EndQuery))
    {
      return false;
    }

    this->EndReady = true;
    this->EndTime = QueryResultNanoseconds(this->EndQuery);
  }
#endif
#endif // VTK_RENDER_TIMER_NO_TIMESTAMP_QUERIES

  return true;
}

//------------------------------------------------------------------------------
float vtkOpenGLRenderTimer::GetElapsedSeconds()
{
#ifndef VTK_RENDER_TIMER_NO_TIMESTAMP_QUERIES
  if (!this->Ready())
  {
    return 0.f;
  }

  return (this->EndTime - this->StartTime) * 1e-9f;
#else  // VTK_RENDER_TIMER_NO_TIMESTAMP_QUERIES
  return 0.f;
#endif // VTK_RENDER_TIMER_NO_TIMESTAMP_QUERIES
}

//------------------------------------------------------------------------------
float vtkOpenGLRenderTimer::GetElapsedMilliseconds()
{
#ifndef VTK_RENDER_TIMER_NO_TIMESTAMP_QUERIES
  if (!this->Ready())
  {
    return 0.f;
  }

  return (this->EndTime - this->StartTime) * 1e-6f;
#else  // VTK_RENDER_TIMER_NO_TIMESTAMP_QUERIES
  return 0.f;
#endif // VTK_RENDER_TIMER_NO_TIMESTAMP_QUERIES
}

//------------------------------------------------------------------------------
vtkTypeUInt64 vtkOpenGLRenderTimer::GetElapsedNanoseconds()
{
#ifndef VTK_RENDER_TIMER_NO_TIMESTAMP_QUERIES
  if (!this->Ready())
  {
    return 0;
  }

  return (this->EndTime - this->StartTime);
#else  // VTK_RENDER_TIMER_NO_TIMESTAMP_QUERIES
  return 0;
#endif // VTK_RENDER_TIMER_NO_TIMESTAMP_QUERIES
}

//------------------------------------------------------------------------------
vtkTypeUInt64 vtkOpenGLRenderTimer::GetStartTime()
{
#ifndef VTK_RENDER_TIMER_NO_TIMESTAMP_QUERIES
  if (!this->Ready())
  {
    return 0;
  }

  return this->StartTime;
#else  // VTK_RENDER_TIMER_NO_TIMESTAMP_QUERIES
  return 0;
#endif // VTK_RENDER_TIMER_NO_TIMESTAMP_QUERIES
}

//------------------------------------------------------------------------------
vtkTypeUInt64 vtkOpenGLRenderTimer::GetStopTime()
{
#ifndef VTK_RENDER_TIMER_NO_TIMESTAMP_QUERIES
  if (!this->Ready())
  {
    return 0;
  }

  return this->EndTime;
#else  // VTK_RENDER_TIMER_NO_TIMESTAMP_QUERIES
  return 0;
#endif // VTK_RENDER_TIMER_NO_TIMESTAMP_QUERIES
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderTimer::ReleaseGraphicsResources()
{
  this->Reset();
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderTimer::ReusableStart()
{
#ifndef VTK_RENDER_TIMER_NO_TIMESTAMP_QUERIES
  if (!this->IsSupported())
  {
    return;
  }

#ifdef VTK_RENDER_TIMER_USE_TIME_ELAPSED
  if (this->ReusableStarted)
  {
    return;
  }

  if (ActiveElapsedQuery != 0)
  {
    // Another timer owns the target. Skip this frame rather than raise a GL
    // error, the next call will try again.
    return;
  }

  if (this->StartQuery == 0)
  {
    glGenQueries(1, static_cast<GLuint*>(&this->StartQuery));
  }
  glBeginQuery(GL_TIME_ELAPSED_EXT, static_cast<GLuint>(this->StartQuery));
  ActiveElapsedQuery = static_cast<GLuint>(this->StartQuery);
  this->ReusableStarted = true;
  this->ReusableEnded = false;
#else
  if (this->StartQuery == 0)
  {
    glGenQueries(1, static_cast<GLuint*>(&this->StartQuery));
    glQueryCounter(static_cast<GLuint>(this->StartQuery), GL_TIMESTAMP);
    this->ReusableStarted = true;
    this->ReusableEnded = false;
  }
  if (!this->ReusableStarted)
  {
    glQueryCounter(static_cast<GLuint>(this->StartQuery), GL_TIMESTAMP);
    this->ReusableStarted = true;
    this->ReusableEnded = false;
  }
#endif
#endif // VTK_RENDER_TIMER_NO_TIMESTAMP_QUERIES
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderTimer::ReusableStop()
{
#ifndef VTK_RENDER_TIMER_NO_TIMESTAMP_QUERIES
  if (!this->IsSupported())
  {
    return;
  }

  if (!this->ReusableStarted)
  {
    vtkGenericWarningMacro("vtkOpenGLRenderTimer::ReusableStop called before "
                           "vtkOpenGLRenderTimer::ReusableStart. Ignoring.");
    return;
  }

#ifdef VTK_RENDER_TIMER_USE_TIME_ELAPSED
  if (this->ReusableEnded)
  {
    return;
  }

  glEndQuery(GL_TIME_ELAPSED_EXT);
  ActiveElapsedQuery = 0;
  this->EndQuery = this->StartQuery;
  this->ReusableEnded = true;
#else
  if (this->EndQuery == 0)
  {
    glGenQueries(1, static_cast<GLuint*>(&this->EndQuery));
    glQueryCounter(static_cast<GLuint>(this->EndQuery), GL_TIMESTAMP);
    this->ReusableEnded = true;
  }
  if (!this->ReusableEnded)
  {
    glQueryCounter(static_cast<GLuint>(this->EndQuery), GL_TIMESTAMP);
    this->ReusableEnded = true;
  }
#endif
#endif // VTK_RENDER_TIMER_NO_TIMESTAMP_QUERIES
}

//------------------------------------------------------------------------------
float vtkOpenGLRenderTimer::GetReusableElapsedSeconds()
{
#ifndef VTK_RENDER_TIMER_NO_TIMESTAMP_QUERIES
  // we do not have an end query yet so we cannot have a time
  if (!this->EndQuery)
  {
    return 0.0;
  }

#ifdef VTK_RENDER_TIMER_USE_TIME_ELAPSED
  if (this->ReusableStarted && this->ReusableEnded && QueryResultAvailable(this->EndQuery))
  {
    if (GpuDisjointOccurred())
    {
      // The measurement is meaningless, drop it instead of reporting a spike.
      this->StartTime = 0;
      this->EndTime = 0;
    }
    else
    {
      // Absolute timestamps are unavailable, report the interval as [0, elapsed].
      this->StartTime = 0;
      this->EndTime = QueryResultNanoseconds(this->EndQuery);
    }
    // it was ready so prepare another flight
    this->ReusableStarted = false;
    this->ReusableEnded = false;
  }
#else
  if (this->ReusableStarted && !this->StartReady)
  {
    if (QueryResultAvailable(this->StartQuery))
    {
      this->StartReady = true;
    }
  }

  if (this->StartReady && this->ReusableEnded && !this->EndReady)
  {
    if (QueryResultAvailable(this->EndQuery))
    {
      this->EndReady = true;
    }
  }

  // if we have a complete flight, then get the times and prepare for another
  // flight. Note that we cannot use the Ready() method as it will not work
  // beyond the first flight.
  if (this->StartReady && this->EndReady)
  {
    this->StartTime = QueryResultNanoseconds(this->StartQuery);
    this->EndTime = QueryResultNanoseconds(this->EndQuery);
    // it was ready so prepare another flight
    this->ReusableStarted = false;
    this->ReusableEnded = false;
    this->StartReady = false;
    this->EndReady = false;
  }
#endif

  return (this->EndTime - this->StartTime) * 1e-9f;
#else  // VTK_RENDER_TIMER_NO_TIMESTAMP_QUERIES
  return 0.f;
#endif // VTK_RENDER_TIMER_NO_TIMESTAMP_QUERIES
}

VTK_ABI_NAMESPACE_END
