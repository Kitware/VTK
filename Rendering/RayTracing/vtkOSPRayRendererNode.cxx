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

#include "vtkAbstractVolumeMapper.h"
#include "vtkBoundingBox.h"
#include "vtkCamera.h"
#include "vtkCollectionIterator.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationStringKey.h"
#include "vtkLight.h"
#include "vtkMapper.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOSPRayActorNode.h"
#include "vtkOSPRayCameraNode.h"
#include "vtkOSPRayLightNode.h"
#include "vtkOSPRayVolumeNode.h"
#include "vtkOSPRayMaterialHelpers.h"
#include "vtkOSPRayMaterialLibrary.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkTexture.h"
#include "vtkTransform.h"
#include "vtkViewNodeCollection.h"
#include "vtkVolume.h"
#include "vtkVolumeCollection.h"
#include "vtkWeakPointer.h"

#include "RTWrapper/RTWrapper.h"

#include <algorithm>
#include <cmath>
#include <map>

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
    OSPTexture getOSPDepthTextureFromOpenGLPerspective(const double &fovy,
                                                         const double &aspect,
                                                         const double &zNear,
                                                         const double &zFar,
                                                         const osp::vec3f &_cameraDir,
                                                         const osp::vec3f &_cameraUp,
                                                         const float *glDepthBuffer,
                                                         float *ospDepthBuffer,
                                                         const size_t &glDepthBufferWidth,
                                                         const size_t &glDepthBufferHeight,
                                                         RTW::Backend *backend)
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
      OSPTexture depthTexture = vtkOSPRayMaterialHelpers::NewTexture2D(backend, (osp::vec2i&)texSize,
                                                                          OSP_TEXTURE_R32F, ospDepthBuffer,
                                                                          OSP_TEXTURE_FILTER_NEAREST,
                                                                          sizeof(float));

      return depthTexture;
    }
  }
}

vtkInformationKeyMacro(vtkOSPRayRendererNode, SAMPLES_PER_PIXEL, Integer);
vtkInformationKeyMacro(vtkOSPRayRendererNode, MAX_FRAMES, Integer);
vtkInformationKeyMacro(vtkOSPRayRendererNode, AMBIENT_SAMPLES, Integer);
vtkInformationKeyMacro(vtkOSPRayRendererNode, COMPOSITE_ON_GL, Integer);
vtkInformationKeyMacro(vtkOSPRayRendererNode, RENDERER_TYPE, String);
vtkInformationKeyMacro(vtkOSPRayRendererNode, NORTH_POLE, DoubleVector);
vtkInformationKeyMacro(vtkOSPRayRendererNode, EAST_POLE, DoubleVector);
vtkInformationKeyMacro(vtkOSPRayRendererNode, MATERIAL_LIBRARY, ObjectBase);
vtkInformationKeyMacro(vtkOSPRayRendererNode, VIEW_TIME, Double);
vtkInformationKeyMacro(vtkOSPRayRendererNode, TIME_CACHE_SIZE, Integer);
vtkInformationKeyMacro(vtkOSPRayRendererNode, DENOISER_THRESHOLD, Integer);
vtkInformationKeyMacro(vtkOSPRayRendererNode, ENABLE_DENOISER, Integer);

class vtkOSPRayRendererNodeInternals
{
  //todo: move the rest of the internal data here too
public:
  vtkOSPRayRendererNodeInternals(vtkOSPRayRendererNode *_owner)
    : Owner(_owner)
  {
    //grumble!
    //vs2013 error C2536:...: cannot specify explicit initializer for arrays
    this->lbgcolor1[0] = 0.;
    this->lbgcolor1[1] = 0.;
    this->lbgcolor1[2] = 0.;
    this->lbgcolor2[0] = 0.;
    this->lbgcolor2[1] = 0.;
    this->lbgcolor2[2] = 0.;
    this->lup[0] = 1.;
    this->lup[1] = 0.;
    this->lup[2] = 0.;
    this->least[0] = 0.;
    this->least[1] = 1.;
    this->least[2] = 0.;
    this->LastViewPort[0] = 0.;
    this->LastViewPort[1] = 0.;
    this->LastParallelScale = 0.0;
    this->Backend = nullptr;
  };

  ~vtkOSPRayRendererNodeInternals() {};

