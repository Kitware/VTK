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

#include "vtkBrush.h"
#include "vtkPen.h"

#include "vtkMatrix4x4.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLError.h"

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
    vtkOpenGLStaticCheckErrorMacro("failed after SetGLCapability");
  }

  void Transpose(double *in, double *transposed)
  {
    transposed[0] = in[0];
    transposed[1] = in[4];
    transposed[2] = in[8];
    transposed[3] = in[12];

    transposed[4] = in[1];
    transposed[5] = in[5];
    transposed[6] = in[9];
    transposed[7] = in[13];

    transposed[8] = in[2];
    transposed[9] = in[6];
    transposed[10] = in[10];
    transposed[11] = in[14];

    transposed[12] = in[3];
    transposed[13] = in[7];
    transposed[14] = in[11];
    transposed[15] = in[15];
  }

  void SetLineType(int type)
  {
    if (type == vtkPen::SOLID_LINE)
      {
      glDisable(GL_LINE_STIPPLE);
      }
    else
      {
      glEnable(GL_LINE_STIPPLE);
      }
    GLushort pattern = 0x0000;
    switch (type)
      {
      case vtkPen::NO_PEN:
        pattern = 0x0000;
        break;
      case vtkPen::DASH_LINE:
        pattern = 0x00FF;
        break;
      case vtkPen::DOT_LINE:
        pattern = 0x0101;
        break;
      case vtkPen::DASH_DOT_LINE:
        pattern = 0x0C0F;
        break;
      case vtkPen::DASH_DOT_DOT_LINE:
        pattern = 0x1C47;
        break;
      default:
        pattern = 0x0000;
      }
    glLineStipple(1, pattern);
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

void vtkOpenGLContextDevice3D::DrawPoly(const float *verts, int n,
                                        const unsigned char *colors, int nc)
{
  assert("verts must be non-null" && verts != NULL);
  assert("n must be greater than 0" && n > 0);

  vtkOpenGLClearErrorMacro();

  this->EnableDepthBuffer();

  this->Storage->SetLineType(this->Pen->GetLineType());
  glLineWidth(this->Pen->GetWidth());

  if (colors)
    {
    glEnableClientState(GL_COLOR_ARRAY);
    glColorPointer(nc, GL_UNSIGNED_BYTE, 0, colors);
    }
  else
    {
    glColor4ubv(this->Pen->GetColor());
    }
  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(3, GL_FLOAT, 0, verts);
  glDrawArrays(GL_LINE_STRIP, 0, n);
  glDisableClientState(GL_VERTEX_ARRAY);
  if (colors)
    {
    glDisableClientState(GL_COLOR_ARRAY);
    }

  this->DisableDepthBuffer();

  vtkOpenGLCheckErrorMacro("failed after DrawPoly");
}

void vtkOpenGLContextDevice3D::DrawPoints(const float *verts, int n,
                                          const unsigned char *colors, int nc)
{
  assert("verts must be non-null" && verts != NULL);
  assert("n must be greater than 0" && n > 0);

  vtkOpenGLClearErrorMacro();

  this->EnableDepthBuffer();

  glPointSize(this->Pen->GetWidth());
  glEnableClientState(GL_VERTEX_ARRAY);
  if (colors && nc)
    {
    glEnableClientState(GL_COLOR_ARRAY);
    glColorPointer(nc, GL_UNSIGNED_BYTE, 0, colors);
    }
  else
    {
    glColor4ubv(this->Pen->GetColor());
    }
  glVertexPointer(3, GL_FLOAT, 0, verts);
  glDrawArrays(GL_POINTS, 0, n);
  glDisableClientState(GL_VERTEX_ARRAY);
  if (colors && nc)
    {
    glDisableClientState(GL_COLOR_ARRAY);
    }

  this->DisableDepthBuffer();

  vtkOpenGLCheckErrorMacro("failed DrawPoints");
}

void vtkOpenGLContextDevice3D::DrawTriangleMesh(const float *mesh, int n,
                                                const unsigned char *colors,
                                                int nc)
{
  assert("mesh must be non-null" && mesh != NULL);
  assert("n must be greater than 0" && n > 0);

  vtkOpenGLClearErrorMacro();

  this->EnableDepthBuffer();

  glPointSize(this->Pen->GetWidth());
  glEnableClientState(GL_VERTEX_ARRAY);
  if (colors && nc)
    {
    glEnableClientState(GL_COLOR_ARRAY);
    glColorPointer(nc, GL_UNSIGNED_BYTE, 0, colors);
    }
  else
    {
    glColor4ubv(this->Pen->GetColor());
    }
  glVertexPointer(3, GL_FLOAT, 0, mesh);
  glDrawArrays(GL_TRIANGLES, 0, n);
  glDisableClientState(GL_VERTEX_ARRAY);
  if (colors && nc)
    {
    glDisableClientState(GL_COLOR_ARRAY);
    }

  this->DisableDepthBuffer();

  vtkOpenGLCheckErrorMacro("failed after DrawTriangleMesh");
}

void vtkOpenGLContextDevice3D::ApplyPen(vtkPen *pen)
{
  this->Pen->DeepCopy(pen);
}

void vtkOpenGLContextDevice3D::ApplyBrush(vtkBrush *brush)
{
  this->Brush->DeepCopy(brush);
}

void vtkOpenGLContextDevice3D::SetMatrix(vtkMatrix4x4 *m)
{
  double matrix[16];
  double *M = m->Element[0];
  this->Storage->Transpose(M, matrix);

  glLoadMatrixd(matrix);
}

void vtkOpenGLContextDevice3D::GetMatrix(vtkMatrix4x4 *m)
{
  double *M = m->Element[0];
  m->Transpose();
  double matrix[16];
  glGetDoublev(GL_MODELVIEW_MATRIX, matrix);
  this->Storage->Transpose(matrix, M);
}

void vtkOpenGLContextDevice3D::MultiplyMatrix(vtkMatrix4x4 *m)
{
  double matrix[16];
  double *M = m->Element[0];
  this->Storage->Transpose(M, matrix);

  glMultMatrixd(matrix);
}

void vtkOpenGLContextDevice3D::PushMatrix()
{
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  vtkOpenGLCheckErrorMacro("failed after PushMatrix");
}

void vtkOpenGLContextDevice3D::PopMatrix()
{
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  vtkOpenGLCheckErrorMacro("failed after PopMatrix");
}

void vtkOpenGLContextDevice3D::SetClipping(const vtkRecti &rect)
{
  // Check the bounds, and clamp if necessary
  GLint vp[4] = { this->Storage->Offset.GetX(), this->Storage->Offset.GetY(),
                  this->Storage->Dim.GetX(), this->Storage->Dim.GetY()};

  if (rect.GetX() > 0 && rect.GetX() < vp[2] )
    {
    vp[0] += rect.GetX();
    }
  if (rect.GetY() > 0 && rect.GetY() < vp[3])
    {
    vp[1] += rect.GetY();
    }
  if (rect.GetWidth() > 0 && rect.GetWidth() < vp[2])
    {
    vp[2] = rect.GetWidth();
    }
  if (rect.GetHeight() > 0 && rect.GetHeight() < vp[3])
    {
    vp[3] = rect.GetHeight();
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

void vtkOpenGLContextDevice3D::EnableClippingPlane(int i, double *planeEquation)
{
  GLenum clipPlaneId = static_cast<GLenum>(GL_CLIP_PLANE0+i);
  glEnable(clipPlaneId);
  glClipPlane(clipPlaneId, planeEquation);
  vtkOpenGLCheckErrorMacro("failed after EnableClippingPlane");
}

void vtkOpenGLContextDevice3D::DisableClippingPlane(int i)
{
  GLenum clipPlaneId = static_cast<GLenum>(GL_CLIP_PLANE0+i);
  glDisable(clipPlaneId);
  vtkOpenGLCheckErrorMacro("failed after DisableClippingPlane");
}

void vtkOpenGLContextDevice3D::EnableDepthBuffer()
{
  glEnable(GL_DEPTH_TEST);
}

void vtkOpenGLContextDevice3D::DisableDepthBuffer()
{
  glDisable(GL_DEPTH_TEST);
}

void vtkOpenGLContextDevice3D::PrintSelf(ostream &os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}
