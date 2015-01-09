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

#include "vtkMathTextUtilities.h"
#include "vtkTextRendererStringToImage.h"

#include "vtkVector.h"
#include "vtkRect.h"
#include "vtkPen.h"
#include "vtkBrush.h"
#include "vtkTextProperty.h"
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
#include "vtkOpenGLError.h"

#include "vtkObjectFactory.h"
#include "vtkOpenGLContextBufferId.h"

#include "vtkOpenGLContextDevice2DPrivate.h"

#include <algorithm>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLContextDevice2D)

//-----------------------------------------------------------------------------
vtkOpenGLContextDevice2D::vtkOpenGLContextDevice2D()
{
  this->Renderer = 0;
  this->InRender = false;
  this->TextRenderer = vtkTextRendererStringToImage::New();
  this->Storage = new vtkOpenGLContextDevice2D::Private;
  this->RenderWindow = NULL;
  this->MaximumMarkerCacheSize = 20;
}

//-----------------------------------------------------------------------------
vtkOpenGLContextDevice2D::~vtkOpenGLContextDevice2D()
{
  while (!this->MarkerCache.empty())
    {
    this->MarkerCache.back().Value->Delete();
    this->MarkerCache.pop_back();
    }

  this->TextRenderer->Delete();
  delete this->Storage;
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::Begin(vtkViewport* viewport)
{
  vtkOpenGLClearErrorMacro();
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
          -2000, 2000);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  // Store the previous state before changing it
  this->Storage->SaveGLState();
  glDisable(GL_LIGHTING);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);

  this->Renderer = vtkRenderer::SafeDownCast(viewport);

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

  // Enable simple line, point and polygon antialiasing if multisampling is on.
  if (this->Renderer->GetRenderWindow()->GetMultiSamples())
    {
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_POLYGON_SMOOTH);
    }

  // Make sure we are on the default texture setting
  glBindTexture(GL_TEXTURE_2D, 0);

  this->InRender = true;
  vtkOpenGLCheckErrorMacro("failed after Begin");
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::End()
{
  if (!this->InRender)
    {
    return;
    }

  vtkOpenGLClearErrorMacro();

  // push a 2D matrix on the stack
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  // Restore the GL state that we changed
  this->Storage->RestoreGLState();

  // Disable simple line, point and polygon antialiasing if multisampling is on.
  if (this->Renderer->GetRenderWindow()->GetMultiSamples())
    {
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_POINT_SMOOTH);
    glDisable(GL_POLYGON_SMOOTH);
    }

  this->RenderWindow = NULL;
  this->InRender = false;

  vtkOpenGLCheckErrorMacro("failed after End");
}

// ----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::BufferIdModeBegin(
  vtkAbstractContextBufferId *bufferId)
{
  assert("pre: not_yet" && !this->GetBufferIdMode());
  assert("pre: bufferId_exists" && bufferId!=0);

  vtkOpenGLClearErrorMacro();

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

  vtkOpenGLCheckErrorMacro("failed after BufferIdModeBegin");

  assert("post: started" && this->GetBufferIdMode());
}

