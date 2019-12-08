/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShadowMapBakerPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

//#include "vtkAbstractTransform.h" // for helper classes stack and concatenation
#include "vtkShadowMapBakerPass.h"
#include "vtkCameraPass.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkLightsPass.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpaquePass.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLState.h"
#include "vtkRenderPassCollection.h"
#include "vtkRenderState.h"
#include "vtkSequencePass.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"

#include "vtkStdString.h"
#include <cassert>
#include <sstream>

// debugging
#include "vtkTimerLog.h"

// to be able to dump intermediate passes into png files for debugging.
// only for vtkShadowMapBakerPass developers.
//#define VTK_SHADOW_MAP_BAKER_PASS_DEBUG
//#define DONT_DUPLICATE_LIGHTS

vtkStandardNewMacro(vtkShadowMapBakerPass);
vtkCxxSetObjectMacro(vtkShadowMapBakerPass, OpaqueSequence, vtkRenderPass);
vtkCxxSetObjectMacro(vtkShadowMapBakerPass, CompositeZPass, vtkRenderPass);

// ----------------------------------------------------------------------------
// helper function to compute the mNearest point in a given direction.
// To be called several times, with initialized = false the first time.
void vtkShadowMapBakerPass::PointNearFar(
  double* v, double* pt, double* dir, double& mNear, double& mFar, bool initialized)
{
  double diff[3];
  diff[0] = v[0] - pt[0];
  diff[1] = v[1] - pt[1];
  diff[2] = v[2] - pt[2];
  double dot = vtkMath::Dot(diff, dir);
  if (initialized)
  {
    if (dot < mNear)
    {
      mNear = dot;
    }
    if (dot > mFar)
    {
      mFar = dot;
    }
  }
  else
  {
    mNear = dot;
    mFar = dot;
  }
}

// ----------------------------------------------------------------------------
// compute the min/max of the projection of a box in a given direction.
void vtkShadowMapBakerPass::BoxNearFar(
  double* bb, double* pt, double* dir, double& mNear, double& mFar)
{
  double v[3];
  v[0] = bb[0];
  v[1] = bb[2];
  v[2] = bb[4];
  PointNearFar(v, pt, dir, mNear, mFar, false);

  v[0] = bb[1];
  v[1] = bb[2];
  v[2] = bb[4];
  PointNearFar(v, pt, dir, mNear, mFar, true);

  v[0] = bb[0];
  v[1] = bb[3];
  v[2] = bb[4];
  PointNearFar(v, pt, dir, mNear, mFar, true);

  v[0] = bb[1];
  v[1] = bb[3];
  v[2] = bb[4];
  PointNearFar(v, pt, dir, mNear, mFar, true);

  v[0] = bb[0];
  v[1] = bb[2];
  v[2] = bb[5];
  PointNearFar(v, pt, dir, mNear, mFar, true);

  v[0] = bb[1];
  v[1] = bb[2];
  v[2] = bb[5];
  PointNearFar(v, pt, dir, mNear, mFar, true);

  v[0] = bb[0];
  v[1] = bb[3];
  v[2] = bb[5];
  PointNearFar(v, pt, dir, mNear, mFar, true);

  v[0] = bb[1];
  v[1] = bb[3];
  v[2] = bb[5];
  PointNearFar(v, pt, dir, mNear, mFar, true);
}

// ----------------------------------------------------------------------------
vtkShadowMapBakerPass::vtkShadowMapBakerPass()
{
  vtkNew<vtkCameraPass> camP;
  vtkNew<vtkSequencePass> seqP;
  vtkNew<vtkLightsPass> lightP;
  vtkNew<vtkOpaquePass> opaqueP;
  camP->SetDelegatePass(seqP);
  vtkNew<vtkRenderPassCollection> rpc;
  rpc->AddItem(lightP);
  rpc->AddItem(opaqueP);
  seqP->SetPasses(rpc);

  this->OpaqueSequence = nullptr;
  this->SetOpaqueSequence(camP);

  this->CompositeZPass = nullptr;

  this->Resolution = 1024;

  this->FrameBufferObject = nullptr;
  this->ShadowMaps = nullptr;
  this->LightCameras = nullptr;

  this->HasShadows = false;
  this->NeedUpdate = true;
}

