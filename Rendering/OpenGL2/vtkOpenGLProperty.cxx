/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLProperty.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLProperty.h"

#include "vtkglVBOHelper.h"

#include "vtkObjectFactory.h"
#include "vtkOpenGLTexture.h"
#include "vtkTexture.h"

#include "vtkOpenGLTextureUnitManager.h"
#include "vtkOpenGLError.h"

#include <cassert>

namespace
{
  void ComputeMaterialColor(GLfloat result[4],
    bool premultiply_colors_with_alpha,
    double color_factor,
    const double color[3],
    double opacity)
    {
    double opacity_factor = premultiply_colors_with_alpha? opacity : 1.0;
    for (int cc=0; cc < 3; cc++)
      {
      result[cc] = static_cast<GLfloat>(
        opacity_factor * color_factor * color[cc]);
      }
    result[3] = opacity;
    }
}

vtkStandardNewMacro(vtkOpenGLProperty);

vtkOpenGLProperty::vtkOpenGLProperty()
{
}

vtkOpenGLProperty::~vtkOpenGLProperty()
{
}


// ----------------------------------------------------------------------------
// Implement base class method.
void vtkOpenGLProperty::Render(vtkActor *anActor, vtkRenderer *ren)
{
  // Set the LineStipple
  if (this->LineStipplePattern != 0xFFFF)
    {
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(this->LineStippleRepeatFactor,
                  static_cast<GLushort>(this->LineStipplePattern));
    //vtkOpenGLGL2PSHelper::EnableStipple(); // must be called after glLineStipple
    }
  else
    {
    // still need to set this although we are disabling.  else the ATI X1600
    // (for example) still manages to stipple under certain conditions.
    glLineStipple(this->LineStippleRepeatFactor,
                  static_cast<GLushort>(this->LineStipplePattern));
    glDisable(GL_LINE_STIPPLE);
    //vtkOpenGLGL2PSHelper::DisableStipple();
    }

  // disable alpha testing (this may have been enabled
  // by another actor in OpenGLTexture)
  glDisable (GL_ALPHA_TEST);

  // turn on/off backface culling
  if (! this->BackfaceCulling && ! this->FrontfaceCulling)
    {
    glDisable (GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
  else if (this->BackfaceCulling)
    {
    glCullFace (GL_BACK);
    glEnable (GL_CULL_FACE);
    }
  else //if both front & back culling on, will fall into backface culling
    { //if you really want both front and back, use the Actor's visibility flag
    glCullFace (GL_FRONT);
    glEnable (GL_CULL_FACE);
    }

  this->RenderTextures(anActor, ren);
  this->Superclass::Render(anActor, ren);
}

//-----------------------------------------------------------------------------
bool vtkOpenGLProperty::RenderTextures(vtkActor*, vtkRenderer* ren)
{
  // render any textures.
  int numTextures = this->GetNumberOfTextures();
  if (numTextures > 0)
    {
    GLint numSupportedTextures;
    glGetIntegerv(GL_MAX_TEXTURE_UNITS, &numSupportedTextures);
    for (int t = 0; t < numTextures; t++)
      {
      int texture_unit = this->GetTextureUnitAtIndex(t);
      if (texture_unit >= numSupportedTextures || texture_unit < 0)
        {
        vtkErrorMacro("Hardware does not support the number of textures defined.");
        continue;
        }

      glActiveTexture(GL_TEXTURE0 +
                           static_cast<GLenum>(texture_unit));
      this->GetTextureAtIndex(t)->Render(ren);
      }
    }

  vtkOpenGLCheckErrorMacro("failed after Render");

  return (numTextures > 0);
}

//-----------------------------------------------------------------------------
void vtkOpenGLProperty::PostRender(vtkActor *actor, vtkRenderer *renderer)
{
  vtkOpenGLClearErrorMacro();

  // Reset the face culling now we are done, leaking into text actor etc.
  if (this->BackfaceCulling || this->FrontfaceCulling)
    {
    glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

  this->Superclass::PostRender(actor, renderer);

  vtkOpenGLCheckErrorMacro("failed after PostRender");
}

//-----------------------------------------------------------------------------
// Implement base class method.
void vtkOpenGLProperty::BackfaceRender(vtkActor *vtkNotUsed(anActor), vtkRenderer *vtkNotUsed(ren))
{
}

//-----------------------------------------------------------------------------
void vtkOpenGLProperty::ReleaseGraphicsResources(vtkWindow *win)
{
  // release any textures.
  int numTextures = this->GetNumberOfTextures();
  if (numTextures > 0)
    {
    for (int i = 0; i < numTextures; i++)
      {
      this->GetTextureAtIndex(i)->ReleaseGraphicsResources(win);
      }
    }

  this->Superclass::ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------------
void vtkOpenGLProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

}
