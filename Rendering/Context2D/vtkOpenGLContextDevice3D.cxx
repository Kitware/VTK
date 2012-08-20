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
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLExtensionManager.h"
#include "vtkgl.h"

#include "vtkImageData.h"
#include "vtkMathTextUtilities.h"
#include "vtkStdString.h"
#include "vtkFreeTypeStringToImage.h"
#include "vtkTransform.h"
#include "vtkTexture.h"
#include "vtkUnicodeString.h"
#include "vtkWindow.h"

#include "vtkSmartPointer.h"

#include "vtkOpenGLContextDevice2DPrivate.h"

#include "vtkObjectFactory.h"

class vtkOpenGLContextDevice3D::Private
{
public:
  Private()
  {
    this->SavedLighting = GL_TRUE;
    this->SavedDepthTest = GL_TRUE;
    this->Texture = NULL;
    this->TextureProperties = vtkContextDevice3D::Linear |
        vtkContextDevice3D::Stretch;
  }

  ~Private()
  {
  if (this->Texture != NULL)
    {
    this->Texture->Delete();
    }
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

  // copied from 2D private
  vtkVector2i FindPowerOfTwo(const vtkVector2i& size)
    {
    vtkVector2i pow2(1, 1);
    for (int i = 0; i < 2; ++i)
      {
      while (pow2[i] < size[i])
        {
        pow2[i] *= 2;
        }
      }
    return pow2;
    }
  GLuint TextureFromImage(vtkImageData *image, vtkVector2f& texCoords)
    {
    if (image->GetScalarType() != VTK_UNSIGNED_CHAR)
      {
      cout << "Error = not an unsigned char..." << endl;
      return 0;
      }
    int bytesPerPixel = image->GetNumberOfScalarComponents();
    int size[3];
    image->GetDimensions(size);
    vtkVector2i newImg = this->FindPowerOfTwo(vtkVector2i(size[0], size[1]));

    for (int i = 0; i < 2; ++i)
      {
      texCoords[i] = size[i] / float(newImg[i]);
      }

    unsigned char *dataPtr =
        new unsigned char[newImg[0] * newImg[1] * bytesPerPixel];
    unsigned char *origPtr =
        static_cast<unsigned char*>(image->GetScalarPointer());

    for (int i = 0; i < newImg[0]; ++i)
      {
      for (int j = 0; j < newImg[1]; ++j)
        {
        for (int k = 0; k < bytesPerPixel; ++k)
          {
          if (i < size[0] && j < size[1])
            {
            dataPtr[i * newImg[0] * bytesPerPixel + j * bytesPerPixel + k] =
                origPtr[i * size[0] * bytesPerPixel + j * bytesPerPixel + k];
            }
          else
            {
            dataPtr[i * newImg[0] * bytesPerPixel + j * bytesPerPixel + k] =
                k == 3 ? 0 : 255;
            }
          }
        }
      }

    GLuint tmpIndex(0);
    GLint glFormat = bytesPerPixel == 3 ? GL_RGB : GL_RGBA;
    GLint glInternalFormat = bytesPerPixel == 3 ? GL_RGB8 : GL_RGBA8;

    glGenTextures(1, &tmpIndex);
    glBindTexture(GL_TEXTURE_2D, tmpIndex);

    glTexEnvf(GL_TEXTURE_ENV, vtkgl::COMBINE_RGB, GL_REPLACE);
    glTexEnvf(GL_TEXTURE_ENV, vtkgl::COMBINE_ALPHA, GL_REPLACE);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                     vtkgl::CLAMP_TO_EDGE );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                     vtkgl::CLAMP_TO_EDGE );

    glTexImage2D(GL_TEXTURE_2D, 0 , glInternalFormat,
                 newImg[0], newImg[1], 0, glFormat,
                 GL_UNSIGNED_BYTE, static_cast<const GLvoid *>(dataPtr));
    glAlphaFunc(GL_GREATER, static_cast<GLclampf>(0));
    glEnable(GL_ALPHA_TEST);
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_TEXTURE_2D);
    delete [] dataPtr;
    return tmpIndex;
    }

  GLuint TextureFromImage(vtkImageData *image)
  {
    if (image->GetScalarType() != VTK_UNSIGNED_CHAR)
      {
      cout << "Error = not an unsigned char..." << endl;
      return 0;
      }
    int bytesPerPixel = image->GetNumberOfScalarComponents();
    int size[3];
    image->GetDimensions(size);

    unsigned char *dataPtr =
        static_cast<unsigned char*>(image->GetScalarPointer());
    GLuint tmpIndex(0);
    GLint glFormat = bytesPerPixel == 3 ? GL_RGB : GL_RGBA;
    GLint glInternalFormat = bytesPerPixel == 3 ? GL_RGB8 : GL_RGBA8;

    glGenTextures(1, &tmpIndex);
    glBindTexture(GL_TEXTURE_2D, tmpIndex);

    glTexEnvf(GL_TEXTURE_ENV, vtkgl::COMBINE_RGB, GL_REPLACE);
    glTexEnvf(GL_TEXTURE_ENV, vtkgl::COMBINE_ALPHA, GL_REPLACE);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                     vtkgl::CLAMP_TO_EDGE );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                     vtkgl::CLAMP_TO_EDGE );

    glTexImage2D(GL_TEXTURE_2D, 0 , glInternalFormat,
                 size[0], size[1], 0, glFormat,
                 GL_UNSIGNED_BYTE, static_cast<const GLvoid *>(dataPtr));
    glAlphaFunc(GL_GREATER, static_cast<GLclampf>(0));
    glEnable(GL_ALPHA_TEST);
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_TEXTURE_2D);
    return tmpIndex;
  }
  vtkTexture *Texture;
  mutable vtkTextureImageCache<TextPropertyKey> TextTextureCache;
  mutable vtkTextureImageCache<TextPropertyKey> MathTextTextureCache;
  bool PowerOfTwoTextures;
  unsigned int TextureProperties;
};

