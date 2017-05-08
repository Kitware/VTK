/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLRenderTimer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLRenderTimer.h"

#include "vtkObjectFactory.h"

#include "vtk_glew.h"

// glQueryCounter unavailable in OpenGL ES:
#if defined(GL_ES_VERSION_3_0)
#define NO_TIMESTAMP_QUERIES
#endif

//------------------------------------------------------------------------------
vtkOpenGLRenderTimer::vtkOpenGLRenderTimer()
  : StartReady(false),
    EndReady(false),
    StartQuery(0),
    EndQuery(0),
    StartTime(0),
    EndTime(0),
    ReusableStarted(false),
    ReusableEnded(false)
{
}

//------------------------------------------------------------------------------
vtkOpenGLRenderTimer::~vtkOpenGLRenderTimer()
{
  this->Reset();
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderTimer::Reset()
{
#ifndef NO_TIMESTAMP_QUERIES
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

  this->StartReady = false;
  this->EndReady = false;
  this->StartTime = 0;
  this->EndTime = 0;
#endif // NO_TIMESTAMP_QUERIES
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderTimer::Start()
{
  this->Reset();

#ifndef NO_TIMESTAMP_QUERIES
  glGenQueries(1, static_cast<GLuint*>(&this->StartQuery));
  glQueryCounter(static_cast<GLuint>(this->StartQuery), GL_TIMESTAMP);
#endif // NO_TIMESTAMP_QUERIES
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderTimer::Stop()
{
#ifndef NO_TIMESTAMP_QUERIES
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

  glGenQueries(1, static_cast<GLuint*>(&this->EndQuery));
  glQueryCounter(static_cast<GLuint>(this->EndQuery), GL_TIMESTAMP);
#endif // NO_TIMESTAMP_QUERIES
}

//------------------------------------------------------------------------------
bool vtkOpenGLRenderTimer::Started()
{
#ifndef NO_TIMESTAMP_QUERIES
  return this->StartQuery != 0;
#else // NO_TIMESTAMP_QUERIES
  return false;
#endif // NO_TIMESTAMP_QUERIES
}

//------------------------------------------------------------------------------
bool vtkOpenGLRenderTimer::Stopped()
{
#ifndef NO_TIMESTAMP_QUERIES
  return this->EndQuery != 0;
#else // NO_TIMESTAMP_QUERIES
  return false;
#endif // NO_TIMESTAMP_QUERIES
}

//------------------------------------------------------------------------------
bool vtkOpenGLRenderTimer::Ready()
{
#ifndef NO_TIMESTAMP_QUERIES
  if (!this->StartReady)
  {
    GLint ready;
    glGetQueryObjectiv(static_cast<GLuint>(this->StartQuery),
                       GL_QUERY_RESULT_AVAILABLE, &ready);
    if (!ready)
    {
      return false;
    }

    this->StartReady = true;
    glGetQueryObjectui64v(static_cast<GLuint>(this->StartQuery),
                          GL_QUERY_RESULT,
                          reinterpret_cast<GLuint64*>(&this->StartTime));
  }

  if (!this->EndReady)
  {
    GLint ready;
    glGetQueryObjectiv(static_cast<GLuint>(this->EndQuery),
                       GL_QUERY_RESULT_AVAILABLE, &ready);
    if (!ready)
    {
      return false;
    }

    this->EndReady = true;
    glGetQueryObjectui64v(static_cast<GLuint>(this->EndQuery),
                          GL_QUERY_RESULT,
                          reinterpret_cast<GLuint64*>(&this->EndTime));
  }
#endif // NO_TIMESTAMP_QUERIES

  return true;
}

//------------------------------------------------------------------------------
float vtkOpenGLRenderTimer::GetElapsedSeconds()
{
#ifndef NO_TIMESTAMP_QUERIES
  if (!this->Ready())
  {
    return 0.f;
  }

  return (this->EndTime - this->StartTime) * 1e-9f;
#else // NO_TIMESTAMP_QUERIES
  return 0.f;
#endif // NO_TIMESTAMP_QUERIES
}

//------------------------------------------------------------------------------
float vtkOpenGLRenderTimer::GetElapsedMilliseconds()
{
#ifndef NO_TIMESTAMP_QUERIES
  if (!this->Ready())
  {
    return 0.f;
  }

  return (this->EndTime - this->StartTime) * 1e-6f;
#else // NO_TIMESTAMP_QUERIES
  return 0.f;
#endif // NO_TIMESTAMP_QUERIES
}

//------------------------------------------------------------------------------
vtkTypeUInt64
vtkOpenGLRenderTimer::GetElapsedNanoseconds()
{
#ifndef NO_TIMESTAMP_QUERIES
  if (!this->Ready())
  {
    return 0;
  }

  return (this->EndTime - this->StartTime);
#else // NO_TIMESTAMP_QUERIES
  return 0;
#endif // NO_TIMESTAMP_QUERIES
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderTimer::ReleaseGraphicsResources()
{
  this->Reset();
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderTimer::ReusableStart()
{

#ifndef NO_TIMESTAMP_QUERIES
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
#endif // NO_TIMESTAMP_QUERIES
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderTimer::ReusableStop()
{

#ifndef NO_TIMESTAMP_QUERIES
  if (!this->ReusableStarted)
  {
    vtkGenericWarningMacro("vtkOpenGLRenderTimer::ReusableStop called before "
                           "vtkOpenGLRenderTimer::ReusableStart. Ignoring.");
    return;
  }

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
#endif // NO_TIMESTAMP_QUERIES
}

//------------------------------------------------------------------------------
float vtkOpenGLRenderTimer::GetReusableElapsedSeconds()
{
#ifndef NO_TIMESTAMP_QUERIES
  // we do not have an end query yet so we cannot have a time
  if (!this->EndQuery)
  {
    return 0.0;
  }

  if (this->ReusableStarted && !this->StartReady)
  {
    GLint ready;
    glGetQueryObjectiv(static_cast<GLuint>(this->StartQuery),
                       GL_QUERY_RESULT_AVAILABLE, &ready);
    if (ready)
    {
     this->StartReady = true;
    }
  }

  if (this->StartReady && this->ReusableEnded && !this->EndReady)
  {
    GLint ready;
    glGetQueryObjectiv(static_cast<GLuint>(this->EndQuery),
                       GL_QUERY_RESULT_AVAILABLE, &ready);
    if (ready)
    {
      this->EndReady = true;
    }
  }

  // if everything is ready read the times to get a new elapsed time
  // and then prep for a new flight. This also has the benefit that
  // if no one is getting the elapsed time then nothing is done
  // beyond the first flight.
  if (this->StartReady && this->EndReady)
  {
    glGetQueryObjectui64v(static_cast<GLuint>(this->StartQuery),
                          GL_QUERY_RESULT,
                          reinterpret_cast<GLuint64*>(&this->StartTime));
    glGetQueryObjectui64v(static_cast<GLuint>(this->EndQuery),
                          GL_QUERY_RESULT,
                          reinterpret_cast<GLuint64*>(&this->EndTime));
    // it was ready so prepare another flight
    this->ReusableStarted = false;
    this->ReusableEnded = false;
    this->StartReady = false;
    this->EndReady = false;
  }

  return (this->EndTime - this->StartTime) * 1e-9f;
#else // NO_TIMESTAMP_QUERIES
  return 0.f;
#endif // NO_TIMESTAMP_QUERIES
}
