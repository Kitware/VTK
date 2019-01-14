/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOptiXRendererNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOptiXRendererNode.h"

#include "vtkProperty.h"
#include "vtkCamera.h"
#include "vtkCollectionIterator.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkObjectFactory.h"
#include "vtkOptiXActorNode.h"
#include "vtkOptiXCameraNode.h"
#include "vtkOptiXLightNode.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkViewNodeCollection.h"
#include "vtkOptiXPtxLoader.h"

#include "CUDA/Light.h"

#include <optixu/optixpp_namespace.h>

#include <cmath>

#if 0
//debug includes
#include "vtkDataSetWriter.h"
#include "vtkImageImport.h"
#include "vtkSmartPointer.h"
#include "vtkWindowToImageFilter.h"
#include <unistd.h>
#endif

struct vtkOptiXRendererNodeInternals
{
  optix::Context       Context;
  optix::GeometryGroup GeometryGroup;
  optix::Buffer        FrameBuffer;
  optix::Buffer        DepthBuffer;
  optix::Buffer        LightBuffer;
};

vtkInformationKeyMacro(vtkOptiXRendererNode, SAMPLES_PER_PIXEL, Integer);
vtkInformationKeyMacro(vtkOptiXRendererNode, MAX_FRAMES, Integer);
vtkInformationKeyMacro(vtkOptiXRendererNode, AMBIENT_SAMPLES, Integer);

//============================================================================
vtkStandardNewMacro(vtkOptiXRendererNode);

//------------------------------------------------------------------------------
vtkOptiXRendererNode::vtkOptiXRendererNode()
  : Internals(new vtkOptiXRendererNodeInternals)
  , OptiXPtxLoader(vtkOptiXPtxLoader::New())
  , ContextValidated(false)
{
  this->Buffer = nullptr;
  this->ZBuffer = nullptr;
  this->NumActors = 0;
  this->ImageX = this->ImageY = -1;
}

//------------------------------------------------------------------------------
vtkOptiXRendererNode::~vtkOptiXRendererNode()
{
  // Delete children first - they may hold OptiX resources that have to be destroyed before the context
  if (this->Children)
  {
    this->Children->Delete();
    this->Children = 0;
  }
  delete[] this->Buffer;
  delete[] this->ZBuffer;

  if( this->Internals->Context )
  {
    this->Internals->Context->destroy();
  }

  delete this->Internals;
  this->OptiXPtxLoader->Delete();
}

//------------------------------------------------------------------------------
void vtkOptiXRendererNode::SetSamplesPerPixel(int value, vtkRenderer *renderer)
{
  if (!renderer)
  {
    return;
  }

  vtkInformation *info = renderer->GetInformation();
  info->Set(vtkOptiXRendererNode::SAMPLES_PER_PIXEL(), value);
}

