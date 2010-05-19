/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShadowMapPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkShadowMapPass.h"
#include "vtkObjectFactory.h"
#include <assert.h>

#include "vtkRenderState.h"
#include "vtkOpenGLRenderer.h"
#include "vtkgl.h"
#include "vtkFrameBufferObject.h"
#include "vtkTextureObject.h"
#include "vtkShaderProgram2.h"
#include "vtkShader2.h"
#include "vtkShader2Collection.h"
#include "vtkUniformVariables.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkTextureUnitManager.h"
#include "vtkInformationIntegerKey.h"
#include "vtkMath.h"

// to be able to dump intermediate passes into png files for debugging.
// only for vtkShadowMapPass developers.
//#define VTK_SHADOW_MAP_PASS_DEBUG
//#define DONT_DUPLICATE_LIGHTS

#include "vtkPNGWriter.h"
#include "vtkImageImport.h"
#include "vtkPixelBufferObject.h"
#include "vtkPixelBufferObject.h"
#include "vtkImageExtractComponents.h"
#include "vtkLightCollection.h"
#include "vtkLight.h"
#include "vtkInformation.h"
#include "vtkCamera.h"
#include "vtkAbstractTransform.h" // for helper classes stack and concatenation
#include "vtkPerspectiveTransform.h"
#include "vtkTransform.h"

#include <vtksys/ios/sstream>
#include "vtkStdString.h"

#include "vtkImageGaussianSource.h"
#include "vtkImageShiftScale.h"
#include "vtkImageExport.h"
#include "vtkImageData.h"
#include "vtkImplicitHalo.h"
#include "vtkSampleFunction.h"

#include "vtkImplicitWindowFunction.h"
#include "vtkImplicitSum.h"

// debugging
#include "vtkOpenGLState.h"
#include "vtkTimerLog.h"

vtkStandardNewMacro(vtkShadowMapPass);
vtkCxxSetObjectMacro(vtkShadowMapPass,OpaquePass,vtkRenderPass);
vtkCxxSetObjectMacro(vtkShadowMapPass,CompositeZPass,vtkRenderPass);

extern const char *vtkShadowMapPassShader_fs;
extern const char *vtkShadowMapPassShader_vs;
extern const char *vtkLighting_s;

vtkInformationKeyMacro(vtkShadowMapPass,OCCLUDER,Integer);
vtkInformationKeyMacro(vtkShadowMapPass,RECEIVER,Integer);

class vtkShadowMapPassTextures
{
public:
  vtksys_stl::vector<vtkSmartPointer<vtkTextureObject> > Vector;
};

class vtkShadowMapPassLightCameras
{
public:
  vtksys_stl::vector<vtkSmartPointer<vtkCamera> > Vector;
};


