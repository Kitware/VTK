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
}

//----------------------------------------------------------------------------
vtkRenderbuffer::~vtkRenderbuffer()
{
  this->Free();
}

//----------------------------------------------------------------------------
bool vtkRenderbuffer::IsSupported(vtkRenderWindow *win)
{
  bool supported = false;

  vtkOpenGLRenderWindow *glwin = dynamic_cast<vtkOpenGLRenderWindow*>(win);
  if (glwin)
    {
    bool floatTex = glewIsSupported("GL_ARB_texture_float");
    bool floatDepth = glewIsSupported("GL_ARB_depth_buffer_float");
    bool fbo = true;

    supported = floatTex && floatDepth && fbo;
    }

  return supported;
}

//----------------------------------------------------------------------------
bool vtkRenderbuffer::LoadRequiredExtensions(vtkRenderWindow *win)
{
  bool supported = false;

  vtkOpenGLRenderWindow *glwin = dynamic_cast<vtkOpenGLRenderWindow*>(win);
  if (glwin)
    {

    bool floatTex = glewIsSupported("GL_ARB_texture_float");
    bool fbo = true;

    supported = floatTex && fbo;

    if (supported)
      {
      // no functions to load for floatTex

      // We'll use floating point depth buffers if they are available.
      this->DepthBufferFloat = glewIsSupported("GL_ARB_depth_buffer_float");
      }
    }

  return supported;
}

//----------------------------------------------------------------------------
void vtkRenderbuffer::Alloc()
{
  glGenRenderbuffersEXT(1, &this->Handle);
  vtkOpenGLCheckErrorMacro("failed at glGenRenderbuffers");
}

//----------------------------------------------------------------------------
void vtkRenderbuffer::Free()
{
  // because we don't hold a reference to the render
  // context we don't have any control on when it is
  // destroyed. In fact it may be destroyed before
  // we are(eg smart pointers), in which case we should
  // do nothing.
  if (this->Context && this->Handle)
    {
    glDeleteRenderbuffersEXT(1, &this->Handle);
    vtkOpenGLCheckErrorMacro("failed at glDeleteRenderBuffers");
    }
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
  assert(this->Context);

  glBindRenderbufferEXT(GL_RENDERBUFFER, (GLuint)this->Handle);
  vtkOpenGLCheckErrorMacro("failed at glBindRenderBuffer");

  glRenderbufferStorageEXT(GL_RENDERBUFFER, (GLenum)format, width, height);
  vtkOpenGLCheckErrorMacro("failed at glRenderbufferStorage");

  return 1;
}

// ----------------------------------------------------------------------------
void vtkRenderbuffer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os
    << indent << "Handle=" << this->Handle << endl
    << indent << "Context=" << this->Context << endl;
}