// ----------------------------------------------------------------------------
vtkShadowMapBakerPass::~vtkShadowMapBakerPass()
{
  if (this->OpaqueSequence != nullptr)
  {
    this->OpaqueSequence->Delete();
  }

  if (this->CompositeZPass != nullptr)
  {
    this->CompositeZPass->Delete();
  }

  if (this->FrameBufferObject != nullptr)
  {
    vtkErrorMacro(<< "FrameBufferObject should have been deleted in ReleaseGraphicsResources().");
  }

  if (this->ShadowMaps != nullptr)
  {
    vtkErrorMacro(<< "ShadowMaps should have been deleted in ReleaseGraphicsResources().");
  }
  if (this->LightCameras != nullptr)
  {
    vtkErrorMacro(<< "LightCameras should have been deleted in ReleaseGraphicsResources().");
  }
}

// ----------------------------------------------------------------------------
void vtkShadowMapBakerPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "OpaqueSequence: ";
  if (this->OpaqueSequence != nullptr)
  {
    this->OpaqueSequence->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }

  os << indent << "CompositeZPass: ";
  if (this->CompositeZPass != nullptr)
  {
    this->CompositeZPass->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }

  os << indent << "Resolution: " << this->Resolution << endl;
}

// ----------------------------------------------------------------------------
bool vtkShadowMapBakerPass::GetHasShadows()
{
  return this->HasShadows;
}

// ----------------------------------------------------------------------------
bool vtkShadowMapBakerPass::LightCreatesShadow(vtkLight* l)
{
  assert("pre: l_exists" && l != nullptr);

  return !l->LightTypeIsHeadlight() && (!l->GetPositional() || l->GetConeAngle() < 90.0);
}

// ----------------------------------------------------------------------------
std::vector<vtkSmartPointer<vtkTextureObject> >* vtkShadowMapBakerPass::GetShadowMaps()
{
  return this->ShadowMaps;
}

// ----------------------------------------------------------------------------
std::vector<vtkSmartPointer<vtkCamera> >* vtkShadowMapBakerPass::GetLightCameras()
{
  return this->LightCameras;
}

// ----------------------------------------------------------------------------
bool vtkShadowMapBakerPass::GetNeedUpdate()
{
  return this->NeedUpdate;
}

// ----------------------------------------------------------------------------
void vtkShadowMapBakerPass::SetUpToDate()
{
  this->NeedUpdate = false;
}