vtkStandardNewMacro(vtkOpenGLContextDevice3D)

vtkOpenGLContextDevice3D::vtkOpenGLContextDevice3D() : Storage(new Private)
{
  this->RenderWindow = NULL;
  this->InRender = false;
  this->TextRenderer = vtkFreeTypeStringToImage::New();
}

vtkOpenGLContextDevice3D::~vtkOpenGLContextDevice3D()
{
  this->TextRenderer->Delete();
  delete Storage;
}

void vtkOpenGLContextDevice3D::DrawPoly(const float *verts, int n,
                                        const unsigned char *colors, int nc)
{
  assert("verts must be non-null" && verts != NULL);
  assert("n must be greater than 0" && n > 0);

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
}

void vtkOpenGLContextDevice3D::DrawPoints(const float *verts, int n,
                                          const unsigned char *colors, int nc)
{
  assert("verts must be non-null" && verts != NULL);
  assert("n must be greater than 0" && n > 0);

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
  this->Storage->Transpose(M, matrix);
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
}

void vtkOpenGLContextDevice3D::PopMatrix()
{
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}

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

  vtkOpenGLRenderer *gl = vtkOpenGLRenderer::SafeDownCast(viewport);
  if (gl)
    {
    this->RenderWindow = vtkOpenGLRenderWindow::SafeDownCast(
        gl->GetRenderWindow());
    }

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

