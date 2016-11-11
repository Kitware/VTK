/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkDepthPeelingPass.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDepthPeelingPass.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkProp.h"
#include "vtkRenderState.h"
#include "vtkRenderer.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"
#include <cassert>
#include <list>

#include "vtkOpenGLHelper.h"

// the 2D blending shaders we use
#include "vtkDepthPeelingPassIntermediateFS.h"
#include "vtkDepthPeelingPassFinalFS.h"
#include "vtkTextureObjectVS.h"

vtkStandardNewMacro(vtkDepthPeelingPass);
vtkCxxSetObjectMacro(vtkDepthPeelingPass,TranslucentPass,vtkRenderPass);

// ----------------------------------------------------------------------------
vtkDepthPeelingPass::vtkDepthPeelingPass()
{
  this->TranslucentPass=0;
  this->IsSupported=false;

  this->OcclusionRatio=0.0;
  this->MaximumNumberOfPeels=4;
  this->DepthPeelingHigherLayer=0;

  this->IntermediateBlendProgram = 0;
  this->FinalBlendProgram = 0;

  this->DepthZData = 0;
  this->OpaqueZTexture = NULL;
  this->TranslucentZTexture = NULL;
  this->OpaqueRGBATexture = NULL;
  this->TranslucentRGBATexture = NULL;
  this->CurrentRGBATexture = NULL;

  this->ViewportX = 0;
  this->ViewportY = 0;
  this->ViewportWidth = 100;
  this->ViewportHeight = 100;

}

// ----------------------------------------------------------------------------
vtkDepthPeelingPass::~vtkDepthPeelingPass()
{
  if(this->TranslucentPass!=0)
  {
    this->TranslucentPass->Delete();
  }
  delete this->DepthZData;
  if (this->OpaqueZTexture)
  {
    this->OpaqueZTexture->UnRegister(this);
    this->OpaqueZTexture = NULL;
  }
  if (this->TranslucentZTexture)
  {
    this->TranslucentZTexture->UnRegister(this);
    this->TranslucentZTexture = NULL;
  }
  if (this->OpaqueRGBATexture)
  {
    this->OpaqueRGBATexture->UnRegister(this);
    this->OpaqueRGBATexture = NULL;
  }
  if (this->TranslucentRGBATexture)
  {
    this->TranslucentRGBATexture->UnRegister(this);
    this->TranslucentRGBATexture = NULL;
  }
  if (this->CurrentRGBATexture)
  {
    this->CurrentRGBATexture->UnRegister(this);
    this->CurrentRGBATexture = NULL;
  }
}

//-----------------------------------------------------------------------------
// Description:
// Destructor. Delete SourceCode if any.
void vtkDepthPeelingPass::ReleaseGraphicsResources(vtkWindow *w)
{
  assert("pre: w_exists" && w!=0);

  if (this->FinalBlendProgram !=0)
  {
    this->FinalBlendProgram->ReleaseGraphicsResources(w);
    delete this->FinalBlendProgram;
    this->FinalBlendProgram = 0;
  }
  if (this->IntermediateBlendProgram !=0)
  {
    this->IntermediateBlendProgram->ReleaseGraphicsResources(w);
    delete this->IntermediateBlendProgram;
    this->IntermediateBlendProgram = 0;
  }
  if(this->TranslucentPass)
  {
    this->TranslucentPass->ReleaseGraphicsResources(w);
  }
  if (this->OpaqueZTexture)
  {
    this->OpaqueZTexture->ReleaseGraphicsResources(w);
  }
  if (this->TranslucentZTexture)
  {
    this->TranslucentZTexture->ReleaseGraphicsResources(w);
  }
  if (this->OpaqueRGBATexture)
  {
    this->OpaqueRGBATexture->ReleaseGraphicsResources(w);
  }
  if (this->TranslucentRGBATexture)
  {
    this->TranslucentRGBATexture->ReleaseGraphicsResources(w);
  }
  if (this->CurrentRGBATexture)
  {
    this->CurrentRGBATexture->ReleaseGraphicsResources(w);
  }
}

