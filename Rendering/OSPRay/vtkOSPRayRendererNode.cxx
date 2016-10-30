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

#ifdef _WIN32
#define _USE_MATH_DEFINES
#endif

#include "vtkOSPRayRendererNode.h"

#include "vtkCamera.h"
#include "vtkCollectionIterator.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationStringKey.h"
#include "vtkMath.h"
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

#include <algorithm>
#include <cmath>

namespace ospray {
  namespace opengl {

    //code borrowed from ospray::modules::opengl to facilitate updating
    //and linking
    //todo: use ospray's copy instead of this
    inline osp::vec3f operator*(const osp::vec3f &a, const osp::vec3f &b)
    {
      return osp::vec3f{a.x*b.x, a.y*b.y, a.z*b.z};
    }
    inline osp::vec3f operator*(const osp::vec3f &a, float b)
    {
      return osp::vec3f{a.x*b, a.y*b, a.z*b};
    }
    inline osp::vec3f operator/(const osp::vec3f &a, float b)
    {
      return osp::vec3f{a.x/b, a.y/b, a.z/b};
    }
    inline osp::vec3f operator*(float b, const osp::vec3f &a)
    {
      return osp::vec3f{a.x*b, a.y*b, a.z*b};
    }
    inline osp::vec3f operator*=(osp::vec3f a, float b)
    {
      return osp::vec3f{a.x*b, a.y*b, a.z*b};
    }
    inline osp::vec3f operator-(const osp::vec3f& a, const osp::vec3f& b)
    {
      return osp::vec3f{a.x-b.x, a.y-b.y, a.z-b.z};
    }
    inline osp::vec3f operator+(const osp::vec3f& a, const osp::vec3f& b)
    {
      return osp::vec3f{a.x+b.x, a.y+b.y, a.z+b.z};
    }
    inline osp::vec3f cross(const osp::vec3f &a, const osp::vec3f &b)
    {
      return osp::vec3f{a.y*b.z-a.z*b.y,
          a.z*b.x-a.x*b.z,
          a.x*b.y-a.y*b.x};
    }

    inline float dot(const osp::vec3f &a, const osp::vec3f &b)
    {
      return a.x*b.x+a.y*b.y+a.z*b.z;
    }
    inline osp::vec3f normalize(const osp::vec3f &v)
    {
      return v/sqrtf(dot(v,v));
    }

