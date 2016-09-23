/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLCompositePainter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLCompositePainter.h"

#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGL.h"
#include "vtkOpenGLProperty.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkPolyDataPainter.h"

vtkStandardNewMacro(vtkOpenGLCompositePainter)

//-----------------------------------------------------------------------------
vtkOpenGLCompositePainter::vtkOpenGLCompositePainter()
{
  this->PushedOpenGLAttribs = false;
}

//-----------------------------------------------------------------------------
vtkOpenGLCompositePainter::~vtkOpenGLCompositePainter()
{
}

//-----------------------------------------------------------------------------
void vtkOpenGLCompositePainter::UpdateRenderingState(
  vtkRenderWindow* window, vtkProperty* property, RenderBlockState& state)
{
  if (state.Opacity.top() == state.RenderedOpacity &&
      state.AmbientColor.top() == state.RenderedAmbientColor &&
      state.DiffuseColor.top() == state.RenderedDiffuseColor &&
      state.SpecularColor.top() == state.RenderedSpecularColor)
  {
    bool something_overridden = (
      state.Opacity.size() > 1 || state.AmbientColor.size() > 1 ||
      state.DiffuseColor.size() > 1 || state.SpecularColor.size() > 1);
    if (something_overridden == this->PushedOpenGLAttribs)
    {
      // nothing to do.
      return;
    }
  }

  state.RenderedOpacity = state.Opacity.top();
  state.RenderedAmbientColor = state.AmbientColor.top();
  state.RenderedDiffuseColor = state.DiffuseColor.top();
  state.RenderedSpecularColor = state.SpecularColor.top();

  if (state.Opacity.size() == 1 &&
    state.AmbientColor.size() == 1 &&
    state.DiffuseColor.size() == 1 &&
    state.SpecularColor.size() == 1)
  {
    // We are returning to root state.
    if (this->PushedOpenGLAttribs)
    {
      glPopAttrib();
      this->PushedOpenGLAttribs = false;
      this->Information->Remove(vtkPolyDataPainter::DISABLE_SCALAR_COLOR());
    }
    else
    {
      vtkWarningMacro("State mismatch. UpdateRenderingState() isn't being called "
        "correctly.");
    }
  }
  else
  {
    if (!this->PushedOpenGLAttribs)
    {
      this->PushedOpenGLAttribs = true;
      glPushAttrib(GL_COLOR_BUFFER_BIT|GL_LIGHTING_BIT|GL_CURRENT_BIT|GL_ENABLE_BIT|
        GL_TEXTURE_BIT);

      // disable state
      glDisable(GL_ALPHA_TEST);
      glDisable(GL_COLOR_MATERIAL);

      // The following seems to overcome the color bleed when scalar coloring
      // with point-data with InterpolateScalarsBeforeMapping ON. The real cause
      // however, is some interactions with the depth-peeling code.
      // That needs to tracked down, rather than just hacking the logic here.
      // glBindTexture(GL_TEXTURE_2D, 0);

      if (state.AmbientColor.size() > 1 || state.DiffuseColor.size() > 1 || state.SpecularColor.size() > 1)
      {
        glDisable(GL_TEXTURE_2D);
        this->Information->Set(vtkPolyDataPainter::DISABLE_SCALAR_COLOR(), 1);
      }
      else
      {
        this->Information->Remove(vtkPolyDataPainter::DISABLE_SCALAR_COLOR());
      }
    }

    vtkOpenGLRenderWindow* windowGL = vtkOpenGLRenderWindow::SafeDownCast(window);
    vtkOpenGLProperty::SetMaterialProperties(
      static_cast<unsigned int>(GL_FRONT_AND_BACK),
      property->GetAmbient(), state.RenderedAmbientColor.GetData(),
      property->GetDiffuse(), state.RenderedDiffuseColor.GetData(),
      property->GetSpecular(), state.RenderedSpecularColor.GetData(),
      property->GetSpecularPower(),
      state.RenderedOpacity,
      windowGL);
  }
}

//-----------------------------------------------------------------------------
void vtkOpenGLCompositePainter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
