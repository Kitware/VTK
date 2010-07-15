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

#ifdef VTK_USE_QT
# include <QApplication>
# include "vtkQtLabelRenderStrategy.h"
#endif
#include "vtkFreeTypeLabelRenderStrategy.h"

#include "vtkVector.h"
#include "vtkPen.h"
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
#include "vtkShaderProgram2.h"
#include "vtkgl.h"

#include "vtkObjectFactory.h"
#include "vtkContextBufferId.h"
#include "vtkOpenGLContextBufferId.h"

//-----------------------------------------------------------------------------
class vtkOpenGLContextDevice2D::Private
{
public:
  Private()
  {
    this->Texture = NULL;
    this->SavedLighting = GL_TRUE;
    this->SavedDepthTest = GL_TRUE;
    this->SavedAlphaTest = GL_TRUE;
    this->SavedStencilTest = GL_TRUE;
    this->SavedBlend = GL_TRUE;
    this->SavedDrawBuffer = 0;
    this->SavedClearColor[0] = this->SavedClearColor[1] =
                               this->SavedClearColor[2] =
                               this->SavedClearColor[3] = 0.0f;
    this->TextCounter = 0;
    this->GLExtensionsLoaded = false;
    this->OpenGL15 = false;
    this->GLSL = false;
  }

  ~Private()
  {
    if (this->Texture)
      {
      this->Texture->Delete();
      this->Texture = NULL;
      }
  }

  void SaveGLState(bool colorBuffer = false)
  {
    this->SavedLighting = glIsEnabled(GL_LIGHTING);
    this->SavedDepthTest = glIsEnabled(GL_DEPTH_TEST);

    if (colorBuffer)
      {
      this->SavedAlphaTest = glIsEnabled(GL_ALPHA_TEST);
      this->SavedStencilTest = glIsEnabled(GL_STENCIL_TEST);
      this->SavedBlend = glIsEnabled(GL_BLEND);
      glGetFloatv(GL_COLOR_CLEAR_VALUE, this->SavedClearColor);
      glGetIntegerv(GL_DRAW_BUFFER, &this->SavedDrawBuffer);
      }
  }

  void RestoreGLState(bool colorBuffer = false)
  {
    this->SetGLCapability(GL_LIGHTING, this->SavedLighting);
    this->SetGLCapability(GL_DEPTH_TEST, this->SavedDepthTest);

    if (colorBuffer)
      {
      this->SetGLCapability(GL_ALPHA_TEST, this->SavedAlphaTest);
      this->SetGLCapability(GL_STENCIL_TEST, this->SavedStencilTest);
      this->SetGLCapability(GL_BLEND, this->SavedBlend);

      if(this->SavedDrawBuffer != GL_BACK_LEFT)
        {
        glDrawBuffer(this->SavedDrawBuffer);
        }

      int i = 0;
      bool colorDiffer = false;
      while(!colorDiffer && i < 4)
        {
        colorDiffer=this->SavedClearColor[i++] != 0.0;
        }
      if(colorDiffer)
        {
        glClearColor(this->SavedClearColor[0],
                     this->SavedClearColor[1],
                     this->SavedClearColor[2],
                     this->SavedClearColor[3]);
        }
      }
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

  vtkTexture *Texture;
  // Store the previous GL state so that we can restore it when complete
  GLboolean SavedLighting;
  GLboolean SavedDepthTest;
  GLboolean SavedAlphaTest;
  GLboolean SavedStencilTest;
  GLboolean SavedBlend;
  GLint SavedDrawBuffer;
  GLfloat SavedClearColor[4];

  int TextCounter;
  vtkVector2i Dim;
  bool GLExtensionsLoaded;
  bool OpenGL15;
  bool GLSL;
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLContextDevice2D);

//-----------------------------------------------------------------------------
vtkOpenGLContextDevice2D::vtkOpenGLContextDevice2D()
{
  this->Renderer = 0;
  this->IsTextDrawn = false;
  this->InRender = false;
#ifdef VTK_USE_QT
  // Can only use the QtLabelRenderStrategy if there is a QApplication
  // instance, otherwise fallback to the FreeTypeLabelRenderStrategy.
  if(QApplication::instance())
    {
    this->TextRenderer = vtkQtLabelRenderStrategy::New();
    }
  else
    {
    this->TextRenderer = vtkFreeTypeLabelRenderStrategy::New();
    }
#else
  this->TextRenderer = vtkFreeTypeLabelRenderStrategy::New();
#endif
  this->Storage = new vtkOpenGLContextDevice2D::Private;
  this->RenderWindow = NULL;
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
  // Need the actual pixel size of the viewport - ask OpenGL.
  GLint vp[4];
  glGetIntegerv(GL_VIEWPORT, vp);
  this->Storage->Dim.Set(static_cast<int>(vp[2]),
                         static_cast<int>(vp[3]));

  // push a 2D matrix on the stack
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho( 0.5, vp[2]+0.5,
           0.5, vp[3]+0.5,
          -1, 1);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  // Store the previous state before changing it
  this->Storage->SaveGLState();
  glDisable(GL_LIGHTING);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);

