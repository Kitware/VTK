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

//------------------------------------------------------------------------------
vtkOpenGLRenderTimer::vtkOpenGLRenderTimer()
  : StartReady(false),
    EndReady(false),
    StartQuery(0),
    EndQuery(0),
    StartTime(0),
    EndTime(0)
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
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderTimer::Start()
{
  this->Reset();

  glGenQueries(1, static_cast<GLuint*>(&this->StartQuery));
  glQueryCounter(static_cast<GLuint>(this->StartQuery), GL_TIMESTAMP);
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderTimer::Stop()
{
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
}

//------------------------------------------------------------------------------
bool vtkOpenGLRenderTimer::Started()
{
  return this->StartQuery != 0;
}

//------------------------------------------------------------------------------
bool vtkOpenGLRenderTimer::Stopped()
{
  return this->EndQuery != 0;
}

//------------------------------------------------------------------------------
bool vtkOpenGLRenderTimer::Ready()
{
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

  return true;
}

//------------------------------------------------------------------------------
float vtkOpenGLRenderTimer::GetElapsedSeconds()
{
  if (!this->Ready())
    {
    return 0.f;
    }

  return (this->EndTime - this->StartTime) * 1e-9f;
}

//------------------------------------------------------------------------------
float vtkOpenGLRenderTimer::GetElapsedMilliseconds()
{
  if (!this->Ready())
    {
    return 0.f;
    }

  return (this->EndTime - this->StartTime) * 1e-6f;
}

//------------------------------------------------------------------------------
vtkTypeUInt64 vtkOpenGLRenderTimer::GetElapsedNanoseconds()
{
  if (!this->Ready())
    {
    return 0;
    }

  return (this->EndTime - this->StartTime);
}