// ----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::BufferIdModeEnd()
{
  assert("pre: started" && this->GetBufferIdMode());

  vtkOpenGLClearErrorMacro();

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

  this->Storage->RestoreGLState(true);

  this->BufferId=0;

  vtkOpenGLCheckErrorMacro("failed after BufferIdModeEnd");

  assert("post: done" && !this->GetBufferIdMode());
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::DrawPoly(float *f, int n, unsigned char *colors,
                                        int nc)
{
  assert("f must be non-null" && f != NULL);
  assert("n must be greater than 0" && n > 0);

  vtkOpenGLClearErrorMacro();

  this->SetLineType(this->Pen->GetLineType());
  this->SetLineWidth(this->Pen->GetWidth());

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
  glVertexPointer(2, GL_FLOAT, 0, f);
  glDrawArrays(GL_LINE_STRIP, 0, n);
  glDisableClientState(GL_VERTEX_ARRAY);
  if (colors)
    {
    glDisableClientState(GL_COLOR_ARRAY);
    }

  // Restore line type and width.
  this->SetLineType(vtkPen::SOLID_LINE);
  this->SetLineWidth(1.0f);

  vtkOpenGLCheckErrorMacro("failed after DrawPoly");
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::DrawPoints(float *f, int n, unsigned char *c,
                                          int nc)
{
  vtkOpenGLClearErrorMacro();

  if (f && n > 0)
    {
    this->SetPointSize(this->Pen->GetWidth());
    glEnableClientState(GL_VERTEX_ARRAY);
    if (c && nc)
      {
      glEnableClientState(GL_COLOR_ARRAY);
      glColorPointer(nc, GL_UNSIGNED_BYTE, 0, c);
      }
    else
      {
      glColor4ubv(this->Pen->GetColor());
      }
    glVertexPointer(2, GL_FLOAT, 0, f);
    glDrawArrays(GL_POINTS, 0, n);
    glDisableClientState(GL_VERTEX_ARRAY);
    if (c && nc)
      {
      glDisableClientState(GL_COLOR_ARRAY);
      }
    }
  else
    {
    vtkWarningMacro(<< "Points supplied that were not of type float.");
    }

  vtkOpenGLCheckErrorMacro("failed after DrawPoints");
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::DrawPointSprites(vtkImageData *sprite,
                                                float *points, int n,
                                                unsigned char *colors,
                                                int nc_comps)
{
  vtkOpenGLClearErrorMacro();
  if (points && n > 0)
    {
    this->SetPointSize(this->Pen->GetWidth());
    if (sprite)
      {
      if (!this->Storage->SpriteTexture)
        {
        this->Storage->SpriteTexture = vtkTexture::New();
        }
      int properties = this->Brush->GetTextureProperties();
      this->Storage->SpriteTexture->SetInputData(sprite);
      this->Storage->SpriteTexture->SetRepeat(properties & vtkContextDevice2D::Repeat);
      this->Storage->SpriteTexture->SetInterpolate(properties & vtkContextDevice2D::Linear);
      glEnable(GL_TEXTURE_2D);
      this->Storage->SpriteTexture->Render(this->Renderer);
      }

    // Must emulate the point sprites - slower but at least they see something.
    GLfloat width = 1.0;
    glGetFloatv(GL_POINT_SIZE, &width);
    width /= 2.0;

    // Need to get the model view matrix for scaling factors...
    GLfloat mv[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, mv);
    float xWidth = width / mv[0];
    float yWidth = width / mv[5];

    // Four 2D points on the quad.
    float p[4 * 2] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

    // This will be the same every time
    float texCoord[] = { 0.0, 0.0,
                         1.0, 0.0,
                         1.0, 1.0,
                         0.0, 1.0 };

    if (!colors || !nc_comps)
      {
      glColor4ubv(this->Pen->GetColor());
      }
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

      // If we have a color array, set the color for each quad.
      if (colors && nc_comps)
        {
        if (nc_comps == 3)
          {
          glColor3ubv(&colors[3 * i]);
          }
        else if (nc_comps == 4)
          {
          glColor4ubv(&colors[4 * i]);
          }
        }

      glVertexPointer(2, GL_FLOAT, 0, p);
      glDrawArrays(GL_QUADS, 0, 4);
      }
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

    if (sprite)
      {
      this->Storage->SpriteTexture->PostRender(this->Renderer);
      glDisable(GL_TEXTURE_2D);
      }
    }
  else
    {
    vtkWarningMacro(<< "Points supplied without a valid image or pointer.");
    }
  vtkOpenGLCheckErrorMacro("failed after DrawPointSprites");
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::DrawMarkers(int shape, bool highlight,
                                           float *points, int n,
                                           unsigned char *colors, int nc_comps)
{
  // Get a point sprite for the shape
  vtkImageData *sprite = this->GetMarker(shape, static_cast<int>(this->Pen->GetWidth()),
                                         highlight);
  this->DrawPointSprites(sprite, points, n, colors, nc_comps);
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::DrawQuad(float *f, int n)
{
  vtkOpenGLClearErrorMacro();
  if (!f || n <= 0)
    {
    vtkWarningMacro(<< "Points supplied that were not of type float.");
    return;
    }
  glColor4ubv(this->Brush->GetColor());
  float* texCoord = 0;
  if (this->Brush->GetTexture())
    {
    this->SetTexture(this->Brush->GetTexture(),
                     this->Brush->GetTextureProperties());
    glEnable(GL_TEXTURE_2D);
    this->Storage->Texture->Render(this->Renderer);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    texCoord = this->Storage->TexCoords(f, n);
    glTexCoordPointer(2, GL_FLOAT, 0, &texCoord[0]);
    }
  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(2, GL_FLOAT, 0, f);
  glDrawArrays(GL_QUADS, 0, n);
  glDisableClientState(GL_VERTEX_ARRAY);
  if (this->Storage->Texture)
    {
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    this->Storage->Texture->PostRender(this->Renderer);
    glDisable(GL_TEXTURE_2D);
    delete [] texCoord;
    }
  vtkOpenGLCheckErrorMacro("failed after DrawQuad");
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::DrawQuadStrip(float *f, int n)
{
  vtkOpenGLClearErrorMacro();
  if (!f || n <= 0)
    {
    vtkWarningMacro(<< "Points supplied that were not of type float.");
    return;
    }
  glColor4ubv(this->Brush->GetColor());
  float* texCoord = 0;
  if (this->Brush->GetTexture())
    {
    this->SetTexture(this->Brush->GetTexture(),
                     this->Brush->GetTextureProperties());
    glEnable(GL_TEXTURE_2D);
    this->Storage->Texture->Render(this->Renderer);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    texCoord = this->Storage->TexCoords(f, n);
    glTexCoordPointer(2, GL_FLOAT, 0, &texCoord[0]);
    }
  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(2, GL_FLOAT, 0, f);
  glDrawArrays(GL_QUAD_STRIP, 0, n);
  glDisableClientState(GL_VERTEX_ARRAY);
  if (this->Storage->Texture)
    {
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    this->Storage->Texture->PostRender(this->Renderer);
    glDisable(GL_TEXTURE_2D);
    delete [] texCoord;
    }
  vtkOpenGLCheckErrorMacro("failed after DrawQuadStrip");
}
//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::DrawPolygon(float *f, int n)
{
  vtkOpenGLClearErrorMacro();
  if (!f || n <= 0)
    {
    vtkWarningMacro(<< "Points supplied that were not of type float.");
    return;
    }
  glColor4ubv(this->Brush->GetColor());
  float* texCoord = 0;
  if (this->Brush->GetTexture())
    {
    this->SetTexture(this->Brush->GetTexture(),
                     this->Brush->GetTextureProperties());
    glEnable(GL_TEXTURE_2D);
    this->Storage->Texture->Render(this->Renderer);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    texCoord = this->Storage->TexCoords(f, n);
    glTexCoordPointer(2, GL_FLOAT, 0, &texCoord[0]);
    }
  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(2, GL_FLOAT, 0, f);
  glDrawArrays(GL_POLYGON, 0, n);
  glDisableClientState(GL_VERTEX_ARRAY);
  if (this->Storage->Texture)
    {
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    this->Storage->Texture->PostRender(this->Renderer);
    glDisable(GL_TEXTURE_2D);
    delete [] texCoord;
    }
  vtkOpenGLCheckErrorMacro("failed after DrawPolygon");
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

  vtkOpenGLClearErrorMacro();

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

  glColor4ubv(this->Brush->GetColor());
  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(2, GL_FLOAT, 0, p);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 2*(iterations+1));
  glDisableClientState(GL_VERTEX_ARRAY);

  delete[] p;

  vtkOpenGLCheckErrorMacro("failed after DrawEllipseWedge");
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

  vtkOpenGLClearErrorMacro();

  int iterations = this->GetNumberOfArcIterations(rX, rY, startAngle, stopAngle);

  float *p = new float[2*(iterations+1)];

  // step in radians.
  double step =
    vtkMath::RadiansFromDegrees(stopAngle-startAngle)/(iterations);

  // step have to be lesser or equal to maxStep computed inside
  // GetNumberOfIterations()
  double rstart=vtkMath::RadiansFromDegrees(startAngle);

  // we are iterating counterclockwise
  for(int i = 0; i <= iterations; ++i)
    {
    double a=rstart+i*step;
    p[2*i  ] = rX * cos(a) + x;
    p[2*i+1] = rY * sin(a) + y;
    }

  this->SetLineType(this->Pen->GetLineType());
  this->SetLineWidth(this->Pen->GetWidth());
  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(2, GL_FLOAT, 0, p);
  glColor4ubv(this->Brush->GetColor());
  glDrawArrays(GL_TRIANGLE_FAN, 0, iterations+1);
  glColor4ubv(this->Pen->GetColor());
  glDrawArrays(GL_LINE_STRIP, 0, iterations+1);
  glDisableClientState(GL_VERTEX_ARRAY);
  // Restore line type and width.
  this->SetLineType(vtkPen::SOLID_LINE);
  this->SetLineWidth(1.0f);

  delete[] p;

  vtkOpenGLCheckErrorMacro("failed after DrawEllipseArc");
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
  double error = 4.0; // experience shows 4.0 is visually enough.

  // The tessellation is the most visible on the biggest radius.
  double maxRadius;
  if(rX >= rY)
    {
    maxRadius = rX;
    }
  else
    {
    maxRadius = rY;
    }

  if(error > maxRadius)
    {
    // to make sure the argument of asin() is in a valid range.
    error = maxRadius;
    }

  // Angle of a sector so that its chord is `error' pixels.
  // This is will be our maximum angle step.
  double maxStep = 2.0 * asin(error / (2.0 * maxRadius));

  // ceil because we want to make sure we don't underestimate the number of
  // iterations by 1.
  return static_cast<int>(
    ceil(vtkMath::RadiansFromDegrees(stopAngle - startAngle) / maxStep));
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::AlignText(double orientation, float width,
                                         float height, float *p)
{
  // Special case multiples of 90 as no transformation is required...
  if (orientation > -0.0001 && orientation < 0.0001)
    {
    switch (this->TextProp->GetJustification())
      {
      case VTK_TEXT_LEFT:
        break;
      case VTK_TEXT_CENTERED:
        p[0] -= floor(width / 2.0);
        break;
      case VTK_TEXT_RIGHT:
        p[0] -= width;
        break;
      }
    switch (this->TextProp->GetVerticalJustification())
      {
      case VTK_TEXT_BOTTOM:
        break;
      case VTK_TEXT_CENTERED:
        p[1] -= floor(height / 2.0);
        break;
      case VTK_TEXT_TOP:
        p[1] -= height;
        break;
      }
    }
  else if (orientation > 89.9999 && orientation < 90.0001)
    {
    switch (this->TextProp->GetJustification())
      {
      case VTK_TEXT_LEFT:
        break;
      case VTK_TEXT_CENTERED:
        p[1] -= floor(height / 2.0);
        break;
      case VTK_TEXT_RIGHT:
        p[1] -= height;
        break;
      }
    switch (this->TextProp->GetVerticalJustification())
      {
      case VTK_TEXT_TOP:
        break;
      case VTK_TEXT_CENTERED:
        p[0] -= floor(width / 2.0);
        break;
      case VTK_TEXT_BOTTOM:
        p[0] -= width;
        break;
      }
    }
  else if (orientation > 179.9999 && orientation < 180.0001)
    {
    switch (this->TextProp->GetJustification())
      {
      case VTK_TEXT_RIGHT:
        break;
      case VTK_TEXT_CENTERED:
        p[0] -= floor(width / 2.0);
        break;
      case VTK_TEXT_LEFT:
        p[0] -= width;
        break;
      }
    switch (this->TextProp->GetVerticalJustification())
      {
      case VTK_TEXT_TOP:
        break;
      case VTK_TEXT_CENTERED:
        p[1] -= floor(height / 2.0);
        break;
      case VTK_TEXT_BOTTOM:
        p[1] -= height;
        break;
      }
    }
  else if (orientation > 269.9999 && orientation < 270.0001)
    {
    switch (this->TextProp->GetJustification())
      {
      case VTK_TEXT_LEFT:
        break;
      case VTK_TEXT_CENTERED:
        p[1] -= floor(height / 2.0);
        break;
      case VTK_TEXT_RIGHT:
        p[1] -= height;
        break;
      }
    switch (this->TextProp->GetVerticalJustification())
      {
      case VTK_TEXT_BOTTOM:
        break;
      case VTK_TEXT_CENTERED:
        p[0] -= floor(width / 2.0);
        break;
      case VTK_TEXT_TOP:
        p[0] -= width;
        break;
      }
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::DrawString(float *point,
                                          const vtkStdString &string)
{
  this->DrawString(point, vtkUnicodeString::from_utf8(string));
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::ComputeStringBounds(const vtkStdString &string,
                                                   float bounds[4])
{
  this->ComputeStringBounds(vtkUnicodeString::from_utf8(string), bounds);
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::DrawString(float *point,
                                          const vtkUnicodeString &string)
{
  vtkOpenGLClearErrorMacro();

  GLfloat mv[16];
  glGetFloatv(GL_MODELVIEW_MATRIX, mv);
  float xScale = mv[0];
  float yScale = mv[5];

  float p[] = { std::floor(point[0] * xScale) / xScale,
                std::floor(point[1] * yScale) / yScale };

  // Cache rendered text strings
  vtkTextureImageCache<UTF16TextPropertyKey>::CacheData &cache =
      this->Storage->TextTextureCache.GetCacheData(
        UTF16TextPropertyKey(this->TextProp, string));
  vtkImageData* image = cache.ImageData;
  if (image->GetNumberOfPoints() == 0 && image->GetNumberOfCells() == 0)
    {
    int textDims[2];
    if (!this->TextRenderer->RenderString(this->TextProp, string, image,
                                          textDims))
      {
      return;
      }
    cache.TextWidth = textDims[0];
    cache.TextHeight = textDims[1];
    }
  vtkTexture* texture = cache.Texture;
  glEnable(GL_TEXTURE_2D);
  texture->Render(this->Renderer);

  int imgDims[3];
  image->GetDimensions(imgDims);

  float width = cache.TextWidth / xScale;
  float height = cache.TextHeight / yScale;

  float xw = cache.TextWidth / static_cast<float>(imgDims[0]);
  float xh = cache.TextHeight / static_cast<float>(imgDims[1]);

  this->AlignText(this->TextProp->GetOrientation(), width, height, p);

  float points[] = { p[0]        , p[1],
                     p[0] + width, p[1],
                     p[0] + width, p[1] + height,
                     p[0]        , p[1] + height };

  float texCoord[] = { 0.0f, 0.0f,
                       xw,   0.0f,
                       xw,   xh,
                       0.0f, xh };

  glColor4ub(255, 255, 255, 255);
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glVertexPointer(2, GL_FLOAT, 0, points);
  glTexCoordPointer(2, GL_FLOAT, 0, texCoord);
  glDrawArrays(GL_QUADS, 0, 4);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  glDisableClientState(GL_VERTEX_ARRAY);

  texture->PostRender(this->Renderer);
  glDisable(GL_TEXTURE_2D);

  vtkOpenGLCheckErrorMacro("failed after DrawString");
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::ComputeStringBounds(const vtkUnicodeString &string,
                                                   float bounds[4])
{
  vtkVector2i box = this->TextRenderer->GetBounds(this->TextProp, string);
  // Check for invalid bounding box
  if (box[0] == VTK_INT_MIN || box[0] == VTK_INT_MAX ||
      box[1] == VTK_INT_MIN || box[1] == VTK_INT_MAX)
    {
    bounds[0] = static_cast<float>(0);
    bounds[1] = static_cast<float>(0);
    bounds[2] = static_cast<float>(0);
    bounds[3] = static_cast<float>(0);
    return;
    }
  GLfloat mv[16];
  glGetFloatv(GL_MODELVIEW_MATRIX, mv);
  float xScale = mv[0];
  float yScale = mv[5];
  bounds[0] = static_cast<float>(0);
  bounds[1] = static_cast<float>(0);
  bounds[2] = static_cast<float>(box.GetX() / xScale);
  bounds[3] = static_cast<float>(box.GetY() / yScale);
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::DrawMathTextString(float point[2],
                                                  const vtkStdString &string)
{
  vtkMathTextUtilities *mathText = vtkMathTextUtilities::GetInstance();
  if (!mathText)
    {
    vtkWarningMacro(<<"MathText is not available to parse string "
                    << string.c_str() << ". Install matplotlib and enable "
                    "python to use MathText.");
    return;
    }

  vtkOpenGLClearErrorMacro();

  float p[] = { std::floor(point[0]), std::floor(point[1]) };

  // Cache rendered text strings
  vtkTextureImageCache<UTF8TextPropertyKey>::CacheData &cache =
    this->Storage->MathTextTextureCache.GetCacheData(
      UTF8TextPropertyKey(this->TextProp, string));
  vtkImageData* image = cache.ImageData;
  if (image->GetNumberOfPoints() == 0 && image->GetNumberOfCells() == 0)
    {
    int textDims[2];
    if (!mathText->RenderString(string.c_str(), image, this->TextProp,
                                this->RenderWindow->GetDPI(), textDims))
      {
      return;
      }
    cache.TextWidth = textDims[0];
    cache.TextHeight = textDims[1];
    }

  vtkTexture* texture = cache.Texture;
  glEnable(GL_TEXTURE_2D);
  texture->Render(this->Renderer);

  GLfloat mv[16];
  glGetFloatv(GL_MODELVIEW_MATRIX, mv);
  float xScale = mv[0];
  float yScale = mv[5];

  int imgDims[3];
  image->GetDimensions(imgDims);

  float width = cache.TextWidth / xScale;
  float height = cache.TextHeight / yScale;

  float xw = cache.TextWidth / static_cast<float>(imgDims[0]);
  float xh = cache.TextHeight / static_cast<float>(imgDims[1]);

  this->AlignText(this->TextProp->GetOrientation(), width, height, p);

  float points[] = { p[0]        , p[1],
                     p[0] + width, p[1],
                     p[0] + width, p[1] + height,
                     p[0]        , p[1] + height };

  float texCoord[] = { 0.0f, 0.0f,
                       xw,   0.0f,
                       xw,   xh,
                       0.0f, xh };

  glColor4ub(255, 255, 255, 255);
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glVertexPointer(2, GL_FLOAT, 0, points);
  glTexCoordPointer(2, GL_FLOAT, 0, texCoord);
  glDrawArrays(GL_QUADS, 0, 4);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  glDisableClientState(GL_VERTEX_ARRAY);

  texture->PostRender(this->Renderer);
  glDisable(GL_TEXTURE_2D);

  vtkOpenGLCheckErrorMacro("failed after DrawMathTexString");
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::DrawImage(float p[2], float scale,
                                         vtkImageData *image)
{
  vtkOpenGLClearErrorMacro();

  this->SetTexture(image);
  glEnable(GL_TEXTURE_2D);
  this->Storage->Texture->Render(this->Renderer);
  int *extent = image->GetExtent();
  float points[] = { p[0]                     , p[1],
                     p[0]+scale*extent[1]+1.0f, p[1],
                     p[0]+scale*extent[1]+1.0f, p[1]+scale*extent[3]+1.0f,
                     p[0]                     , p[1]+scale*extent[3]+1.0f };

  float texCoord[] = { 0.0f, 0.0f,
                       1.0f, 0.0f,
                       1.0f, 1.0f,
                       0.0f, 1.0f };

  glColor4ub(255, 255, 255, 255);
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glVertexPointer(2, GL_FLOAT, 0, &points[0]);
  glTexCoordPointer(2, GL_FLOAT, 0, &texCoord[0]);
  glDrawArrays(GL_QUADS, 0, 4);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  glDisableClientState(GL_VERTEX_ARRAY);

  this->Storage->Texture->PostRender(this->Renderer);
  glDisable(GL_TEXTURE_2D);

  vtkOpenGLCheckErrorMacro("failed after DrawImage");
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::DrawImage(const vtkRectf& pos,
                                         vtkImageData *image)
{
  vtkOpenGLClearErrorMacro();

  vtkVector2f tex(1.0, 1.0);
  glEnable(GL_TEXTURE_2D);
  GLuint index = 0;
  if (this->Storage->PowerOfTwoTextures)
    {
    index = this->Storage->TextureFromImage(image, tex);
    }
  else
    {
    index = this->Storage->TextureFromImage(image, tex);
    }
//  this->SetTexture(image);
//  this->Storage->Texture->Render(this->Renderer);
  float points[] = { pos.GetX()              , pos.GetY(),
                     pos.GetX() + pos.GetWidth(), pos.GetY(),
                     pos.GetX() + pos.GetWidth(), pos.GetY() + pos.GetHeight(),
                     pos.GetX()              , pos.GetY() + pos.GetHeight() };

  float texCoord[] = { 0.0   , 0.0,
                       tex[0], 0.0,
                       tex[0], tex[1],
                       0.0   , tex[1] };

  glColor4ub(255, 255, 255, 255);
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glVertexPointer(2, GL_FLOAT, 0, &points[0]);
  glTexCoordPointer(2, GL_FLOAT, 0, &texCoord[0]);
  glDrawArrays(GL_QUADS, 0, 4);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  glDisableClientState(GL_VERTEX_ARRAY);

//  this->Storage->Texture->PostRender(this->Renderer);
  glDisable(GL_TEXTURE_2D);
  glDeleteTextures(1, &index);

  vtkOpenGLCheckErrorMacro("failed after DrawImage");
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
void vtkOpenGLContextDevice2D::SetTexture(vtkImageData* image, int properties)
{
  if (image == NULL)
    {
    if (this->Storage->Texture)
      {
      this->Storage->Texture->Delete();
      this->Storage->Texture = 0;
      }
    return;
    }
  if (this->Storage->Texture == NULL)
    {
    this->Storage->Texture = vtkTexture::New();
    }
  this->Storage->Texture->SetInputData(image);
  this->Storage->TextureProperties = properties;
  this->Storage->Texture->SetRepeat(properties & vtkContextDevice2D::Repeat);
  this->Storage->Texture->SetInterpolate(properties & vtkContextDevice2D::Linear);
  this->Storage->Texture->EdgeClampOn();
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
void vtkOpenGLContextDevice2D::GetMatrix(vtkMatrix3x3 *m)
{
  assert("pre: non_null" && m != NULL);
  // We must construct a 4x4 matrix from the 3x3 matrix for OpenGL
  double *M = m->GetData();
  double matrix[16];
  glGetDoublev(GL_MODELVIEW_MATRIX, matrix);

  // Convert from row major (C++ two dimensional arrays) to OpenGL
  M[0] = matrix[0];
  M[1] = matrix[4];
  M[2] = matrix[12];
  M[3] = matrix[1];
  M[4] = matrix[5];
  M[5] = matrix[13];
  M[6] = matrix[3];
  M[7] = matrix[7];
  M[8] = matrix[15];

  m->Modified();
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::PushMatrix()
{
  vtkOpenGLClearErrorMacro();
  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
  vtkOpenGLCheckErrorMacro("failed after PushMatrix");
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::PopMatrix()
{
  vtkOpenGLClearErrorMacro();
  glMatrixMode( GL_MODELVIEW );
  glPopMatrix();
  vtkOpenGLCheckErrorMacro("failed after PopMatrix");
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::SetClipping(int *dim)
{
  // Check the bounds, and clamp if necessary
  GLint vp[4] = { this->Storage->Offset.GetX(), this->Storage->Offset.GetY(),
    this->Storage->Dim.GetX(),this->Storage->Dim.GetY()};

  if (dim[0] > 0 && dim[0] < vp[2] )
    {
    vp[0] += dim[0];
    }
  if (dim[1] > 0 && dim[1] < vp[3])
    {
    vp[1] += dim[1];
    }
  if (dim[2] > 0 && dim[2] < vp[2])
    {
    vp[2] = dim[2];
    }
  if (dim[3] > 0 && dim[3] < vp[3])
    {
    vp[3] = dim[3];
    }

  glScissor(vp[0], vp[1], vp[2], vp[3]);
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::EnableClipping(bool enable)
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

//-----------------------------------------------------------------------------
bool vtkOpenGLContextDevice2D::SetStringRendererToFreeType()
{
  // FreeType is the only choice - nothing to do here
  return true;
}

//-----------------------------------------------------------------------------
bool vtkOpenGLContextDevice2D::SetStringRendererToQt()
{
  // The Qt based strategy is not available
  return false;
}

//----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::ReleaseGraphicsResources(vtkWindow *window)
{
  if (this->Storage->Texture)
    {
    this->Storage->Texture->ReleaseGraphicsResources(window);
    }
  if (this->Storage->SpriteTexture)
    {
    this->Storage->SpriteTexture->ReleaseGraphicsResources(window);
    }
  this->Storage->TextTextureCache.ReleaseGraphicsResources(window);
  this->Storage->MathTextTextureCache.ReleaseGraphicsResources(window);
}

//----------------------------------------------------------------------------
bool vtkOpenGLContextDevice2D::HasGLSL()
{
  return this->Storage->GLSL;
}

//-----------------------------------------------------------------------------
bool vtkOpenGLContextDevice2D::LoadExtensions(vtkOpenGLExtensionManager *m)
{
  if(m->ExtensionSupported("GL_ARB_texture_non_power_of_two"))
    {
    m->LoadExtension("GL_ARB_texture_non_power_of_two");
    this->Storage->PowerOfTwoTextures = false;
    this->TextRenderer->SetScaleToPowerOfTwo(false);
    }
  else
    {
    this->Storage->PowerOfTwoTextures = true;
    this->TextRenderer->SetScaleToPowerOfTwo(true);
    }
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

  // disable NPOT textures for Mesa
  // NPOT textures work in OS Mesa >= 8.0.0
  if ( m->DriverIsMesa()
     && !(m->DriverGLRendererIsOSMesa() && m->DriverVersionAtLeast(8)))
    {
    this->Storage->PowerOfTwoTextures = true;
    this->TextRenderer->SetScaleToPowerOfTwo(true);
    }

  this->Storage->GLExtensionsLoaded = true;
  return true;
}

//-----------------------------------------------------------------------------
vtkImageData *vtkOpenGLContextDevice2D::GetMarker(int shape, int size,
                                                  bool highlight)
{
  // Generate the cache key for this marker
  vtkTypeUInt64 key = highlight ? (1U << 31) : 0U;
  key |= static_cast<vtkTypeUInt16>(shape);
  key <<= 32;
  key |= static_cast<vtkTypeUInt32>(size);

  // Try to find the marker in the cache
  std::list<vtkMarkerCacheObject>::iterator match =
      std::find(this->MarkerCache.begin(), this->MarkerCache.end(), key);

  // Was it in the cache?
  if (match != this->MarkerCache.end())
    {
    // Yep -- move it to the front and return the data.
    if (match == this->MarkerCache.begin())
      {
      return match->Value;
      }
    else
      {
      vtkMarkerCacheObject result = *match;
      this->MarkerCache.erase(match);
      this->MarkerCache.push_front(result);
      return result.Value;
      }
    }

  // Nope -- we'll need to generate it. Create the image data:
  vtkMarkerCacheObject result;
  result.Key = key;
  result.Value = this->GenerateMarker(shape, size, highlight);

  // If there was an issue generating the marker, just return NULL.
  if (!result.Value)
    {
    vtkErrorMacro(<<"Error generating marker: shape,size: "
                  << shape << "," << size)
    return NULL;
    }

  // Check the current cache size.
  while (this->MarkerCache.size() >
         static_cast<size_t>(this->MaximumMarkerCacheSize - 1) &&
         !this->MarkerCache.empty())
    {
    this->MarkerCache.back().Value->Delete();
    this->MarkerCache.pop_back();
    }

   // Add to the cache
   this->MarkerCache.push_front(result);
   return result.Value;
}

//-----------------------------------------------------------------------------
vtkImageData *vtkOpenGLContextDevice2D::GenerateMarker(int shape, int width,
                                                       bool highlight)
{
  // Set up the image data, if highlight then the mark shape is different
  vtkImageData *result = vtkImageData::New();

  result->SetExtent(0, width - 1, 0, width - 1, 0, 0);
  result->AllocateScalars(VTK_UNSIGNED_CHAR, 4);

  unsigned char* image =
      static_cast<unsigned char*>(result->GetScalarPointer());
  memset(image, 0, width * width * 4);

  // Generate the marker image at the required size
  switch (shape)
    {
    case VTK_MARKER_CROSS:
      {
      int center = (width + 1) / 2;
      for (int i = 0; i < center; ++i)
        {
        int j = width - i - 1;
        memset(image + (4 * (width * i + i)), 255, 4);
        memset(image + (4 * (width * i + j)), 255, 4);
        memset(image + (4 * (width * j + i)), 255, 4);
        memset(image + (4 * (width * j + j)), 255, 4);
        if (highlight)
          {
          memset(image + (4 * (width * (j-1) + (i))), 255, 4);
          memset(image + (4 * (width * (i+1) + (i))), 255, 4);
          memset(image + (4 * (width * (i) + (i+1))), 255, 4);
          memset(image + (4 * (width * (i) + (j-1))), 255, 4);
          memset(image + (4 * (width * (i+1) + (j))), 255, 4);
          memset(image + (4 * (width * (j-1) + (j))), 255, 4);
          memset(image + (4 * (width * (j) + (j-1))), 255, 4);
          memset(image + (4 * (width * (j) + (i+1))), 255, 4);
          }
        }
      break;
      }
    default: // Maintaining old behavior, which produces plus for unknown shape
      vtkWarningMacro(<<"Invalid marker shape: " << shape);
    case VTK_MARKER_PLUS:
      {
      int center = (width + 1) / 2;
      for (int i = 0; i < center; ++i)
        {
        int j = width - i - 1;
        int c = center - 1;
        memset(image + (4 * (width * c + i)), 255, 4);
        memset(image + (4 * (width * c + j)), 255, 4);
        memset(image + (4 * (width * i + c)), 255, 4);
        memset(image + (4 * (width * j + c)), 255, 4);
        if (highlight)
          {
          memset(image + (4 * (width * (c - 1) + i)), 255, 4);
          memset(image + (4 * (width * (c + 1) + i)), 255, 4);
          memset(image + (4 * (width * (c - 1) + j)), 255, 4);
          memset(image + (4 * (width * (c + 1) + j)), 255, 4);
          memset(image + (4 * (width * i + (c - 1))), 255, 4);
          memset(image + (4 * (width * i + (c + 1))), 255, 4);
          memset(image + (4 * (width * j + (c - 1))), 255, 4);
          memset(image + (4 * (width * j + (c + 1))), 255, 4);
          }
        }
      break;
      }
    case VTK_MARKER_SQUARE:
      {
      memset(image, 255, width * width * 4);
      break;
      }
    case VTK_MARKER_CIRCLE:
      {
      double r = width/2.0;
      double r2 = r * r;
      for (int i = 0; i < width; ++i)
        {
        double dx2 = (i - r) * (i - r);
        for (int j = 0; j < width; ++j)
          {
          double dy2 = (j - r) * (j - r);
          if ((dx2 + dy2) < r2)
            {
            memset(image + (4 * width * i) + (4 * j), 255, 4);
            }
          }
        }
      break;
      }
    case VTK_MARKER_DIAMOND:
      {
      int r = width/2;
      for (int i = 0; i < width; ++i)
        {
        int dx = abs(i - r);
        for (int j = 0; j < width; ++j)
          {
          int dy = abs(j - r);
          if (r - dx >= dy)
            {
            memset(image + (4 * width * i) + (4 * j), 255, 4);
            }
          }
        }
      break;
      }
    }
  return result;
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
  os << indent << "MaximumMarkerCacheSize: "
     << this->MaximumMarkerCacheSize << endl;
  os << indent << "MarkerCache: " << this->MarkerCache.size()
     << " entries." << endl;
}
