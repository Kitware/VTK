/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGL2ContextDevice2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGL2ContextDevice2D.h"

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
#include "vtkOpenGLError.h"

#include "vtkObjectFactory.h"

#include "vtkOpenGLContextDevice2DPrivate.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGL2ContextDevice2D);

//-----------------------------------------------------------------------------
bool vtkOpenGL2ContextDevice2D::IsSupported(vtkViewport *)
{
  return true;
}

//-----------------------------------------------------------------------------
vtkOpenGL2ContextDevice2D::vtkOpenGL2ContextDevice2D()
{
}

//-----------------------------------------------------------------------------
vtkOpenGL2ContextDevice2D::~vtkOpenGL2ContextDevice2D()
{
}

//-----------------------------------------------------------------------------
void vtkOpenGL2ContextDevice2D::DrawPointSprites(vtkImageData *sprite,
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
      this->Storage->SpriteTexture->Render(this->Renderer);
      glEnable(GL_TEXTURE_2D);
      }

    // We can actually use point sprites here
    glEnable(GL_POINT_SPRITE);
    glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);
    glPointParameteri(GL_POINT_SPRITE_COORD_ORIGIN, GL_LOWER_LEFT);

    this->DrawPoints(points, n, colors, nc_comps);

    glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_FALSE);
    glDisable(GL_POINT_SPRITE);

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
void vtkOpenGL2ContextDevice2D::DrawImage(float p[2], float scale,
                                         vtkImageData *image)
{
  vtkOpenGLClearErrorMacro();
  this->SetTexture(image);
  this->Storage->Texture->Render(this->Renderer);
  glEnable(GL_TEXTURE_2D);
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
void vtkOpenGL2ContextDevice2D::DrawImage(const vtkRectf& pos,
                                         vtkImageData *image)
{
  vtkOpenGLClearErrorMacro();
  GLuint index = this->Storage->TextureFromImage(image);
//  this->SetTexture(image);
//  this->Storage->Texture->Render(this->Renderer);
  glEnable(GL_TEXTURE_2D);
  float points[] = { pos.GetX()              , pos.GetY(),
                     pos.GetX() + pos.GetWidth(), pos.GetY(),
                     pos.GetX() + pos.GetWidth(), pos.GetY() + pos.GetHeight(),
                     pos.GetX()              , pos.GetY() + pos.GetHeight() };

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

//  this->Storage->Texture->PostRender(this->Renderer);
  glDisable(GL_TEXTURE_2D);
  glDeleteTextures(1, &index);
  vtkOpenGLCheckErrorMacro("failed after DrawImage");
}

//----------------------------------------------------------------------------
void vtkOpenGL2ContextDevice2D::ReleaseGraphicsResources(vtkWindow *window)
{
  this->vtkOpenGLContextDevice2D::ReleaseGraphicsResources(window);
}

//-----------------------------------------------------------------------------
void vtkOpenGL2ContextDevice2D::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