// ----------------------------------------------------------------------------
void vtkDepthPeelingPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "OcclusionRation: " << this->OcclusionRatio << endl;

  os << indent << "MaximumNumberOfPeels: " << this->MaximumNumberOfPeels
     << endl;

  os << indent << "TranslucentPass:";
  if(this->TranslucentPass!=0)
  {
    this->TranslucentPass->PrintSelf(os,indent);
  }
  else
  {
    os << "(none)" <<endl;
  }
}

vtkTextureObject *vtkDepthPeelingPassCreateTextureObject(
  vtkOpenGLRenderWindow *context, int width, int height,
  int numComponents, bool isDepth, void *initialData)
{
  vtkTextureObject *result = vtkTextureObject::New();
  result->SetContext(context);

  if (isDepth == true)
  {
    if (initialData)
    {
      result->CreateDepthFromRaw(
          width, height, vtkTextureObject::Float32, VTK_FLOAT, initialData);
    }
    else
    {
      result->AllocateDepth(width, height, vtkTextureObject::Float32);
    }
  }
  else
  {
    result->Allocate2D(width, height, numComponents, VTK_UNSIGNED_CHAR);
  }

  result->SetMinificationFilter(vtkTextureObject::Nearest);
  result->SetMagnificationFilter(vtkTextureObject::Nearest);
  result->SetWrapS(vtkTextureObject::ClampToEdge);
  result->SetWrapT(vtkTextureObject::ClampToEdge);
  result->SetWrapR(vtkTextureObject::ClampToEdge);
  return result;
}

void vtkDepthPeelingPass::BlendIntermediatePeels(
  vtkOpenGLRenderWindow *renWin,
  bool done)
{
  this->CurrentRGBATexture->CopyFromFrameBuffer(this->ViewportX, this->ViewportY,
    this->ViewportX, this->ViewportY,
    this->ViewportWidth, this->ViewportHeight);

  // take the TranslucentRGBA texture and blend it with the current frame buffer
  if (!this->IntermediateBlendProgram)
  {
    this->IntermediateBlendProgram = new vtkOpenGLHelper;
    std::string VSSource = vtkTextureObjectVS;
    std::string FSSource = vtkDepthPeelingPassIntermediateFS;
    std::string GSSource;
    this->IntermediateBlendProgram->Program =
      renWin->GetShaderCache()->ReadyShaderProgram(
        VSSource.c_str(),
        FSSource.c_str(),
        GSSource.c_str());
  }
  else
  {
    renWin->GetShaderCache()->ReadyShaderProgram(
      this->IntermediateBlendProgram->Program);
  }
  this->IntermediateBlendProgram->Program->SetUniformi(
    "translucentRGBATexture", this->TranslucentRGBATexture->GetTextureUnit());
  this->IntermediateBlendProgram->Program->SetUniformi(
    "currentRGBATexture", this->CurrentRGBATexture->GetTextureUnit());
  this->IntermediateBlendProgram->Program->SetUniformi(
    "lastpass", done ? 1 : 0);

  glDisable(GL_DEPTH_TEST);
  this->CurrentRGBATexture->CopyToFrameBuffer(0, 0,
         this->ViewportWidth-1, this->ViewportHeight-1,
         0, 0, this->ViewportWidth, this->ViewportHeight,
         this->IntermediateBlendProgram->Program,
         this->IntermediateBlendProgram->VAO);
}


void vtkDepthPeelingPass::BlendFinalPeel(vtkOpenGLRenderWindow *renWin)
{
  if (!this->FinalBlendProgram)
  {
    this->FinalBlendProgram = new vtkOpenGLHelper;
    std::string VSSource = vtkTextureObjectVS;
    std::string FSSource = vtkDepthPeelingPassFinalFS;
    std::string GSSource;
    this->FinalBlendProgram->Program =
      renWin->GetShaderCache()->ReadyShaderProgram(
        VSSource.c_str(),
        FSSource.c_str(),
        GSSource.c_str());
  }
  else
  {
    renWin->GetShaderCache()->ReadyShaderProgram(
      this->FinalBlendProgram->Program);
  }

  this->FinalBlendProgram->Program->SetUniformi(
    "translucentRGBATexture", this->TranslucentRGBATexture->GetTextureUnit());

  this->OpaqueRGBATexture->Activate();
  this->FinalBlendProgram->Program->SetUniformi(
    "opaqueRGBATexture", this->OpaqueRGBATexture->GetTextureUnit());

  this->OpaqueZTexture->Activate();
  this->FinalBlendProgram->Program->SetUniformi(
    "opaqueZTexture", this->OpaqueZTexture->GetTextureUnit());

  // blend in OpaqueRGBA
  glEnable(GL_DEPTH_TEST);
  glDepthFunc( GL_ALWAYS );
  this->OpaqueRGBATexture->CopyToFrameBuffer(0, 0,
         this->ViewportWidth-1, this->ViewportHeight-1,
         0, 0, this->ViewportWidth, this->ViewportHeight,
         this->FinalBlendProgram->Program,
         this->FinalBlendProgram->VAO);
  glDepthFunc( GL_LEQUAL );
}



