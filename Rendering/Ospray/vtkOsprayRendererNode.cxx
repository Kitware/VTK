/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOsprayRendererNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOsprayRendererNode.h"

#include "vtkCamera.h"
#include "vtkCollectionIterator.h"
#include "vtkObjectFactory.h"
#include "vtkOsprayActorNode.h"
#include "vtkOsprayCameraNode.h"
#include "vtkOsprayLightNode.h"
#include "vtkRenderer.h"
#include "vtkViewNodeCollection.h"

#include "ospray/ospray.h"
#include "include/ospray/version.h"

#include <cmath>

#if 0
//debug includes
#include "vtkDataSetWriter.h"
#include "vtkImageImport.h"
#include "vtkSmartPointer.h"
#include "vtkWindowToImageFilter.h"
#include <unistd.h>
#endif

int vtkOsprayRendererNode::maxframes = 1;
int vtkOsprayRendererNode::doshadows=0;
int vtkOsprayRendererNode::spp=1;
//============================================================================
vtkStandardNewMacro(vtkOsprayRendererNode);

//----------------------------------------------------------------------------
vtkOsprayRendererNode::vtkOsprayRendererNode()
{
  this->Buffer = NULL;
  this->ZBuffer = NULL;
  this->OModel = NULL;
  this->ORenderer = NULL;
}

//----------------------------------------------------------------------------
vtkOsprayRendererNode::~vtkOsprayRendererNode()
{
  delete[] this->Buffer;
  delete[] this->ZBuffer;
  ospRelease((OSPModel)this->OModel);
  ospRelease((OSPRenderer)this->ORenderer);
}

//----------------------------------------------------------------------------
void vtkOsprayRendererNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkOsprayRendererNode::Render()
{
  OSPRenderer oRenderer = NULL;
  if (!this->ORenderer)
    {
    ospRelease((osp::Renderer*)this->ORenderer);
#if  OSPRAY_VERSION_MAJOR == 0 && OSPRAY_VERSION_MINOR < 9
    oRenderer = (osp::Renderer*)ospNewRenderer("obj");
#else
    oRenderer = (osp::Renderer*)ospNewRenderer("scivis");
#endif
    this->ORenderer = oRenderer;
    }
  else
    {
    oRenderer = (osp::Renderer*)this->ORenderer;
    }

  if (doshadows)
    {
    ospSet1i(oRenderer,"shadowsEnabled",1);
    }
  else
    {
    ospSet1i(oRenderer,"shadowsEnabled",0);
    }
  ospSet1i(oRenderer,"spp",spp);

  ospSet3f(oRenderer,"bgColor",
           this->Background[0],
           this->Background[1],
           this->Background[2]);

  //camera
  //TODO: this repeated traversal to find things of particular types
  //is bad, find something smarter
  vtkViewNodeCollection *nodes = this->GetChildren();
  vtkCollectionIterator *it = nodes->NewIterator();
  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    vtkOsprayCameraNode *child =
      vtkOsprayCameraNode::SafeDownCast(it->GetCurrentObject());
    if (child)
      {
      OSPCamera oCamera = ospNewCamera("perspective");
      ospSetObject(oRenderer,"camera", oCamera);
      child->ORender(this->TiledSize, oCamera);
      ospCommit(oCamera);
      ospRelease(oCamera);
      break;
      }
    it->GoToNextItem();
    }

  //lights
  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    vtkOsprayLightNode *child =
      vtkOsprayLightNode::SafeDownCast(it->GetCurrentObject());
    if (child)
      {
      child->ORender(oRenderer);
      break;
      }
    it->GoToNextItem();
    }

  //actors
  OSPModel oModel=NULL;
  it->InitTraversal();
  unsigned int recent = 0;
  while (!it->IsDoneWithTraversal())
    {
    vtkOsprayActorNode *child =
      vtkOsprayActorNode::SafeDownCast(it->GetCurrentObject());
    if (child)
      {
      if (child->RenderTime > recent)
        {
        recent = child->RenderTime;
        }
      }
    it->GoToNextItem();
    }
  if (recent > this->RenderTime || !this->OModel)
    {
    ospRelease((OSPModel)this->OModel);
    oModel = ospNewModel();
    it->InitTraversal();
    while (!it->IsDoneWithTraversal())
      {
      vtkOsprayActorNode *child =
        vtkOsprayActorNode::SafeDownCast(it->GetCurrentObject());
      if (child)
        {
        child->ORender(oRenderer, oModel);
        }
      it->GoToNextItem();
      }
    this->OModel = oModel;
    this->RenderTime.Modified();
    ospSetObject(oRenderer,"model", oModel);
    ospCommit(oModel);
    }
  else
    {
    oModel = (OSPModel)this->OModel;
    }
  it->Delete();
  ospCommit(oRenderer);

