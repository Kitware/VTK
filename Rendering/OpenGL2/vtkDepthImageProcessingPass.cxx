/*=========================================================================

   Program: ParaView
   Module:    vtkDepthImageProcessingPass.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
/*----------------------------------------------------------------------
Acknowledgement:
This algorithm is the result of joint work by Electricité de France,
CNRS, Collège de France and Université J. Fourier as part of the
Ph.D. thesis of Christian BOUCHENY.
------------------------------------------------------------------------*/

#include "vtkDepthImageProcessingPass.h"
#include "vtkObjectFactory.h"
#include <cassert>
#include "vtkRenderState.h"
#include "vtkRenderer.h"
#include "vtkFrameBufferObject.h"
#include "vtkTextureObject.h"
#include "vtkOpenGLRenderWindow.h"

#include "vtkPixelBufferObject.h"
#include "vtkCamera.h"
#include "vtkMath.h"

vtkCxxSetObjectMacro(vtkDepthImageProcessingPass,DelegatePass,vtkRenderPass);

// ----------------------------------------------------------------------------
vtkDepthImageProcessingPass::vtkDepthImageProcessingPass()
{
  this->DelegatePass = 0;
  this->Width = 0;
  this->Height = 0;
  this->W = 0;
  this->H = 0;
  this->ExtraPixels = 0;
}

// ----------------------------------------------------------------------------
vtkDepthImageProcessingPass::~vtkDepthImageProcessingPass()
{
  if(this->DelegatePass!=0)
  {
    this->DelegatePass->Delete();
    this->DelegatePass=0;
  }
}

// ----------------------------------------------------------------------------
void vtkDepthImageProcessingPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "DelegatePass:";
  if(this->DelegatePass!=0)
  {
    this->DelegatePass->PrintSelf(os,indent);
  }
  else
  {
    os << "(none)" <<endl;
  }
}
// ----------------------------------------------------------------------------
// Description:
// Render delegate with a image of different dimensions than the
// original one.
// \pre s_exists: s!=0
// \pre fbo_exists: fbo!=0
// \pre fbo_has_context: fbo->GetContext()!=0
// \pre target_exists: target!=0
// \pre target_has_context: target->GetContext()!=0
void vtkDepthImageProcessingPass::RenderDelegate(const vtkRenderState *s,
                                            int width,
                                            int height,
                                            int newWidth,
                                            int newHeight,
                                            vtkFrameBufferObject *fbo,
                                            vtkTextureObject *colortarget,
                                            vtkTextureObject *depthtarget)
{
  assert("pre: s_exists" && s!=0);
  assert("pre: fbo_exists" && fbo!=0);
  assert("pre: fbo_has_context" && fbo->GetContext()!=0);
  assert("pre: colortarget_exists" && colortarget!=0);
  assert("pre: colortarget_has_context" && colortarget->GetContext()!=0);
  assert("pre: depthtarget_exists" && depthtarget!=0);
  assert("pre: depthtarget_has_context" && depthtarget->GetContext()!=0);

  vtkRenderer *r=s->GetRenderer();
  vtkRenderState s2(r);
  s2.SetPropArrayAndCount(s->GetPropArray(),s->GetPropArrayCount());

  // Adapt camera to new window size
  vtkCamera *savedCamera=r->GetActiveCamera();
  savedCamera->Register(this);
  vtkCamera *newCamera=vtkCamera::New();
  newCamera->DeepCopy(savedCamera);

  r->SetActiveCamera(newCamera);

  if(newCamera->GetParallelProjection())
  {
    newCamera->SetParallelScale(
      newCamera->GetParallelScale()*newHeight/static_cast<double>(height));
  }
  else
  {
    double large;
    double small;
    if(newCamera->GetUseHorizontalViewAngle())
    {
      large=newWidth;
      small=width;
    }
    else
    {
      large=newHeight;
      small=height;

    }
    double angle=vtkMath::RadiansFromDegrees(newCamera->GetViewAngle());
    angle = 2.0*atan(tan(angle/2.0)*large/static_cast<double>(small));

    newCamera->SetViewAngle(vtkMath::DegreesFromRadians(angle));
  }

  s2.SetFrameBuffer(fbo);

  fbo->SetNumberOfRenderTargets(1);
  fbo->SetColorBuffer(0,colortarget);

  // because the same FBO can be used in another pass but with several color
  // buffers, force this pass to use 1, to avoid side effects from the
  // render of the previous frame.
  fbo->SetActiveBuffer(0);

  fbo->SetDepthBuffer(depthtarget);
  fbo->StartNonOrtho(newWidth,newHeight,false);

  // 2. Delegate render in FBO
  //glEnable(GL_DEPTH_TEST);
  this->DelegatePass->Render(&s2);
  this->NumberOfRenderedProps+=
    this->DelegatePass->GetNumberOfRenderedProps();

  newCamera->Delete();
  r->SetActiveCamera(savedCamera);
  savedCamera->UnRegister(this);
}

// ----------------------------------------------------------------------------
// Description:
// Read window size from parent
// \pre s_exists: s!=0
//
void vtkDepthImageProcessingPass::ReadWindowSize(const vtkRenderState* s)
{
    assert("pre: s_exists" && s!=0);

    vtkFrameBufferObject *fbo=vtkFrameBufferObject::SafeDownCast
      (s->GetFrameBuffer());
    vtkRenderer *r = s->GetRenderer();
    if(fbo==0)
    {
      r->GetTiledSize(&this->Width,&this->Height);
    }
    else
    {
      int size[2];
      fbo->GetLastSize(size);
      this->Width=size[0];
      this->Height=size[1];
    }
}

// ----------------------------------------------------------------------------
// Description:
// Release graphics resources and ask components to release their own
// resources.
// \pre w_exists: w!=0
void vtkDepthImageProcessingPass::ReleaseGraphicsResources(vtkWindow *w)
{
  assert("pre: w_exists" && w!=0);
  if(this->DelegatePass!=0)
  {
    this->DelegatePass->ReleaseGraphicsResources(w);
  }
}
