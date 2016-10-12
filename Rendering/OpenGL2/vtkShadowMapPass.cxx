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

#include "vtkAbstractTransform.h" // for helper classes stack and concatenation
#include "vtkCamera.h"
#include "vtkFrameBufferObject.h"
#include "vtkImageData.h"
#include "vtkImplicitSum.h"
#include "vtkInformation.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkLightsPass.h"
#include "vtkMath.h"
#include "vtkMatrixToLinearTransform.h"
#include "vtkNew.h"
#include "vtkOpaquePass.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPerspectiveTransform.h"
#include "vtkRenderPassCollection.h"
#include "vtkRenderState.h"
#include "vtkSequencePass.h"
#include "vtkShaderProgram.h"
#include "vtkShadowMapBakerPass.h"
#include "vtkTextureObject.h"
#include "vtkTextureUnitManager.h"
#include "vtkTransform.h"
#include "vtk_glew.h"

// debugging
#include "vtkTimerLog.h"

#include <cassert>
#include <sstream>
#include "vtkStdString.h"


// to be able to dump intermediate passes into png files for debugging.
// only for vtkShadowMapPass developers.
//#define VTK_SHADOW_MAP_PASS_DEBUG
//#define DONT_DUPLICATE_LIGHTS


vtkStandardNewMacro(vtkShadowMapPass);
vtkCxxSetObjectMacro(vtkShadowMapPass,ShadowMapBakerPass,
                     vtkShadowMapBakerPass);
vtkCxxSetObjectMacro(vtkShadowMapPass,OpaqueSequence,
                     vtkRenderPass);

vtkInformationKeyMacro(vtkShadowMapPass,ShadowMapPass,ObjectBase);


// ----------------------------------------------------------------------------
vtkShadowMapPass::vtkShadowMapPass()
{
  this->ShadowMapBakerPass=0;

  vtkNew<vtkSequencePass> seqP;
  vtkNew<vtkLightsPass> lightP;
  vtkNew<vtkOpaquePass> opaqueP;
  vtkNew<vtkRenderPassCollection> rpc;
  rpc->AddItem(lightP.Get());
  rpc->AddItem(opaqueP.Get());
  seqP->SetPasses(rpc.Get());

  this->OpaqueSequence=0;
  this->SetOpaqueSequence(seqP.Get());

  vtkNew<vtkShadowMapBakerPass> bp;
  this->ShadowMapBakerPass = 0;
  this->SetShadowMapBakerPass(bp.Get());
}

