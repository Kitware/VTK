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
#include "vtkImageExport.h"
#include "vtkImplicitHalo.h"
#include "vtkImplicitSum.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIntegerVectorKey.h"
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
#include "vtkSampleFunction.h"
#include "vtkSequencePass.h"
#include "vtkShadowMapBakerPass.h"
#include "vtkTextureObject.h"
#include "vtkTextureUnitManager.h"
#include "vtkTransform.h"
#include "vtk_glew.h"

// debugging
#include "vtkTimerLog.h"
//#include "vtkBreakPoint.h"

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

vtkInformationKeyMacro(vtkShadowMapPass,ShadowMapTextures,IntegerVector);
vtkInformationKeyMacro(vtkShadowMapPass,ShadowMapTransforms,DoubleVector);


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
  if(this->OpaqueSequence!=0)
    {
    this->OpaqueSequence->Delete();
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
    std::vector<int> shadowTextureUnits;
    shadowTextureUnits.resize(lights->GetNumberOfItems());

    // get the shadow maps and activate them
    int shadowingLightIndex = 0;
    int lightIndex = 0;
    vtkLight *light = 0;
    for (lights->InitTraversal(), light = lights->GetNextItem();
          light != 0; light = lights->GetNextItem(), lightIndex++)
      {
      shadowTextureUnits[lightIndex] = -1;
      if(light->GetSwitch() &&
         this->ShadowMapBakerPass->LightCreatesShadow(light) )
        {
        vtkTextureObject *map=
          (*this->ShadowMapBakerPass->GetShadowMaps())[
            static_cast<size_t>(shadowingLightIndex)];
        // activate the texture map
        map->Activate();
        shadowTextureUnits[lightIndex] = map->GetTextureUnit();
        // create some extra shader code for this light

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

    shadowingLightIndex = 0;
    std::vector<double> shadowTransforms;
    for (lights->InitTraversal(), light = lights->GetNextItem(), lightIndex = 0;
          light != 0; light = lights->GetNextItem(), lightIndex++)
      {
      if (shadowTextureUnits[lightIndex] >= 0)
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
        for (int i = 0; i < 16; i++)
          {
          shadowTransforms.push_back(*(tmp->Element[0] + i));
          }
        ++shadowingLightIndex;
        }
      }

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
      info->Set(vtkShadowMapPass::ShadowMapTextures(),
        &(shadowTextureUnits[0]),
        static_cast<int>(shadowTextureUnits.size()));
      info->Set(vtkShadowMapPass::ShadowMapTransforms(),
        &(shadowTransforms[0]),
        static_cast<int>(shadowTransforms.size()));
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

#ifdef VTK_SHADOW_MAP_PASS_DEBUG
    cout << "finish after rendering geometry without shadowing lights" << endl;
    glFinish();
#endif

    }
  else
    {
    vtkWarningMacro(<<" no ShadowMapBakerPass or no OpaqueSequence on the ShadowMapBakerPass.");
    }

  vtkOpenGLCheckErrorMacro("failed after Render");
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

  if(this->IntensityMap!=0)
    {
    this->IntensityMap->Delete();
    this->IntensityMap=0;
    }
}