    /*! \brief Compute and return OpenGL depth values from the depth component of the given
      OSPRay framebuffer, using parameters of the current OpenGL context and assuming a
      perspective projection.

      This function automatically determines the parameters of the OpenGL perspective
      projection and camera direction / up vectors. It assumes these values match those
      provided to OSPRay (fovy, aspect, camera direction / up vectors). It then maps the
      OSPRay depth buffer and transforms it to OpenGL depth values according to the OpenGL
      perspective projection.

      The OSPRay frame buffer object must have been constructed with the OSP_FB_DEPTH flag.
    */
    OSPTexture2D getOSPDepthTextureFromOpenGLPerspective(const double &fovy,
                                                         const double &aspect,
                                                         const double &zNear,
                                                         const double &zFar,
                                                         const osp::vec3f &_cameraDir,
                                                         const osp::vec3f &_cameraUp,
                                                         const float *glDepthBuffer,
                                                         float *ospDepthBuffer,
                                                         const size_t &glDepthBufferWidth,
                                                         const size_t &glDepthBufferHeight)
    {
      osp::vec3f cameraDir = (osp::vec3f&)_cameraDir;
      osp::vec3f cameraUp = (osp::vec3f&)_cameraUp;
      // this should later be done in ISPC...

      // transform OpenGL depth to linear depth
      for (size_t i=0; i<glDepthBufferWidth*glDepthBufferHeight; i++)
      {
        const double z_n = 2.0 * glDepthBuffer[i] - 1.0;
        ospDepthBuffer[i] = 2.0 * zNear * zFar / (zFar + zNear - z_n * (zFar - zNear));
        if (vtkMath::IsNan(ospDepthBuffer[i]))
        {
          ospDepthBuffer[i] = FLT_MAX;
        }
      }

      // transform from orthogonal Z depth to ray distance t
      osp::vec3f dir_du = normalize(cross(cameraDir, cameraUp));
      osp::vec3f dir_dv = normalize(cross(dir_du, cameraDir));

      const float imagePlaneSizeY = 2.f * tanf(fovy/2.f * M_PI/180.f);
      const float imagePlaneSizeX = imagePlaneSizeY * aspect;

      dir_du *= imagePlaneSizeX;
      dir_dv *= imagePlaneSizeY;

      const osp::vec3f dir_00 = cameraDir - .5f * dir_du - .5f * dir_dv;

      for (size_t j=0; j<glDepthBufferHeight; j++)
      {
        for (size_t i=0; i<glDepthBufferWidth; i++)
        {
          const osp::vec3f dir_ij = normalize(dir_00 +
                                              float(i)/float(glDepthBufferWidth-1) * dir_du +
                                              float(j)/float(glDepthBufferHeight-1) * dir_dv);

          const float t = ospDepthBuffer[j*glDepthBufferWidth+i] / dot(cameraDir, dir_ij);
          ospDepthBuffer[j*glDepthBufferWidth+i] = t;
        }
      }

      // nearest texture filtering required for depth textures -- we don't want interpolation of depth values...
      osp::vec2i texSize = {static_cast<int>(glDepthBufferWidth),
                            static_cast<int>(glDepthBufferHeight)};
      OSPTexture2D depthTexture = ospNewTexture2D((osp::vec2i&)texSize,
                                                  OSP_TEXTURE_R32F, ospDepthBuffer,
                                                  OSP_TEXTURE_FILTER_NEAREST);

      return depthTexture;
    }
  }
}

vtkInformationKeyMacro(vtkOSPRayRendererNode, SAMPLES_PER_PIXEL, Integer);
vtkInformationKeyMacro(vtkOSPRayRendererNode, MAX_FRAMES, Integer);
vtkInformationKeyMacro(vtkOSPRayRendererNode, AMBIENT_SAMPLES, Integer);
vtkInformationKeyMacro(vtkOSPRayRendererNode, COMPOSITE_ON_GL, Integer);
vtkInformationKeyMacro(vtkOSPRayRendererNode, RENDERER_TYPE, String);

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
  this->ComputeDepth = true;
  this->OFrameBuffer = nullptr;
  this->ImageX = this->ImageY = -1;
  this->Accumulate = false;
  this->CompositeOnGL = false;
}

