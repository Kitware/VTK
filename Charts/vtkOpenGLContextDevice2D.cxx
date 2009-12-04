/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLContextDevice2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLContextDevice2D.h"

#include "vtkPoints2D.h"
#include "vtkMatrix3x3.h"
#include "vtkFloatArray.h"
#include "vtkSmartPointer.h"

#include "vtkMath.h"
#include "vtkObjectFactory.h"

#include "vtkViewport.h"
#include "vtkWindow.h"

#include "vtkTexture.h"
#include "vtkImageData.h"

#include "vtkRenderer.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLExtensionManager.h"
#include "vtkgl.h"

#ifdef VTK_USE_QT
  #include "vtkQtLabelRenderStrategy.h"
#endif
#include "vtkFreeTypeLabelRenderStrategy.h"

#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
class vtkOpenGLContextDevice2D::Private
{
public:
  Private()
  {
    this->texture = NULL;
    this->lightingEnabled = GL_TRUE;
    this->depthTestEnabled = GL_TRUE;
  }
  ~Private()
  {
    if (this->texture)
      {
      this->texture->Delete();
      this->texture = NULL;
      }
  }

  vtkTexture *texture;
  // Store the previous GL state so that we can restore it when complete
  GLboolean lightingEnabled;
  GLboolean depthTestEnabled;
};

//-----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkOpenGLContextDevice2D, "1.10");
vtkStandardNewMacro(vtkOpenGLContextDevice2D);

//-----------------------------------------------------------------------------
vtkOpenGLContextDevice2D::vtkOpenGLContextDevice2D()
{
  this->Renderer = 0;
  this->IsTextDrawn = false;
#ifdef VTK_USE_QT
  this->TextRenderer = vtkQtLabelRenderStrategy::New();
#else
  this->TextRenderer = vtkFreeTypeLabelRenderStrategy::New();
#endif
  this->Storage = new vtkOpenGLContextDevice2D::Private;
}

