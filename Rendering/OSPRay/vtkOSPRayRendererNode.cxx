/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSPRayRendererNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOSPRayRendererNode.h"

#include "vtkCamera.h"
#include "vtkCollectionIterator.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkObjectFactory.h"
#include "vtkOSPRayActorNode.h"
#include "vtkOSPRayCameraNode.h"
#include "vtkOSPRayLightNode.h"
#include "vtkOSPRayVolumeNode.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkViewNodeCollection.h"

#include "ospray/ospray.h"
#include "ospray/version.h"

#include <cmath>

#if 0
//debug includes
#include "vtkDataSetWriter.h"
#include "vtkImageImport.h"
#include "vtkSmartPointer.h"
#include "vtkWindowToImageFilter.h"
#include <unistd.h>
#endif

vtkInformationKeyMacro(vtkOSPRayRendererNode, SAMPLES_PER_PIXEL, Integer);
vtkInformationKeyMacro(vtkOSPRayRendererNode, MAX_FRAMES, Integer);
vtkInformationKeyMacro(vtkOSPRayRendererNode, AMBIENT_SAMPLES, Integer);

//============================================================================
vtkStandardNewMacro(vtkOSPRayRendererNode);

//----------------------------------------------------------------------------
vtkOSPRayRendererNode::vtkOSPRayRendererNode()
{
  this->Buffer = NULL;
  this->ZBuffer = NULL;
  this->OModel = NULL;
  this->ORenderer = NULL;
  this->NumActors = 0;
  this->MaxDepth = NULL;
}

//----------------------------------------------------------------------------
vtkOSPRayRendererNode::~vtkOSPRayRendererNode()
{
  delete[] this->Buffer;
  delete[] this->ZBuffer;
  ospRelease((OSPModel)this->OModel);
  ospRelease((OSPRenderer)this->ORenderer);
}

//----------------------------------------------------------------------------
void vtkOSPRayRendererNode::SetSamplesPerPixel(int value, vtkRenderer *renderer)
{
  if (!renderer)
    {
    return;
    }
  vtkInformation *info = renderer->GetInformation();
  info->Set(vtkOSPRayRendererNode::SAMPLES_PER_PIXEL(), value);
}