//----------------------------------------------------------------------------
vtkOSPRayRendererNode::~vtkOSPRayRendererNode()
{
  delete[] this->Buffer;
  delete[] this->ZBuffer;
  if (this->OModel)
  {
    ospRelease((OSPModel)this->OModel);
  }
  if (this->ORenderer)
  {
    ospRelease((OSPRenderer)this->ORenderer);
  }
  if (this->OFrameBuffer)
  {
    ospRelease(this->OFrameBuffer);
  }
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
void vtkOSPRayRendererNode::SetRendererType(std::string name, vtkRenderer *renderer)
{
  if (!renderer)
  {
    return;
  }
  vtkInformation *info = renderer->GetInformation();
  info->Set(vtkOSPRayRendererNode::RENDERER_TYPE(), name);
}

//----------------------------------------------------------------------------
std::string vtkOSPRayRendererNode::GetRendererType(vtkRenderer *renderer)
{
  if (!renderer)
  {
    return std::string("scivis");
  }
  vtkInformation *info = renderer->GetInformation();
  if (info && info->Has(vtkOSPRayRendererNode::RENDERER_TYPE()))
  {
    return (info->Get(vtkOSPRayRendererNode::RENDERER_TYPE()));
  }
  return std::string("scivis");
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
void vtkOSPRayRendererNode::SetCompositeOnGL(int value, vtkRenderer *renderer)
{
  if (!renderer)
  {
    return;
  }
  vtkInformation *info = renderer->GetInformation();
  info->Set(vtkOSPRayRendererNode::COMPOSITE_ON_GL(), value);
}

//----------------------------------------------------------------------------
int vtkOSPRayRendererNode::GetCompositeOnGL(vtkRenderer *renderer)
{
  if (!renderer)
  {
    return 0;
  }
  vtkInformation *info = renderer->GetInformation();
  if (info && info->Has(vtkOSPRayRendererNode::COMPOSITE_ON_GL()))
  {
    return (info->Get(vtkOSPRayRendererNode::COMPOSITE_ON_GL()));
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
  vtkMTimeType recent = 0;
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
      recent = std::max(recent, child->GetMTime());
    }
    if (vchild)
    {
      numAct++;
      recent = std::max(recent, vchild->GetMTime());
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
    //ospRelease((OSPModel)this->OModel);
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
    ospCommit(oModel);
    ospSetObject(oRenderer,"model", oModel);
    ospCommit(oRenderer);
  }
  else
  {
    oModel = (OSPModel)this->OModel;
  }
  it->Delete();

  this->Apply(operation,false);
}

//----------------------------------------------------------------------------
void vtkOSPRayRendererNode::Invalidate(bool prepass)
{
  if (prepass)
  {
      this->RenderTime = 0;
  }
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
  vtkRenderer *ren = vtkRenderer::SafeDownCast(this->GetRenderable());
  if (!ren)
  {
    return;
  }

  if (prepass)
  {
    OSPRenderer oRenderer = NULL;
    static std::string previousType;
    std::string type = this->GetRendererType(static_cast<vtkRenderer*>(this->Renderable));
    if (!this->ORenderer || previousType != type)
    {
      this->Traverse(invalidate);
      ospRelease((osp::Renderer*)this->ORenderer);
      oRenderer = (osp::Renderer*)ospNewRenderer(type.c_str());
      this->ORenderer = oRenderer;
      previousType = type;
    }
    else
    {
      oRenderer = (osp::Renderer*)this->ORenderer;
    }
    ospCommit(this->ORenderer);

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
    this->CompositeOnGL =
      this->GetCompositeOnGL(static_cast<vtkRenderer*>(this->Renderable));

    double *bg = ren->GetBackground();
    ospSet3f(oRenderer,"bgColor", bg[0], bg[1], bg[2]);
  }
  else
  {
    OSPRenderer oRenderer = (osp::Renderer*)this->ORenderer;
    ospCommit(oRenderer);

    osp::vec2i isize = {this->Size[0], this->Size[1]};
    if (this->ImageX != this->Size[0] || this->ImageY != this->Size[1])
    {
      this->ImageX = this->Size[0];
      this->ImageY = this->Size[1];
      this->OFrameBuffer = ospNewFrameBuffer
        (isize,
         OSP_FB_RGBA8,
         OSP_FB_COLOR | (this->ComputeDepth? OSP_FB_DEPTH : 0) | (this->Accumulate? OSP_FB_ACCUM : 0));
      ospSet1f(this->OFrameBuffer, "gamma", 1.0f);
      ospCommit(this->OFrameBuffer);
      ospFrameBufferClear
        (this->OFrameBuffer,
         OSP_FB_COLOR | (this->ComputeDepth ? OSP_FB_DEPTH : 0) | (this->Accumulate ? OSP_FB_ACCUM : 0));
      delete[] this->Buffer;
      this->Buffer = new unsigned char[this->Size[0]*this->Size[1]*4];
      delete[] this->ZBuffer;
      this->ZBuffer = new float[this->Size[0]*this->Size[1]];
      if (this->CompositeOnGL)
      {
        ODepthBuffer = new float[this->Size[0] * this->Size[1]];
      }
    }
    else if (this->Accumulate)
    {
      ospFrameBufferClear
        (this->OFrameBuffer,
         OSP_FB_COLOR | (this->ComputeDepth ? OSP_FB_DEPTH : 0) | (this->Accumulate ? OSP_FB_ACCUM : 0));
    }

    vtkCamera *cam = vtkRenderer::SafeDownCast(this->Renderable)->GetActiveCamera();

    ospSet1i(oRenderer, "backgroundEnabled", ren->GetErase());
    if (this->CompositeOnGL)
    {
      OSPTexture2D glDepthTex=NULL;
      /*
      if (glDepthTex)
        {
        ospRelease(glDepthTex);
        }
      */
      vtkRenderWindow *rwin =
      vtkRenderWindow::SafeDownCast(ren->GetVTKWindow());
      int viewportX, viewportY;
      int viewportWidth, viewportHeight;
      ren->GetTiledSizeAndOrigin(&viewportWidth,&viewportHeight,
        &viewportX,&viewportY);
      rwin->GetZbufferData(
        viewportX,  viewportY,
        viewportX+viewportWidth-1,
        viewportY+viewportHeight-1,
        this->GetZBuffer());

      double zNear, zFar;
      double fovy, aspect;
      fovy = cam->GetViewAngle();
      aspect = double(viewportWidth)/double(viewportHeight);
      cam->GetClippingRange(zNear,zFar);
      double camUp[3];
      double camDir[3];
      cam->GetViewUp(camUp);
      cam->GetFocalPoint(camDir);
      osp::vec3f cameraUp = {static_cast<float>(camUp[0]),
                             static_cast<float>(camUp[1]),
                             static_cast<float>(camUp[2])};
      osp::vec3f cameraDir = {static_cast<float>(camDir[0]),
                              static_cast<float>(camDir[1]),
                              static_cast<float>(camDir[2])};
      double cameraPos[3];
      cam->GetPosition(cameraPos);
      cameraDir.x -= cameraPos[0];
      cameraDir.y -= cameraPos[1];
      cameraDir.z -= cameraPos[2];
      cameraDir = ospray::opengl::normalize(cameraDir);

      glDepthTex = ospray::opengl::getOSPDepthTextureFromOpenGLPerspective
        (fovy, aspect, zNear, zFar,
         (osp::vec3f&)cameraDir, (osp::vec3f&)cameraUp,
         this->GetZBuffer(), ODepthBuffer, viewportWidth, viewportHeight);

      ospSetObject(oRenderer, "maxDepthTexture", glDepthTex);
    }
    else
    {
      ospSetObject(oRenderer, "maxDepthTexture", 0);
    }
    ospCommit(oRenderer);

    ospRenderFrame(this->OFrameBuffer, oRenderer,
      OSP_FB_COLOR | (this->ComputeDepth ? OSP_FB_DEPTH : 0) | (this->Accumulate ? OSP_FB_ACCUM : 0));

    const void* rgba = ospMapFrameBuffer(this->OFrameBuffer, OSP_FB_COLOR);
    memcpy((void*)this->Buffer, rgba, this->Size[0]*this->Size[1]*sizeof(char)*4);
    ospUnmapFrameBuffer(rgba, this->OFrameBuffer);

    if (this->ComputeDepth)
    {
      double *clipValues = cam->GetClippingRange();
      double clipMin = clipValues[0];
      double clipMax = clipValues[1];
      double clipDiv = 1.0 / (clipMax - clipMin);

      const void *Z = ospMapFrameBuffer(this->OFrameBuffer, OSP_FB_DEPTH);
      float *s = (float *)Z;
      float *d = this->ZBuffer;
      for (int i = 0; i < (this->Size[0]*this->Size[1]); i++, s++, d++)
      {
        *d = (*s<clipMin? 1.0 : (*s - clipMin) * clipDiv);
      }
      ospUnmapFrameBuffer(Z, this->OFrameBuffer);
    }
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
          if (this->CompositeOnGL)
          {
            //ospray is cooperating with GL (osprayvolumemapper)
            unsigned char a = (*(iptr+2));
            float A = (float)a/255;
            for (int h = 0; h<3; h++)
            {
              *optr = (unsigned char)(((float)*iptr)*(1-A) + ((float)*optr)*(A));
              optr++; iptr++;
            }
            optr++;
            iptr++;
          }
          else
          {
            //ospray owns all layers in window
            *optr++ = *iptr++;
            *optr++ = *iptr++;
            *optr++ = *iptr++;
            *optr++ = *iptr++;
          }
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
