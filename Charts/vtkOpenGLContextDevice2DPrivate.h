/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLContextDevice2DPrivate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkOpenGL2ContextDevice2DPrivate - Private class with storage and
// utility functions for the vtkOpenGLContextDevice2D.
//
// .SECTION Description
// This class is for internal use only, it should not be included from anything
// outside of the vtkCharts kit. It provides a shared private class that can be
// used by vtkOpenGLContextDevice2D and derived classes.
//
// .Section Caveats
// Internal use only.
//
// .Section See Also
// vtkOpenGLContextDevice2D vtkOpenGL2ContextDevice2D

#ifndef __vtkOpenGLContextDevice2DPrivate_h
#define __vtkOpenGLContextDevice2DPrivate_h

#include "vtkOpenGLContextDevice2D.h"

class vtkOpenGLContextDevice2D::Private
{
public:
  Private()
  {
    this->Texture = NULL;
    this->TextureProperties = vtkContextDevice2D::Linear |
        vtkContextDevice2D::Stretch;
    this->SpriteTexture = NULL;
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
    this->OpenGL20 = false;
    this->GLSL = false;
    this->PowerOfTwoTextures = true;
  }

  ~Private()
  {
    if (this->Texture)
      {
      this->Texture->Delete();
      this->Texture = NULL;
      }
    if (this->SpriteTexture)
      {
      this->SpriteTexture->Delete();
      this->SpriteTexture = NULL;
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

  float* TexCoords(float* f, int n)
  {
    float* texCoord = new float[2*n];
    float minX = f[0]; float minY = f[1];
    float maxX = f[0]; float maxY = f[1];
    float* fptr = f;
    for(int i = 0; i < n; ++i)
      {
      minX = fptr[0] < minX ? fptr[0] : minX;
      maxX = fptr[0] > maxX ? fptr[0] : maxX;
      minY = fptr[1] < minY ? fptr[1] : minY;
      maxY = fptr[1] > maxY ? fptr[1] : maxY;
      fptr+=2;
      }
    fptr = f;
    if (this->TextureProperties & vtkContextDevice2D::Repeat)
      {
      double* textureBounds = this->Texture->GetInput()->GetBounds();
      float rangeX = (textureBounds[1] - textureBounds[0]) ?
        textureBounds[1] - textureBounds[0] : 1.;
      float rangeY = (textureBounds[3] - textureBounds[2]) ?
        textureBounds[3] - textureBounds[2] : 1.;
      for (int i = 0; i < n; ++i)
        {
        texCoord[i*2] = (fptr[0]-minX) / rangeX;
        texCoord[i*2+1] = (fptr[1]-minY) / rangeY;
        fptr+=2;
        }
      }
    else // this->TextureProperties & vtkContextDevice2D::Stretch
      {
      float rangeX = (maxX - minX)? maxX - minX : 1.f;
      float rangeY = (maxY - minY)? maxY - minY : 1.f;
      for (int i = 0; i < n; ++i)
        {
        texCoord[i*2] = (fptr[0]-minX)/rangeX;
        texCoord[i*2+1] = (fptr[1]-minY)/rangeY;
        fptr+=2;
        }
      }
    return texCoord;
  }

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
  unsigned int TextureProperties;
  vtkTexture *SpriteTexture;
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
  vtkVector2i Offset;
  bool GLExtensionsLoaded;
  bool OpenGL15;
  bool OpenGL20;
  bool GLSL;
  bool PowerOfTwoTextures;
};

#endif // VTKOPENGLCONTEXTDEVICE2DPRIVATE_H