//-----------------------------------------------------------------------------
vtkOpenGLContextDevice2D::~vtkOpenGLContextDevice2D()
{
  this->TextRenderer->Delete();
  this->TextRenderer = 0;
  delete this->Storage;
  this->Storage = 0;
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::Begin(vtkViewport* viewport)
{
  int size[2];
  size[0] = viewport->GetSize()[0];
  size[1] = viewport->GetSize()[1];

  // push a 2D matrix on the stack
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho( 0.5, size[0]+0.5,
           0.5, size[1]+0.5,
          -1, 1);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  // Store the previous state before changing it
  this->Storage->lightingEnabled = glIsEnabled(GL_LIGHTING);
  this->Storage->depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);
  glDisable(GL_DEPTH_TEST);

  this->Renderer = vtkRenderer::SafeDownCast(viewport);
  this->TextRenderer->SetRenderer(this->Renderer);
  this->IsTextDrawn = false;

  vtkOpenGLRenderer *gl = vtkOpenGLRenderer::SafeDownCast(viewport);
  if (gl)
    {
    vtkOpenGLRenderWindow *glWin = vtkOpenGLRenderWindow::SafeDownCast(gl->GetRenderWindow());
    if (glWin)
      {
      this->LoadExtensions(glWin->GetExtensionManager());
      }
    }

  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::End()
{
  if (this->IsTextDrawn)
    {
    this->TextRenderer->EndFrame();
    }
  this->TextRenderer->SetRenderer(0);
  // push a 2D matrix on the stack
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  // Restore the GL state that we changed
  if (this->Storage->lightingEnabled)
    {
    glEnable(GL_LIGHTING);
    }
  if (this->Storage->depthTestEnabled)
    {
    glEnable(GL_DEPTH_TEST);
    }

  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::DrawPoly(float *f, int n)
{
  if(f && n > 0)
    {
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, f);
    glDrawArrays(GL_LINE_STRIP, 0, n);
    glDisableClientState(GL_VERTEX_ARRAY);
    }
  else
    {
    vtkWarningMacro(<< "Points supplied that were not of type float.");
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::DrawPoints(float *f, int n)
{
  if (f && n > 0)
    {
    if (this->Storage->texture)
      {
      this->Storage->texture->Render(this->Renderer);
      glEnable(vtkgl::POINT_SPRITE);
      glTexEnvi(vtkgl::POINT_SPRITE, vtkgl::COORD_REPLACE, GL_TRUE);
      vtkgl::PointParameteri(vtkgl::POINT_SPRITE_COORD_ORIGIN, vtkgl::LOWER_LEFT);
      }

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, f);
    glDrawArrays(GL_POINTS, 0, n);
    glDisableClientState(GL_VERTEX_ARRAY);

    if (this->Storage->texture)
      {
      glTexEnvi(vtkgl::POINT_SPRITE, vtkgl::COORD_REPLACE, GL_FALSE);
      glDisable(vtkgl::POINT_SPRITE);
      this->Storage->texture->PostRender(this->Renderer);
      glDisable(GL_TEXTURE_2D);
      }
    }
  else
    {
    vtkWarningMacro(<< "Points supplied that were not of type float.");
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::DrawQuad(float *f, int n)
{
  if (f && n > 0)
    {
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, f);
    glDrawArrays(GL_QUADS, 0, n);
    glDisableClientState(GL_VERTEX_ARRAY);
    }
  else
    {
    vtkWarningMacro(<< "Points supplied that were not of type float.");
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::DrawString(float *point, vtkTextProperty *prop,
                                          const vtkStdString &string)
{
  if (!this->IsTextDrawn)
    {
    this->IsTextDrawn = true;
    this->TextRenderer->StartFrame();
    }

  int p[] = { static_cast<int>(point[0]),
              static_cast<int>(point[1]) };
  this->TextRenderer->RenderLabel(&p[0], prop, string);
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::DrawImage(float *p, int, vtkImageData *image)
{
  vtkTexture *tex =vtkTexture::New();
  tex->SetInput(image);
  tex->Render(this->Renderer);
  int *extent = image->GetExtent();
  float points[] = { p[0]          , p[1],
                     p[0]+extent[1], p[1],
                     p[0]+extent[1], p[1]+extent[3],
                     p[0]          , p[1]+extent[3] };

  float texCoord[] = { 0.0, 0.0,
                       1.0, 0.0,
                       1.0, 1.0,
                       0.0, 1.0 };

  glColor4ub(255, 255, 255, 255);
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glVertexPointer(2, GL_FLOAT, 0, &points[0]);
  glTexCoordPointer(2, GL_FLOAT, 0, &texCoord[0]);
  glDrawArrays(GL_QUADS, 0, 4);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  glDisableClientState(GL_VERTEX_ARRAY);

  tex->PostRender(this->Renderer);
  glDisable(GL_TEXTURE_2D);
  tex->Delete();
}

//-----------------------------------------------------------------------------
unsigned int vtkOpenGLContextDevice2D::AddPointSprite(vtkImageData *image)
{
  this->Storage->texture = vtkTexture::New();
  this->Storage->texture->SetInput(image);
  return 0;
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::SetColor4(unsigned char *color)
{
  glColor4ubv(color);
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::SetColor(unsigned char *color)
{
  glColor3ubv(color);
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::SetPointSize(float size)
{
  glPointSize(size);
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::SetLineWidth(float width)
{
  glLineWidth(width);
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::SetMatrix(vtkMatrix3x3 *m)
{
  // We must construct a 4x4 matrix from the 3x3 matrix for OpenGL
  glLoadIdentity();
  double *M = m->GetData();
  double matrix[16];

  // Convert from row major (C++ two dimensional arrays) to OpenGL
  matrix[0] = M[0];
  matrix[1] = M[3];
  matrix[2] = 0.0;
  matrix[3] = M[6];

  matrix[4] = M[1];
  matrix[5] = M[4];
  matrix[6] = 0.0;
  matrix[7] = M[7];

  matrix[8] = 0.0;
  matrix[9] = 0.0;
  matrix[10] = 1.0;
  matrix[11] = 0.0;

  matrix[12] = M[2];
  matrix[13] = M[5];
  matrix[14] = 0.0;
  matrix[15] = M[8];

  glLoadMatrixd(matrix);
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::PushMatrix()
{
  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::PopMatrix()
{
  glMatrixMode( GL_MODELVIEW );
  glPopMatrix();
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::SetClipping(int *x)
{
  // Test the glScissor function
  vtkDebugMacro(<< "Clipping area: " << x[0] << "\t" << x[1]
                << "\t" << x[2] << "\t" << x[3]);
  glScissor(x[0], x[1], x[2], x[3]);
  glEnable(GL_SCISSOR_TEST);
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::DisableClipping()
{
  glDisable(GL_SCISSOR_TEST);
}

//-----------------------------------------------------------------------------
bool vtkOpenGLContextDevice2D::SetStringRendererToFreeType()
{
#ifdef VTK_USE_QT
  // We will likely be using the Qt rendering strategy
  if (this->TextRenderer->IsA("vtkQtLabelRenderStrategy"))
    {
    this->TextRenderer->Delete();
    this->TextRenderer = vtkFreeTypeLabelRenderStrategy::New();
    }
#endif
  // FreeType is the only choice - nothing to do here
  return true;
}

//-----------------------------------------------------------------------------
bool vtkOpenGLContextDevice2D::SetStringRendererToQt()
{
#ifdef VTK_USE_QT
  // We will likely be using the Qt rendering strategy
  if (this->TextRenderer->IsA("vtkQtLabelRenderStrategy"))
    {
    return true;
    }
  else
    {
    this->TextRenderer->Delete();
    this->TextRenderer = vtkQtLabelRenderStrategy::New();
    }
#endif
  // The Qt based strategy is not available
  return false;
}

//-----------------------------------------------------------------------------
bool vtkOpenGLContextDevice2D::LoadExtensions(vtkOpenGLExtensionManager *m)
{
  if(m->ExtensionSupported("GL_VERSION_1_5"))
    {
    m->LoadExtension("GL_VERSION_1_5");
    return true;
    }
  else
    {
    return false;
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Renderer: ";
  if (this->Renderer)
  {
    os << endl;
    this->Renderer->PrintSelf(os, indent.GetNextIndent());
  }
  else
    {
    os << "(none)" << endl;
    }
  os << indent << "Text Renderer: ";
  if (this->Renderer)
  {
    os << endl;
    this->TextRenderer->PrintSelf(os, indent.GetNextIndent());
  }
  else
    {
    os << "(none)" << endl;
    }
}
