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
#include <cassert>

#include "vtkRenderState.h"
#include "vtkOpenGLRenderer.h"
#include "vtk_glew.h"
#include "vtkFrameBufferObject.h"
#include "vtkTextureObject.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLError.h"
#include "vtkTextureUnitManager.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIntegerVectorKey.h"
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

#include "vtkInformationDoubleVectorKey.h"

// debugging
#include "vtkTimerLog.h"
//#include "vtkBreakPoint.h"

vtkStandardNewMacro(vtkShadowMapPass);
vtkCxxSetObjectMacro(vtkShadowMapPass,ShadowMapBakerPass,
                     vtkShadowMapBakerPass);
vtkCxxSetObjectMacro(vtkShadowMapPass,OpaquePass,
                     vtkRenderPass);

vtkInformationKeyMacro(vtkShadowMapPass,ShadowMapTextures,IntegerVector);
vtkInformationKeyMacro(vtkShadowMapPass,ShadowMapTransforms,DoubleVector);


// ----------------------------------------------------------------------------
vtkShadowMapPass::vtkShadowMapPass()
{
  this->ShadowMapBakerPass=0;
  this->OpaquePass=0;

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

  vtkOpenGLClearErrorMacro();

  this->NumberOfRenderedProps=0;

  vtkOpenGLRenderer *r = static_cast<vtkOpenGLRenderer *>(s->GetRenderer());
  vtkOpenGLCamera *cam = static_cast<vtkOpenGLCamera *>(r->GetActiveCamera());
  vtkOpenGLRenderWindow *context = static_cast<vtkOpenGLRenderWindow *>(
    r->GetRenderWindow());

  if(this->ShadowMapBakerPass != 0 &&
     this->OpaquePass != 0)
    {
     // Test for Hardware support. If not supported, just render the delegate.
    bool supported=vtkFrameBufferObject::IsSupported(context);

    if(!supported)
      {
      vtkErrorMacro("FBOs are not supported by the context. Cannot use shadow mapping.");
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
          this->ShadowMapBakerPass->GetShadowMaps()->Vector[
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
    this->OpaquePass->Render(s);
    this->NumberOfRenderedProps+=
      this->OpaquePass->GetNumberOfRenderedProps();

    // now deactivate the shadow maps
    shadowingLightIndex = 0;
    for (lights->InitTraversal(), light = lights->GetNextItem(), lightIndex = 0;
          light != 0; light = lights->GetNextItem(), lightIndex++)
      {
      if(light->GetSwitch() &&
         this->ShadowMapBakerPass->LightCreatesShadow(light) )
        {
        vtkTextureObject *map=
          this->ShadowMapBakerPass->GetShadowMaps()->Vector[
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
    vtkWarningMacro(<<" no ShadowMapBakerPass or no OpaquePass on the ShadowMapBakerPass.");
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