// ----------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void vtkDepthPeelingPass::Render(const vtkRenderState *s)
{
  assert("pre: s_exists" && s!=0);

  this->NumberOfRenderedProps=0;

  if(this->TranslucentPass==0)
  {
    vtkWarningMacro(<<"No TranslucentPass delegate set. Nothing can be rendered.");
    return;
  }

  // Any prop to render?
  bool hasTranslucentPolygonalGeometry=false;
  int i=0;
  while(!hasTranslucentPolygonalGeometry && i<s->GetPropArrayCount())
  {
    hasTranslucentPolygonalGeometry=
      s->GetPropArray()[i]->HasTranslucentPolygonalGeometry()==1;
    ++i;
  }
  if(!hasTranslucentPolygonalGeometry)
  {
    return; // nothing to render.
  }

  // check driver support
  vtkOpenGLRenderWindow *renWin
    = vtkOpenGLRenderWindow::SafeDownCast(s->GetRenderer()->GetRenderWindow());

  // we need alpha planes
  int rgba[4];
  renWin->GetColorBufferSizes(rgba);

  if (rgba[3] < 8)
  {
    // just use alpha blending
    this->TranslucentPass->Render(s);
    return;
  }

  // Depth peeling.
  vtkRenderer *r=s->GetRenderer();

  if(s->GetFrameBuffer()==0)
  {
    // get the viewport dimensions
    r->GetTiledSizeAndOrigin(&this->ViewportWidth,&this->ViewportHeight,
                             &this->ViewportX,&this->ViewportY);
  }
  else
  {
    int size[2];
    s->GetWindowSize(size);
    this->ViewportWidth=size[0];
    this->ViewportHeight=size[1];
    this->ViewportX=0;
    this->ViewportY=0;
  }

  // has the size changed?
  if (this->OpaqueRGBATexture && (
      this->OpaqueRGBATexture->GetWidth() != static_cast<unsigned int>(this->ViewportWidth) ||
      this->OpaqueRGBATexture->GetHeight() != static_cast<unsigned int>(this->ViewportHeight)))
  {
    delete this->DepthZData;
    this->DepthZData = 0;

    this->OpaqueZTexture->UnRegister(this);
    this->OpaqueZTexture = 0;

    this->OpaqueRGBATexture->UnRegister(this);
    this->OpaqueRGBATexture = 0;

    this->TranslucentRGBATexture->UnRegister(this);
    this->TranslucentRGBATexture = 0;

    this->CurrentRGBATexture->UnRegister(this);
    this->CurrentRGBATexture = 0;
  }

  // create textures we need if not done already
  if (this->OpaqueRGBATexture == NULL)
  {
    this->OpaqueZTexture = vtkDepthPeelingPassCreateTextureObject(
      renWin, this->ViewportWidth, this->ViewportHeight, 1, true, NULL);
    this->OpaqueRGBATexture = vtkDepthPeelingPassCreateTextureObject(
      renWin, this->ViewportWidth, this->ViewportHeight, 4, false, NULL);
    this->TranslucentRGBATexture = vtkDepthPeelingPassCreateTextureObject(
      renWin, this->ViewportWidth, this->ViewportHeight, 4, false, NULL);
    this->CurrentRGBATexture = vtkDepthPeelingPassCreateTextureObject(
      renWin, this->ViewportWidth, this->ViewportHeight, 4, false, NULL);
    this->DepthZData = new std::vector<float>(this->ViewportWidth * this->ViewportHeight, 0.0);
  }

  this->TranslucentZTexture = vtkDepthPeelingPassCreateTextureObject(
    renWin, this->ViewportWidth, this->ViewportHeight, 1, true, &((*this->DepthZData)[0]));

  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glClearColor(0.0,0.0,0.0,0.0); // always clear to black
 // glClearDepth(static_cast<GLclampf>(1.0));
#ifdef GL_MULTISAMPLE
  GLboolean multiSampleStatus = glIsEnabled(GL_MULTISAMPLE);
  glDisable(GL_MULTISAMPLE);
#endif
  glDisable(GL_BLEND);

  // Get opaqueRGBA and opaqueZ
  this->OpaqueRGBATexture->CopyFromFrameBuffer(this->ViewportX, this->ViewportY,
    this->ViewportX, this->ViewportY,
    this->ViewportWidth, this->ViewportHeight);
  this->OpaqueRGBATexture->Deactivate(); // deactivate & unbind to save texture resources
  this->OpaqueZTexture->CopyFromFrameBuffer(this->ViewportX, this->ViewportY,
    this->ViewportX, this->ViewportY,
    this->ViewportWidth, this->ViewportHeight);

  this->TranslucentZTexture->Activate();
  this->OpaqueZTexture->Activate();

  // Setup property keys for actors:
  this->PreRender(s);

  // Enable the depth buffer (otherwise it's disabled for translucent geometry)
  assert("Render state valid." && s);
  int numProps = s->GetPropArrayCount();
  for (int j = 0; j < numProps; ++j)
  {
    vtkProp *prop = s->GetPropArray()[j];
    vtkInformation *info = prop->GetPropertyKeys();
    if (!info)
    {
      info = vtkInformation::New();
      prop->SetPropertyKeys(info);
      info->FastDelete();
    }
    info->Set(vtkOpenGLActor::GLDepthMaskOverride(), 1);
  }

  // Do render loop until complete
  unsigned int threshold=
    static_cast<unsigned int>(this->ViewportWidth*this->ViewportHeight*OcclusionRatio);

#if GL_ES_VERSION_2_0 != 1
  GLuint queryId;
  glGenQueries(1,&queryId);
#endif

  bool done = false;
  GLuint nbPixels = threshold + 1;
  int peelCount = 0;
  glDepthFunc( GL_LEQUAL );
  while(!done)
  {
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);

    // clear the zbuffer and color buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // render the translucent geometry
#if GL_ES_VERSION_2_0 != 1
    glBeginQuery(GL_SAMPLES_PASSED,queryId);
#endif

    // check if we are going to exceed the max number of peels or if we
    // exceeded the pixel threshold last time
    peelCount++;
    if ((this->MaximumNumberOfPeels && peelCount >= this->MaximumNumberOfPeels) ||
       nbPixels <= threshold)
    {
      done = true;
      // if so we do this last render using alpha blending for all
      // the stuff that is left
      glEnable(GL_BLEND);
      glDepthFunc( GL_ALWAYS );
    }
    this->TranslucentPass->Render(s);
    glDepthFunc( GL_LEQUAL );
    glDisable(GL_BLEND);

#if GL_ES_VERSION_2_0 != 1
    glEndQuery(GL_SAMPLES_PASSED);
    glGetQueryObjectuiv(queryId,GL_QUERY_RESULT,&nbPixels);
#endif
    // cerr << "Pass " << peelCount << " pixels Drawn " << nbPixels << "\n";

    // if something was drawn, blend it in
    if (nbPixels > 0)
    {
      // update translucentZ
      this->TranslucentZTexture->CopyFromFrameBuffer(this->ViewportX, this->ViewportY,
          this->ViewportX, this->ViewportY,
          this->ViewportWidth, this->ViewportHeight);


      // blend the last two peels together
      if (peelCount > 1)
      {
        this->BlendIntermediatePeels(renWin,done);
      }

      // update translucent RGBA
      this->TranslucentRGBATexture->CopyFromFrameBuffer(this->ViewportX, this->ViewportY,
        this->ViewportX, this->ViewportY,
        this->ViewportWidth, this->ViewportHeight);
    }
    else // if we drew nothing we are done
    {
      // if we drew nothing on the very first frame we still
      // need a valid texture to blend with so copy it
      if (peelCount == 1)
      {
        this->TranslucentRGBATexture->CopyFromFrameBuffer(this->ViewportX, this->ViewportY,
          this->ViewportX, this->ViewportY,
          this->ViewportWidth, this->ViewportHeight);
      }
      done = true;
    }
  }