//----------------------------------------------------------------------------
int vtkOSPRayRendererNode::GetSamplesPerPixel(vtkRenderer *renderer)
{
  if (!renderer)
    {
    return 1;
    }
  vtkInformation *info = renderer->GetInformation();
  if (info && info->Has(vtkOSPRayRendererNode::SAMPLES_PER_PIXEL()))
    {
    return (info->Get(vtkOSPRayRendererNode::SAMPLES_PER_PIXEL()));
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkOSPRayRendererNode::SetMaxFrames(int value, vtkRenderer *renderer)
{
  if (!renderer)
    {
    return;
    }
  vtkInformation *info = renderer->GetInformation();
  info->Set(vtkOSPRayRendererNode::MAX_FRAMES(), value);
}

//----------------------------------------------------------------------------
int vtkOSPRayRendererNode::GetMaxFrames(vtkRenderer *renderer)
{
  if (!renderer)
    {
    return 1;
    }
  vtkInformation *info = renderer->GetInformation();
  if (info && info->Has(vtkOSPRayRendererNode::MAX_FRAMES()))
    {
    return (info->Get(vtkOSPRayRendererNode::MAX_FRAMES()));
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkOSPRayRendererNode::SetAmbientSamples(int value, vtkRenderer *renderer)
{
  if (!renderer)
    {
    return;
    }
  vtkInformation *info = renderer->GetInformation();
  info->Set(vtkOSPRayRendererNode::AMBIENT_SAMPLES(), value);
}

//----------------------------------------------------------------------------
int vtkOSPRayRendererNode::GetAmbientSamples(vtkRenderer *renderer)
{
  if (!renderer)
    {
    return 0;
    }
  vtkInformation *info = renderer->GetInformation();
  if (info && info->Has(vtkOSPRayRendererNode::AMBIENT_SAMPLES()))
    {
    return (info->Get(vtkOSPRayRendererNode::AMBIENT_SAMPLES()));
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkOSPRayRendererNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkOSPRayRendererNode::Traverse(int operation)
{
  // do not override other passes
  if (operation != render)
    {
    this->Superclass::Traverse(operation);
    return;
    }

  this->Apply(operation,true);

  OSPRenderer oRenderer = (osp::Renderer*)this->ORenderer;
  if (this->MaxDepth)
    {
    OSPTexture2D glDepthTex = static_cast<OSPTexture2D>(this->MaxDepth);
    ospSetObject(oRenderer, "maxDepthTexture", glDepthTex);
    //ospSet1i(oRenderer, "backgroundEnabled",ren->GetErase());
    }

  //camera
  //TODO: this repeated traversal to find things of particular types
  //is bad, find something smarter
  vtkViewNodeCollection *nodes = this->GetChildren();
  vtkCollectionIterator *it = nodes->NewIterator();
  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    vtkOSPRayCameraNode *child =
      vtkOSPRayCameraNode::SafeDownCast(it->GetCurrentObject());
    if (child)
      {
      child->Traverse(operation);
      break;
      }
    it->GoToNextItem();
    }

  //lights
  this->Lights.clear();
  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    vtkOSPRayLightNode *child =
      vtkOSPRayLightNode::SafeDownCast(it->GetCurrentObject());
    if (child)
      {
      child->Traverse(operation);
      }
    it->GoToNextItem();
    }
  OSPData lightArray = ospNewData(this->Lights.size(), OSP_OBJECT,
    (this->Lights.size()?&this->Lights[0]:NULL), 0);
  ospSetData(oRenderer, "lights", lightArray);

  //actors
  OSPModel oModel=NULL;
  it->InitTraversal();
  //since we have to spatially sort everything
  //let's see if we can avoid that in the common case when
  //the objects have not changed. Note we also cache in actornodes
  //to reuse already created ospray meshes
  unsigned int recent = 0;
  int numAct = 0; //catches removed actors
  while (!it->IsDoneWithTraversal())
    {
    vtkOSPRayActorNode *child =
      vtkOSPRayActorNode::SafeDownCast(it->GetCurrentObject());
    vtkOSPRayVolumeNode *vchild =
      vtkOSPRayVolumeNode::SafeDownCast(it->GetCurrentObject());
    if (child)
      {
      numAct++;
      unsigned int mtime = child->GetMTime();
      if (mtime > recent)
        {
        recent = mtime;
        }
      }
    if (vchild)
      {
      numAct++;
      unsigned int mtime = vchild->GetMTime();
      if (mtime > recent)
        {
        recent = mtime;
        }
      }

    it->GoToNextItem();
    }

  bool enable_cache = true; //turn off to force rebuilds for debugging
  if (!this->OModel ||
      !enable_cache ||
      (recent > this->RenderTime) ||
      (numAct != this->NumActors))
    {
    this->NumActors = numAct;
    ospRelease((OSPModel)this->OModel);
    oModel = ospNewModel();
    this->OModel = oModel;
    it->InitTraversal();
    while (!it->IsDoneWithTraversal())
      {
      vtkOSPRayActorNode *child =
        vtkOSPRayActorNode::SafeDownCast(it->GetCurrentObject());
      if (child)
        {
        child->Traverse(operation);
        }
      vtkOSPRayVolumeNode *vchild =
        vtkOSPRayVolumeNode::SafeDownCast(it->GetCurrentObject());
      if (vchild)
        {
        vchild->Traverse(operation);
        }
      it->GoToNextItem();
      }
    this->RenderTime = recent;
    ospSetObject(oRenderer,"model", oModel);
    ospCommit(oModel);
    }
  else
    {
    oModel = (OSPModel)this->OModel;
    }
  it->Delete();

  this->Apply(operation,false);
}

//----------------------------------------------------------------------------
void vtkOSPRayRendererNode::Build(bool prepass)
{
  if (prepass)
    {
    vtkRenderer *aren = vtkRenderer::SafeDownCast(this->Renderable);
    // make sure we have a camera
    if ( !aren->IsActiveCameraCreated() )
      {
      aren->ResetCamera();
      }
    }
  this->Superclass::Build(prepass);
}

//----------------------------------------------------------------------------
void vtkOSPRayRendererNode::Render(bool prepass)
{
  if (prepass)
    {
    OSPRenderer oRenderer = NULL;
    if (!this->ORenderer)
      {
      ospRelease((osp::Renderer*)this->ORenderer);
      oRenderer = (osp::Renderer*)ospNewRenderer("scivis");
      this->ORenderer = oRenderer;
      }
    else
      {
      oRenderer = (osp::Renderer*)this->ORenderer;
      }

    vtkRenderer *ren = vtkRenderer::SafeDownCast(this->GetRenderable());
    int *tmp = ren->GetSize();
    this->Size[0] = tmp[0];
    this->Size[1] = tmp[1];
    if (ren->GetUseShadows())
      {
      ospSet1i(oRenderer,"shadowsEnabled",1);
      }
    else
      {
      ospSet1i(oRenderer,"shadowsEnabled",0);
      }
    ospSet1i(oRenderer,"aoSamples",
             this->GetAmbientSamples(static_cast<vtkRenderer*>(this->Renderable)));
    ospSet1i(oRenderer,"spp",
             this->GetSamplesPerPixel(static_cast<vtkRenderer*>(this->Renderable)));

    double *bg = ren->GetBackground();
    ospSet3f(oRenderer,"bgColor", bg[0], bg[1], bg[2]);
    }
  else
    {
    OSPRenderer oRenderer = (osp::Renderer*)this->ORenderer;
    ospCommit(oRenderer);

    osp::vec2i isize = {this->Size[0], this->Size[1]};
    OSPFrameBuffer osp_framebuffer = ospNewFrameBuffer
      (isize,
  #if OSPRAY_VERSION_MAJOR < 1 && OSPRAY_VERSION_MINOR < 10 && OSPRAY_VERSION_PATCH < 2
       OSP_RGBA_I8, OSP_FB_COLOR | OSP_FB_DEPTH | OSP_FB_ACCUM);
  #else
       OSP_FB_RGBA8, OSP_FB_COLOR | OSP_FB_DEPTH | OSP_FB_ACCUM);
  #endif
    ospSet1f(osp_framebuffer, "gamma", 1.0f);
    ospCommit(osp_framebuffer);
    ospFrameBufferClear(osp_framebuffer, OSP_FB_COLOR|OSP_FB_DEPTH|OSP_FB_ACCUM);
    for (int i = 0; i < this->GetMaxFrames(static_cast<vtkRenderer*>(this->Renderable)); i++)
      {
      ospRenderFrame(osp_framebuffer, oRenderer,
                     OSP_FB_COLOR|OSP_FB_DEPTH|OSP_FB_ACCUM);
      }
    const void* rgba = ospMapFrameBuffer(osp_framebuffer, OSP_FB_COLOR);
    delete[] this->Buffer;
    this->Buffer = new unsigned char[this->Size[0]*this->Size[1]*4];
    memcpy((void*)this->Buffer, rgba, this->Size[0]*this->Size[1]*sizeof(char)*4);
    ospUnmapFrameBuffer(rgba, osp_framebuffer);

    vtkCamera *cam = vtkRenderer::SafeDownCast(this->Renderable)->
      GetActiveCamera();
    double *clipValues = cam->GetClippingRange();
    double clipMin = clipValues[0];
    double clipMax = clipValues[1];
    double clipDiv = 1.0 / (clipMax - clipMin);

    const void *Z = ospMapFrameBuffer(osp_framebuffer, OSP_FB_DEPTH);
    delete[] this->ZBuffer;
    this->ZBuffer = new float[this->Size[0]*this->Size[1]];
    float *s = (float *)Z;
    float *d = this->ZBuffer;
    /*
    float minS = 1000.0;
    float maxS = -1000.0;
    float minD = 1000.0;
    float maxD = -10000.0;
    */
    for (int i = 0; i < (this->Size[0]*this->Size[1]); i++, s++, d++)
      {
      *d = (*s<clipMin? 1.0 : (*s - clipMin) * clipDiv);
      /*
      if (*d < minD) minD = *d;
      if (*d > maxD) maxD = *d;
      if (*s < minS) minS = *s;
      if (*s > maxS) maxS = *s;
      */
      }
    /*
    cerr << "CmM" << clipMin << "," << clipMax << "\t";
    cerr << "SmM " << minS << "," << maxS << "\t";
    cerr << "DmM " << minD << "," << maxD << endl;
    */
    ospUnmapFrameBuffer(Z, osp_framebuffer);

    ospRelease(osp_framebuffer);
    }
}

//----------------------------------------------------------------------------
void vtkOSPRayRendererNode::WriteLayer(unsigned char *buffer, float *Z,
                                       int buffx, int buffy, int layer)
{
  if (layer == 0)
    {
    for (int j = 0; j < buffy && j < this->Size[1]; j++)
      {
      unsigned char *iptr = this->Buffer + j*this->Size[0]*4;
      float *zptr = this->ZBuffer + j*this->Size[0];
      unsigned char *optr = buffer + j*buffx*4;
      float *ozptr = Z +  j*buffx;
      for (int i = 0; i < buffx && i < this->Size[0]; i++)
        {
        *optr++ = *iptr++;
        *optr++ = *iptr++;
        *optr++ = *iptr++;
        *optr++ = *iptr++;
        *ozptr++ = *zptr;
        zptr++;
        }
      }
    }
  else
    {
    //TODO: blending needs to be optional
    for (int j = 0; j < buffy && j < this->Size[1]; j++)
      {
      unsigned char *iptr = this->Buffer + j*this->Size[0]*4;
      float *zptr = this->ZBuffer + j*this->Size[0];
      unsigned char *optr = buffer + j*buffx*4;
      float *ozptr = Z +  j*buffx;
      for (int i = 0; i < buffx && i < this->Size[0]; i++)
        {
        if (*zptr<1.0)
          {
          unsigned char a = (*(iptr+2));
          float A = (float)a/255;
          for (int h = 0; h<3; h++)
            {
            *optr = (unsigned char)(((float)*iptr)*(1-A) + ((float)*optr)*(A));
            optr++; iptr++;
            }
          optr++;
          iptr++;
          *ozptr = *zptr;
          }
        else
          {
          optr+=4;
          iptr+=4;
          }
        ozptr++;
        zptr++;
        }
      }
    }
}

//------------------------------------------------------------------------------
void vtkOSPRayRendererNode::SetMaxDepthTexture(void *dt)
{
  //TODO: streamline this
  OSPTexture2D DT = static_cast<OSPTexture2D>(dt);
  //delete this->MaxDepth;
  this->MaxDepth = DT;
}
