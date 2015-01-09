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
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkObjectFactory.h"
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

#include "vtkglVBOHelper.h"

// the 2D blending shaders we use
#include "vtkDepthPeelingPassIntermediateFS.h"
#include "vtkDepthPeelingPassFinalFS.h"
#include "vtkTextureObjectVS.h"

vtkStandardNewMacro(vtkDepthPeelingPass);
vtkCxxSetObjectMacro(vtkDepthPeelingPass,TranslucentPass,vtkRenderPass);

vtkInformationKeyMacro(vtkDepthPeelingPass,OpaqueZTextureUnit,Integer);
vtkInformationKeyMacro(vtkDepthPeelingPass,TranslucentZTextureUnit,Integer);
vtkInformationKeyMacro(vtkDepthPeelingPass,DestinationSize,IntegerVector);

// ----------------------------------------------------------------------------
vtkDepthPeelingPass::vtkDepthPeelingPass()
{
  this->TranslucentPass=0;
  this->IsSupported=false;

  this->OcclusionRatio=0.0;
  this->MaximumNumberOfPeels=4;
  this->LastRenderingUsedDepthPeeling=false;
  this->DepthPeelingHigherLayer=0;

  this->IntermediateBlendProgram = 0;
  this->FinalBlendProgram = 0;

  this->DepthZData = 0;
  this->OpaqueZTexture = NULL;
  this->TranslucentZTexture = NULL;
  this->OpaqueRGBATexture = NULL;
  this->TranslucentRGBATexture = NULL;
  this->CurrentRGBATexture = NULL;
}

// ----------------------------------------------------------------------------
vtkDepthPeelingPass::~vtkDepthPeelingPass()
{
  if(this->TranslucentPass!=0)
    {
    this->TranslucentPass->Delete();
    }
  if (this->DepthZData)
    {
    delete this->DepthZData;
    this->DepthZData = 0;
    }
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

  if(this->TranslucentPass)
    {
    this->TranslucentPass->ReleaseGraphicsResources(w);
    }
}

// ----------------------------------------------------------------------------
void vtkDepthPeelingPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "OcclusionRation: " << this->OcclusionRatio << endl;

  os << indent << "MaximumNumberOfPeels: " << this->MaximumNumberOfPeels
     << endl;

  os << indent << "LastRenderingUsedDepthPeeling: ";
  if(this->LastRenderingUsedDepthPeeling)
    {
    os << "On" << endl;
    }
  else
    {
    os << "Off" << endl;
    }

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

void vtkDepthPeelingPass::BlendIntermediatePeels(vtkOpenGLRenderWindow *renWin)
{
  this->CurrentRGBATexture->CopyFromFrameBuffer(this->ViewportX, this->ViewportY,
    this->ViewportX, this->ViewportY,
    this->ViewportWidth, this->ViewportHeight);

  // take the TranslucentRGBA texture and blend it with the current frame buffer
  if (!this->IntermediateBlendProgram)
    {
    this->IntermediateBlendProgram = new vtkgl::CellBO;
    std::string VSSource = vtkTextureObjectVS;
    std::string FSSource = vtkDepthPeelingPassIntermediateFS;
    std::string GSSource;
    this->IntermediateBlendProgram->Program =
        renWin->GetShaderCache()->ReadyShader(VSSource.c_str(),
                                              FSSource.c_str(),
                                              GSSource.c_str());
    }
  else
    {
    renWin->GetShaderCache()->ReadyShader(this->IntermediateBlendProgram->Program);
    }
  this->IntermediateBlendProgram->Program->SetUniformi(
    "translucentRGBATexture", this->TranslucentRGBATexture->GetTextureUnit());
  this->IntermediateBlendProgram->Program->SetUniformi(
    "currentRGBATexture", this->CurrentRGBATexture->GetTextureUnit());

  glDisable(GL_DEPTH_TEST);
  this->CurrentRGBATexture->CopyToFrameBuffer(0, 0,
         this->ViewportWidth-1, this->ViewportHeight-1,
         0, 0, this->ViewportWidth, this->ViewportHeight,
         this->IntermediateBlendProgram->Program,
         &this->IntermediateBlendProgram->vao);
}


