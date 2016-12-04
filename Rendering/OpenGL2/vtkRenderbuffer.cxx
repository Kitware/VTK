/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderbuffer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRenderbuffer.h"

#include "vtk_glew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLError.h"

#include <cassert>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkRenderbuffer);

//----------------------------------------------------------------------------
vtkRenderbuffer::vtkRenderbuffer()
{
  this->Context = NULL;
  this->Handle = 0U;
  this->DepthBufferFloat = 0;
  this->Samples = 0;
  this->Format = GL_RGBA;
}

//----------------------------------------------------------------------------
vtkRenderbuffer::~vtkRenderbuffer()
{
  this->Free();
}

//----------------------------------------------------------------------------
bool vtkRenderbuffer::IsSupported(vtkRenderWindow *)
{
  return true;
}

//----------------------------------------------------------------------------
bool vtkRenderbuffer::LoadRequiredExtensions(vtkRenderWindow *)
{
  // both texture float and depth float are part of OpenGL 3.0 and later
  this->DepthBufferFloat = true;
  return true;
}

//----------------------------------------------------------------------------
void vtkRenderbuffer::Alloc()
{
  glGenRenderbuffers(1, &this->Handle);
  vtkOpenGLCheckErrorMacro("failed at glGenRenderbuffers");
}

void vtkRenderbuffer::ReleaseGraphicsResources(vtkWindow *)
{
  if (this->Context && this->Handle)
  {
    glDeleteRenderbuffers(1, &this->Handle);
    vtkOpenGLCheckErrorMacro("failed at glDeleteRenderBuffers");
  }
}

//----------------------------------------------------------------------------
void vtkRenderbuffer::Free()
{
  this->ReleaseGraphicsResources(NULL);
}

//----------------------------------------------------------------------------
vtkRenderWindow *vtkRenderbuffer::GetContext()
{
  return this->Context;
}

//----------------------------------------------------------------------------
void vtkRenderbuffer::SetContext(vtkRenderWindow *renWin)
{
  // avoid pointless re-assignment
  if (this->Context==renWin){ return; }

  // free previous resources
  this->Free();
  this->Context = NULL;
  this->DepthBufferFloat = 0;
  this->Modified();

  // check for supported context
  vtkOpenGLRenderWindow *context = dynamic_cast<vtkOpenGLRenderWindow*>(renWin);
  if ( !context
    || !this->LoadRequiredExtensions(renWin) )
  {
    vtkErrorMacro("Unsupported render context");
    return;
  }

  // allocate new fbo
  this->Context=renWin;
  this->Context->MakeCurrent();
  this->Alloc();
}
//----------------------------------------------------------------------------
int vtkRenderbuffer::CreateColorAttachment(
      unsigned int width,
      unsigned int height)
{
  assert(this->Context);
  return this->Create(GL_RGBA32F, width, height);
}

//----------------------------------------------------------------------------
int vtkRenderbuffer::CreateDepthAttachment(
      unsigned int width,
      unsigned int height)
{
  assert(this->Context);

  // typically DEPTH_COMPONENT will end up being a 32 bit floating
  // point format however it's not a guarantee and does not seem
  // to be the case with mesa hence the need to explicitly specify
  // it as such if at all possible.
  if (this->DepthBufferFloat)
  {
    return this->Create(
          GL_DEPTH_COMPONENT32F,
          width,
          height);
  }

  return this->Create(
        GL_DEPTH_COMPONENT,
        width,
        height);
}

//----------------------------------------------------------------------------
int vtkRenderbuffer::Create(
      unsigned int format,
      unsigned int width,
      unsigned int height)
{
  return this->Create(format, width, height, 0);
}

int vtkRenderbuffer::Create(
      unsigned int format,
      unsigned int width,
      unsigned int height,
      unsigned int samples)
{
  assert(this->Context);

  glBindRenderbuffer(GL_RENDERBUFFER, (GLuint)this->Handle);
  vtkOpenGLCheckErrorMacro("failed at glBindRenderBuffer");

  if (samples)
  {
    glRenderbufferStorageMultisample(
      GL_RENDERBUFFER,
      samples, (GLenum)format,
      width, height);
  }
  else
  {
    glRenderbufferStorage(GL_RENDERBUFFER, (GLenum)format, width, height);
  }
  vtkOpenGLCheckErrorMacro("failed at glRenderbufferStorage");

  this->Width = width;
  this->Height = height;
  this->Format = format;
  this->Samples = samples;

  return 1;
}

void vtkRenderbuffer::Resize(unsigned int width, unsigned int height)
{
  if (this->Width == width && this->Height == height)
  {
    return;
  }

  if (this->Context && this->Handle)
  {
    glBindRenderbuffer(GL_RENDERBUFFER, (GLuint)this->Handle);
    if (this->Samples)
    {
    glRenderbufferStorageMultisample(
      GL_RENDERBUFFER,
      this->Samples, (GLenum)this->Format,
      width, height);
    }
    else
    {
      glRenderbufferStorage(GL_RENDERBUFFER,
        (GLenum)this->Format, width, height);
    }
  }
  this->Width = width;
  this->Height = height;
}

// ----------------------------------------------------------------------------
void vtkRenderbuffer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os
    << indent << "Handle=" << this->Handle << endl
    << indent << "Context=" << this->Context << endl;
}