  bool CanReuseBG()
  {
    bool retval = true;

    vtkRenderer *ren = vtkRenderer::SafeDownCast(this->Owner->GetRenderable());
    double *up = vtkOSPRayRendererNode::GetNorthPole(ren);
    if (up)
    {
      if (this->lup[0] != up[0] ||
          this->lup[1] != up[1] ||
          this->lup[2] != up[2])
      {
        this->lup[0] = up[0];
        this->lup[1] = up[1];
        this->lup[2] = up[2];
        retval = false;
      }
    }
    double *east = vtkOSPRayRendererNode::GetEastPole(ren);
    if (east)
    {
      if (this->least[0] != east[0] ||
          this->least[1] != east[1] ||
          this->least[2] != east[2])
      {
        this->least[0] = east[0];
        this->least[1] = east[1];
        this->least[2] = east[2];
        retval = false;
      }
    }
    bool usebgtexture = ren->GetTexturedBackground();
    if (this->lusebgtexture != usebgtexture)
    {
      this->lusebgtexture = usebgtexture;
      retval = false;
    }
    vtkTexture *bgtexture = ren->GetBackgroundTexture();
    vtkMTimeType bgttime = 0;
    if (bgtexture)
    {
      bgttime = bgtexture->GetMTime();
    }
    if (this->lbgtexture != bgtexture || bgttime > this->lbgttime)
    {
      this->lbgtexture = bgtexture;
      this->lbgttime = bgttime;
      retval = false;
    }
    bool usegradient = ren->GetGradientBackground();
    if (this->lusegradient != usegradient)
    {
      this->lusegradient = usegradient;
      retval = false;
    }
    double *nbgcolor1 = ren->GetBackground();
    double *nbgcolor2 = ren->GetBackground2();
    if (this->lbgcolor1[0] != nbgcolor1[0] ||
        this->lbgcolor1[1] != nbgcolor1[1] ||
        this->lbgcolor1[2] != nbgcolor1[2] ||
        this->lbgcolor2[0] != nbgcolor2[0] ||
        this->lbgcolor2[1] != nbgcolor2[1] ||
        this->lbgcolor2[2] != nbgcolor2[2])
    {
      this->lbgcolor1[0] = nbgcolor1[0];
      this->lbgcolor1[1] = nbgcolor1[1];
      this->lbgcolor1[2] = nbgcolor1[2];
      this->lbgcolor2[0] = nbgcolor2[0];
      this->lbgcolor2[1] = nbgcolor2[1];
      this->lbgcolor2[2] = nbgcolor2[2];
      retval = false;
    }
    return retval;
  }

  bool SetupPathTraceBackground(RTW::Backend* backend)
  {
    vtkRenderer *ren = vtkRenderer::SafeDownCast
      (this->Owner->GetRenderable());
    if (std::string(this->Owner->GetRendererType(ren)).find(std::string("pathtracer")) == std::string::npos)
    {
      return true;
    }
    bool reuseable = this->CanReuseBG();
    if (!reuseable)
    {
      double *bg1 = ren->GetBackground();
      unsigned char *ochars;
      int isize = 1;
      int jsize = 1;
      vtkTexture *text = ren->GetBackgroundTexture();
      if (ren->GetTexturedBackground() && text)
      {
        vtkImageData *vColorTextureMap = text->GetInput();
        //todo, fallback to gradient when either of above return nullptr
        //otherwise can't load texture in PV when in OSP::PT mode
        //todo: this code is duplicated from vtkOSPRayPolyDataMapperNode
        jsize = vColorTextureMap->GetExtent()[1];
        isize = vColorTextureMap->GetExtent()[3];
        unsigned char *ichars =
          (unsigned char *)vColorTextureMap->GetScalarPointer();
        ochars = new unsigned char[(isize+1)*(jsize+1)*3];
        unsigned char *oc = ochars;
        int comps = vColorTextureMap->GetNumberOfScalarComponents();
        for (int i = 0; i < isize+1; i++)
        {
          for (int j = 0; j < jsize+1; j++)
          {
            oc[0] = ichars[0];
            oc[1] = ichars[1];
            oc[2] = ichars[2];
            oc+=3;
            ichars+=comps;
          }
        }
        isize++;
        jsize++;
      }
      else if (ren->GetGradientBackground())
      {
        double *bg2 = ren->GetBackground2();
        isize=256; //todo: configurable
        jsize=2;
        ochars = new unsigned char[isize*jsize*3];
        unsigned char *oc = ochars;
        for (int i = 0; i < isize; i++)
        {
          double frac = (double)i/(double)isize;
          *(oc+0) = (bg1[0]*(1.0-frac) + bg2[0]*frac)*255;
          *(oc+1) = (bg1[1]*(1.0-frac) + bg2[1]*frac)*255;
          *(oc+2) = (bg1[2]*(1.0-frac) + bg2[2]*frac)*255;
          *(oc+3) = (bg1[0]*(1.0-frac) + bg2[0]*frac)*255;
          *(oc+4) = (bg1[1]*(1.0-frac) + bg2[1]*frac)*255;
          *(oc+5) = (bg1[2]*(1.0-frac) + bg2[2]*frac)*255;
          oc+=6;
        }
      }
      else
      {
        ochars = new unsigned char[3];
        ochars[0] = bg1[0]*255;
        ochars[1] = bg1[1]*255;
        ochars[2] = bg1[2]*255;
      }

      OSPTexture t2d = vtkOSPRayMaterialHelpers::NewTexture2D
        (
         backend,
         osp::vec2i{jsize,isize},
         OSP_TEXTURE_RGB8,
         ochars,
         0,
         3*sizeof(char));

      OSPLight ospLight = ospNewLight3("hdri");
      ospSetObject(ospLight, "map", t2d);
      ospRelease(t2d);

      double *up = vtkOSPRayRendererNode::GetNorthPole(ren);
      if (up)
        {
        ospSet3f(ospLight, "up", (float)up[0],(float)up[1],(float)up[2]);
        }
      else
        {
        ospSet3f(ospLight, "up",1.0f, 0.0f, 0.0f); //todo: configurable
        }
      double *east = vtkOSPRayRendererNode::GetEastPole(ren);
      if (east)
        {
        ospSet3f(ospLight, "dir", (float)east[0],(float)east[1],(float)east[2]);
        }
      else
        {
        ospSet3f(ospLight, "dir", 0.0f, 1.0f, 0.0f); //todo: configurable
        }
      ospCommit(t2d);
      ospCommit(ospLight); //todo: make sure osp frees its side
      delete[] ochars;
      this->BGLight = ospLight;
    }
    this->Owner->AddLight(this->BGLight);
    return reuseable;
  }