// ----------------------------------------------------------------------------
vtkShadowMapPass::~vtkShadowMapPass()
{
  if(this->ShadowMapBakerPass!=0)
  {
    this->ShadowMapBakerPass->Delete();
  }
  if(this->OpaqueSequence!=0)
  {
    this->OpaqueSequence->Delete();
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
  os << indent << "OpaqueSequence: ";
  if(this->OpaqueSequence!=0)
  {
    this->OpaqueSequence->PrintSelf(os,indent);
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

  vtkOpenGLClearErrorMacro();

  this->NumberOfRenderedProps=0;

  vtkOpenGLRenderer *r = static_cast<vtkOpenGLRenderer *>(s->GetRenderer());
  vtkOpenGLCamera *cam = static_cast<vtkOpenGLCamera *>(r->GetActiveCamera());
  vtkOpenGLRenderWindow *context = static_cast<vtkOpenGLRenderWindow *>(
    r->GetRenderWindow());

  if(this->ShadowMapBakerPass != 0 &&
     this->OpaqueSequence != 0)
  {
     // Test for Hardware support. If not supported, just render the delegate.
    bool supported=vtkFrameBufferObject::IsSupported(context);

    if(!supported)
    {
      vtkErrorMacro("FBOs are not supported by the context. Cannot use shadow mapping.");
      this->OpaqueSequence->Render(s);
      this->NumberOfRenderedProps+=
        this->OpaqueSequence->GetNumberOfRenderedProps();
      return;
    }

    if(!this->ShadowMapBakerPass->GetHasShadows())
    {
      this->OpaqueSequence->Render(s);
      this->NumberOfRenderedProps+=
        this->OpaqueSequence->GetNumberOfRenderedProps();
      return;
    }

    vtkLightCollection *lights=r->GetLights();
    this->ShadowTextureUnits.clear();
    this->ShadowTextureUnits.resize(lights->GetNumberOfItems());
    this->ShadowAttenuation.clear();
    this->ShadowAttenuation.resize(lights->GetNumberOfItems());

    // get the shadow maps and activate them
    int shadowingLightIndex = 0;
    int lightIndex = 0;
    vtkLight *light = 0;
    for (lights->InitTraversal(), light = lights->GetNextItem();
          light != 0; light = lights->GetNextItem(), lightIndex++)
    {
      this->ShadowTextureUnits[lightIndex] = -1;
      if(light->GetSwitch() &&
         this->ShadowMapBakerPass->LightCreatesShadow(light) )
      {
        vtkTextureObject *map=
          (*this->ShadowMapBakerPass->GetShadowMaps())[
            static_cast<size_t>(shadowingLightIndex)];
        // activate the texture map
        map->Activate();
        this->ShadowTextureUnits[lightIndex] = map->GetTextureUnit();
        this->ShadowAttenuation[lightIndex] = light->GetShadowAttenuation();
        shadowingLightIndex++;
      }
    }

    vtkMatrix4x4 *tmp = vtkMatrix4x4::New();
    vtkMatrix4x4 *mat = vtkMatrix4x4::New();
    vtkPerspectiveTransform *transform = vtkPerspectiveTransform::New();

    vtkMatrix4x4 *wcdc;
    vtkMatrix4x4 *wcvc;
    vtkMatrix3x3 *norms;
    vtkMatrix4x4 *vcdc;
    cam->GetKeyMatrices(r,wcvc,norms,vcdc,wcdc);

    mat->DeepCopy(wcvc);
    mat->Transpose();
    mat->Invert();

    vtkMatrixToLinearTransform *viewCamera_Inv
      = vtkMatrixToLinearTransform::New();
    viewCamera_Inv->SetInput(mat);
    mat->Delete();

    // identity. pre-multiply mode
    transform->Translate(0.5,0.5,0.5); // bias
    transform->Scale(0.5,0.5,0.5); // scale

    this->ShadowTransforms.clear();
    shadowingLightIndex = 0;
    for (lights->InitTraversal(), light = lights->GetNextItem(), lightIndex = 0;
          light != 0; light = lights->GetNextItem(), lightIndex++)
    {
      if (this->ShadowTextureUnits[lightIndex] >= 0)
      {
        vtkCamera *lightCamera=
          (*this->ShadowMapBakerPass->GetLightCameras())[
          static_cast<size_t>(shadowingLightIndex)];
        transform->Push();
        transform->Concatenate(
          lightCamera->GetProjectionTransformObject(1,-1,1));
        transform->Concatenate(lightCamera->GetViewTransformObject());
        transform->Concatenate(viewCamera_Inv);
        transform->GetMatrix(tmp);
        transform->Pop();
        tmp->Transpose();
        for (int i = 0; i < 4; i++)
        {
          for (int j = 0; j < 4; j++)
          {
            this->ShadowTransforms.push_back(tmp->Element[i][j]);
          }
        }
        ++shadowingLightIndex;
      }
    }

    // build the shader code
    this->BuildShaderCode();

    // set the prop keys
    int c = s->GetPropArrayCount();
    for (int i = 0; i < c; i++)
    {
      vtkProp *p=s->GetPropArray()[i];
      vtkInformation *info = p->GetPropertyKeys();
      if (!info)
      {
        info = vtkInformation::New();
        p->SetPropertyKeys(info);
        info->Delete();
      }
      info->Set(vtkShadowMapPass::ShadowMapPass(), this);
    }

    viewCamera_Inv->Delete();
    transform->Delete();
    tmp->Delete();

    // render with shadows
    // note this time we use the list of props after culling.
    this->OpaqueSequence->Render(s);
    this->NumberOfRenderedProps+=
      this->OpaqueSequence->GetNumberOfRenderedProps();

    // now deactivate the shadow maps
    shadowingLightIndex = 0;
    for (lights->InitTraversal(), light = lights->GetNextItem(), lightIndex = 0;
          light != 0; light = lights->GetNextItem(), lightIndex++)
    {
      if(light->GetSwitch() &&
         this->ShadowMapBakerPass->LightCreatesShadow(light) )
      {
        vtkTextureObject *map=
          (*this->ShadowMapBakerPass->GetShadowMaps())[
            static_cast<size_t>(shadowingLightIndex)];
        // activate the texture map
        map->Deactivate();
        shadowingLightIndex++;
      }
    }

  }
  else
  {
    vtkWarningMacro(<<" no ShadowMapBakerPass or no OpaqueSequence on the ShadowMapBakerPass.");
  }

  vtkOpenGLCheckErrorMacro("failed after Render");
}

void vtkShadowMapPass::SetUniforms(vtkShaderProgram *program)
{
  size_t numLights = this->ShadowTextureUnits.size();

  // how many lights have shadow maps
  int numSMT = 0;
  float transform[16];
  std::ostringstream toString;

  for (size_t i = 0; i < numLights; i++)
  {
    if (this->ShadowTextureUnits[i] >= 0)
    {
      for (int j = 0; j < 16; j++)
      {
        transform[j] = this->ShadowTransforms[numSMT*16 + j];
      }
      toString.str("");
      toString.clear();
      toString << numSMT;
      program->SetUniformf(
        std::string("shadowAttenuation"+toString.str()).c_str(),
        this->ShadowAttenuation[i]);
      program->SetUniformi(
        std::string("shadowMap"+toString.str()).c_str(),
        this->ShadowTextureUnits[i]);
      program->SetUniformMatrix4x4(
        std::string("shadowTransform"+toString.str()).c_str(),
        transform);
      numSMT++;
    }
  }
}

void vtkShadowMapPass::BuildShaderCode()
{
  size_t numLights = this->ShadowTextureUnits.size();

  // count how many lights have shadow maps
  int numSMT = 0;
  for (size_t i = 0; i < numLights; i++)
  {
    if (this->ShadowTextureUnits[i] >= 0)
    {
      numSMT++;
    }
  }

  std::ostringstream toString;
  toString.str("");
  toString.clear();
  toString << this->ShadowMapBakerPass->GetResolution();

  std::string fdec = "//VTK::Light::Dec\n"
    "float calcShadow(in vec4 vert,\n"
    "                  in sampler2D shadowMap,\n"
    "                  in mat4 shadowTransform,\n"
    "                  in float attenuation)\n"
    "{\n"
    "  vec4 shadowCoord = shadowTransform*vert;\n"
    "  float result = 1.0;\n"
    "  if(shadowCoord.w > 0.0)\n"
    "    {\n"
    "    vec2 projected = shadowCoord.xy/shadowCoord.w;\n"
    "    if(projected.x >= 0.0 && projected.x <= 1.0\n"
    "       && projected.y >= 0.0 && projected.y <= 1.0)\n"
    "      {\n"
    "      result = 0.0;\n"
    "      float zval = shadowCoord.z - 0.005;\n"
    "      vec2 projT = projected*" + toString.str() + ";\n"
    "      projT = fract(projT);\n"
    "      if (texture2D(shadowMap,projected + (vec2(-1.0,-1.0)/" + toString.str() + ")).r - zval > 0.0) { result = result + (1.0-projT.x)*(1.0-projT.y); }\n"
    "      if (texture2D(shadowMap,projected + (vec2(0.0,-1.0)/" + toString.str() + ")).r - zval > 0.0) { result = result + (1.0-projT.y); }\n"
    "      if (texture2D(shadowMap,projected + (vec2(1.0,-1.0)/" + toString.str() + ")).r - zval > 0.0) { result = result + projT.x*(1.0-projT.y); }\n"
    "      if (texture2D(shadowMap,projected + (vec2(1.0,0.0)/" + toString.str() + ")).r - zval > 0.0) { result = result + projT.x; }\n"
    "      if (texture2D(shadowMap,projected + (vec2(0.0,0.0)/" + toString.str() + ")).r - zval > 0.0) { result = result + 1.0; }\n"
    "      if (texture2D(shadowMap,projected + (vec2(-1.0,0.0)/" + toString.str() + ")).r - zval > 0.0) { result = result + (1.0-projT.x); }\n"
    "      if (texture2D(shadowMap,projected + (vec2(0.0,1.0)/" + toString.str() + ")).r - zval > 0.0) { result = result + projT.y; }\n"
    "      if (texture2D(shadowMap,projected + (vec2(-1.0,1.0)/" + toString.str() + ")).r - zval > 0.0) { result = result + (1.0-projT.x)*projT.y; }\n"
    "      if (texture2D(shadowMap,projected + (vec2(1.0,1.0)/" + toString.str() + ")).r - zval > 0.0) { result = result + projT.x*projT.y; }\n"
    "      result /= 4.0;\n"
    "      }\n"
    "    }\n"
    "  return (1.0 - attenuation + attenuation*result);\n"
//    "  return result;\n"
    "}\n";

  for (int i = 0; i < numSMT; i++)
  {
    toString.str("");
    toString.clear();
    toString << i;
    fdec +=
    "uniform float shadowAttenuation" + toString.str() + ";\n"
    "uniform sampler2D shadowMap" + toString.str() + ";\n"
    "uniform mat4 shadowTransform" + toString.str() + ";\n";
  }

  // build the code for the lighting factors
  std::string fimpl = "float factors[6];\n";
  numSMT = 0;
  for (size_t i = 0; i < 6; i++)
  {
    toString.str("");
    toString.clear();
    toString << i;
    fimpl += "  factors[" + toString.str() + "] = ";
    if (i < numLights && this->ShadowTextureUnits[i] >= 0)
    {
      std::ostringstream toString2;
      toString2 << numSMT;
      fimpl += "calcShadow(vertexVC, shadowMap" +toString2.str() +
        ", shadowTransform" + toString2.str() +
        ", shadowAttenuation" + toString2.str() +");\n";
      numSMT++;
    }
    else
    {
      fimpl += "1.0;\n";
    }
  }

  // compute the factors then do the normal lighting
  fimpl += "//VTK::Light::Impl\n";
  this->FragmentDeclaration = fdec;
  this->FragmentImplementation = fimpl;
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
}
