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

#include "vtkImageShiftScale.h"
#include "vtkImageExport.h"
#include "vtkImageData.h"
#include "vtkImplicitHalo.h"
#include "vtkSampleFunction.h"

#include "vtkImplicitWindowFunction.h"
#include "vtkImplicitSum.h"

// For vtkShadowMapBakerPassTextures, vtkShadowMapBakerPassLightCameras
#include "vtkShadowMapPassInternal.h"
#include "vtkShadowMapBakerPass.h"
#include "vtkMatrixToLinearTransform.h"

// debugging
#include "vtkOpenGLState.h"
#include "vtkTimerLog.h"
//#include "vtkBreakPoint.h"

vtkStandardNewMacro(vtkShadowMapPass);
vtkCxxSetObjectMacro(vtkShadowMapPass,ShadowMapBakerPass,
                     vtkShadowMapBakerPass);
vtkCxxSetObjectMacro(vtkShadowMapPass,OpaquePass,
                     vtkRenderPass);

extern const char *vtkShadowMapPassShader_fs;
extern const char *vtkShadowMapPassShader_vs;
extern const char *vtkLighting_s;

// ----------------------------------------------------------------------------
vtkShadowMapPass::vtkShadowMapPass()
{
  this->ShadowMapBakerPass=0;
  this->OpaquePass=0;

  this->Program=0;

  this->IntensityMap=0;
  this->IntensitySource=0;
  this->IntensityExporter=0;
  this->Halo=0;
}