// ----------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void vtkShadowMapBakerPass::Render(const vtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);

  vtkOpenGLClearErrorMacro();

  this->NumberOfRenderedProps = 0;
  this->HasShadows = false;

  vtkOpenGLRenderer* r = static_cast<vtkOpenGLRenderer*>(s->GetRenderer());
  vtkOpenGLRenderWindow* context = static_cast<vtkOpenGLRenderWindow*>(r->GetRenderWindow());
  vtkOpenGLState* ostate = context->GetState();

  if (this->OpaqueSequence != nullptr)
  {
    // Disable the scissor test during the shadow map pass.
    vtkOpenGLState::ScopedglEnableDisable ssaver(ostate, GL_SCISSOR_TEST);
    ostate->vtkglDisable(GL_SCISSOR_TEST);

    // Shadow mapping requires:
    // 1. at least one spotlight, not front light
    // 2. at least one receiver, in the list of visible props after culling
    // 3. at least one occluder, in the list of visible props before culling

    vtkLightCollection* lights = r->GetLights();
    lights->InitTraversal();
    vtkLight* l = lights->GetNextItem();
    bool hasLight = false;
    bool hasOccluder = false;
    while (!hasLight && l != nullptr)
    {
      hasLight = l->GetSwitch() && this->LightCreatesShadow(l);
      l = lights->GetNextItem();
    }

    int propArrayCount = 0;
    vtkProp** propArray = nullptr;
    vtkMTimeType latestPropTime = 0;

    if (hasLight)
    {
      // at least one receiver?
      vtkCollectionSimpleIterator pit;
      vtkPropCollection* props = r->GetViewProps();
      props->InitTraversal(pit);
      vtkProp* p = props->GetNextProp(pit);
      propArray = new vtkProp*[props->GetNumberOfItems()];
      while (p != nullptr)
      {
        vtkMTimeType mTime = p->GetMTime();
        if (latestPropTime < mTime)
        {
          latestPropTime = mTime;
        }
        if (p->GetVisibility())
        {
          propArray[propArrayCount] = p;
          ++propArrayCount;
          hasOccluder = true;
        }
        p = props->GetNextProp(pit);
      }
    }
    this->HasShadows = hasOccluder;
    if (!hasOccluder)
    {
      delete[] propArray;
      // Nothing to bake.
      return;
    }

    // Shadow mapping starts here.
    // 1. Create a shadow map for each spotlight.

    // Do we need to recreate shadow maps?
    this->NeedUpdate = this->LastRenderTime < lights->GetMTime();
    if (!this->NeedUpdate)
    {
      lights->InitTraversal();
      l = lights->GetNextItem();
      while (!this->NeedUpdate && l != nullptr)
      {
        this->NeedUpdate = this->LastRenderTime < l->GetMTime();
        l = lights->GetNextItem();
      }
    }
    if (!this->NeedUpdate)
    {
      this->NeedUpdate = this->LastRenderTime < r->GetViewProps()->GetMTime() ||
        this->LastRenderTime < latestPropTime;
    }

    if (!this->NeedUpdate)
    {
      int i = 0;
      while (i < propArrayCount)
      {
        this->NeedUpdate = this->LastRenderTime < propArray[i]->GetMTime();
        ++i;
      }
    }

    bool autoLight = r->GetAutomaticLightCreation() == 1;
    vtkCamera* realCamera = r->GetActiveCamera();
    vtkRenderState s2(r);
    if (this->NeedUpdate) // create or re-create the shadow maps.
    {
#ifdef VTK_SHADOW_MAP_BAKER_PASS_DEBUG
      cout << "update the shadow maps" << endl;
#endif

      realCamera->Register(this);

      // 1. Create a new render state with an FBO.

      // We need all the visible props, including the one culled out by the
      // camera,  because they can cast shadows too (ie being visible from the
      // light cameras)
      s2.SetPropArrayAndCount(propArray, propArrayCount);

      if (this->FrameBufferObject == nullptr)
      {
        this->FrameBufferObject = vtkOpenGLFramebufferObject::New();
        this->FrameBufferObject->SetContext(context);
        context->GetState()->PushFramebufferBindings();
        this->FrameBufferObject->Resize(this->Resolution, this->Resolution);
        this->FrameBufferObject->Bind();
        this->FrameBufferObject->AddDepthAttachment();
      }
      else
      {
        context->GetState()->PushFramebufferBindings();
      }
      this->FrameBufferObject->Bind();
      s2.SetFrameBuffer(this->FrameBufferObject);

      lights->InitTraversal();
      l = lights->GetNextItem();
      size_t numberOfShadowLights = 0;
      while (l != nullptr)
      {
        if (l->GetSwitch() && this->LightCreatesShadow(l))
        {
          ++numberOfShadowLights;
        }
        l = lights->GetNextItem();
      }

      if (this->ShadowMaps != nullptr && this->ShadowMaps->size() != numberOfShadowLights)
      {
        delete this->ShadowMaps;
        this->ShadowMaps = nullptr;
      }

      if (this->ShadowMaps == nullptr)
      {
        this->ShadowMaps = new std::vector<vtkSmartPointer<vtkTextureObject> >();
        this->ShadowMaps->resize(numberOfShadowLights);
      }

      if (this->LightCameras != nullptr && this->LightCameras->size() != numberOfShadowLights)
      {
        delete this->LightCameras;
        this->LightCameras = nullptr;
      }

      if (this->LightCameras == nullptr)
      {
        this->LightCameras = new std::vector<vtkSmartPointer<vtkCamera> >();
        this->LightCameras->resize(numberOfShadowLights);
      }

      r->SetAutomaticLightCreation(false);

      r->UpdateLightsGeometryToFollowCamera();
      double bb[6];
      vtkMath::UninitializeBounds(bb);
      vtkPropCollection* props = r->GetViewProps();
      vtkCollectionSimpleIterator cookie;
      props->InitTraversal(cookie);
      vtkProp* prop;
      bool first = true;
      while ((prop = props->GetNextProp(cookie)) != nullptr)
      {
        const double* bounds = prop->GetBounds();
        if (!bounds)
        {
          continue;
        }
        if (first)
        {
          bb[0] = bounds[0];
          bb[1] = bounds[1];
          bb[2] = bounds[2];
          bb[3] = bounds[3];
          bb[4] = bounds[4];
          bb[5] = bounds[5];
        }
        else
        {
          bb[0] = (bb[0] < bounds[0] ? bb[0] : bounds[0]);
          bb[1] = (bb[1] > bounds[1] ? bb[1] : bounds[1]);
          bb[2] = (bb[2] < bounds[2] ? bb[2] : bounds[2]);
          bb[3] = (bb[3] > bounds[3] ? bb[3] : bounds[3]);
          bb[4] = (bb[4] < bounds[4] ? bb[4] : bounds[4]);
          bb[5] = (bb[5] > bounds[5] ? bb[5] : bounds[5]);
        }
        first = false;
      }

      lights->InitTraversal();
      l = lights->GetNextItem();
      this->CurrentLightIndex = 0;

      // Setup property keys for actors:
      this->PreRender(s);

      while (l != nullptr)
      {
        if (l->GetSwitch() && this->LightCreatesShadow(l))
        {
          vtkTextureObject* map = (*this->ShadowMaps)[this->CurrentLightIndex];
          if (map == nullptr)
          {
            map = vtkTextureObject::New();
            (*this->ShadowMaps)[this->CurrentLightIndex] = map;
            map->SetMagnificationFilter(vtkTextureObject::Linear);
            map->SetMinificationFilter(vtkTextureObject::Linear);
            map->SetWrapS(vtkTextureObject::ClampToEdge);
            map->SetWrapT(vtkTextureObject::ClampToEdge);
            map->SetWrapR(vtkTextureObject::ClampToEdge);
            map->SetContext(context);
            map->Allocate2D(this->Resolution, this->Resolution, 1, VTK_FLOAT);
            map->Delete();
          }
          vtkCamera* lightCamera = (*this->LightCameras)[this->CurrentLightIndex];
          if (lightCamera == nullptr)
          {
            lightCamera = vtkOpenGLCamera::New();
            (*this->LightCameras)[this->CurrentLightIndex] = lightCamera;
            lightCamera->Delete();
          }

          // Build light camera
          this->BuildCameraLight(l, bb, lightCamera);
          r->SetActiveCamera(lightCamera);

          // map->Activate();
          this->FrameBufferObject->AddColorAttachment(0, map);
          this->FrameBufferObject->ActivateBuffer(0);
          this->FrameBufferObject->Resize(
            static_cast<int>(this->Resolution), static_cast<int>(this->Resolution));
          this->FrameBufferObject->StartNonOrtho(
            static_cast<int>(this->Resolution), static_cast<int>(this->Resolution));

          //          glColorMask(GL_TRUE,GL_FALSE,GL_FALSE,GL_FALSE);
          ostate->vtkglDepthMask(GL_TRUE);
          // glClear(GL_DEPTH_BUFFER_BIT);

          ostate->vtkglEnable(GL_DEPTH_TEST);
          this->OpaqueSequence->Render(&s2);

          this->NumberOfRenderedProps += this->OpaqueSequence->GetNumberOfRenderedProps();

          if (this->CompositeZPass != nullptr)
          {
            this->CompositeZPass->Render(&s2);
          }

          r->SetActiveCamera(realCamera); // reset the camera

#ifdef VTK_SHADOW_MAP_BAKER_PASS_DEBUG
          cout << "finish1 lightIndex=" << lightIndex << endl;
          glFinish();
#endif

          map->Deactivate();
          ++this->CurrentLightIndex;
        }
        l = lights->GetNextItem();
      }
      this->PostRender(s);
      this->LastRenderTime.Modified(); // was a BUG

      // back to the original frame buffer.
      context->GetState()->PopFramebufferBindings();

      // Restore real camera.
      r->SetActiveCamera(realCamera);
      realCamera->UnRegister(this);

      ostate->vtkglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      ostate->vtkglEnable(GL_DEPTH_TEST);
      ostate->vtkglDepthFunc(GL_LEQUAL);

      r->SetAutomaticLightCreation(autoLight);

    } // end of the shadow map creations.
    delete[] propArray;
  }
  else
  {
    vtkWarningMacro(<< " no delegate.");
  }

  vtkOpenGLCheckErrorMacro("failed after Render");
}