  std::map<vtkProp3D *, vtkAbstractMapper3D *> LastMapperFor;
  vtkOSPRayRendererNode *Owner;

  bool lusebgtexture = false;
  vtkWeakPointer<vtkTexture> lbgtexture = nullptr;
  vtkMTimeType lbgttime = 0;
  bool lusegradient = false;
  double lbgcolor1[3]; //not initializing here bc visstudio2013
  double lbgcolor2[3];
  double lup[3];
  double least[3];
  double LastViewPort[2];
  double LastParallelScale;
  double LastFocalDisk = -1.0;
  double LastFocalDistance = -1.0;

  OSPLight BGLight;
  RTW::Backend *Backend;
};

//============================================================================
vtkStandardNewMacro(vtkOSPRayRendererNode);

//----------------------------------------------------------------------------
vtkOSPRayRendererNode::vtkOSPRayRendererNode()
{
  this->Buffer = nullptr;
  this->ZBuffer = nullptr;
  this->ColorBufferTex = 0;
  this->DepthBufferTex = 0;
  this->ODepthBuffer = nullptr;
  this->OModel = nullptr;
  this->ORenderer = nullptr;
  this->OLightArray = nullptr;
  this->NumActors = 0;
  this->ComputeDepth = true;
  this->OFrameBuffer = nullptr;
  this->ImageX = this->ImageY = -1;
  this->CompositeOnGL = false;
  this->Accumulate = true;
  this->AccumulateCount = 0;
  this->ActorCount = 0;
  this->AccumulateTime = 0;
  this->AccumulateMatrix = vtkMatrix4x4::New();
  this->Internal = new vtkOSPRayRendererNodeInternals(this);
  this->PreviousType = "none";

#ifdef VTKOSPRAY_ENABLE_DENOISER
  this->DenoiserDevice = oidn::newDevice();
  this->DenoiserDevice.commit();
  this->DenoiserFilter = this->DenoiserDevice.newFilter("RT");
#endif
}

