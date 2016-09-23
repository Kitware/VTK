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
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLTexture.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"

#include "vtkOpenGL.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLExtensionManager.h"

#include "vtkgl.h" // vtkgl namespace

vtkStandardNewMacro(vtkOpenGLScalarsToColorsPainter);

//-----------------------------------------------------------------------------
vtkOpenGLScalarsToColorsPainter::vtkOpenGLScalarsToColorsPainter()
{
  this->InternalColorTexture = 0;
  this->AlphaBitPlanes = -1;
  this->AcquiredGraphicsResources = false;
  this->SupportsSeparateSpecularColor = false;
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
  this->AcquiredGraphicsResources = false;
  this->Superclass::ReleaseGraphicsResources(win);
}

//-----------------------------------------------------------------------------
int vtkOpenGLScalarsToColorsPainter::GetPremultiplyColorsWithAlpha(
  vtkActor* actor)
{
  // Use the AlphaBitPlanes member, which should already be initialized. If
  // not, initialize it.
  GLint alphaBits;
  if (this->AlphaBitPlanes < 0)
  {
    glGetIntegerv(GL_ALPHA_BITS, &alphaBits);
    this->AlphaBitPlanes = (int)alphaBits;
  }
  alphaBits = (GLint)this->AlphaBitPlanes;

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
  vtkOpenGLClearErrorMacro();

  // If we have not yet set the alpha bit planes, do it based on the
  // render window so we're not querying GL in the middle of render.
  if (this->AlphaBitPlanes < 0)
  {
    vtkOpenGLRenderer* oglRenderer = vtkOpenGLRenderer::SafeDownCast(renderer);
    if (oglRenderer)
    {
      vtkOpenGLRenderWindow* context = vtkOpenGLRenderWindow::SafeDownCast(
        oglRenderer->GetRenderWindow());
      if (context)
      {
        this->AlphaBitPlanes = context->GetAlphaBitPlanes();
      }
    }
  }

  // check for separate specular color support
  if (!this->AcquiredGraphicsResources)
  {
    vtkOpenGLRenderer *oglRenderer = vtkOpenGLRenderer::SafeDownCast(renderer);
    if (oglRenderer)
    {
      vtkOpenGLRenderWindow *oglRenderWindow =
        vtkOpenGLRenderWindow::SafeDownCast(oglRenderer->GetRenderWindow());
      if (oglRenderWindow)
      {
        vtkOpenGLExtensionManager *oglExtensionManager =
          oglRenderWindow->GetExtensionManager();

        if (oglExtensionManager)
        {
          this->SupportsSeparateSpecularColor =
            (oglExtensionManager->ExtensionSupported("GL_EXT_separate_specular_color") != 0);
        }
      }
    }

    this->AcquiredGraphicsResources = true;
  }

  // If we are coloring by texture, then load the texture map.
  if (this->ColorTextureMap)
  {
    if (this->InternalColorTexture == 0)
    {
      this->InternalColorTexture = vtkOpenGLTexture::New();
      this->InternalColorTexture->RepeatOff();
      this->InternalColorTexture->EdgeClampOn();
    }
    this->InternalColorTexture->SetInputData(this->ColorTextureMap);


    this->LastWindow = renderer->GetRenderWindow();
  }
  else if (this->LastWindow)
  {
    // release the texture.
    this->ReleaseGraphicsResources(this->LastWindow);
    this->LastWindow = 0;
  }

  vtkProperty* prop = actor->GetProperty();
  // if we are doing vertex colors then set lmcolor to adjust
  // the current materials ambient and diffuse values using
  // vertex color commands otherwise tell it not to.
  glDisable(GL_COLOR_MATERIAL);

  if (this->UsingScalarColoring)
  {
    GLenum lmcolorMode;
    if (this->ScalarMaterialMode == VTK_MATERIALMODE_DEFAULT)
    {
      lmcolorMode = (prop->GetAmbient() > prop->GetDiffuse()) ?
        GL_AMBIENT : GL_DIFFUSE;
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

    glColorMaterial(GL_FRONT_AND_BACK, lmcolorMode);
    glEnable(GL_COLOR_MATERIAL);

    if (this->ColorTextureMap)
    {
      this->InternalColorTexture->Load(renderer);
      // Keep the surface color from interacting with color loaded from texture.
      // We don't simple use (GL_TEXTURE_ENV_MODE, GL_REPLACE) since that
      // implies all the lighting colors are lost too i.e. no diffuse
      // highlights.
      glColor3f(1.0, 1.0, 1.0);
    }
  }

  int pre_multiplied_by_alpha =  this->GetPremultiplyColorsWithAlpha(actor);

  if (pre_multiplied_by_alpha || this->InterpolateScalarsBeforeMapping)
  {
    // save the blend function.
    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT);
  }

  // We colors were premultiplied by alpha then we change the blending
  // function to one that will compute correct blended destination alpha
  // value, otherwise we stick with the default.
  if (pre_multiplied_by_alpha)
  {
    // the following function is not correct with textures because there are
    // not premultiplied by alpha.
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  }

  if (this->InterpolateScalarsBeforeMapping && this->SupportsSeparateSpecularColor)
  {
    // Turn on color sum and separate specular color so specular works
    // with texturing.
    glEnable(vtkgl::COLOR_SUM);
    glLightModeli(vtkgl::LIGHT_MODEL_COLOR_CONTROL, vtkgl::SEPARATE_SPECULAR_COLOR);
  }

  this->Superclass::RenderInternal(renderer, actor, typeflags, forceCompileOnly);

  if (this->InterpolateScalarsBeforeMapping && this->SupportsSeparateSpecularColor)
  {
    glLightModeli(vtkgl::LIGHT_MODEL_COLOR_CONTROL, vtkgl::SINGLE_COLOR);
    glDisable(vtkgl::COLOR_SUM);
  }

  if (pre_multiplied_by_alpha || this->InterpolateScalarsBeforeMapping)
  {
    // restore the blend function & lights
    glPopAttrib();
  }
  vtkOpenGLCheckErrorMacro("failed after RenderInternal");
}

//-----------------------------------------------------------------------------
void vtkOpenGLScalarsToColorsPainter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AlphaBitPlanes: " << this->AlphaBitPlanes << endl;
}