// helper function to compute the mNearest point in a given direction.
// To be called several times, with initialized = false the first time.
void  PointNearFar(double *v, double *pt, double* dir, double& mNear, double &mFar, bool initialized)
{
  double diff[3];
  diff[0] =  v[0] - pt[0]; diff[1] =  v[1] - pt[1]; diff[2] =  v[2] - pt[2];
  double dot = vtkMath::Dot(diff, dir);
  if(initialized)
    {
    if(dot < mNear)
      {
      mNear = dot;
      }
    if(dot > mFar)
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

// compute the min/max of the projection of a box in a given direction.
void BoxNearFar(double *bb, double *pt, double *dir, double& mNear, double &mFar)
{
  double v[3];
  v[0] = bb[0]; v[1] = bb[2]; v[2] = bb[4];
  PointNearFar(v, pt, dir, mNear, mFar, false);

  v[0] = bb[1]; v[1] = bb[2]; v[2] = bb[4];
  PointNearFar(v, pt, dir, mNear, mFar, true);

  v[0] = bb[0]; v[1] = bb[3]; v[2] = bb[4];
  PointNearFar(v, pt, dir, mNear, mFar, true);

  v[0] = bb[1]; v[1] = bb[3]; v[2] = bb[4];
  PointNearFar(v, pt, dir, mNear, mFar, true);

  v[0] = bb[0]; v[1] = bb[2]; v[2] = bb[5];
  PointNearFar(v, pt, dir, mNear, mFar, true);

  v[0] = bb[1]; v[1] = bb[2]; v[2] = bb[5];
  PointNearFar(v, pt, dir, mNear, mFar, true);

  v[0] = bb[0]; v[1] = bb[3]; v[2] = bb[5];
  PointNearFar(v, pt, dir, mNear, mFar, true);

  v[0] = bb[1]; v[1] = bb[3]; v[2] = bb[5];
  PointNearFar(v, pt, dir, mNear, mFar, true);
}

// ----------------------------------------------------------------------------
vtkShadowMapPass::vtkShadowMapPass()
{
  this->OpaquePass=0;
  this->Resolution=256;

  this->PolygonOffsetFactor=1.1f;
  this->PolygonOffsetUnits=4.0f;

  this->FrameBufferObject=0;
  this->ShadowMaps=0;
  this->LightCameras=0;
  this->Program=0;

  this->IntensityMap=0;
  this->IntensitySource=0;
  this->IntensityExporter=0;
  this->Halo=0;

  this->CompositeZPass=0;
}

// ----------------------------------------------------------------------------
vtkShadowMapPass::~vtkShadowMapPass()
{
  if(this->OpaquePass!=0)
    {
      this->OpaquePass->Delete();
    }

  if(this->CompositeZPass!=0)
    {
      this->CompositeZPass->Delete();
    }

  if(this->FrameBufferObject!=0)
    {
    vtkErrorMacro(<<"FrameBufferObject should have been deleted in ReleaseGraphicsResources().");
    }

  if(this->ShadowMaps!=0)
    {
    vtkErrorMacro(<<"ShadowMaps should have been deleted in ReleaseGraphicsResources().");
    }
  if(this->LightCameras!=0)
    {
    vtkErrorMacro(<<"LightCameras should have been deleted in ReleaseGraphicsResources().");
    }
  if(this->Program!=0)
     {
     this->Program->Delete();
     }

  if(this->IntensityMap!=0)
    {
    vtkErrorMacro(<<"IntensityMap should have been deleted in ReleaseGraphicsResources().");
    }

  if(this->IntensitySource!=0)
    {
    this->IntensitySource->Delete();
    }

  if(this->IntensityExporter!=0)
    {
    this->IntensityExporter->Delete();
    }

  if(this->Halo!=0)
    {
    this->Halo->Delete();
    }
}

// ----------------------------------------------------------------------------
void vtkShadowMapPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "OpaquePass: ";
  if(this->OpaquePass!=0)
    {
    this->OpaquePass->PrintSelf(os,indent);
    }
  else
    {
    os << "(none)" <<endl;
    }

  os << indent << "CompositeZPass: ";
  if(this->CompositeZPass!=0)
    {
    this->CompositeZPass->PrintSelf(os,indent);
    }
  else
    {
    os << "(none)" <<endl;
    }

  os << indent << "Resolution: " << this->Resolution << endl;

  os << indent << "PolygonOffsetFactor: " <<  this->PolygonOffsetFactor
     << endl;
  os << indent << "PolygonOffsetUnits: " << this->PolygonOffsetUnits << endl;
}

// ----------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void vtkShadowMapPass::Render(const vtkRenderState *s)
{
  assert("pre: s_exists" && s!=0);

  this->NumberOfRenderedProps=0;

  vtkOpenGLRenderer *r=static_cast<vtkOpenGLRenderer *>(s->GetRenderer());
  vtkOpenGLRenderWindow *context=static_cast<vtkOpenGLRenderWindow *>(
    r->GetRenderWindow());
#ifdef VTK_SHADOW_MAP_PASS_DEBUG
  vtkOpenGLState *state=new vtkOpenGLState(context);
#endif

  if(this->OpaquePass!=0)
    {
     // Test for Hardware support. If not supported, just render the delegate.
    bool supported=vtkFrameBufferObject::IsSupported(r->GetRenderWindow());

    if(!supported)
      {
      vtkErrorMacro("FBOs are not supported by the context. Cannot use shadow mapping.");
      }
    if(supported)
      {
      supported=vtkTextureObject::IsSupported(r->GetRenderWindow());
      if(!supported)
        {
        vtkErrorMacro("Texture Objects are not supported by the context. Cannot use shadow mapping.");
        }
      }

    if(supported)
      {
      supported=
        vtkShaderProgram2::IsSupported(static_cast<vtkOpenGLRenderWindow *>(
                                         r->GetRenderWindow()));
      if(!supported)
        {
        vtkErrorMacro("GLSL is not supported by the context. Cannot use shadow mapping.");
        }
      }

    if(!supported)
      {
      this->OpaquePass->Render(s);
      this->NumberOfRenderedProps+=
        this->OpaquePass->GetNumberOfRenderedProps();
      return;
      }



    // Shadow mapping requires:
    // 1. at least one spotlight, not front light
    // 2. at least one receiver, in the list of visible props after culling
    // 3. at least one occluder, in the list of visible props before culling

    vtkLightCollection *lights=r->GetLights();
    lights->InitTraversal();
    vtkLight *l=lights->GetNextItem();
    bool hasLight=false;
    bool hasReceiver=false;
    bool hasOccluder=false;
    while(!hasLight && l!=0)
      {
      hasLight=l->GetSwitch() && (!l->GetPositional() || l->GetConeAngle()<180.0);

      l=lights->GetNextItem();
      }

    int propArrayCount=0;
    vtkProp **propArray=0;
    unsigned long latestPropTime=0;

    vtkInformation *requiredKeys=0;
    if(hasLight)
      {
      // at least one receiver?
      requiredKeys=vtkInformation::New();
      requiredKeys->Set(vtkShadowMapPass::RECEIVER(),0); // dummy val.

      int i=0;
      int count=s->GetPropArrayCount();
      while(!hasReceiver && i<count)
        {
        hasReceiver=s->GetPropArray()[i]->HasKeys(requiredKeys);
        ++i;
        }
      if(hasReceiver)
        {
        requiredKeys->Remove(vtkShadowMapPass::RECEIVER());
        requiredKeys->Set(vtkShadowMapPass::OCCLUDER(),0); // dummy val.

        // at least one occluder?

        vtkCollectionSimpleIterator pit;
        vtkPropCollection *props=r->GetViewProps();
        props->InitTraversal(pit);
        vtkProp *p=props->GetNextProp(pit);
        propArray=new vtkProp*[props->GetNumberOfItems()];
        while(p!=0)
          {
          unsigned long mTime=p->GetMTime();
          if(latestPropTime<mTime)
            {
            latestPropTime=mTime;
            }
          if(p->GetVisibility())
            {
            propArray[propArrayCount]=p;
            ++propArrayCount;
            hasOccluder|=p->HasKeys(requiredKeys);
            }
          p=props->GetNextProp(pit);
          }
        }
      }

    if(!hasOccluder)
      {
      // No shadows, just render the scene and return.
      if(requiredKeys!=0)
        {
        requiredKeys->Delete();
        }
#ifdef VTK_SHADOW_MAP_PASS_DEBUG
      if(!hasLight)
        {
        cout << "no spotlight" << endl;
        }
      else
        {

        if(!hasReceiver)
          {
          cout << "no receiver" << endl;
          }
        else
          {
          cout << "no occluder" << endl;
          }
        }
#endif
      if(propArray!=0)
        {
        delete[] propArray;
        }
      this->OpaquePass->Render(s);
      this->NumberOfRenderedProps+=
        this->OpaquePass->GetNumberOfRenderedProps();
      return;
      }

    // Shadow mapping starts here.
    // 1. Create a shadow map for each spotlight.

    // Do we need to recreate shadow maps?
    bool needUpdate=this->LastRenderTime<lights->GetMTime();
    if(!needUpdate)
      {
      lights->InitTraversal();
      l=lights->GetNextItem();
      while(!needUpdate && l!=0)
        {
        // comparison should be this->LastRenderTime<l->GetMTime() but
        // we modify the lights during rendering (enable/disable state)
        // so cannot rely on this time, we use the list time instead.
        needUpdate=this->LastRenderTime<l->GetMTime();
        l=lights->GetNextItem();
        }
      }
    if(!needUpdate)
      {
      needUpdate=this->LastRenderTime<r->GetViewProps()->GetMTime()
        || this->LastRenderTime<latestPropTime;
      }

    if(!needUpdate)
      {
      int i=0;
      while(i<propArrayCount)
        {
        needUpdate=this->LastRenderTime<propArray[i]->GetMTime();
       ++i;
        }
      }
    size_t lightIndex=0;
    bool autoLight=r->GetAutomaticLightCreation()==1;
    vtkCamera *realCamera=r->GetActiveCamera();
    vtkRenderState s2(r);
    if(needUpdate) // create or re-create the shadow maps.
      {
#ifdef VTK_SHADOW_MAP_PASS_DEBUG
      cout << "update the shadow maps" << endl;
#endif
      GLint savedDrawBuffer;
      glGetIntegerv(GL_DRAW_BUFFER,&savedDrawBuffer);

      realCamera->Register(this);

      // 1. Create a new render state with an FBO.

      // We need all the visible props, including the one culled out by the
      // camera,  because they can cast shadows too (ie being visible from the
      // light cameras)
      s2.SetPropArrayAndCount(propArray,propArrayCount);

      if(this->FrameBufferObject==0)
        {
        this->FrameBufferObject=vtkFrameBufferObject::New();
        this->FrameBufferObject->SetContext(context);
        }
      s2.SetFrameBuffer(this->FrameBufferObject);
      requiredKeys->Remove(vtkShadowMapPass::RECEIVER());
      requiredKeys->Set(vtkShadowMapPass::OCCLUDER(),0);
      s2.SetRequiredKeys(requiredKeys);

      lights->InitTraversal();
      l=lights->GetNextItem();
      size_t numberOfSpotLights=0;
      while(l!=0)
        {
        if(l->GetSwitch() && (!l->GetPositional() || l->GetConeAngle()<180.0) )
          {
          ++numberOfSpotLights;
          }
        l=lights->GetNextItem();
        }

      if(this->ShadowMaps!=0 &&
         this->ShadowMaps->Vector.size()!=numberOfSpotLights)
        {
        delete this->ShadowMaps;
        this->ShadowMaps=0;
        }

      if(this->ShadowMaps==0)
        {
        this->ShadowMaps=new vtkShadowMapPassTextures;
        this->ShadowMaps->Vector.resize(numberOfSpotLights);
        }

      if(this->LightCameras!=0 &&
         this->LightCameras->Vector.size()!=numberOfSpotLights)
        {
        delete this->LightCameras;
        this->LightCameras=0;
        }

      if(this->LightCameras==0)
        {
        this->LightCameras=new vtkShadowMapPassLightCameras;
        this->LightCameras->Vector.resize(numberOfSpotLights);
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
      while((prop = props->GetNextProp(cookie)) != NULL)
        {
        double* bounds = prop->GetBounds();
        if(first)
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
      l=lights->GetNextItem();
      lightIndex=0;
      while(l!=0)
        {
        if(l->GetSwitch() && (!l->GetPositional() || l->GetConeAngle()<180.0) )
          {
          vtkTextureObject *map=this->ShadowMaps->Vector[lightIndex];
          if(map==0)
            {
            map=vtkTextureObject::New();
            this->ShadowMaps->Vector[lightIndex]=map;
            map->Delete();
            }

          map->SetContext(context);
          map->SetMinificationFilter(vtkTextureObject::Nearest);
          map->SetLinearMagnification(false);
          map->SetWrapS(vtkTextureObject::ClampToEdge);
          map->SetWrapT(vtkTextureObject::ClampToEdge);
          map->SetWrapR(vtkTextureObject::ClampToEdge);
          if(map->GetWidth()!=this->Resolution ||
             map->GetHeight()!=this->Resolution)
            {
            map->Create2D(this->Resolution,this->Resolution,
                          1,VTK_VOID,false);
            }
          this->FrameBufferObject->SetDepthBufferNeeded(true);
          this->FrameBufferObject->SetDepthBuffer(map);
          this->FrameBufferObject->StartNonOrtho(
            static_cast<int>(this->Resolution),
            static_cast<int>(this->Resolution),false);


          vtkCamera *lightCamera=this->LightCameras->Vector[lightIndex];
          if(lightCamera==0)
            {
            lightCamera=vtkCamera::New();
            this->LightCameras->Vector[lightIndex]=lightCamera;
            lightCamera->Delete();
            }

          // Build light camera
          r->SetActiveCamera(realCamera);


          this->BuildCameraLight(l,bb, lightCamera);
          r->SetActiveCamera(lightCamera);

          glShadeModel(GL_FLAT);
          glDisable(GL_LIGHTING);
          glDisable(GL_COLOR_MATERIAL);
          glDisable(GL_NORMALIZE);
          glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);

          glEnable(GL_POLYGON_OFFSET_FILL);
          glPolygonOffset(this->PolygonOffsetFactor,this->PolygonOffsetUnits);

          glEnable(GL_DEPTH_TEST);
          this->OpaquePass->Render(&s2);

          this->NumberOfRenderedProps+=
            this->OpaquePass->GetNumberOfRenderedProps();

          if(this->CompositeZPass!=0)
            {
            this->CompositeZPass->Render(&s2);
            }

          r->SetActiveCamera(realCamera); //reset the camera

#ifdef VTK_SHADOW_MAP_PASS_DEBUG
          cout << "finish1 lightIndex=" <<lightIndex << endl;
          glFinish();

          state->Update();
          vtkIndent indent;

          vtksys_ios::ostringstream ost00;
          ost00.setf(ios::fixed,ios::floatfield);
          ost00.precision(5);
          ost00 << "OpenGLState_" << pthread_self() << "_"
                << vtkTimerLog::GetUniversalTime() << "_.txt";
          ofstream outfile(ost00.str().c_str());
          state->PrintSelf(outfile,indent);
          outfile.close();
#endif

#ifdef VTK_SHADOW_MAP_PASS_DEBUG
          state->Update();
          vtksys_ios::ostringstream ost01;
          ost01.setf(ios::fixed,ios::floatfield);
          ost01.precision(5);
          ost01 << "OpenGLState_" << pthread_self() << "_"
                << vtkTimerLog::GetUniversalTime() << "_after_compositez.txt";
          ofstream outfile1(ost01.str().c_str());
          state->PrintSelf(outfile1,indent);
          outfile1.close();
#endif

          ++lightIndex;
          }
        l=lights->GetNextItem();
        }
      this->LastRenderTime.Modified(); // was a BUG

      glDisable(GL_POLYGON_OFFSET_FILL);
      glPolygonOffset(0.0f,0.0f);

      // back to the original frame buffer.
      this->FrameBufferObject->UnBind();
      glDrawBuffer(static_cast<GLenum>(savedDrawBuffer));

      // Restore real camera.
      r->SetActiveCamera(realCamera);
      realCamera->UnRegister(this);

      } // end of the shadow map creations.
    delete[] propArray;

    // Copy the list of lights and the lights. We cannot just modify them in
    // place because it will change their modification time.
    // Modification time is used directly (or indirectly if there is some
    // light actors) to avoid rebuilding the shadow maps.


    vtkLightCollection *lights2;
#ifdef DONT_DUPLICATE_LIGHTS
    if(this->CompositeZPass==0)
      {
#endif
      // parallel rendering hangs with this technique
      lights2=vtkLightCollection::New();
      lights->InitTraversal();
      l=lights->GetNextItem();
      lightIndex=0;
      while(l!=0)
        {
        vtkLight *l2=l->ShallowClone();
        lights2->AddItem(l2);
        l2->Delete();
        l=lights->GetNextItem();
        ++lightIndex;
        }

      // save the original light collection.
      lights->Register(this);
      // make the copy the current light collection on the renderer
      r->SetLightCollection(lights2);
#ifdef DONT_DUPLICATE_LIGHTS
      }
    else
      {
      // safe and slow for parallel rendering
      lights2=lights;
      }
#endif

    // Render scene with shadowing lights off.
    // depth writing and testing on.

    // save the lights switches.
    bool *lightSwitches=new bool[lights2->GetNumberOfItems()];

    lights2->InitTraversal();
    l=lights2->GetNextItem();
    lightIndex=0;
    while(l!=0)
      {
      lightSwitches[lightIndex]=l->GetSwitch()==1;
      l=lights2->GetNextItem();
      ++lightIndex;
      }

    r->SetAutomaticLightCreation(false);

    // switch the shadowing lights off.
    lights2->InitTraversal();
    l=lights2->GetNextItem();
    lightIndex=0;
    while(l!=0)
      {
      if(lightSwitches[lightIndex] && (!l->GetPositional() || l->GetConeAngle()<180.0) )
        {
        l->SetSwitch(false);
        }
      l=lights2->GetNextItem();
      ++lightIndex;
      }

    glShadeModel(GL_SMOOTH);
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_NORMALIZE);
    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

#ifdef VTK_SHADOW_MAP_PASS_DEBUG
    cout << "finish before rendering geometry without shadowing lights" << endl;
    glFinish();
#endif

    // render for real for non shadowing lights.
    // note this time we use the list of props after culling.
    this->OpaquePass->Render(s);
    this->NumberOfRenderedProps+=
      this->OpaquePass->GetNumberOfRenderedProps();

    #ifdef VTK_SHADOW_MAP_PASS_DEBUG
    cout << "finish after rendering geometry without shadowing lights" << endl;
    glFinish();
#endif

    // now disable depth writing,
    // For each shadowing light,
    //
    glDepthMask(GL_FALSE);

    // maybe ivar:
    if(this->Program==0)
      {
      this->Program=vtkShaderProgram2::New();
      }
    this->Program->SetContext(context);
    vtkShader2Collection *shaders=this->Program->GetShaders();

    if(needUpdate)
      {
      // we have to perform a concatenation. remove all the shaders first.
      this->Program->ReleaseGraphicsResources();
      shaders->RemoveAllItems();
      size_t nbLights=this->ShadowMaps->Vector.size();
       vtksys_ios::ostringstream ostVS;

       ostVS << "#define VTK_LIGHTING_NUMBER_OF_LIGHTS " << nbLights
             << endl;
       ostVS << vtkShadowMapPassShader_vs;

       vtkStdString *vsCode=new vtkStdString;
       (*vsCode)=ostVS.str();

       vtksys_ios::ostringstream ostLightingVS;

       ostLightingVS << "#define VTK_LIGHTING_NUMBER_OF_LIGHTS " << nbLights
                     << endl;
       ostLightingVS << vtkLighting_s;

       vtkStdString *lightingVsCode=new vtkStdString;
       (*lightingVsCode)=ostLightingVS.str();


       vtksys_ios::ostringstream ostFS;
       ostFS << "#define VTK_LIGHTING_NUMBER_OF_LIGHTS " << nbLights
             << endl;
       ostFS << vtkShadowMapPassShader_fs;

       vtkStdString *fsCode=new vtkStdString;
       (*fsCode)=ostFS.str();



       vtkShader2 *vs=vtkShader2::New();
       vs->SetContext(context);
       vs->SetType(VTK_SHADER_TYPE_VERTEX);
       vs->SetSourceCode(vsCode->c_str());
       delete vsCode;
       shaders->AddItem(vs);
       vs->Delete();

       vtkShader2 *lightingVS=vtkShader2::New();
       lightingVS->SetContext(context);
       lightingVS->SetType(VTK_SHADER_TYPE_VERTEX);
       lightingVS->SetSourceCode(lightingVsCode->c_str());
       delete lightingVsCode;
       shaders->AddItem(lightingVS);
       lightingVS->Delete();

       vtkShader2 *fs=vtkShader2::New();
       fs->SetContext(context);
       fs->SetType(VTK_SHADER_TYPE_FRAGMENT);
       fs->SetSourceCode(fsCode->c_str());
       delete fsCode;
       shaders->AddItem(fs);
       fs->Delete();
      }

#ifdef VTK_SHADOW_MAP_PASS_DEBUG
    this->Program->Build();

    if(this->Program->GetLastBuildStatus()!=VTK_SHADER_PROGRAM2_LINK_SUCCEEDED)
      {
      vtkErrorMacro("Couldn't build the shader program. At this point , it can be an error in a shader or a driver bug.");
      return;
      }

    this->Program->PrintActiveUniformVariablesOnCout();
#endif

    r->SetShaderProgram(this->Program);

    if(this->IntensityMap==0)
      {
      this->IntensityMap=vtkTextureObject::New();
      this->IntensityMap->SetContext(context);
      this->IntensityMap->SetWrapS(vtkTextureObject::Clamp);
      this->IntensityMap->SetWrapT(vtkTextureObject::Clamp);
      this->IntensityMap->SetMinificationFilter(vtkTextureObject::Linear);
      this->IntensityMap->SetLinearMagnification(true);
      }
    if(this->IntensityMap->GetWidth()!=this->Resolution)
      {
       // Load the spotlight intensity map.
      vtkPixelBufferObject *pbo=vtkPixelBufferObject::New();
      pbo->SetContext(context);
      this->BuildSpotLightIntensityMap();
      this->IntensityExporter->Update();

#ifdef VTK_SHADOW_MAP_PASS_DEBUG
      vtkPNGWriter *writer=vtkPNGWriter::New();
      writer->SetFileName("IntensityMap.png");
      writer->SetInput(this->IntensityExporter->GetInput());
      writer->Write();
      writer->Delete();
#endif
      unsigned char *rawPointer=
        static_cast<unsigned char*>(
          this->IntensityExporter->GetPointerToData());

      vtkIdType continuousInc[3];
      vtkImageData *im=this->IntensityExporter->GetInput();
      im->GetContinuousIncrements(im->GetExtent(),continuousInc[0],
                                  continuousInc[1],continuousInc[2]);

      unsigned int dims[2];
      dims[0]=this->Resolution;
      dims[1]=this->Resolution;
      pbo->Upload2D(VTK_UNSIGNED_CHAR,rawPointer,dims,1,continuousInc);

      this->IntensityMap->Create2D(this->Resolution,this->Resolution,1,pbo,
                                   false);
      pbo->Delete();
      }

    // set uniforms
    // set TO,TU

    vtkUniformVariables *u=this->Program->GetUniformVariables();

    vtkMatrix4x4 *tmp=vtkMatrix4x4::New();

    vtkLinearTransform *viewCamera_Inv=
      realCamera->GetViewTransformObject()->GetLinearInverse();

    vtkPerspectiveTransform *transform=vtkPerspectiveTransform::New();

    // identity. pre-multiply mode

    transform->Translate(0.5,0.5,0.5); // bias
    transform->Scale(0.5,0.5,0.5); // scale


    // switch the shadowing lights on.
    lights2->InitTraversal();
    l=lights2->GetNextItem();
    lightIndex=0;
    int shadowingLightIndex=0;
    while(l!=0)
      {
      if(lightSwitches[lightIndex] && (!l->GetPositional() || l->GetConeAngle()<180.0) )
        {
        l->SetSwitch(true);

        // setup texture matrix.
        glMatrixMode(GL_TEXTURE);
        vtkgl::ActiveTexture(vtkgl::TEXTURE0+
                             static_cast<GLenum>(shadowingLightIndex));
        glPushMatrix();
        // scale_bias*projection_light[i]*view_light[i]*view_camera_inv

        vtkCamera *lightCamera=this->LightCameras->Vector[
          static_cast<size_t>(shadowingLightIndex)];
        transform->Push();
        transform->Concatenate(
          lightCamera->GetProjectionTransformObject(1,-1,1));
        transform->Concatenate(lightCamera->GetViewTransformObject());
        transform->Concatenate(viewCamera_Inv);
        transform->GetMatrix(tmp);
        transform->Pop();
        tmp->Transpose();
        glLoadMatrixd(tmp->Element[0]);

        // setup shadowMap texture object and texture unit
        vtkTextureObject *map=this->ShadowMaps->Vector[
          static_cast<size_t>(shadowingLightIndex)];
        map->SetDepthTextureCompare(true);
        map->SetLinearMagnification(true);
        map->SetMinificationFilter(vtkTextureObject::Linear);
        map->Bind();

        vtksys_ios::ostringstream ostShadowMapTextureUnit;
        ostShadowMapTextureUnit << "shadowMaps[" << shadowingLightIndex << "]";

       vtkStdString *shadowMapTextureUnitString=new vtkStdString;
       (*shadowMapTextureUnitString)=ostShadowMapTextureUnit.str();

       u->SetUniformi(shadowMapTextureUnitString->c_str(),1,
                      &shadowingLightIndex);

        delete shadowMapTextureUnitString;

        ++shadowingLightIndex;
        }
      else
        {
        l->SetSwitch(false); // any other light
        }
      l=lights2->GetNextItem();
      ++lightIndex;
      }

    vtkgl::ActiveTexture(vtkgl::TEXTURE0+
                         static_cast<GLenum>(shadowingLightIndex));
    this->IntensityMap->Bind();
    u->SetUniformi("spotLightShape",1,&shadowingLightIndex);

    // DO not delete iewCamera_Inv, this is an internal ivar of vtkTransform.
    transform->Delete();
    tmp->Delete();

#ifdef VTK_SHADOW_MAP_PASS_DEBUG
    cout << "finish before rendering geometry with shadowing lights" << endl;
    glFinish();
#endif

    s2.SetFrameBuffer(s->GetFrameBuffer());
    requiredKeys->Remove(vtkShadowMapPass::OCCLUDER());
    requiredKeys->Set(vtkShadowMapPass::RECEIVER(),0);
    s2.SetRequiredKeys(requiredKeys);
    s2.SetPropArrayAndCount(s->GetPropArray(),s->GetPropArrayCount());

    // Blend the result with the exising scene
    glAlphaFunc(GL_GREATER,0.9f);
    glEnable(GL_ALPHA_TEST);
    // render scene

    bool rendererEraseFlag=r->GetErase()==1;
    r->SetErase(0);

    glMatrixMode(GL_MODELVIEW); // cancel texture matrix mode

    this->OpaquePass->Render(&s2);
    this->NumberOfRenderedProps+=
      this->OpaquePass->GetNumberOfRenderedProps();

    requiredKeys->Delete();

    r->SetErase(rendererEraseFlag);
    glDisable(GL_ALPHA_TEST);

#ifdef VTK_SHADOW_MAP_PASS_DEBUG
    cout << "finish after rendering geometry with shadowing lights" << endl;
    glFinish();
#endif

    // restore texture matrices
    int i=0;
    glMatrixMode(GL_TEXTURE);
    while(i<shadowingLightIndex)
      {
      vtkgl::ActiveTexture(vtkgl::TEXTURE0+
                           static_cast<GLenum>(i));
      glPopMatrix();
      ++i;
      }
    vtkgl::ActiveTexture(vtkgl::TEXTURE0);

    r->SetShaderProgram(0);

    glMatrixMode(GL_MODELVIEW);

#ifdef DONT_DUPLICATE_LIGHTS
    if(this->CompositeZPass==0)
      {
#endif
       // restore original light collection
      r->SetLightCollection(lights);
      lights->Delete();
      lights2->Delete();
#ifdef DONT_DUPLICATE_LIGHTS
      }
    else
      {
      // restore original light switchs.
      lights->InitTraversal();
      l=lights->GetNextItem();
      lightIndex=0;
      while(l!=0)
        {
        l->SetSwitch(lightSwitches[lightIndex]);
        l=lights->GetNextItem();
        ++lightIndex;
        }
      }
#endif
    delete[] lightSwitches;

    r->SetAutomaticLightCreation(autoLight);
    glDepthMask(GL_TRUE);

    }
  else
    {
    vtkWarningMacro(<<" no delegate.");
    }
}

// ----------------------------------------------------------------------------
// Description:
// Build a camera from spot light parameters.
// \pre light_exists: light!=0
// \pre light_is_spotlight: light->LightTypeIsSceneLight() && light->GetPositional() && light->GetConeAngle()<180.0
// \pre camera_exists: camera!=0
void vtkShadowMapPass::BuildCameraLight(vtkLight *light,
                                        double *bb,
                                        vtkCamera* lcamera)
{
  assert("pre: light_exists" && light!=0);
  assert("pre: camera_exists" && lcamera!=0);

  lcamera->SetPosition(light->GetTransformedPosition());
  lcamera->SetFocalPoint(light->GetTransformedFocalPoint());

  double dir[3];
  dir[0] = lcamera->GetFocalPoint()[0]-lcamera->GetPosition()[0];
  dir[1] = lcamera->GetFocalPoint()[1]-lcamera->GetPosition()[1];
  dir[2] = lcamera->GetFocalPoint()[2]-lcamera->GetPosition()[2];
  vtkMath::Normalize(dir);
  double vx[3], vup[3];
  vtkMath::Perpendiculars(dir, vx, vup, 0);
  double mNear, mFar;
  BoxNearFar(bb, lcamera->GetPosition(), dir, mNear, mFar);
  lcamera->SetViewUp(vup);

  if(light->GetPositional())
    {
    assert("pre: cone_angle_is_inf_180" && light->GetConeAngle()<180.0);

    lcamera->SetParallelProjection(0);
    // view angle is an aperture, but cone (or light) angle is between
    // the axis of the cone and a ray along the edge  of the cone.
    lcamera->SetViewAngle(light->GetConeAngle()*2.0);
    // initial clip=(0.1,1000). mNear>0, mFar>mNear);
    double mNearmin = (mFar - mNear) / 100.0;
    if(mNear < mNearmin)
      mNear = mNearmin;
    if(mFar < mNearmin)
      mFar = 2.0*mNearmin;
    lcamera->SetClippingRange(mNear,mFar);
    }
  else
    {
    lcamera->SetParallelProjection(1);

    double minx, maxx, miny, maxy, minz, maxz;
    double orig[3] = {0, 0, 0};
    BoxNearFar(bb, orig, vx, minx, maxx);
    BoxNearFar(bb, orig, vup, miny, maxy);
    BoxNearFar(bb, orig, dir, minz, maxz);

    double sizex, sizey;
    sizex = maxx-minx;
    sizey = maxy-miny;

    double realPos[3];
    realPos[0] = dir[0] * (minz - 1.0) + (minx+maxx) / 2.0 * vx[0] + (miny+maxy) / 2.0 * vup[0];
    realPos[1] = dir[1] * (minz - 1.0) + (minx+maxx) / 2.0 * vx[1] + (miny+maxy) / 2.0 * vup[1];
    realPos[2] = dir[2] * (minz - 1.0) + (minx+maxx) / 2.0 * vx[2] + (miny+maxy) / 2.0 * vup[2];

    lcamera->SetPosition(realPos);
    lcamera->SetFocalPoint(realPos[0] + dir[0], realPos[1] + dir[1], realPos[2] + dir[2]);
    double scale = (sizex > sizey ? sizex: sizey);
    lcamera->SetParallelScale(scale);
    lcamera->SetClippingRange(1.0, 1.0 + maxz - minz);

    }
}

// ----------------------------------------------------------------------------
void vtkShadowMapPass::BuildSpotLightIntensityMap()
{
   if(this->IntensitySource==0)
     {
     this->IntensitySource=vtkSampleFunction::New();
     this->IntensityExporter=vtkImageExport::New();
     this->Halo=vtkImplicitHalo::New();

     vtkImplicitSum *scale=vtkImplicitSum::New();
     scale->AddFunction(this->Halo,255.0);
     scale->SetNormalizeByWeight(false);
     this->IntensitySource->SetImplicitFunction(scale);
     scale->Delete();
     }
   this->Halo->SetRadius(this->Resolution/2.0);
   this->Halo->SetCenter(this->Resolution/2.0,this->Resolution/2.0,0.0);
   this->Halo->SetFadeOut(0.1);

   this->IntensitySource->SetOutputScalarType(VTK_UNSIGNED_CHAR);
   this->IntensitySource->SetSampleDimensions(
     static_cast<int>(this->Resolution),
     static_cast<int>(this->Resolution),1);
   this->IntensitySource->SetModelBounds(0.0,this->Resolution-1.0,
                                         0.0,this->Resolution-1.0,
                                         0.0,0.0);
   this->IntensitySource->SetComputeNormals(false);

   this->IntensityExporter->SetInputConnection(
     this->IntensitySource->GetOutputPort());
}

// ----------------------------------------------------------------------------
// Description:
// Release graphics resources and ask components to release their own
// resources.
// \pre w_exists: w!=0
void vtkShadowMapPass::ReleaseGraphicsResources(vtkWindow *w)
{
  assert("pre: w_exists" && w!=0);
  if(this->OpaquePass!=0)
    {
    this->OpaquePass->ReleaseGraphicsResources(w);
    }

  if(this->CompositeZPass!=0)
    {
    this->CompositeZPass->ReleaseGraphicsResources(w);
    }

  if(this->FrameBufferObject!=0)
    {
    this->FrameBufferObject->Delete();
    this->FrameBufferObject=0;
    }

  if(this->ShadowMaps!=0)
    {
    delete this->ShadowMaps;
    this->ShadowMaps=0;
    }

  if(this->LightCameras!=0)
    {
    delete this->LightCameras;
    this->LightCameras=0;
    }
  if(this->Program!=0)
    {
    this->Program->ReleaseGraphicsResources();
    }

  if(this->IntensityMap!=0)
    {
    this->IntensityMap->Delete();
    this->IntensityMap=0;
    }
}