void vtkOpenGLContextDevice3D::PrintSelf(ostream &os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice3D::AlignText(double orientation, float width,
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
void vtkOpenGLContextDevice3D::DrawString(float *point,
                                          const vtkStdString &string)
{
  GLfloat mv[16];
  glGetFloatv(GL_MODELVIEW_MATRIX, mv);
  float xScale = mv[0];
  float yScale = mv[5];

  float p[] = { std::floor(point[0] * xScale) / xScale, std::floor(point[1] * yScale) / yScale };
 
  // Cache rendered text strings
  vtkTextureImageCache<TextPropertyKey>::CacheData cache =
    this->Storage->TextTextureCache.GetCacheData(
      TextPropertyKey(this->TextProp, string));
  vtkImageData* image = cache.ImageData;
  if (image->GetNumberOfPoints() == 0 && image->GetNumberOfCells() == 0)
    {
    if (!this->TextRenderer->RenderString(this->TextProp, string, image))
      {
      return;
      }
    }
  vtkTexture* texture = cache.Texture;
  texture->Render(this->Renderer);

  float width = static_cast<float>(image->GetOrigin()[0]) / xScale;
  float height = static_cast<float>(image->GetOrigin()[1]) / yScale;

  float xw = static_cast<float>(image->GetSpacing()[0]);
  float xh = static_cast<float>(image->GetSpacing()[1]);

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
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice3D::DrawZAxisLabel(float *point,
                                              const vtkStdString &string)
{
  GLfloat mv[16];
  glGetFloatv(GL_MODELVIEW_MATRIX, mv);
  float xScale = mv[0];
  float yScale = mv[5];

  float p[] = { std::floor(point[0] * xScale) / xScale, std::floor(point[1] * yScale) / yScale, 1 };
 
  // Cache rendered text strings
  vtkTextureImageCache<TextPropertyKey>::CacheData cache =
    this->Storage->TextTextureCache.GetCacheData(
      TextPropertyKey(this->TextProp, string));
  vtkImageData* image = cache.ImageData;
  if (image->GetNumberOfPoints() == 0 && image->GetNumberOfCells() == 0)
    {
    if (!this->TextRenderer->RenderString(this->TextProp, string, image))
      {
      return;
      }
    }
  vtkTexture* texture = cache.Texture;


  vtkNew<vtkTransform> rotateZ;
  rotateZ->RotateZ(-90.0);
  texture->SetTransform(rotateZ.GetPointer());
  texture->Render(this->Renderer);

  float width = static_cast<float>(image->GetOrigin()[0]) / xScale;
  float height = static_cast<float>(image->GetOrigin()[1]) / yScale;

  float xw = static_cast<float>(image->GetSpacing()[0]);
  float xh = static_cast<float>(image->GetSpacing()[1]);

  rotateZ->TransformPoint(p, p);
  //this->AlignText(this->TextProp->GetOrientation(), width, height, p);

  float points[] = { p[0]        , p[1], 0,
                     p[0] + width, p[1], 0,
                     p[0] + width, p[1] + height, width,
                     p[0]        , p[1] + height, width
                   };

  float texCoord[] = { 0.0f, 0.0f, 0,
                       xw,   0.0f, 0,
                       xw,   xh, 0,
                       0.0f, xh, 0 };

  glColor4ub(255, 255, 255, 255);
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glVertexPointer(3, GL_FLOAT, 0, points);
  glTexCoordPointer(3, GL_FLOAT, 0, texCoord);
  glDrawArrays(GL_QUADS, 0, 4);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  glDisableClientState(GL_VERTEX_ARRAY);

  texture->PostRender(this->Renderer);
  glDisable(GL_TEXTURE_2D);
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice3D::ComputeStringBounds(const vtkStdString &string,
                                                   float bounds[4])
{
  vtkVector2i box = this->TextRenderer->GetBounds(this->TextProp, string);
  bounds[0] = static_cast<float>(0);
  bounds[1] = static_cast<float>(0);
  bounds[2] = static_cast<float>(box.X());
  bounds[3] = static_cast<float>(box.Y());
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice3D::DrawString(float *point,
                                          const vtkUnicodeString &string)
{
  int p[] = { static_cast<int>(point[0]),
              static_cast<int>(point[1]) };

  //TextRenderer draws in window, not viewport coords
  p[0]+=this->Storage->Offset.GetX();
  p[1]+=this->Storage->Offset.GetY();
  vtkImageData *data = vtkImageData::New();
  this->TextRenderer->RenderString(this->TextProp, string, data);
  this->DrawImage(point, 1.0, data);
  data->Delete();
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice3D::ComputeStringBounds(const vtkUnicodeString &string,
                                                   float bounds[4])
{
  vtkVector2i box = this->TextRenderer->GetBounds(this->TextProp, string);
  bounds[0] = static_cast<float>(0);
  bounds[1] = static_cast<float>(0);
  bounds[2] = static_cast<float>(box.X());
  bounds[3] = static_cast<float>(box.Y());
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice3D::DrawMathTextString(float point[2],
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

  float p[] = { std::floor(point[0]), std::floor(point[1]) };

  // Cache rendered text strings
  vtkTextureImageCache<TextPropertyKey>::CacheData cache =
    this->Storage->MathTextTextureCache.GetCacheData(
      TextPropertyKey(this->TextProp, string));
  vtkImageData* image = cache.ImageData;
  if (image->GetNumberOfPoints() == 0 && image->GetNumberOfCells() == 0)
    {
    if (!mathText->RenderString(string.c_str(), image, this->TextProp,
                                this->RenderWindow->GetDPI()))
      {
      return;
      }
    }

  vtkTexture* texture = cache.Texture;
  texture->Render(this->Renderer);

  int *dims = image->GetDimensions();
  float width = static_cast<float>(dims[0]);
  float height = static_cast<float>(dims[1]);

  this->AlignText(this->TextProp->GetOrientation(), width, height, p);

  float points[] = { p[0]        , p[1],
                     p[0] + width, p[1],
                     p[0] + width, p[1] + height,
                     p[0]        , p[1] + height };

  float texCoord[] = { 0.0f, 0.0f,
                       1.0f, 0.0f,
                       1.0f, 1.0f,
                       0.0f, 1.0f };

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
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice3D::DrawImage(float p[2], float scale,
                                         vtkImageData *image)
{
  this->SetTexture(image);
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
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice3D::DrawImage(const vtkRectf& pos,
                                         vtkImageData *image)
{
  vtkVector2f tex(1.0, 1.0);
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
  float points[] = { pos.X()              , pos.Y(),
                     pos.X() + pos.Width(), pos.Y(),
                     pos.X() + pos.Width(), pos.Y() + pos.Height(),
                     pos.X()              , pos.Y() + pos.Height() };

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
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice3D::SetTexture(vtkImageData* image, int properties)
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

//----------------------------------------------------------------------------
void vtkOpenGLContextDevice3D::ReleaseGraphicsResources(vtkWindow *window)
{
  if (this->Storage->Texture)
    {
    this->Storage->Texture->ReleaseGraphicsResources(window);
    }
  this->Storage->TextTextureCache.ReleaseGraphicsResources(window);
  this->Storage->MathTextTextureCache.ReleaseGraphicsResources(window);
}