  this->Renderer = vtkRenderer::SafeDownCast(viewport);
  this->TextRenderer->SetRenderer(this->Renderer);
  this->IsTextDrawn = false;

  vtkOpenGLRenderer *gl = vtkOpenGLRenderer::SafeDownCast(viewport);
  if (gl)
    {
    this->RenderWindow = vtkOpenGLRenderWindow::SafeDownCast(
        gl->GetRenderWindow());
    }

  if (!this->Storage->GLExtensionsLoaded)
    {
    if (this->RenderWindow)
      {
      this->LoadExtensions(this->RenderWindow->GetExtensionManager());
      }
    }

  this->InRender = true;

  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::End()
{
  if (!this->InRender)
    {
    return;
    }

  if (this->IsTextDrawn)
    {
    this->TextRenderer->EndFrame();
#ifdef VTK_USE_QT
    if (++this->Storage->TextCounter > 300)
      {
      // Delete and recreate the label render strategy, this is a short term
      // fix for a bug observed in ParaView/VTK charts where memory utilization
      // would grow and grow if the chart had a large number of unique strings.
      // The number chosen is fairly arbitrary, and a real fix should be made in
      // the label render strategy.
      if (this->TextRenderer->IsA("vtkQtLabelRenderStrategy"))
        {
        this->TextRenderer->Delete();
        this->TextRenderer = vtkQtLabelRenderStrategy::New();
        this->Storage->TextCounter = 0;
        }
      }
#endif
    this->IsTextDrawn = false;
    }
  this->TextRenderer->SetRenderer(0);
  // push a 2D matrix on the stack
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  // Restore the GL state that we changed
  this->Storage->RestoreGLState();

  this->RenderWindow = NULL;

  this->InRender = false;

  this->Modified();
}

// ----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::BufferIdModeBegin(
  vtkAbstractContextBufferId *bufferId)
{
  assert("pre: not_yet" && !this->GetBufferIdMode());
  assert("pre: bufferId_exists" && bufferId!=0);

  this->BufferId=bufferId;

  // Save OpenGL state.
  this->Storage->SaveGLState(true);

  int lowerLeft[2];
  int usize, vsize;
  this->Renderer->GetTiledSizeAndOrigin(&usize,&vsize,lowerLeft,lowerLeft+1);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho( 0.5, usize+0.5,
           0.5, vsize+0.5,
          -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  glDrawBuffer(GL_BACK_LEFT);
  glClearColor(0.0,0.0,0.0,0.0); // id=0 means no hit, just background
  glClear(GL_COLOR_BUFFER_BIT);
  glDisable(GL_LIGHTING);
  glDisable(GL_ALPHA_TEST);
  glDisable(GL_STENCIL_TEST);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);

  this->TextRenderer->SetRenderer(this->Renderer);
  this->IsTextDrawn = false;

  assert("post: started" && this->GetBufferIdMode());
}