// ----------------------------------------------------------------------------
vtkShadowMapPass::~vtkShadowMapPass()
{
  if(this->ShadowMapBakerPass!=0)
    {
    this->ShadowMapBakerPass->Delete();
    }
  if(this->OpaquePass!=0)
    {
    this->OpaquePass->Delete();
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

  os << indent << "ShadowMapBackerPass: ";
  if(this->ShadowMapBakerPass!=0)
    {
    this->ShadowMapBakerPass->PrintSelf(os,indent);
    }
  else
    {
    os << "(none)" <<endl;
    }
  os << indent << "OpaquePass: ";
  if(this->OpaquePass!=0)
    {
    this->OpaquePass->PrintSelf(os,indent);
    }
  else
    {
    os << "(none)" <<endl;
    }
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

  if(this->ShadowMapBakerPass!=0 &&
//     this->ShadowMapBakerPass->GetOpaquePass()!=0
     this->OpaquePass!=0)
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

    if(!this->ShadowMapBakerPass->GetHasShadows())
      {
      this->OpaquePass->Render(s);
      this->NumberOfRenderedProps+=
        this->OpaquePass->GetNumberOfRenderedProps();
      return;
      }

    // Copy the list of lights and the lights. We cannot just modify them in
    // place because it will change their modification time.
    // Modification time is used directly (or indirectly if there is some
    // light actors) to avoid rebuilding the shadow maps.

    vtkLightCollection *lights=r->GetLights();

    vtkLightCollection *lights2;
#ifdef DONT_DUPLICATE_LIGHTS
    if(this->CompositeZPass==0)
      {
#endif
      // parallel rendering hangs with this technique
      lights2=vtkLightCollection::New();
      lights->InitTraversal();
      vtkLight *l=lights->GetNextItem();
      size_t lightIndex=0;
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

    bool autoLight=r->GetAutomaticLightCreation()==1;
    r->SetAutomaticLightCreation(false);

    // switch the shadowing lights off.
    lights2->InitTraversal();
    l=lights2->GetNextItem();
    lightIndex=0;
    while(l!=0)
      {
      if(lightSwitches[lightIndex] &&
         this->ShadowMapBakerPass->LightCreatesShadow(l))
        {
        l->SetSwitch(false);
        }
      l=lights2->GetNextItem();
      ++lightIndex;
      }

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

    if(this->ShadowMapBakerPass->GetNeedUpdate())
      {
      this->ShadowMapBakerPass->SetUpToDate();
      // we have to perform a concatenation. remove all the shaders first.
      this->Program->ReleaseGraphicsResources();
      shaders->RemoveAllItems();
      size_t nbLights=
        this->ShadowMapBakerPass->GetShadowMaps()->Vector.size();
       vtksys_ios::ostringstream ostVS;

       vtksys_ios::ostringstream numLights;
       numLights << endl << "#define VTK_LIGHTING_NUMBER_OF_LIGHTS " << nbLights << endl;

       vtkStdString vertShader(vtkShadowMapPassShader_vs);
       size_t version_loc = vertShader.find("#version 110");

       vertShader.insert(version_loc + 13, numLights.str());

       ostVS << vertShader;

       vtkStdString *vsCode=new vtkStdString;
       (*vsCode)=ostVS.str();

       vtksys_ios::ostringstream ostLightingVS;

       vtkStdString lightShader(vtkLighting_s);
       version_loc = lightShader.find("#version 110");

       lightShader.insert(version_loc + 13, numLights.str());

       ostLightingVS << lightShader;

       vtkStdString *lightingVsCode=new vtkStdString;
       (*lightingVsCode)=ostLightingVS.str();


       vtksys_ios::ostringstream ostFS;

       vtkStdString fragShader(vtkShadowMapPassShader_fs);
       version_loc = fragShader.find("#version 110");

       fragShader.insert(version_loc + 13, numLights.str());

       ostFS << fragShader;

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
      delete[] lightSwitches;
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
    if(this->IntensityMap->GetWidth()!=
       this->ShadowMapBakerPass->GetResolution())
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
      dims[0]=this->ShadowMapBakerPass->GetResolution();
      dims[1]=dims[0];
      pbo->Upload2D(VTK_UNSIGNED_CHAR,rawPointer,dims,1,continuousInc);

      this->IntensityMap->Create2D(this->ShadowMapBakerPass->GetResolution(),
                                   this->ShadowMapBakerPass->GetResolution(),
                                   1,pbo,false);
      pbo->Delete();
      }

    // set uniforms
    // set TO,TU

    vtkUniformVariables *u=this->Program->GetUniformVariables();

    vtkMatrix4x4 *tmp=vtkMatrix4x4::New();

    // WE CANNOT USE THIS WITH Ice-T
//    vtkLinearTransform *viewCamera_Inv=r->GetActiveCamera()
//      ->GetViewTransformObject()->GetLinearInverse();
//    vtkBreakPoint::Break();
    // REQUIRED with Ice-T
    // We assume that at this point of the execution
    // modelview matrix  is actually on the view matrix, that is
    // model matrix is identity.

    GLfloat m[16];
    glGetFloatv(GL_MODELVIEW_MATRIX,m);
    vtkMatrix4x4 *mat=vtkMatrix4x4::New();
    int row=0;
    while(row<4)
      {
      int column=0;
      while(column<4)
        {
        mat->SetElement(row,column,static_cast<double>(m[column*4+row]));
        ++column;
        }
      ++row;
      }
    mat->Invert();
    vtkMatrixToLinearTransform *viewCamera_Inv
      =vtkMatrixToLinearTransform::New();
    viewCamera_Inv->SetInput(mat);
    mat->Delete();

    vtkPerspectiveTransform *transform=vtkPerspectiveTransform::New();

    // identity. pre-multiply mode

    transform->Translate(0.5,0.5,0.5); // bias
    transform->Scale(0.5,0.5,0.5); // scale


    // switch the shadowing lights on.
    lights2->InitTraversal();
    l=lights2->GetNextItem();
    lightIndex=0;
    int shadowingLightIndex=0;

    GLint savedMatrixMode;
    glGetIntegerv(GL_MATRIX_MODE,&savedMatrixMode);

    while(l!=0)
      {
      if(lightSwitches[lightIndex] &&
         this->ShadowMapBakerPass->LightCreatesShadow(l) )
        {
        l->SetSwitch(true);

        // setup texture matrix.
        glMatrixMode(GL_TEXTURE);
        vtkgl::ActiveTexture(vtkgl::TEXTURE0+
                             static_cast<GLenum>(shadowingLightIndex));
        glPushMatrix();
        // scale_bias*projection_light[i]*view_light[i]*view_camera_inv

        vtkCamera *lightCamera=
          this->ShadowMapBakerPass->GetLightCameras()->Vector[
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
        vtkTextureObject *map=
          this->ShadowMapBakerPass->GetShadowMaps()->Vector[
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

    // Ice-T
    viewCamera_Inv->Delete();

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

    vtkRenderState s2(r);
    s2.SetFrameBuffer(s->GetFrameBuffer());
    vtkInformation *requiredKeys=vtkInformation::New();
    requiredKeys->Set(vtkShadowMapBakerPass::RECEIVER(),0);
    s2.SetRequiredKeys(requiredKeys);
    s2.SetPropArrayAndCount(s->GetPropArray(),s->GetPropArrayCount());

    // Blend the result with the exising scene
    glAlphaFunc(GL_GREATER,0.9f);
    glEnable(GL_ALPHA_TEST);
    // render scene

    bool rendererEraseFlag=r->GetErase()==1;
    r->SetErase(0);

    glMatrixMode(savedMatrixMode);

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

    glMatrixMode(savedMatrixMode);

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
    vtkWarningMacro(<<" no ShadowMapBakerPass or no OpaquePass on the ShadowMapBakerPass.");
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
   unsigned int resolution=this->ShadowMapBakerPass->GetResolution();

   this->Halo->SetRadius(resolution/2.0);
   this->Halo->SetCenter(resolution/2.0,
                         resolution/2.0,0.0);
   this->Halo->SetFadeOut(0.1);

   this->IntensitySource->SetOutputScalarType(VTK_UNSIGNED_CHAR);
   this->IntensitySource->SetSampleDimensions(
     static_cast<int>(resolution),
     static_cast<int>(resolution),1);
   this->IntensitySource->SetModelBounds(0.0,resolution-1.0,
                                         0.0,resolution-1.0,
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
  if(this->ShadowMapBakerPass!=0)
    {
    this->ShadowMapBakerPass->ReleaseGraphicsResources(w);
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