#if OSPRAY_VERSION_MINOR < 9
  osp::vec2i isize(this->Size[0], this->Size[1]);
#else
  osp::vec2i isize = {this->Size[0], this->Size[1]};
#endif
  OSPFrameBuffer osp_framebuffer = ospNewFrameBuffer
    (isize,
     OSP_RGBA_I8, OSP_FB_COLOR | OSP_FB_DEPTH | OSP_FB_ACCUM);
  ospFrameBufferClear(osp_framebuffer, OSP_FB_COLOR|OSP_FB_DEPTH|OSP_FB_ACCUM);
  for (int i = 0; i < maxframes; i++)
    {
    ospRenderFrame(osp_framebuffer, oRenderer, OSP_FB_COLOR|OSP_FB_DEPTH|OSP_FB_ACCUM);
    }
  const void* rgba = ospMapFrameBuffer(osp_framebuffer, OSP_FB_COLOR);
  delete[] this->Buffer;
  this->Buffer = new unsigned char[this->Size[0]*this->Size[1]*4];
  memcpy((void*)this->Buffer, rgba, this->Size[0]*this->Size[1]*sizeof(char)*4);
  ospUnmapFrameBuffer(rgba, osp_framebuffer);

  vtkCamera *cam = vtkRenderer::SafeDownCast(this->Renderable)->
    GetActiveCamera();
  double *clipValues = cam->GetClippingRange();
  double viewAngle = cam->GetViewAngle();
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

  /*
  int pid = getpid();
  char fname[100];
  vtkSmartPointer<vtkImageImport> wiff1 = vtkSmartPointer<vtkImageImport>::New();
  wiff1->CopyImportVoidPointer(this->Buffer,(int)(this->Size[0]*this->Size[1]*sizeof(char)*4));
  wiff1->SetDataScalarTypeToUnsignedChar();
  wiff1->SetNumberOfScalarComponents(4);
  wiff1->SetWholeExtent(0,this->Size[0]-1,0,this->Size[1]-1,0,0);
  wiff1->SetDataExtentToWholeExtent();

  vtkSmartPointer<vtkDataSetWriter> dw1 = vtkSmartPointer<vtkDataSetWriter>::New();
  dw1->SetInputConnection(wiff1->GetOutputPort());
  sprintf(fname, "color_%d.vtk", pid);
  cerr << fname << endl;
  dw1->SetFileName(fname);
  dw1->Write();

  vtkSmartPointer<vtkImageImport> wiff2 = vtkSmartPointer<vtkImageImport>::New();
  wiff2->CopyImportVoidPointer(this->ZBuffer,(int)(this->Size[0]*this->Size[1]*sizeof(float)));
  wiff2->SetDataScalarTypeToFloat();
  wiff2->SetNumberOfScalarComponents(1);
  wiff2->SetWholeExtent(0,this->Size[0]-1,0,this->Size[1]-1,0,0);
  wiff2->SetDataExtentToWholeExtent();

  vtkSmartPointer<vtkDataSetWriter> dw2 = vtkSmartPointer<vtkDataSetWriter>::New();
  dw2->SetInputConnection(wiff2->GetOutputPort());
  sprintf(fname, "depth_%d.vtk", pid);
  cerr << fname << endl;
  dw2->SetFileName(fname);
  dw2->Write();
  */
}

//----------------------------------------------------------------------------
void vtkOsprayRendererNode::WriteLayer(unsigned char *buffer, float *Z,
                                       int buffx, int buffy)
{
  //TODO: have to keep depth information around in VTK side for
  //parallel compositing to work too.
  unsigned char *iptr = this->Buffer;
  float *zptr = this->ZBuffer;
  unsigned char *optr = buffer;
  float *ozptr = Z;
  if (this->Layer == 0)
    {
    for (int i = 0; i < buffx && i < this->Size[0]; i++)
      {
      for (int j = 0; j < buffy && i < this->Size[1]; j++)
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
    for (int i = 0; i < buffx && i < this->Size[0]; i++)
      {
      for (int j = 0; j < buffy && i < this->Size[1]; j++)
        {
        if (*zptr<1.0)
          {
          *optr++ = *iptr++;
          *optr++ = *iptr++;
          *optr++ = *iptr++;
          *optr++ = *iptr++;
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