// ----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::BufferIdModeEnd()
{
  assert("pre: started" && this->GetBufferIdMode());

  // Assume the renderer has been set previously during rendering (sse Begin())
  int lowerLeft[2];
  int usize, vsize;
  this->Renderer->GetTiledSizeAndOrigin(&usize,&vsize,lowerLeft,lowerLeft+1);
  this->BufferId->SetValues(lowerLeft[0],lowerLeft[1]);

  // Restore OpenGL state (only if it's different to avoid too much state
  // change).
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  this->TextRenderer->SetRenderer(0);

  this->Storage->RestoreGLState(true);

  this->BufferId=0;
  assert("post: done" && !this->GetBufferIdMode());
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
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, f);
    glDrawArrays(GL_POINTS, 0, n);
    glDisableClientState(GL_VERTEX_ARRAY);
    }
  else
    {
    vtkWarningMacro(<< "Points supplied that were not of type float.");
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::DrawPointSprites(vtkImageData *sprite,
                                                float *points, int n)
{
  if (points && n > 0)
    {
    if (sprite)
      {
      if (!this->Storage->Texture)
        {
        this->Storage->Texture = vtkTexture::New();
        this->Storage->Texture->SetRepeat(false);
        }
      this->Storage->Texture->SetInput(sprite);
      this->Storage->Texture->Render(this->Renderer);
      }
    if (this->Storage->OpenGL15)
      {
      // We can actually use point sprites here
      glEnable(vtkgl::POINT_SPRITE);
      glTexEnvi(vtkgl::POINT_SPRITE, vtkgl::COORD_REPLACE, GL_TRUE);
      vtkgl::PointParameteri(vtkgl::POINT_SPRITE_COORD_ORIGIN, vtkgl::LOWER_LEFT);

      this->DrawPoints(points, n);

      glTexEnvi(vtkgl::POINT_SPRITE, vtkgl::COORD_REPLACE, GL_FALSE);
      glDisable(vtkgl::POINT_SPRITE);

      }
    else
      {
      // Must emulate the point sprites - slower but at least they see something.
      GLfloat width = 1.0;
      glGetFloatv(GL_POINT_SIZE, &width);
      width /= 2.0;

      // Need to get the model view matrix for scaling factors...
      GLfloat mv[16];
      glGetFloatv(GL_MODELVIEW_MATRIX, mv);
      float xWidth = width / mv[0];
      float yWidth = width / mv[5];

      float p[8] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

      // This will be the same everytime
      float texCoord[] = { 0.0, 0.0,
                           1.0, 0.0,
                           1.0, 1.0,
                           0.0, 1.0 };

      glEnableClientState(GL_VERTEX_ARRAY);
      glEnableClientState(GL_TEXTURE_COORD_ARRAY);
      glTexCoordPointer(2, GL_FLOAT, 0, texCoord);

      for (int i = 0; i < n; ++i)
        {
        p[0] = points[2*i] - xWidth;
        p[1] = points[2*i+1] - yWidth;
        p[2] = points[2*i] + xWidth;
        p[3] = points[2*i+1] - yWidth;
        p[4] = points[2*i] + xWidth;
        p[5] = points[2*i+1] + yWidth;
        p[6] = points[2*i] - xWidth;
        p[7] = points[2*i+1] + yWidth;

        glVertexPointer(2, GL_FLOAT, 0, p);
        glDrawArrays(GL_QUADS, 0, 4);
        }
      glDisableClientState(GL_TEXTURE_COORD_ARRAY);
      glDisableClientState(GL_VERTEX_ARRAY);
      }
    if (sprite)
      {
      this->Storage->Texture->PostRender(this->Renderer);
      glDisable(GL_TEXTURE_2D);
      }
    }
  else
    {
    vtkWarningMacro(<< "Points supplied without a valid image or pointer.");
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
void vtkOpenGLContextDevice2D::DrawEllipseWedge(float x, float y, float outRx,
                                                float outRy, float inRx,
                                                float inRy, float startAngle,
                                                float stopAngle)

{
  assert("pre: positive_outRx" && outRx>=0.0f);
  assert("pre: positive_outRy" && outRy>=0.0f);
  assert("pre: positive_inRx" && inRx>=0.0f);
  assert("pre: positive_inRy" && inRy>=0.0f);
  assert("pre: ordered_rx" && inRx<=outRx);
  assert("pre: ordered_ry" && inRy<=outRy);

  if(outRy==0.0f && outRx==0.0f)
    {
    // we make sure maxRadius will never be null.
    return;
    }

  int iterations=this->GetNumberOfArcIterations(outRx,outRy,startAngle,
                                                stopAngle);

  float *p=new float[4*(iterations+1)];

  // step in radians.
  double step =
    vtkMath::RadiansFromDegrees(stopAngle-startAngle)/(iterations);

  // step have to be lesser or equal to maxStep computed inside
  // GetNumberOfIterations()

  double rstart=vtkMath::RadiansFromDegrees(startAngle);

  // the A vertices (0,2,4,..) are on the inner side
  // the B vertices (1,3,5,..) are on the outer side
  // (A and B vertices terms come from triangle strip definition in
  // OpenGL spec)
  // we are iterating counterclockwise

  int i=0;
  while(i<=iterations)
    {
    // A vertex (inner side)
    double a=rstart+i*step;
    p[4*i  ] = inRx * cos(a) + x;
    p[4*i+1] = inRy * sin(a) + y;

    // B vertex (outer side)
    p[4*i+2] = outRx * cos(a) + x;
    p[4*i+3] = outRy * sin(a) + y;

    ++i;
    }

  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(2, GL_FLOAT, 0, p);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 2*(iterations+1));
  glDisableClientState(GL_VERTEX_ARRAY);

  delete[] p;
}

// ----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::DrawEllipticArc(float x, float y, float rX,
                                               float rY, float startAngle,
                                               float stopAngle)
{
  assert("pre: positive_rX" && rX>=0);
  assert("pre: positive_rY" && rY>=0);

  if(rX==0.0f && rY==0.0f)
    {
    // we make sure maxRadius will never be null.
    return;
    }
  int iterations=this->GetNumberOfArcIterations(rX,rY,startAngle,stopAngle);

  float *p=new float[2*(iterations+1)];

  // step in radians.
  double step =
    vtkMath::RadiansFromDegrees(stopAngle-startAngle)/(iterations);

  // step have to be lesser or equal to maxStep computed inside
  // GetNumberOfIterations()

  double rstart=vtkMath::RadiansFromDegrees(startAngle);

  // we are iterating counterclockwise
  int i=0;
  while(i<=iterations)
    {
    double a=rstart+i*step;
    p[2*i  ] = rX * cos(a) + x;
    p[2*i+1] = rY * sin(a) + y;
    ++i;
    }

  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(2, GL_FLOAT, 0, p);
  glDrawArrays(GL_LINE_STRIP, 0, iterations+1);
  glDisableClientState(GL_VERTEX_ARRAY);

  delete[] p;
}

// ----------------------------------------------------------------------------
int vtkOpenGLContextDevice2D::GetNumberOfArcIterations(float rX,
                                                       float rY,
                                                       float startAngle,
                                                       float stopAngle)
{
  assert("pre: positive_rX" && rX>=0.0f);
  assert("pre: positive_rY" && rY>=0.0f);
  assert("pre: not_both_null" && (rX>0.0 || rY>0.0));

// 1.0: pixel precision. 0.5 (subpixel precision, useful with multisampling)
  double error=4.0; // experience shows 4.0 is visually enough.

  // The tessellation is the most visible on the biggest radius.
  double maxRadius;
  if(rX>=rY)
    {
    maxRadius=rX;
    }
  else
    {
    maxRadius=rY;
    }

  if(error>maxRadius)
    {
    error=0.5; // to make sure the argument of asin() is in a valid range.
    }

  // Angle of a sector so that its chord is `error' pixels.
  // This is will be our maximum angle step.
  double maxStep=2.0*asin(error/(2.0*maxRadius));

  // ceil because we want to make sure we don't underestimate the number of
  // iterations by 1.
  return static_cast<int>(
    ceil(vtkMath::RadiansFromDegrees(stopAngle-startAngle)/maxStep));
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
void vtkOpenGLContextDevice2D::ComputeStringBounds(const vtkStdString &string,
                                                   vtkTextProperty *prop,
                                                   float bounds[4])
{
  double b[4];
  this->TextRenderer->ComputeLabelBounds(prop, string, b);

  // Go from the format used in the label render strategy (x1, x2, y1, y2)
  // to the format specified by this function (x, y, w, h).
  bounds[0] = static_cast<float>(b[0]);
  bounds[1] = static_cast<float>(b[2]);
  bounds[2] = static_cast<float>(b[1] - b[0]);
  bounds[3] = static_cast<float>(b[3] - b[2]);
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
void vtkOpenGLContextDevice2D::SetLineType(int type)
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

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::MultiplyMatrix(vtkMatrix3x3 *m)
{
  // We must construct a 4x4 matrix from the 3x3 matrix for OpenGL
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

  glMultMatrixd(matrix);
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::SetMatrix(vtkMatrix3x3 *m)
{
  // We must construct a 4x4 matrix from the 3x3 matrix for OpenGL
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
void vtkOpenGLContextDevice2D::SetClipping(int *dim)
{
  // Check the bounds, and clamp if necessary
  int vp[4] = {0, 0, this->Storage->Dim.X(), this->Storage->Dim.Y()};
  if (dim[0] > 0 && dim[0] < this->Storage->Dim.X())
    {
    vp[0] = dim[0];
    }
  if (dim[1] > 0 && dim[1] < this->Storage->Dim.Y())
    {
    vp[1] = dim[1];
    }
  if (dim[2] > 0 && dim[2] < this->Storage->Dim.X())
    {
    vp[2] = dim[2];
    }
  if (dim[3] > 0 && dim[3] < this->Storage->Dim.Y())
    {
    vp[3] = dim[3];
    }

  glScissor(vp[0], vp[1], vp[2], vp[3]);
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

//----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::ReleaseGraphicsResources(vtkWindow *window)
{
  this->TextRenderer->ReleaseGraphicsResources(window);
  if (this->Storage->Texture)
    {
    this->Storage->Texture->ReleaseGraphicsResources(window);
    }
}

//----------------------------------------------------------------------------
bool vtkOpenGLContextDevice2D::HasGLSL()
{
  return this->Storage->GLSL;
}

//-----------------------------------------------------------------------------
bool vtkOpenGLContextDevice2D::LoadExtensions(vtkOpenGLExtensionManager *m)
{
  if(m->ExtensionSupported("GL_VERSION_1_5"))
    {
    m->LoadExtension("GL_VERSION_1_5");
    this->Storage->OpenGL15 = true;
    }
  else
    {
    this->Storage->OpenGL15 = false;
    }
  if(vtkShaderProgram2::IsSupported(
      static_cast<vtkOpenGLRenderWindow *>(m->GetRenderWindow())))
    {
    this->Storage->GLSL = true;
    }
  else
    {
    this->Storage->GLSL = false;
    }

  this->Storage->GLExtensionsLoaded = true;
  return true;
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
