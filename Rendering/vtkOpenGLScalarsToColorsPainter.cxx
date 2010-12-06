/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLScalarsToColorsPainter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLScalarsToColorsPainter.h"

#include "vtkActor.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkMapper.h" //for VTK_MATERIALMODE_*
#include "vtkObjectFactory.h"
#include "vtkOpenGLTexture.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"


#ifndef VTK_IMPLEMENT_MESA_CXX
#  include "vtkOpenGL.h"
#endif

#include "vtkgl.h" // vtkgl namespace

#ifndef VTK_IMPLEMENT_MESA_CXX
vtkStandardNewMacro(vtkOpenGLScalarsToColorsPainter);
#endif
//-----------------------------------------------------------------------------
vtkOpenGLScalarsToColorsPainter::vtkOpenGLScalarsToColorsPainter()
{
  this->InternalColorTexture = 0;
}

//-----------------------------------------------------------------------------
vtkOpenGLScalarsToColorsPainter::~vtkOpenGLScalarsToColorsPainter()
{
  if (this->InternalColorTexture)
    {
    this->InternalColorTexture->Delete();
    this->InternalColorTexture = 0;
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLScalarsToColorsPainter::ReleaseGraphicsResources(vtkWindow* win)
{
  if (this->InternalColorTexture)
    {
    this->InternalColorTexture->ReleaseGraphicsResources(win);
    }
  this->Superclass::ReleaseGraphicsResources(win);
}

//-----------------------------------------------------------------------------
int vtkOpenGLScalarsToColorsPainter::GetPremultiplyColorsWithAlpha(
  vtkActor* actor)
{
  GLint alphaBits;
  glGetIntegerv(GL_ALPHA_BITS, &alphaBits);
  
  // Dealing with having a correct alpha (none square) in the framebuffer
  // is only required if there is an alpha component in the framebuffer
  // (doh...) and if we cannot deal directly with BlendFuncSeparate.
  
  return vtkgl::BlendFuncSeparate==0 && alphaBits>0 &&
    this->Superclass::GetPremultiplyColorsWithAlpha(actor);
}

//-----------------------------------------------------------------------------
vtkIdType vtkOpenGLScalarsToColorsPainter::GetTextureSizeLimit()
{
  GLint textureSize = 0;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &textureSize);
  return static_cast<vtkIdType>(textureSize);
}

//-----------------------------------------------------------------------------
void vtkOpenGLScalarsToColorsPainter::RenderInternal(vtkRenderer *renderer, 
                                                     vtkActor *actor,
                                                     unsigned long typeflags,
                                                     bool forceCompileOnly)
{
  vtkProperty* prop = actor->GetProperty();

  // If we are coloring by texture, then load the texture map.
  if (this->ColorTextureMap)
    {
    if (this->InternalColorTexture == 0)
      {
      this->InternalColorTexture = vtkOpenGLTexture::New();
      this->InternalColorTexture->RepeatOff();
      }
    this->InternalColorTexture->SetInput(this->ColorTextureMap);
    // Keep color from interacting with texture.
    float info[4];
    info[0] = info[1] = info[2] = info[3] = 1.0;
    glMaterialfv( GL_FRONT_AND_BACK, GL_DIFFUSE, info );
    
    this->LastWindow = renderer->GetRenderWindow();
    }
  else if (this->LastWindow)
    {
    // release the texture.
    this->ReleaseGraphicsResources(this->LastWindow);
    this->LastWindow = 0;
    }


  // if we are doing vertex colors then set lmcolor to adjust 
  // the current materials ambient and diffuse values using   
  // vertex color commands otherwise tell it not to.          
  glDisable( GL_COLOR_MATERIAL );
  if (this->UsingScalarColoring)
    {
    GLenum lmcolorMode;
    if (this->ScalarMaterialMode == VTK_MATERIALMODE_DEFAULT)
      {
      if (prop->GetAmbient() > prop->GetDiffuse())
        {
        lmcolorMode = GL_AMBIENT;
        }
      else
        {
        lmcolorMode = GL_DIFFUSE;
        }
      }
    else if (this->ScalarMaterialMode == VTK_MATERIALMODE_AMBIENT_AND_DIFFUSE)
      {
      lmcolorMode = GL_AMBIENT_AND_DIFFUSE;
      }
    else if (this->ScalarMaterialMode == VTK_MATERIALMODE_AMBIENT)
      {
      lmcolorMode = GL_AMBIENT;
      }
    else // if (this->ScalarMaterialMode == VTK_MATERIALMODE_DIFFUSE)
      {
      lmcolorMode = GL_DIFFUSE;
      } 

    if (this->ColorTextureMap)
      {
      this->InternalColorTexture->Load(renderer);
      }
    else
      {
      glColorMaterial( GL_FRONT_AND_BACK, lmcolorMode);
      glEnable( GL_COLOR_MATERIAL );
      }
    }

  int pre_multiplied_by_alpha =  this->GetPremultiplyColorsWithAlpha(actor);
  
  // We colors were premultiplied by alpha then we change the blending
  // function to one that will compute correct blended destination alpha
  // value, otherwise we stick with the default.
  if (pre_multiplied_by_alpha)
    {
    // save the blend function.
    glPushAttrib(GL_COLOR_BUFFER_BIT);
    
    // the following function is not correct with textures because there are
    // not premultiplied by alpha.
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    }

  this->Superclass::RenderInternal(renderer, actor, typeflags,forceCompileOnly);

  if (pre_multiplied_by_alpha)
    {
    // restore the blend function
    glPopAttrib();
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLScalarsToColorsPainter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