//  std::cout << "Number of peels: " << peelCount << "\n";

  // unload the textures we are done with
  this->CurrentRGBATexture->Deactivate();
  this->TranslucentZTexture->UnRegister(this);
  this->TranslucentZTexture = 0;

  // do the final blend
  this->BlendFinalPeel(renWin);

#ifdef GL_MULTISAMPLE
   if(multiSampleStatus)
   {
      glEnable(GL_MULTISAMPLE);
   }
#endif

  this->OpaqueZTexture->Deactivate();
  // unload the last two textures
  this->TranslucentRGBATexture->Deactivate();
  this->OpaqueRGBATexture->Deactivate();

  // restore blending
  glEnable(GL_BLEND);

  this->PostRender(s);
  for (int j = 0; j < numProps; ++j)
  {
    vtkProp *prop = s->GetPropArray()[j];
    vtkInformation *info = prop->GetPropertyKeys();
    if (info)
    {
      info->Remove(vtkOpenGLActor::GLDepthMaskOverride());
    }
  }

  this->NumberOfRenderedProps = this->TranslucentPass->GetNumberOfRenderedProps();

  vtkOpenGLCheckErrorMacro("failed after Render");
}

//------------------------------------------------------------------------------
bool vtkDepthPeelingPass::ReplaceShaderValues(std::string &,
                                              std::string &,
                                              std::string &fragmentShader,
                                              vtkAbstractMapper *,
                                              vtkProp *)
{
  vtkShaderProgram::Substitute(
        fragmentShader, "//VTK::DepthPeeling::Dec",
        "uniform vec2 screenSize;\n"
        "uniform sampler2D opaqueZTexture;\n"
        "uniform sampler2D translucentZTexture;\n"
        );

  // Set gl_FragDepth if it isn't set already. It may have already been replaced
  // by the mapper, in which case the substitution will fail and the previously
  // set depth value will be used.
  vtkShaderProgram::Substitute(
        fragmentShader, "//VTK::Depth::Impl",
        "gl_FragDepth = gl_FragCoord.z;");

  // the .0000001 below is an epsilon.  It turns out that
  // graphics cards can render the same polygon two times
  // in a row with different z values. I suspect it has to
  // do with how rasterization of the polygon is broken up.
  // A different breakup across fragment shaders can result in
  // very slightly different z values for some of the pixels.
  // The end result is that with depth peeling, you can end up
  // counting/accumulating pixels of the same surface twice
  // simply due to this randomness in z values. So we introduce
  // an epsilon into the transparent test to require some
  // minimal z seperation between pixels
  vtkShaderProgram::Substitute(
        fragmentShader, "//VTK::DepthPeeling::Impl",
        "float odepth = texture2D(opaqueZTexture, gl_FragCoord.xy/screenSize).r;\n"
        "  if (gl_FragDepth >= odepth) { discard; }\n"
        "  float tdepth = texture2D(translucentZTexture, gl_FragCoord.xy/screenSize).r;\n"
        "  if (gl_FragDepth <= tdepth + .0000001) { discard; }\n"
        );

  return true;
}

//------------------------------------------------------------------------------
bool vtkDepthPeelingPass::SetShaderParameters(vtkShaderProgram *program,
                                              vtkAbstractMapper*, vtkProp*)
{
  program->SetUniformi("opaqueZTexture",
                       this->OpaqueZTexture->GetTextureUnit());
  program->SetUniformi("translucentZTexture",
                       this->TranslucentZTexture->GetTextureUnit());

  float screenSize[2] = { static_cast<float>(this->ViewportWidth),
                          static_cast<float>(this->ViewportHeight) };
  program->SetUniform2f("screenSize", screenSize);

  return true;
}
