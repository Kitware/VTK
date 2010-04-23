/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLContextBufferId.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLContextBufferId.h"

#include "vtkIntArray.h"
#include <cassert>
#include "vtkObjectFactory.h"
#include "vtkOpenGLTexture.h"
#include "vtkTextureObject.h"
#include "vtkgl.h"
#include "vtkOpenGLRenderWindow.h"

vtkStandardNewMacro(vtkOpenGLContextBufferId);

// ----------------------------------------------------------------------------
vtkOpenGLContextBufferId::vtkOpenGLContextBufferId()
{
  this->Texture=0;
  this->Context=0;
}

// ----------------------------------------------------------------------------
vtkOpenGLContextBufferId::~vtkOpenGLContextBufferId()
{
  if(this->Texture!=0)
    {
    vtkErrorMacro("texture should have been released.");
    }
}

// ----------------------------------------------------------------------------
void vtkOpenGLContextBufferId::ReleaseGraphicsResources()
{
  if(this->Texture!=0)
    {
    this->Texture->Delete();
    this->Texture=0;
    }
}

//----------------------------------------------------------------------------
void vtkOpenGLContextBufferId::SetContext(vtkOpenGLRenderWindow *context)
{
  if(this->Context!=context)
    {
    this->ReleaseGraphicsResources();
    this->Context=context;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
vtkOpenGLRenderWindow *vtkOpenGLContextBufferId::GetContext()
{
  return this->Context;
}

// ----------------------------------------------------------------------------
void vtkOpenGLContextBufferId::Allocate()
{
  assert("pre: positive_width" && this->GetWidth()>0);
  assert("pre: positive_height" && this->GetHeight()>0);
  
  if(this->Texture==0)
    {
    this->Texture=vtkTextureObject::New();
    this->Texture->SetContext(this->Context);
    }
  this->Context->MakeCurrent();
  // 3: RGB
  this->Texture->Allocate2D(static_cast<unsigned int>(this->GetWidth()),
                            static_cast<unsigned int>(this->GetHeight()),3,
                            VTK_UNSIGNED_CHAR);
}

// ----------------------------------------------------------------------------
bool vtkOpenGLContextBufferId::IsAllocated() const
{

  return this->Texture!=0 &&
    this->Texture->GetWidth()==static_cast<unsigned int>(this->Width) &&
    this->Texture->GetHeight()==static_cast<unsigned int>(this->Height);
}

// ----------------------------------------------------------------------------
void vtkOpenGLContextBufferId::SetValues(int srcXmin,
                                         int srcYmin)
{
  assert("pre: is_allocated" && this->IsAllocated());
  
  // copy the current read buffer to the texture.
  this->Texture->CopyFromFrameBuffer(srcXmin,srcYmin,0,0,this->Width,
                                     this->Height);
}

// ----------------------------------------------------------------------------
vtkIdType vtkOpenGLContextBufferId::GetPickedItem(int x, int y)
{
  assert("pre: is_allocated" && this->IsAllocated());

  vtkIdType result=-1;
  if(x<0 || x>=this->Width)
    {
    vtkDebugMacro(<<"x mouse position out of range: x=" << x << " (width="
                    << this->Width <<")");
    }
  else
    {
    if(y<0 || y>=this->Height)
      {
      vtkDebugMacro(<<"y mouse position out of range: y="<< y << " (height="
                      << this->Height << ")");
      }
    else
      {
      this->Context->MakeCurrent();
      // Render texture to current write buffer. Texel x,y is rendered at
      // pixel x,y (instead of pixel 0,0 to work around pixel ownership test).
      GLint savedDrawBuffer;
      glGetIntegerv(GL_DRAW_BUFFER,&savedDrawBuffer);
      bool savedDepthTest=glIsEnabled(GL_DEPTH_TEST)==GL_TRUE;
      bool savedAlphaTest=glIsEnabled(GL_ALPHA_TEST)==GL_TRUE;
      bool savedStencilTest=glIsEnabled(GL_STENCIL_TEST)==GL_TRUE;
      bool savedBlend=glIsEnabled(GL_BLEND)==GL_TRUE;
      
      if(savedDrawBuffer!=GL_BACK_LEFT)
        {
        glDrawBuffer(GL_BACK_LEFT);
        }
      if(savedDepthTest)
        {
        glDisable(GL_DEPTH_TEST);
        }
      if(savedAlphaTest)
        {
        glDisable(GL_ALPHA_TEST);
        }
      if(savedStencilTest)
        {
        glDisable(GL_STENCIL_TEST);
        }
      if(savedBlend)
        {
        glDisable(GL_BLEND);
        }
      
      // Fixed-pipeline stuff
      vtkgl::ActiveTexture(vtkgl::TEXTURE0);
      this->Texture->Bind();
      glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
      glEnable(GL_TEXTURE_2D);
      this->Texture->CopyToFrameBuffer(x,y,x,y,x,y,this->Width,this->Height);
      glDisable(GL_TEXTURE_2D);
      glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); // restore
      
      GLint savedReadBuffer;
      glGetIntegerv(GL_READ_BUFFER,&savedReadBuffer);
      glReadBuffer(GL_BACK_LEFT);
      
      // To workaround pixel ownership test,
      // get value from current read buffer at pixel (x,y) instead of just
      // (0,0).
      glPixelStorei(GL_PACK_ALIGNMENT,1);
      unsigned char rgb[3];
      rgb[0]=5;
      rgb[1]=1;
      rgb[2]=8;
      glReadPixels(x,y,1,1,GL_RGB,GL_UNSIGNED_BYTE,rgb);
      
      if(savedReadBuffer!=GL_BACK_LEFT)
        {
        glReadBuffer(static_cast<GLenum>(savedReadBuffer));
        }
      if(savedDrawBuffer!=GL_BACK_LEFT)
        {
        glDrawBuffer(static_cast<GLenum>(savedDrawBuffer));
        }
      if(savedDepthTest)
        {
        glEnable(GL_DEPTH_TEST);
        }
      if(savedAlphaTest)
        {
        glEnable(GL_ALPHA_TEST);
        }
      if(savedStencilTest)
        {
        glEnable(GL_STENCIL_TEST);
        }
      if(savedBlend)
        {
        glEnable(GL_BLEND);
        }
      
      int value=(static_cast<int>(rgb[0])<<16)|(static_cast<int>(rgb[1])<<8)
        |static_cast<int>(rgb[2]);
    
      result=static_cast<vtkIdType>(value-1);
      }
    }

  assert("post: valid_result" && result>=-1 );
  return result;
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextBufferId::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
