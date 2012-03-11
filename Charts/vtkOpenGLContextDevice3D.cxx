/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLContextDevice3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLContextDevice3D.h"

#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLExtensionManager.h"
#include "vtkgl.h"

#include "vtkObjectFactory.h"

class vtkOpenGLContextDevice3D::Private
{
public:
  Private()
  {
    this->SavedLighting = GL_TRUE;
    this->SavedDepthTest = GL_TRUE;
  }

  ~Private()
  {
  }

  void SaveGLState()
  {
    this->SavedLighting = glIsEnabled(GL_LIGHTING);
    this->SavedDepthTest = glIsEnabled(GL_DEPTH_TEST);
    this->SavedBlending = glIsEnabled(GL_BLEND);
  }

  void RestoreGLState()
  {
    this->SetGLCapability(GL_LIGHTING, this->SavedLighting);
    this->SetGLCapability(GL_DEPTH_TEST, this->SavedDepthTest);
    this->SetGLCapability(GL_BLEND, this->SavedBlending);
  }

  void SetGLCapability(GLenum capability, GLboolean state)
  {
    if (state)
      {
      glEnable(capability);
      }
    else
      {
      glDisable(capability);
      }
  }

  // Store the previous GL state so that we can restore it when complete
  GLboolean SavedLighting;
  GLboolean SavedDepthTest;
  GLboolean SavedBlending;

  vtkVector2i Dim;
  vtkVector2i Offset;
};

vtkStandardNewMacro(vtkOpenGLContextDevice3D)

vtkOpenGLContextDevice3D::vtkOpenGLContextDevice3D() : Storage(new Private)
{
}

vtkOpenGLContextDevice3D::~vtkOpenGLContextDevice3D()
{
  delete Storage;
}

void vtkOpenGLContextDevice3D::PrintSelf(ostream &os, vtkIndent indent)
{
}

void vtkOpenGLContextDevice3D::DrawLine(const vtkVector3f &start,
                                        const vtkVector3f &end)
{

}

void vtkOpenGLContextDevice3D::DrawPoint(const vtkVector3f &point)
{

}

void vtkOpenGLContextDevice3D::ApplyPen(vtkPen *pen)
{

}

void vtkOpenGLContextDevice3D::ApplyBrush(vtkBrush *brush)
{

}

void vtkOpenGLContextDevice3D::SetMatrix(vtkMatrix4x4 *m)
{

}

void vtkOpenGLContextDevice3D::GetMatrix(vtkMatrix4x4 *m)
{

}

void vtkOpenGLContextDevice3D::MultiplyMatrix(vtkMatrix4x4 *m)
{

}

void vtkOpenGLContextDevice3D::PushMatrix()
{
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
}

void vtkOpenGLContextDevice3D::PopMatrix()
{
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice3D::SetClipping(const vtkRecti &rect)
{
  // Check the bounds, and clamp if necessary
  GLint vp[4] = { this->Storage->Offset.GetX(), this->Storage->Offset.GetY(),
                  this->Storage->Dim.GetX(), this->Storage->Dim.GetY()};

  if (rect.X() > 0 && rect.X() < vp[2] )
    {
    vp[0] += rect.X();
    }
  if (rect.Y() > 0 && rect.Y() < vp[3])
    {
    vp[1] += rect.Y();
    }
  if (rect.Width() > 0 && rect.Width() < vp[2])
    {
    vp[2] = rect.Width();
    }
  if (rect.Height() > 0 && rect.Height() < vp[3])
    {
    vp[3] = rect.Height();
    }

  glScissor(vp[0], vp[1], vp[2], vp[3]);
}

void vtkOpenGLContextDevice3D::EnableClipping(bool enable)
{
  if (enable)
    {
    glEnable(GL_SCISSOR_TEST);
    }
  else
    {
    glDisable(GL_SCISSOR_TEST);
    }
}

void vtkOpenGLContextDevice3D::Begin(vtkViewport* viewport)
{
  // Need the actual pixel size of the viewport - ask OpenGL.
  GLint vp[4];
  glGetIntegerv(GL_VIEWPORT, vp);
  this->Storage->Offset.Set(static_cast<int>(vp[0]),
                            static_cast<int>(vp[1]));

  this->Storage->Dim.Set(static_cast<int>(vp[2]),
                         static_cast<int>(vp[3]));

  // push a 2D matrix on the stack
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  float offset = 0.5;
  glOrtho(offset, vp[2]+offset-1.0,
          offset, vp[3]+offset-1.0,
          -1000, 1000);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  // Store the previous state before changing it
  this->Storage->SaveGLState();
  glDisable(GL_LIGHTING);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);

  this->Renderer = vtkRenderer::SafeDownCast(viewport);

  this->InRender = true;
}

void vtkOpenGLContextDevice3D::End()
{
  if (!this->InRender)
    {
    return;
    }

  // push a 2D matrix on the stack
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  // Restore the GL state that we changed
  this->Storage->RestoreGLState();

  this->InRender = false;
}