//------------------------------------------------------------------------------
bool vtkShadowMapBakerPass::SetShaderParameters(vtkShaderProgram* program, vtkAbstractMapper*,
  vtkProp*, vtkOpenGLVertexArrayObject* vtkNotUsed(VAO))
{
  vtkCamera* lightCamera = (*this->LightCameras)[this->CurrentLightIndex];
  double* crange = lightCamera->GetClippingRange();

  program->SetUniformf("depthC", 11.0);
  program->SetUniformf("nearZ", crange[0]);
  program->SetUniformf("farZ", crange[1]);

  // clipz = (2.0*frag.z - 1)/frag.w
  // Eyez = 2fn/(- f - n) + (f - n)*clipz/( - f - n);
  // Store 0 to 1.0 as near to far

  // for perspective..
  // use (1.0/fragCoord.w - near)/(far - near)
  // which is distance from the lights near plane
  // scaled from 0.0 to 1.0 where 1.0 is the far plane

  // for parallel it is linear in Z

  return true;
}

//------------------------------------------------------------------------------
bool vtkShadowMapBakerPass::PreReplaceShaderValues(
  std::string&, std::string&, std::string& fragmentShader, vtkAbstractMapper*, vtkProp*)
{
  vtkShaderProgram::Substitute(fragmentShader, "//VTK::Light::Dec",
    "//VTK::Light::Dec\n"
    "uniform float nearZ;\n"
    "uniform float farZ;\n"
    "uniform float depthC;\n",
    false);
  vtkShaderProgram::Substitute(fragmentShader, "//VTK::Light::Impl",
    "//VTK::Light::Impl\n"
    "float ldepth =  gl_FragCoord.z;\n" // parallel proj
    "if (cameraParallel == 0) { ldepth =  (1.0/gl_FragCoord.w - nearZ)/(farZ - nearZ); }\n"
    //"gl_FragData[0].r = ldepth;\n",
    "gl_FragData[0].r = exp(depthC * ldepth);\n",
    //"gl_FragDepth = gl_FragDepth;\n",
    false);

  return true;
}