//------------------------------------------------------------------------------
int vtkOptiXRendererNode::GetSamplesPerPixel(vtkRenderer *renderer)
{
  if (!renderer)
  {
    return 1;
  }
  vtkInformation *info = renderer->GetInformation();
  if (info && info->Has(vtkOptiXRendererNode::SAMPLES_PER_PIXEL()))
  {
    return (info->Get(vtkOptiXRendererNode::SAMPLES_PER_PIXEL()));
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkOptiXRendererNode::SetMaxFrames(int value, vtkRenderer *renderer)
{
  if (!renderer)
  {
    return;
  }
  vtkInformation *info = renderer->GetInformation();
  info->Set(vtkOptiXRendererNode::MAX_FRAMES(), value);
}

//------------------------------------------------------------------------------
int vtkOptiXRendererNode::GetMaxFrames(vtkRenderer *renderer)
{
  if (!renderer)
  {
    return 1;
  }
  vtkInformation *info = renderer->GetInformation();
  if (info && info->Has(vtkOptiXRendererNode::MAX_FRAMES()))
  {
    return (info->Get(vtkOptiXRendererNode::MAX_FRAMES()));
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkOptiXRendererNode::SetAmbientSamples(int value, vtkRenderer *renderer)
{
  if (!renderer)
  {
    return;
  }
  vtkInformation *info = renderer->GetInformation();
  info->Set(vtkOptiXRendererNode::AMBIENT_SAMPLES(), value);
}

//------------------------------------------------------------------------------
int vtkOptiXRendererNode::GetAmbientSamples(vtkRenderer *renderer)
{
  if (!renderer)
  {
    return 0;
  }
  vtkInformation *info = renderer->GetInformation();
  if (info && info->Has(vtkOptiXRendererNode::AMBIENT_SAMPLES()))
  {
    return (info->Get(vtkOptiXRendererNode::AMBIENT_SAMPLES()));
  }
  return 0;
}

//------------------------------------------------------------------------------
void vtkOptiXRendererNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
optix::ContextObj* vtkOptiXRendererNode::GetOptiXContext()
{
  return this->Internals->Context.get();
}

//------------------------------------------------------------------------------
optix::GeometryGroupObj* vtkOptiXRendererNode::GetOptiXGeometryGroup()
{
  return this->Internals->GeometryGroup.get();
}

//------------------------------------------------------------------------------
vtkOptiXPtxLoader* vtkOptiXRendererNode::GetOptiXPtxLoader()
{
  return this->OptiXPtxLoader;
}

//------------------------------------------------------------------------------
void vtkOptiXRendererNode::AddLight(const vtkopt::Light& light)
{
  this->Lights.push_back(light);
}

//------------------------------------------------------------------------------
void vtkOptiXRendererNode::Traverse(int operation)
{
  // do not override other passes
  if (operation != render)
  {
    this->Superclass::Traverse(operation);
    return;
  }

  this->Apply(operation,true);

  //TODO: this repeated traversal to find things of particular types
  //is bad, find something smarter
  vtkViewNodeCollection *nodes = this->GetChildren();
  vtkCollectionIterator *it = nodes->NewIterator();
  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
  {
    vtkOptiXCameraNode *child =
      vtkOptiXCameraNode::SafeDownCast(it->GetCurrentObject());
    if(child)
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
    vtkOptiXLightNode *child =
      vtkOptiXLightNode::SafeDownCast(it->GetCurrentObject());
    if (child)
    {
      child->Traverse(operation);
    }
    it->GoToNextItem();
  }
  this->Internals->LightBuffer->setSize( this->Lights.size() );
  if( !this->Lights.empty() )
  {
    memcpy(
      this->Internals->LightBuffer->map(),
      &this->Lights[0],
      this->Lights.size()*sizeof(vtkopt::Light) );
  }
  this->Internals->LightBuffer->unmap();

  //actors
  it->InitTraversal();
  //since we have to spatially sort everything
  //let's see if we can avoid that in the common case when
  //the objects have not changed. Note we also cache in actornodes
  //to reuse already created OptiX meshes
  unsigned int recent = 0;
  int numAct = 0; //catches removed actors
  while (!it->IsDoneWithTraversal())
  {
    vtkOptiXActorNode *child =
      vtkOptiXActorNode::SafeDownCast(it->GetCurrentObject());
    if (child)
    {
      numAct++;
      unsigned int mtime = child->GetMTime();
      if (mtime > recent)
      {
        recent = mtime;
      }
    }
    it->GoToNextItem();
  }

  //bool enable_cache = true; //turn off to force rebuilds for debugging
  if (/*!enable_cache ||*/
    (recent > this->RenderTime) ||
    (numAct != this->NumActors))
  {
    // Reset the geometry group
    this->GetOptiXGeometryGroup()->setChildCount(0);
    this->GetOptiXGeometryGroup()->getAcceleration()->markDirty();

    // Traverse actors to fill the group
    this->NumActors = numAct;
    it->InitTraversal();
    while (!it->IsDoneWithTraversal())
    {
      vtkOptiXActorNode *child =
        vtkOptiXActorNode::SafeDownCast(it->GetCurrentObject());
      if (child)
      {
        child->Traverse(operation);
      }
      it->GoToNextItem();
    }
    this->RenderTime = recent;
  }

  it->Delete();

  this->Apply(operation,false);
}

//------------------------------------------------------------------------------
void vtkOptiXRendererNode::Synchronize(bool prepass)
{
  if (prepass)
  {
    vtkRenderer *mine = vtkRenderer::SafeDownCast(this->GetRenderable());
    if (!mine)
    {
      return;
    }
    int *tmp = mine->GetSize();

    if (this->Internals->FrameBuffer.get() &&
      (tmp[0] != this->Size[0] || tmp[1] != this->Size[1])
      )
    {
      this->Internals->FrameBuffer->destroy();
      this->Internals->DepthBuffer->destroy();

      this->Internals->FrameBuffer = this->Internals->Context->createBuffer(
        RT_BUFFER_OUTPUT,
        RT_FORMAT_UNSIGNED_BYTE4,
        tmp[0],
        tmp[1]
      );

      this->Internals->DepthBuffer = this->Internals->Context->createBuffer(
        RT_BUFFER_OUTPUT,
        RT_FORMAT_FLOAT,
        tmp[0],
        tmp[1]
      );

      this->Internals->Context["frame_buffer"]->setBuffer(
        this->Internals->FrameBuffer);
      this->Internals->Context["depth_buffer"]->setBuffer(
        this->Internals->DepthBuffer);
    }
  }

  this->Superclass::Synchronize(prepass);
}

//------------------------------------------------------------------------------
void vtkOptiXRendererNode::Build(bool prepass)
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

//------------------------------------------------------------------------------
void vtkOptiXRendererNode::Render(bool prepass)
{
  if (prepass)
  {
    vtkRenderer *ren = vtkRenderer::SafeDownCast(this->GetRenderable());
    int *tmp = ren->GetSize();
    this->Size[0] = tmp[0];
    this->Size[1] = tmp[1];

    // Initialize optix
    if( !this->Internals->Context )
    {
      this->Internals->Context = optix::Context::create();

      // Load the programs as soon as the context has been created.
      this->OptiXPtxLoader->LoadPrograms(this->Internals->Context.get());

      this->Internals->Context->setRayTypeCount( 2 );
      this->Internals->Context->setEntryPointCount( 1 );

      this->Internals->FrameBuffer = this->Internals->Context->createBuffer(
        RT_BUFFER_OUTPUT,
        RT_FORMAT_UNSIGNED_BYTE4,
        this->Size[0],
        this->Size[1]
        );

      this->Internals->DepthBuffer = this->Internals->Context->createBuffer(
        RT_BUFFER_OUTPUT,
        RT_FORMAT_FLOAT,
        this->Size[0],
        this->Size[1]
        );

      this->Internals->LightBuffer= this->Internals->Context->createBuffer(
        RT_BUFFER_INPUT,
        RT_FORMAT_USER
        );
      this->Internals->LightBuffer->setElementSize( sizeof( vtkopt::Light ) );
      this->Internals->LightBuffer->setSize( 0 );

      this->Internals->GeometryGroup =
        this->Internals->Context->createGeometryGroup();
      this->Internals->GeometryGroup->setAcceleration(
        this->Internals->Context->createAcceleration( "Trbvh", "Bvh" ) );

      this->Internals->Context[ "frame_buffer" ]->setBuffer(
        this->Internals->FrameBuffer );
      this->Internals->Context[ "depth_buffer" ]->setBuffer(
        this->Internals->DepthBuffer );
      this->Internals->Context[ "lights"       ]->setBuffer(
        this->Internals->LightBuffer );
      this->Internals->Context[ "top_object"   ]->set(
        this->Internals->GeometryGroup );

      this->Cached_useShadows = ren->GetUseShadows();
      this->Internals->Context[ "shadows_enabled" ]->setInt(this->Cached_useShadows);

      this->Cached_samplesPerPixel = this->GetSamplesPerPixel(ren);
      int sqrtSamples = vtkMath::Max<int>(sqrt(this->Cached_samplesPerPixel), 1);
      this->Internals->Context[ "sqrt_num_samples" ]->setInt(sqrtSamples);

      this->Cached_AOSamples = this->GetAmbientSamples(ren);
      this->Internals->Context["num_ambient_samples"]->setInt(this->Cached_AOSamples);
      this->Internals->Context["ambient_occlusion_dist"]->setFloat(1.0e20f);

      this->Internals->Context->setMissProgram(0,
        this->OptiXPtxLoader->MissProgram);

      this->Cached_bgColor[0] = 0.0f;
      this->Cached_bgColor[1] = 0.0f;
      this->Cached_bgColor[2] = 0.0f;
      optix::float3 bgColor =
        optix::make_float3(
          this->Cached_bgColor[0],
          this->Cached_bgColor[1],
          this->Cached_bgColor[2]);
      this->Internals->Context->getMissProgram(0)["bg_color"]->setFloat(bgColor);
    }
    else
    {
      double *bg = ren->GetBackground();

      if (this->Cached_bgColor[0] != bg[0] ||
        this->Cached_bgColor[1] != bg[1] ||
        this->Cached_bgColor[2] != bg[2])
      {
        const optix::float3 bgColor = optix::make_float3(bg[0], bg[1], bg[2]);
        this->Internals->Context->getMissProgram(0)["bg_color"]->setFloat(bgColor);
        memcpy(this->Cached_bgColor, bg, 3*sizeof(float));
      }

      if (this->Cached_useShadows != ren->GetUseShadows())
      {
        this->Cached_useShadows = ren->GetUseShadows();
        this->Internals->Context["shadows_enabled"]->setInt(this->Cached_useShadows);
      }

      if (this->Cached_samplesPerPixel != this->GetSamplesPerPixel(ren))
      {
        this->Cached_samplesPerPixel = this->GetSamplesPerPixel(ren);
        int sqrtSamples = vtkMath::Max<int>(sqrt(this->Cached_samplesPerPixel), 1);
        this->Internals->Context["sqrt_num_samples"]->setInt(sqrtSamples);
      }

      if( this->Cached_AOSamples != this->GetAmbientSamples(ren) )
      {
        this->Cached_AOSamples = this->GetAmbientSamples(ren);
        this->Internals->Context["num_ambient_samples"]->setInt(this->Cached_AOSamples);
      }
    }
  }
  else
  {
    // Validate only after all nodes have completed the prepass
    // so all context subcomponents have been initialized
    if (!this->ContextValidated)
    {
      this->Internals->Context->validate();
      this->ContextValidated = true;
    }

    // DO RENDER
    this->Internals->Context->launch( 0, this->Size[0], this->Size[1] );

    if (this->ImageX != this->Size[0] || this->ImageY != this->Size[1])
    {
      this->ImageX = this->Size[0];
      this->ImageY = this->Size[1];
      delete[] this->Buffer;
      this->Buffer = new unsigned char[this->Size[0]*this->Size[1]*4];
      delete[] this->ZBuffer;
      this->ZBuffer = new float[this->Size[0]*this->Size[1]];
    }

    memcpy( this->Buffer, this->Internals->FrameBuffer->map(),
      this->Size[0]*this->Size[1]*4 );
    this->Internals->FrameBuffer->unmap();

    vtkCamera *cam = vtkRenderer::SafeDownCast(this->Renderable)->
      GetActiveCamera();
    double *clipValues = cam->GetClippingRange();
    double clipMin = clipValues[0];
    double clipMax = clipValues[1];
    double clipDiv = 1.0 / (clipMax - clipMin);

    const void *Z = this->Internals->DepthBuffer->map();
    float *s = (float *)Z;
    float *d = this->ZBuffer;
    for (int i = 0; i < (this->Size[0] * this->Size[1]); i++, s++, d++)
    {
      *d = (*s<clipMin ? 1.0 : (*s - clipMin) * clipDiv);
    }
    this->Internals->DepthBuffer->unmap();
  }
}

//------------------------------------------------------------------------------
void vtkOptiXRendererNode::WriteLayer(unsigned char *buffer, float *Z,
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