//----------------------------------------------------------------------------
vtkOSPRayRendererNode::~vtkOSPRayRendererNode()
{
  delete[] this->Buffer;
  delete[] this->ZBuffer;
  delete[] this->ODepthBuffer;
  if (this->Internal->Backend != nullptr)
  {
      RTW::Backend* backend = this->Internal->Backend;
      ospRelease((OSPModel)this->OModel);
      ospRelease((OSPRenderer)this->ORenderer);
      ospRelease(this->OFrameBuffer);
  }
  this->AccumulateMatrix->Delete();
  delete this->Internal;
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
void vtkOSPRayRendererNode::SetMaterialLibrary(vtkOSPRayMaterialLibrary *value, vtkRenderer *renderer)
{
  if (!renderer)
  {
    return;
  }
  vtkInformation *info = renderer->GetInformation();
  info->Set(vtkOSPRayRendererNode::MATERIAL_LIBRARY(), value);
}

//----------------------------------------------------------------------------
vtkOSPRayMaterialLibrary *vtkOSPRayRendererNode::GetMaterialLibrary(vtkRenderer *renderer)
{
  if (!renderer)
  {
    return nullptr;
  }
  vtkInformation *info = renderer->GetInformation();
  if (info && info->Has(vtkOSPRayRendererNode::MATERIAL_LIBRARY()))
  {
    vtkObjectBase *obj = info->Get(vtkOSPRayRendererNode::MATERIAL_LIBRARY());
    return (vtkOSPRayMaterialLibrary::SafeDownCast(obj));
  }
  return nullptr;
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

#ifdef VTK_ENABLE_OSPRAY
  if ("scivis" == name || "OSPRay raycaster" == name)
  {
    info->Set(vtkOSPRayRendererNode::RENDERER_TYPE(), "scivis");
  }
  if ("pathtracer" == name || "OSPRay pathtracer" == name)
  {
    info->Set(vtkOSPRayRendererNode::RENDERER_TYPE(), "pathtracer");
  }
#endif

#ifdef VTK_ENABLE_VISRTX
  if ("optix pathtracer" == name || "OptiX pathtracer" == name)
  {
    info->Set(vtkOSPRayRendererNode::RENDERER_TYPE(), "optix pathtracer");
  }
#endif
}

//----------------------------------------------------------------------------
std::string vtkOSPRayRendererNode::GetRendererType(vtkRenderer *renderer)
{
  if (!renderer)
  {
#ifdef VTK_ENABLE_OSPRAY
    return std::string("scivis");
#else
    return std::string("optix pathtracer");
#endif
  }
  vtkInformation *info = renderer->GetInformation();
  if (info && info->Has(vtkOSPRayRendererNode::RENDERER_TYPE()))
  {
    return (info->Get(vtkOSPRayRendererNode::RENDERER_TYPE()));
  }
#ifdef VTK_ENABLE_OSPRAY
  return std::string("scivis");
#else
  return std::string("optix pathtracer");
#endif
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
void vtkOSPRayRendererNode::SetNorthPole(double *value, vtkRenderer *renderer)
{
  if (!renderer)
  {
    return;
  }
  vtkInformation *info = renderer->GetInformation();
  info->Set(vtkOSPRayRendererNode::NORTH_POLE(), value, 3);
}

//----------------------------------------------------------------------------
double * vtkOSPRayRendererNode::GetNorthPole(vtkRenderer *renderer)
{
  if (!renderer)
  {
    return nullptr;
  }
  vtkInformation *info = renderer->GetInformation();
  if (info && info->Has(vtkOSPRayRendererNode::NORTH_POLE()))
  {
    return (info->Get(vtkOSPRayRendererNode::NORTH_POLE()));
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void vtkOSPRayRendererNode::SetEastPole(double *value, vtkRenderer *renderer)
{
  if (!renderer)
  {
    return;
  }
  vtkInformation *info = renderer->GetInformation();
  info->Set(vtkOSPRayRendererNode::EAST_POLE(), value, 3);
}

//----------------------------------------------------------------------------
double * vtkOSPRayRendererNode::GetEastPole(vtkRenderer *renderer)
{
  if (!renderer)
  {
    return nullptr;
  }
  vtkInformation *info = renderer->GetInformation();
  if (info && info->Has(vtkOSPRayRendererNode::EAST_POLE()))
  {
    return (info->Get(vtkOSPRayRendererNode::EAST_POLE()));
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void vtkOSPRayRendererNode::SetViewTime(double value, vtkRenderer *renderer)
{
  if (!renderer)
  {
    return;
  }
  vtkInformation *info = renderer->GetInformation();
  info->Set(vtkOSPRayRendererNode::VIEW_TIME(), value);
}

//----------------------------------------------------------------------------
double vtkOSPRayRendererNode::GetViewTime(vtkRenderer *renderer)
{
  if (!renderer)
  {
    return 0;
  }
  vtkInformation *info = renderer->GetInformation();
  if (info && info->Has(vtkOSPRayRendererNode::VIEW_TIME()))
  {
    return (info->Get(vtkOSPRayRendererNode::VIEW_TIME()));
  }
  return 0;
}

//----------------------------------------------------------------------------
void vtkOSPRayRendererNode::SetTimeCacheSize(int value, vtkRenderer *renderer)
{
  if (!renderer)
  {
    return;
  }
  vtkInformation *info = renderer->GetInformation();
  info->Set(vtkOSPRayRendererNode::TIME_CACHE_SIZE(), value);
}

//----------------------------------------------------------------------------
int vtkOSPRayRendererNode::GetTimeCacheSize(vtkRenderer *renderer)
{
  if (!renderer)
  {
    return 0;
  }
  vtkInformation *info = renderer->GetInformation();
  if (info && info->Has(vtkOSPRayRendererNode::TIME_CACHE_SIZE()))
  {
    return (info->Get(vtkOSPRayRendererNode::TIME_CACHE_SIZE()));
  }
  return 0;
}

//----------------------------------------------------------------------------
void vtkOSPRayRendererNode::SetDenoiserThreshold(int value, vtkRenderer *renderer)
{
  if (!renderer)
  {
    return;
  }
  vtkInformation *info = renderer->GetInformation();
  info->Set(vtkOSPRayRendererNode::DENOISER_THRESHOLD(), value);
}

//----------------------------------------------------------------------------
int vtkOSPRayRendererNode::GetDenoiserThreshold(vtkRenderer *renderer)
{
  if (!renderer)
  {
    return 4;
  }
  vtkInformation *info = renderer->GetInformation();
  if (info && info->Has(vtkOSPRayRendererNode::DENOISER_THRESHOLD()))
  {
    return (info->Get(vtkOSPRayRendererNode::DENOISER_THRESHOLD()));
  }
  return 4;
}

//----------------------------------------------------------------------------
void vtkOSPRayRendererNode::SetEnableDenoiser(int value, vtkRenderer *renderer)
{
  if (!renderer)
  {
    return;
  }
  vtkInformation *info = renderer->GetInformation();
  info->Set(vtkOSPRayRendererNode::ENABLE_DENOISER(), value);
}

//----------------------------------------------------------------------------
int vtkOSPRayRendererNode::GetEnableDenoiser(vtkRenderer *renderer)
{
  if (!renderer)
  {
    return 0;
  }
  vtkInformation *info = renderer->GetInformation();
  if (info && info->Has(vtkOSPRayRendererNode::ENABLE_DENOISER()))
  {
    return (info->Get(vtkOSPRayRendererNode::ENABLE_DENOISER()));
  }
  return 0;
}

//----------------------------------------------------------------------------
void vtkOSPRayRendererNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkOSPRayRendererNode::Traverse(int operation)
{
  // do not override other passes
  if (operation != render)
  {
    this->Superclass::Traverse(operation);
    return;
  }



  this->Apply(operation,true);

  OSPRenderer oRenderer = this->ORenderer;

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
  bool hasAmbient = false;
  while (!it->IsDoneWithTraversal())
  {
    vtkOSPRayLightNode *child =
      vtkOSPRayLightNode::SafeDownCast(it->GetCurrentObject());
    if (child)
    {
      child->Traverse(operation);
      if (child->GetIsAmbient(vtkLight::SafeDownCast(child->GetRenderable())))
      {
        hasAmbient = true;
      }
    }
    it->GoToNextItem();
  }

  RTW::Backend* backend = this->Internal->Backend;
  if (backend == nullptr)
    return;

  if (!hasAmbient &&
      (this->GetAmbientSamples(static_cast<vtkRenderer*>(this->Renderable)) > 0)
      )
  {
    //hardcode an ambient light for AO since OSP 1.2 stopped doing so.
    OSPLight ospAmbient = ospNewLight3("AmbientLight");
    ospSetString(ospAmbient, "name", "default_ambient");
    ospSet3f(ospAmbient, "color", 1.f, 1.f, 1.f);
    ospSet1f(ospAmbient, "intensity",
             0.13f*vtkOSPRayLightNode::GetLightScale()*vtkMath::Pi());
    ospCommit(ospAmbient);
    this->Lights.push_back(ospAmbient);
  }

  bool bgreused = this->Internal->SetupPathTraceBackground(backend);
  ospRelease(this->OLightArray);
  this->OLightArray = ospNewData(this->Lights.size(), OSP_OBJECT,
    (this->Lights.size()?&this->Lights[0]:nullptr), 0);
  ospSetData(oRenderer, "lights", this->OLightArray);

  //actors
  OSPModel oModel=nullptr;
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
    ospCommit(oModel);
    ospSetObject(oRenderer,"model", oModel);
    ospCommit(oRenderer);
  }
  else
  {
    oModel = (OSPModel)this->OModel;
    ospSetObject(oRenderer,"model", oModel);
    ospCommit(oRenderer);
  }
  it->Delete();

  if (!bgreused)
  {
    //hack to ensure progressive rendering resets when background changes
    this->AccumulateTime = 0;
  }
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

  RTW::Backend* backend = this->Internal->Backend;

  if (prepass)
  {
    OSPRenderer oRenderer = nullptr;

    std::string type = this->GetRendererType(static_cast<vtkRenderer*>(this->Renderable));
    if (!this->ORenderer || this->PreviousType != type)
    {
      this->Traverse(invalidate);
      //std::cerr << "initializing backend...\n";
      //std::set<RTWBackendType> availableBackends = rtwGetAvailableBackends();
      //for (RTWBackendType backend : availableBackends)
      //{
      //    switch (backend)
      //    {
      //    case OSP_BACKEND_OSPRAY:
      //        std::cerr << "OSPRay backend is available\n";
      //        break;
      //    case OSP_BACKEND_VISRTX:
      //        std::cerr << "VisRTX backend is available\n";
      //        break;
      //    default:
      //        std::cerr << "Unknown backend type listed as available \n";
      //    }
      //}
      this->Internal->Backend = rtwSwitch(type.c_str());
      if (this->Internal->Backend == nullptr)
      {
        return;
      }
      backend = this->Internal->Backend;

      oRenderer = ospNewRenderer(type.c_str());
      this->ORenderer = oRenderer;
      this->PreviousType = type;
    }
    else
    {
    oRenderer = this->ORenderer;
    }
    ospSet1f(this->ORenderer, "maxContribution", 2.f);
    ospSet1f(this->ORenderer, "minContribution", 0.01f);
    ospSet1f(this->ORenderer, "varianceThreshold", 0.3f);
    ospCommit(this->ORenderer);

    int viewportOrigin[2];
    int viewportSize[2];
    ren->GetTiledSizeAndOrigin(
      &viewportSize[0], &viewportSize[1], &viewportOrigin[0], &viewportOrigin[1]);
    this->Size[0] = viewportSize[0];
    this->Size[1] = viewportSize[1];
    if (ren->GetUseShadows())
    {
      ospSet1i(oRenderer,"shadowsEnabled",1);
    }
    else
    {
      ospSet1i(oRenderer,"shadowsEnabled",0);
    }

    //todo: this can be expensive and should be cached
    //also the user might want to control
    vtkBoundingBox bbox(ren->ComputeVisiblePropBounds());
    if (bbox.IsValid())
    {
      float diam = static_cast<float>(bbox.GetDiagonalLength());
      float logDiam = log(diam);
      if (logDiam < 0.f)
        {
        logDiam = 1.f/(fabs(logDiam));
        }
      float epsilon = 1e-5*logDiam;
      ospSet1f(oRenderer, "epsilon", epsilon);
      ospSet1f(oRenderer, "aoDistance", diam*0.3);
      ospSet1i(oRenderer, "autoEpsilon", 0);
    }
    else
    {
      ospSet1f(oRenderer, "epsilon", 0.001f);
    }

    vtkVolumeCollection *vc = ren->GetVolumes();
    if (vc->GetNumberOfItems())
    {
      ospSet1i(oRenderer, "aoTransparencyEnabled", 1);
    }

    ospSet1i(oRenderer,"aoSamples",
             this->GetAmbientSamples(ren));
    ospSet1i(oRenderer,"spp",
             this->GetSamplesPerPixel(ren));
    this->CompositeOnGL =
      (this->GetCompositeOnGL(ren)!=0);

    double *bg = ren->GetBackground();
    ospSet4f(oRenderer,"bgColor", bg[0], bg[1], bg[2], ren->GetBackgroundAlpha());
  }
  else
  {
    OSPRenderer oRenderer = this->ORenderer;
    ospCommit(oRenderer);

    osp::vec2i isize = { this->Size[0], this->Size[1] };
    if (this->ImageX != this->Size[0] || this->ImageY != this->Size[1])
    {
      this->ImageX = this->Size[0];
      this->ImageY = this->Size[1];
      const size_t size = this->ImageX*this->ImageY;
      ospRelease(this->OFrameBuffer);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextra"
      this->OFrameBuffer = ospNewFrameBuffer
      (isize,
#ifdef VTKOSPRAY_ENABLE_DENOISER
        OSP_FB_RGBA32F,
#else
        OSP_FB_RGBA8,
#endif
        OSP_FB_COLOR
        | (this->ComputeDepth ? OSP_FB_DEPTH : 0)
        | (this->Accumulate ? OSP_FB_ACCUM : 0)
#ifdef VTKOSPRAY_ENABLE_DENOISER
        | OSP_FB_NORMAL | OSP_FB_ALBEDO
#endif
      );
      this->DenoisedBuffer.resize(size);
      this->ColorBuffer.resize(size);
      this->NormalBuffer.resize(size);
      this->AlbedoBuffer.resize(size);
      this->DenoiserDirty = true;
      ospSet1f(this->OFrameBuffer, "gamma", 1.0f);
      ospCommit(this->OFrameBuffer);
      ospFrameBufferClear
      (this->OFrameBuffer,
        OSP_FB_COLOR
        | (this->ComputeDepth ? OSP_FB_DEPTH : 0)
        | (this->Accumulate ? OSP_FB_ACCUM : 0)
      );
#pragma GCC diagnostic pop
      delete[] this->Buffer;
      this->Buffer = new unsigned char[this->Size[0] * this->Size[1] * 4];
      delete[] this->ZBuffer;
      this->ZBuffer = new float[this->Size[0] * this->Size[1]];
      if (this->CompositeOnGL)
      {
        delete[] this->ODepthBuffer;
        this->ODepthBuffer = new float[this->Size[0] * this->Size[1]];
      }
    }
    else if (this->Accumulate)
    {
      //check if something has changed
      //if so we clear and start over, otherwise we continue to accumulate
      bool canReuse = true;

      //TODO: these all need some work as checks are not necessarily fast
      //nor sufficient for all cases that matter

      //check for stereo and disable so don't get left in right
      vtkRenderWindow *rwin =
        vtkRenderWindow::SafeDownCast(ren->GetVTKWindow());
      if (rwin && rwin->GetStereoRender())
      {
        canReuse = false;
      }

      //check for tiling, ie typically putting together large images to save high res pictures
      double *vp = rwin->GetTileViewport();
      if (this->Internal->LastViewPort[0] != vp[0] ||
        this->Internal->LastViewPort[1] != vp[1])
      {
        canReuse = false;
        this->Internal->LastViewPort[0] = vp[0];
        this->Internal->LastViewPort[1] = vp[1];
      }

      //check actors (and time)
      vtkMTimeType m = 0;
      vtkActorCollection *ac = ren->GetActors();
      int nitems = ac->GetNumberOfItems();
      if (nitems != this->ActorCount)
      {
        //TODO: need a hash or something to really check for added/deleted
        this->ActorCount = nitems;
        this->AccumulateCount = 0;
        canReuse = false;
      }
      if (canReuse)
      {
        ac->InitTraversal();
        vtkActor *nac = ac->GetNextActor();
        while (nac)
        {
          if (nac->GetRedrawMTime() > m)
          {
            m = nac->GetRedrawMTime();
          }
          if (this->Internal->LastMapperFor[nac] != nac->GetMapper())
          {
            // a check to ensure vtkPVLODActor restarts on LOD swap
            this->Internal->LastMapperFor[nac] = nac->GetMapper();
            canReuse = false;
          }
          nac = ac->GetNextActor();
        }
        if (this->AccumulateTime < m)
        {
          this->AccumulateTime = m;
          canReuse = false;
        }
      }

      if (canReuse)
      {
        m = 0;
        vtkVolumeCollection *vc = ren->GetVolumes();
        vc->InitTraversal();
        vtkVolume* nvol = vc->GetNextVolume();
        while (nvol)
        {
          if (nvol->GetRedrawMTime() > m)
          {
            m = nvol->GetRedrawMTime();
          }
          if (this->Internal->LastMapperFor[nvol] != nvol->GetMapper())
          {
            // a check to ensure vtkPVLODActor restarts on LOD swap
            this->Internal->LastMapperFor[nvol] = nvol->GetMapper();
            canReuse = false;
          }
          nvol = vc->GetNextVolume();
        };
        if (this->AccumulateTime < m)
        {
          this->AccumulateTime = m;
          canReuse = false;
        }
      }

      if (canReuse)
      {
        //check camera
        //Why not cam->mtime?
        // cam->mtime is bumped by synch after this in parallel so never reuses
        //Why not cam->MVTO->mtime?
        //  cam set's elements directly, so the mtime doesn't bump with motion
        vtkMatrix4x4 *camnow =
          ren->GetActiveCamera()->GetModelViewTransformObject()->GetMatrix();
        for (int i = 0; i < 4; i++)
        {
          for (int j = 0; j < 4; j++)
          {
            if (this->AccumulateMatrix->GetElement(i, j) !=
              camnow->GetElement(i, j))
            {
              this->AccumulateMatrix->DeepCopy(camnow);
              canReuse = false;
              i = 4; j = 4;
            }
          }
        }
        if (this->Internal->LastParallelScale !=
          ren->GetActiveCamera()->GetParallelScale())
        {
          this->Internal->LastParallelScale = ren->GetActiveCamera()->GetParallelScale();
          canReuse = false;
        }

        if (this->Internal->LastFocalDisk !=
          ren->GetActiveCamera()->GetFocalDisk())
        {
          this->Internal->LastFocalDisk = ren->GetActiveCamera()->GetFocalDisk();
          canReuse = false;
        }

        if (this->Internal->LastFocalDistance !=
            ren->GetActiveCamera()->GetFocalDistance())
        {
            this->Internal->LastFocalDistance = ren->GetActiveCamera()->GetFocalDistance();
            canReuse = false;
        }
      }
      if (!canReuse)
      {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextra"
        ospFrameBufferClear
        (this->OFrameBuffer,
          OSP_FB_COLOR
          | (this->ComputeDepth ? OSP_FB_DEPTH : 0)
          | OSP_FB_ACCUM);
#pragma GCC diagnostic pop
        this->AccumulateCount = 0;
      }
    }
    else if (!this->Accumulate)
    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextra"
      ospFrameBufferClear
      (this->OFrameBuffer,
        OSP_FB_COLOR | (this->ComputeDepth ? OSP_FB_DEPTH : 0));
#pragma GCC diagnostic pop
    }

    vtkCamera *cam = vtkRenderer::SafeDownCast(this->Renderable)->GetActiveCamera();

    ospSet1i(oRenderer, "backgroundEnabled", ren->GetErase());
    if (this->CompositeOnGL && backend->IsSupported(RTW_DEPTH_COMPOSITING))
    {
      vtkRenderWindow *rwin =
        vtkRenderWindow::SafeDownCast(ren->GetVTKWindow());
      int viewportX, viewportY;
      int viewportWidth, viewportHeight;
      ren->GetTiledSizeAndOrigin(&viewportWidth, &viewportHeight,
        &viewportX, &viewportY);
      rwin->GetZbufferData(
        viewportX, viewportY,
        viewportX + viewportWidth - 1,
        viewportY + viewportHeight - 1,
        this->GetZBuffer());

      double zNear, zFar;
      double fovy, aspect;
      fovy = cam->GetViewAngle();
      aspect = double(viewportWidth) / double(viewportHeight);
      cam->GetClippingRange(zNear, zFar);
      double camUp[3];
      double camDir[3];
      cam->GetViewUp(camUp);
      cam->GetFocalPoint(camDir);
      osp::vec3f cameraUp = { static_cast<float>(camUp[0]),
                             static_cast<float>(camUp[1]),
                             static_cast<float>(camUp[2]) };
      osp::vec3f cameraDir = { static_cast<float>(camDir[0]),
                              static_cast<float>(camDir[1]),
                              static_cast<float>(camDir[2]) };
      double cameraPos[3];
      cam->GetPosition(cameraPos);
      cameraDir.x -= cameraPos[0];
      cameraDir.y -= cameraPos[1];
      cameraDir.z -= cameraPos[2];
      cameraDir = ospray::opengl::normalize(cameraDir);

      OSPTexture glDepthTex = ospray::opengl::getOSPDepthTextureFromOpenGLPerspective
      (fovy, aspect, zNear, zFar,
        (osp::vec3f&)cameraDir, (osp::vec3f&)cameraUp,
        this->GetZBuffer(), this->ODepthBuffer, viewportWidth, viewportHeight,
        this->Internal->Backend);

      ospSetObject(oRenderer, "maxDepthTexture", glDepthTex);
    }
    else
    {
      ospSetObject(oRenderer, "maxDepthTexture", 0);
    }

    // Enable VisRTX denoiser
    this->AccumulateCount += this->GetSamplesPerPixel(ren);
    bool useDenoiser = this->GetEnableDenoiser(ren) && (this->AccumulateCount >= this->GetDenoiserThreshold(ren));
    ospSet1i(oRenderer, "denoise", useDenoiser ? 1 : 0); // for VisRTX backend only

    ospCommit(oRenderer);

    const bool backendDepthNormalization = backend->IsSupported(RTW_DEPTH_NORMALIZATION);
    if (backendDepthNormalization)
    {
      const double* clipValues = cam->GetClippingRange();
      const double clipMin = clipValues[0];
      const double clipMax = clipValues[1];
      backend->SetDepthNormalizationGL(this->OFrameBuffer, clipMin, clipMax);
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextra"
    ospRenderFrame(this->OFrameBuffer, oRenderer,
      OSP_FB_COLOR
      | (this->ComputeDepth ? OSP_FB_DEPTH : 0)
      | (this->Accumulate ? OSP_FB_ACCUM : 0)
#ifdef VTKOSPRAY_ENABLE_DENOISER
      | OSP_FB_NORMAL | OSP_FB_ALBEDO
#endif
    );
#pragma GCC diagnostic pop

    // Check if backend can do direct OpenGL display using textures
    bool useOpenGLInterop = backend->IsSupported(RTW_OPENGL_INTEROP);

    // Only layer 0 can currently display using OpenGL
    if (ren->GetLayer() != 0)
      useOpenGLInterop = false;

    if (useOpenGLInterop)
    {
      // Check if we actually have an OpenGL window
      vtkRenderWindow *rwin = vtkRenderWindow::SafeDownCast(ren->GetVTKWindow());
      vtkOpenGLRenderWindow* windowOpenGL = vtkOpenGLRenderWindow::SafeDownCast(rwin);
      if (windowOpenGL != nullptr)
      {
        windowOpenGL->MakeCurrent();
        this->ColorBufferTex = backend->GetColorTextureGL(this->OFrameBuffer);
        this->DepthBufferTex = backend->GetDepthTextureGL(this->OFrameBuffer);

        useOpenGLInterop = (this->ColorBufferTex != 0 && this->DepthBufferTex != 0);
      }
      else
      {
        useOpenGLInterop = false;
      }
    }

    if (!useOpenGLInterop)
    {
      const void* rgba = ospMapFrameBuffer(this->OFrameBuffer, OSP_FB_COLOR);
#ifdef VTKOSPRAY_ENABLE_DENOISER
      //std::copy(rgba, this->Size[0]*this->Size[1]*4*sizeof(float), &this->ColorBuffer[0]);
      memcpy(&this->ColorBuffer[0], rgba, this->Size[0] * this->Size[1] * 4 * sizeof(float));
      if (useDenoiser)
      {
        this->Denoise();
      }
      //VTK appears to need an RGBA8 buffer, but the denoiser only supports floats right now.  Convert.
      for (size_t i = 0; i < static_cast<size_t>(this->ImageX*this->ImageY); i++)
      {
        const int bi = i * 4;
        this->Buffer[bi] = static_cast<unsigned char>(std::min(this->ColorBuffer[i].x*255.f, 255.f));
        this->Buffer[bi + 1] = static_cast<unsigned char>(std::min(this->ColorBuffer[i].y*255.f, 255.f));
        this->Buffer[bi + 2] = static_cast<unsigned char>(std::min(this->ColorBuffer[i].z*255.f, 255.f));
        this->Buffer[bi + 3] = 255;
      }
#else
      //std::copy((unsigned char*)rgba, this->Size[0]*this->Size[1]*4*sizeof(float), &this->Buffer[0]);
      memcpy((void*)this->Buffer, rgba, this->Size[0] * this->Size[1] * sizeof(char) * 4);
#endif
      ospUnmapFrameBuffer(rgba, this->OFrameBuffer);

      if (this->ComputeDepth)
      {
        const void *Z = ospMapFrameBuffer(this->OFrameBuffer, OSP_FB_DEPTH);

        if (backendDepthNormalization)
        {
          memcpy((void*)this->ZBuffer, Z, this->Size[0] * this->Size[1] * sizeof(float));
        }
        else
        {
          double *clipValues = cam->GetClippingRange();
          double clipMin = clipValues[0];
          double clipMax = clipValues[1];
          double clipDiv = 1.0 / (clipMax - clipMin);


          float *s = (float *)Z;
          float *d = this->ZBuffer;
          for (int i = 0; i < (this->Size[0] * this->Size[1]); i++, s++, d++)
          {
            *d = (*s < clipMin ? 1.0 : (*s - clipMin) * clipDiv);
          }
        }

        ospUnmapFrameBuffer(Z, this->OFrameBuffer);
      }
    }
  }
}

void vtkOSPRayRendererNode::Denoise()
{
#ifdef VTKOSPRAY_ENABLE_DENOISER
  RTW::Backend* backend = this->Internal->Backend;
  this->DenoisedBuffer = this->ColorBuffer;
  if (this->DenoiserDirty)
  {
    this->DenoiserFilter.setImage("color", (void*)this->ColorBuffer.data(),
        oidn::Format::Float3, this->ImageX, this->ImageY, 0, sizeof(osp::vec4f));

    this->DenoiserFilter.setImage("normal", (void*)this->NormalBuffer.data(),
        oidn::Format::Float3, this->ImageX, this->ImageY, 0, sizeof(osp::vec3f));

    this->DenoiserFilter.setImage("albedo", (void*)this->AlbedoBuffer.data(),
        oidn::Format::Float3, this->ImageX, this->ImageY, 0, sizeof(osp::vec3f));

    this->DenoiserFilter.setImage("output", (void*)this->DenoisedBuffer.data(),
        oidn::Format::Float3, this->ImageX, this->ImageY, 0, sizeof(osp::vec4f));

    this->DenoiserFilter.commit();
    this->DenoiserDirty = false;
  }

  const auto size = this->ImageX*this->ImageY;
  const osp::vec4f* rgba = static_cast<const osp::vec4f*>
    (ospMapFrameBuffer(this->OFrameBuffer, OSP_FB_COLOR));
  std::copy(rgba, rgba+size, this->ColorBuffer.begin());
  ospUnmapFrameBuffer(rgba, this->OFrameBuffer);
  const osp::vec3f* normal = static_cast<const osp::vec3f*>
    (ospMapFrameBuffer(this->OFrameBuffer, OSP_FB_NORMAL));
  std::copy(normal, normal+size, this->NormalBuffer.begin());
  ospUnmapFrameBuffer(normal, this->OFrameBuffer);
  const osp::vec3f* albedo = static_cast<const osp::vec3f*>
    (ospMapFrameBuffer(this->OFrameBuffer, OSP_FB_ALBEDO));
  std::copy(albedo, albedo+size, this->AlbedoBuffer.begin());
  ospUnmapFrameBuffer(albedo, this->OFrameBuffer);

  this->DenoiserFilter.execute();
  //Carson: not sure we need two buffers
  this->ColorBuffer = this->DenoisedBuffer;
#endif
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

//------------------------------------------------------------------------------
vtkRenderer *vtkOSPRayRendererNode::GetRenderer()
{
  return vtkRenderer::SafeDownCast(this->GetRenderable());
}

//------------------------------------------------------------------------------
vtkOSPRayRendererNode *vtkOSPRayRendererNode::GetRendererNode(vtkViewNode *self)
{
    return
      static_cast<vtkOSPRayRendererNode *>(
        self->GetFirstAncestorOfType("vtkOSPRayRendererNode"));
}

//------------------------------------------------------------------------------
RTW::Backend *vtkOSPRayRendererNode::GetBackend()
{
  return this->Internal->Backend;
}