void vtkDepthPeelingPass::BlendFinalPeel(vtkOpenGLRenderWindow *renWin)
{
  if (!this->FinalBlendProgram)
    {
    this->FinalBlendProgram = new vtkgl::CellBO;
    std::string VSSource = vtkTextureObjectVS;
    std::string FSSource = vtkDepthPeelingPassFinalFS;
    std::string GSSource;
    this->FinalBlendProgram->Program =
        renWin->GetShaderCache()->ReadyShader(VSSource.c_str(),
                                              FSSource.c_str(),
                                              GSSource.c_str());
    }
  else
    {
    renWin->GetShaderCache()->ReadyShader(this->FinalBlendProgram->Program);
    }

  this->FinalBlendProgram->Program->SetUniformi(
    "translucentRGBATexture", this->TranslucentRGBATexture->GetTextureUnit());

  this->OpaqueRGBATexture->Activate();
  this->FinalBlendProgram->Program->SetUniformi(
    "opaqueRGBATexture", this->OpaqueRGBATexture->GetTextureUnit());

  // blend in OpaqueRGBA
  glDisable(GL_DEPTH_TEST);
  this->OpaqueRGBATexture->CopyToFrameBuffer(0, 0,
         this->ViewportWidth-1, this->ViewportHeight-1,
         0, 0, this->ViewportWidth, this->ViewportHeight,
         this->FinalBlendProgram->Program,
         &this->FinalBlendProgram->vao);
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

  // we need alpha planes
  GLint alphaBits;
  glGetIntegerv(GL_ALPHA_BITS, &alphaBits);
  if (alphaBits < 8)
    {
    // just use alpha blending
    this->TranslucentPass->Render(s);
    return;
    }

  // check driver support
  vtkOpenGLRenderWindow *renWin
    = vtkOpenGLRenderWindow::SafeDownCast(s->GetRenderer()->GetRenderWindow());

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
  if (this->OpaqueZTexture == NULL)
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

  // Have to be set before a call to UpdateTranslucentPolygonalGeometry()
  // because UpdateTranslucentPolygonalGeometry() will eventually call
  // vtkOpenGLActor::Render() that uses this flag.
  this->LastRenderingUsedDepthPeeling = true;
  this->SetLastRenderingUsedDepthPeeling(s->GetRenderer(), true);

  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
//  glClearColor(0.0,0.0,0.0,0.0); // always clear to black
 // glClearDepth(static_cast<GLclampf>(1.0));
#ifdef GL_MULTISAMPLE
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

  // set the required keys on the props for the txture units
  int destSize[2];
  destSize[0] = this->ViewportWidth;
  destSize[1] = this->ViewportHeight;

  int c = s->GetPropArrayCount();
  for (i = 0; i < c; i++)
    {
    vtkProp *p=s->GetPropArray()[i];
    vtkInformation *info = p->GetPropertyKeys();
    if (!info)
      {
      info = vtkInformation::New();
      p->SetPropertyKeys(info);
      info->Delete();
      }
    info->Set(vtkDepthPeelingPass::OpaqueZTextureUnit(),
      this->OpaqueZTexture->GetTextureUnit());
    info->Set(vtkDepthPeelingPass::TranslucentZTextureUnit(),
      this->TranslucentZTexture->GetTextureUnit());
    info->Set(vtkDepthPeelingPass::DestinationSize(),destSize,2);
    }

  // Do render loop until complete
  unsigned int threshold=
    static_cast<unsigned int>(this->ViewportWidth*this->ViewportHeight*OcclusionRatio);

#if GL_ES_VERSION_2_0 != 1
  GLuint queryId;
  glGenQueries(1,&queryId);
#endif

  bool done = false;
  GLuint nbPixels = 0;
  int peelCount = 0;
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

    this->TranslucentPass->Render(s);

    // update translucentZ
    this->TranslucentZTexture->CopyFromFrameBuffer(this->ViewportX, this->ViewportY,
        this->ViewportX, this->ViewportY,
        this->ViewportWidth, this->ViewportHeight);

#if GL_ES_VERSION_2_0 != 1
    glEndQuery(GL_SAMPLES_PASSED);
    glGetQueryObjectuiv(queryId,GL_QUERY_RESULT,&nbPixels);
    if (nbPixels <= threshold)
      {
      done = true;
      }
#endif
    peelCount++;
    if(this->MaximumNumberOfPeels && peelCount >= this->MaximumNumberOfPeels)
      {
      done = true;
      }
    //cerr << "Pass " << peelCount << " pixels Drawn " << nbPixels << "\n";

    // blend the last two peels together
    if (peelCount > 1)
      {
      this->BlendIntermediatePeels(renWin);
      }

    // update translucent RGBA
    this->TranslucentRGBATexture->CopyFromFrameBuffer(this->ViewportX, this->ViewportY,
      this->ViewportX, this->ViewportY,
      this->ViewportWidth, this->ViewportHeight);
    }

  // unload the textures we are done with
  this->CurrentRGBATexture->Deactivate();
  this->OpaqueZTexture->Deactivate();
  this->TranslucentZTexture->UnRegister(this);
  this->TranslucentZTexture = 0;

  // do the final blend
  this->BlendFinalPeel(renWin);

  // unload the last two textures
  this->TranslucentRGBATexture->Deactivate();
  this->OpaqueRGBATexture->Deactivate();

  // restore blending
  glEnable(GL_BLEND);

  this->NumberOfRenderedProps = this->TranslucentPass->GetNumberOfRenderedProps();

  vtkOpenGLCheckErrorMacro("failed after Render");
}