// ----------------------------------------------------------------------------
// Description:
// Build a camera from spot light parameters.
// \pre light_exists: light!=0
// \pre light_is_spotlight: light->LightTypeIsSceneLight() && light->GetPositional() &&
// light->GetConeAngle() < 90.0 \pre camera_exists: camera!=0
void vtkShadowMapBakerPass::BuildCameraLight(vtkLight* light, double* bb, vtkCamera* lcamera)
{
  assert("pre: light_exists" && light != nullptr);
  assert("pre: camera_exists" && lcamera != nullptr);

  lcamera->SetPosition(light->GetTransformedPosition());
  lcamera->SetFocalPoint(light->GetTransformedFocalPoint());

  double dir[3];
  dir[0] = lcamera->GetFocalPoint()[0] - lcamera->GetPosition()[0];
  dir[1] = lcamera->GetFocalPoint()[1] - lcamera->GetPosition()[1];
  dir[2] = lcamera->GetFocalPoint()[2] - lcamera->GetPosition()[2];
  vtkMath::Normalize(dir);
  double vx[3], vup[3];
  vtkMath::Perpendiculars(dir, vx, vup, 0);
  double mNear, mFar;
  BoxNearFar(bb, lcamera->GetPosition(), dir, mNear, mFar);
  lcamera->SetViewUp(vup);

  if (light->GetPositional())
  {
    assert("pre: cone_angle_is_inf_90" && light->GetConeAngle() < 90.0);

    lcamera->SetParallelProjection(0);
    // view angle is an aperture, but cone (or light) angle is between
    // the axis of the cone and a ray along the edge of the cone.
    lcamera->SetViewAngle(light->GetConeAngle() * 2.0);
    // initial clip=(0.1,1000). mNear>0, mFar>mNear);
    double mNearmin = (mFar - mNear) / 100.0;
    if (mNear < mNearmin)
    {
      mNear = mNearmin;
    }
    if (mFar < mNearmin)
    {
      mFar = 2.0 * mNearmin;
    }
    lcamera->SetClippingRange(mNear, mFar);
  }
  else
  {
    lcamera->SetParallelProjection(1);

    double minx, maxx, miny, maxy, minz, maxz;
    double orig[3] = { 0, 0, 0 };
    this->BoxNearFar(bb, orig, vx, minx, maxx);
    this->BoxNearFar(bb, orig, vup, miny, maxy);
    this->BoxNearFar(bb, orig, dir, minz, maxz);

    double sizex, sizey;
    sizex = maxx - minx;
    sizey = maxy - miny;

    double realPos[3];
    realPos[0] = dir[0] * (minz - 1.0) + (minx + maxx) / 2.0 * vx[0] + (miny + maxy) / 2.0 * vup[0];
    realPos[1] = dir[1] * (minz - 1.0) + (minx + maxx) / 2.0 * vx[1] + (miny + maxy) / 2.0 * vup[1];
    realPos[2] = dir[2] * (minz - 1.0) + (minx + maxx) / 2.0 * vx[2] + (miny + maxy) / 2.0 * vup[2];

    lcamera->SetPosition(realPos);
    lcamera->SetFocalPoint(realPos[0] + dir[0], realPos[1] + dir[1], realPos[2] + dir[2]);
    double scale = (sizex > sizey ? sizex : sizey);
    lcamera->SetParallelScale(scale / 2.0);
    lcamera->SetClippingRange(1.0, 1.0 + maxz - minz);
  }
}

// ----------------------------------------------------------------------------
// Description:
// Release graphics resources and ask components to release their own
// resources.
// \pre w_exists: w!=0
void vtkShadowMapBakerPass::ReleaseGraphicsResources(vtkWindow* w)
{
  assert("pre: w_exists" && w != nullptr);
  if (this->OpaqueSequence != nullptr)
  {
    this->OpaqueSequence->ReleaseGraphicsResources(w);
  }

  if (this->CompositeZPass != nullptr)
  {
    this->CompositeZPass->ReleaseGraphicsResources(w);
  }

  if (this->FrameBufferObject != nullptr)
  {
    this->FrameBufferObject->Delete();
    this->FrameBufferObject = nullptr;
  }

  delete this->ShadowMaps;
  this->ShadowMaps = nullptr;

  delete this->LightCameras;
  this->LightCameras = nullptr;
}
