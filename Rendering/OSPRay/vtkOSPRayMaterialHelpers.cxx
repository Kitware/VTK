/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSPRayMaterialHelpers.cpp

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOSPRayMaterialHelpers.h"
#include "vtkImageData.h"
#include "vtkOSPRayMaterialLibrary.h"
#include "vtkOSPRayRendererNode.h"
#include "vtkProperty.h"
#include "vtkTexture.h"

#include "ospray/ospray.h"
#include "ospray/version.h"

#define VTK_OSPRAY_VERSION \
  OSPRAY_VERSION_MAJOR * 10000 + \
  OSPRAY_VERSION_MINOR * 100 + \
  OSPRAY_VERSION_PATCH

//------------------------------------------------------------------------------
OSPTexture2D vtkOSPRayMaterialHelpers::VTKToOSPTexture
  (vtkImageData *vColorTextureMap)
{
  unsigned char *ochars = nullptr;
  void *obuffer;
  int xsize = vColorTextureMap->GetExtent()[1];
  int ysize = vColorTextureMap->GetExtent()[3];
  bool incompatible = false;
  int scalartype = vColorTextureMap->GetScalarType();
  if (scalartype != VTK_UNSIGNED_CHAR &&
      scalartype != VTK_CHAR &&
      scalartype != VTK_FLOAT)
  {
    incompatible = true;
  }
  int comps = vColorTextureMap->GetNumberOfScalarComponents();
  if (comps != 1 && comps != 3 && comps != 4)
  {
    incompatible = true;
  }
  if (incompatible)
  {
    vtkGenericWarningMacro("Problem, incompatible texture type. Defaulting to black texture.");
    ochars = new unsigned char[(xsize+1)*(ysize+1)*3];
    unsigned char *oc = ochars;
    for (int i = 0; i <= xsize; ++i)
    {
      for (int j = 0; j <= ysize; ++j)
      {
        oc[0] = 0;
        oc[1] = 0;
        oc[2] = 0;
        oc+=3;
      }
    }
    obuffer = (void*)ochars;
  } else {
    obuffer = vColorTextureMap->GetScalarPointer();
  }
  OSPTexture2D t2d;
  OSPTextureFormat ospformat = OSP_TEXTURE_RGB8;
  if (scalartype == VTK_FLOAT)
  {
    if (comps == 1)
    {
      ospformat = OSP_TEXTURE_R32F;
    }
    else if (comps == 3)
    {
      ospformat = OSP_TEXTURE_RGB32F;
    }
    else if (comps == 4)
    {
      ospformat = OSP_TEXTURE_RGBA32F;
    }
  }
  else
  {
    if (comps == 1)
    {
      ospformat = OSP_TEXTURE_R8;
    }
    else if (comps == 3)
    {
      ospformat = OSP_TEXTURE_RGB8;
    }
    else if (comps == 4)
    {
      ospformat = OSP_TEXTURE_RGBA8;
    }
  }
  t2d = ospNewTexture2D
    (
     osp::vec2i{xsize+1,
         ysize+1},
     ospformat,
     obuffer,
     OSP_TEXTURE_FILTER_NEAREST|OSP_TEXTURE_SHARED_BUFFER);
  ospCommit(t2d);
  if (incompatible)
  {
    delete[] ochars;
  }
  return t2d;
}

//------------------------------------------------------------------------------
void vtkOSPRayMaterialHelpers::MakeMaterials
  (vtkOSPRayRendererNode *orn,
   OSPRenderer oRenderer,
   std::map<std::string, OSPMaterial> &mats)
{
  vtkOSPRayMaterialLibrary *ml = vtkOSPRayRendererNode::GetMaterialLibrary(orn->GetRenderer());
  if (!ml)
  {
    cout << "No material Library in this renderer." << endl;
    return;
  }
  std::set<std::string > nicknames = ml->GetMaterialNames();
  std::set<std::string >::iterator it = nicknames.begin();
  while (it != nicknames.end())
  {
    OSPMaterial newmat = vtkOSPRayMaterialHelpers::MakeMaterial
      (orn, oRenderer, *it);
    mats[*it] = newmat;
    ++it;
  }
}

#define OSPSETNF(attname) \
  std::vector<double> attname = \
    ml->GetDoubleShaderVariable(nickname, #attname); \
  if (attname.size() > 0) \
  { \
    float *fname = new float[attname.size()]; \
    for (size_t i = 0; i < attname.size(); i++) \
    { \
      fname[i] = static_cast<float>(attname[i]); \
    } \
    OSPData data = ospNewData(attname.size()/3, OSP_FLOAT3, fname); \
    ospSetData(oMaterial, #attname, data); \
    delete[] fname; \
  }

#define OSPSET3F(attname)                                   \
  std::vector<double> attname = ml->GetDoubleShaderVariable \
      (nickname, #attname); \
  if (attname.size() == 3) \
  { \
    float fname[3] = {static_cast<float>(attname[0]), \
                       static_cast<float>(attname[1]), \
                       static_cast<float>(attname[2])}; \
    ospSet3fv(oMaterial, #attname, fname); \
  }

#define OSPSET1F(attname) \
  std::vector<double> attname = ml->GetDoubleShaderVariable \
      (nickname, #attname); \
  if (attname.size() == 1) \
  { \
    ospSetf(oMaterial, #attname, static_cast<float>(attname[0]));     \
  }

#define OSPSETTEXTURE(texname) \
  vtkTexture *texname = ml->GetTexture(nickname, #texname); \
  if (texname) \
  { \
    vtkImageData* vColorTextureMap = vtkImageData::SafeDownCast(texname->GetInput()); \
    OSPTexture2D t2d = vtkOSPRayMaterialHelpers::VTKToOSPTexture(vColorTextureMap); \
    ospSetObject(oMaterial, #texname, ((OSPTexture2D)(t2d))); \
  }

//------------------------------------------------------------------------------
OSPMaterial vtkOSPRayMaterialHelpers::MakeMaterial
  (vtkOSPRayRendererNode *orn,
  OSPRenderer oRenderer, std::string nickname)
{
  OSPMaterial oMaterial;
  vtkOSPRayMaterialLibrary *ml = vtkOSPRayRendererNode::GetMaterialLibrary(orn->GetRenderer());
  if (!ml)
    {
    cout << "No material Library in this renderer. Using OBJMaterial by default." << endl;
    oMaterial = NewMaterial(orn, oRenderer, "OBJMaterial");
    return oMaterial;
    }
  //todo: add a level of indirection and/or versioning so we aren't stuck with
  //these names forever
  std::string implname = ml->LookupImplName(nickname);
  if (implname == "Glass")
  {
    oMaterial = NewMaterial(orn, oRenderer, implname);
    OSPSET3F(attenuationColor);
    OSPSET1F(etaInside);
    OSPSET1F(etaOutside);
    OSPSET3F(attenuationColorOutside);
    OSPSET1F(attenuationDistance);
  }
  else if (implname == "Metal")
  {
    oMaterial = NewMaterial(orn, oRenderer, implname);
    OSPSET3F(eta);
    OSPSET3F(k);
    OSPSET1F(roughness);
    OSPSET3F(reflectance);
    OSPSETNF(ior);
  }
  else if (implname == "MetallicPaint")
  {
    oMaterial = NewMaterial(orn, oRenderer, implname);
    OSPSET3F(baseColor)
    OSPSET3F(color)
    OSPSET1F(flakeAmount)
    OSPSET3F(flakeColor)
    OSPSET1F(flakeSpread)
    OSPSET1F(eta)
  }
  else if (implname == "OBJMaterial")
  {
    oMaterial = NewMaterial(orn, oRenderer, implname);
    OSPSET1F(alpha);//aka "d", default 1.0
    OSPSET1F(d);//aka "d", default 1.0
    OSPSET3F(color);//aka "Kd" aka "kd", default (0.8,0.8,0.8)
    OSPSET3F(kd);//aka "Kd" aka "kd", default (0.8,0.8,0.8)
    OSPSET3F(Kd);//aka "Kd" aka "kd", default (0.8,0.8,0.8)
    OSPSET3F(ks);//aka "Ks", default (0.0,0.0,0.0)
    OSPSET3F(Ks);//aka "Ks", default (0.0,0.0,0.0)
    OSPSET1F(ns);//aka "Ns", default 10.0
    OSPSET1F(Ns);//aka "Ns", default 10.0
    OSPSET3F(tf);//aka "Tf", default (0.0,0.0,0.0)
    OSPSET3F(Tf);//aka "Tf", default (0.0,0.0,0.0)
    OSPSETTEXTURE(map_d);
    OSPSETTEXTURE(map_kd);
    OSPSETTEXTURE(map_Kd);
    OSPSETTEXTURE(colorMap);
    OSPSETTEXTURE(map_ks);
    OSPSETTEXTURE(map_Ks);
    OSPSETTEXTURE(map_ns);
    OSPSETTEXTURE(map_Ns);
    OSPSETTEXTURE(map_bump);
    OSPSETTEXTURE(map_Bump);
    OSPSETTEXTURE(normalmap);
    OSPSETTEXTURE(BumpMap);

    /*
    //todo hookup these texture transforms up, for now could be just in 9 long double vectors, but should really be a 3x3
    affine2f xform_d  = getTextureTransform("map_d");
    affine2f xform_Kd = getTextureTransform("map_Kd") * getTextureTransform("map_kd") * getTextureTransform("colorMap");
    affine2f xform_Ks = getTextureTransform("map_Ks") * getTextureTransform("map_ks");
    affine2f xform_Ns = getTextureTransform("map_Ns") * getTextureTransform("map_ns");
    affine2f xform_Bump = getTextureTransform("map_Bump") * getTextureTransform("map_bump") * getTextureTransform("normalMap") * getTextureTransform("BumpMap");
    */
  }
  else if (implname == "ThinGlass")
  {
    oMaterial = NewMaterial(orn, oRenderer, implname);
    OSPSET3F(transmission);
    OSPSET3F(color);
    OSPSET3F(attenuationColor);
    OSPSET1F(attenuationDistance);
    OSPSET1F(eta);
    OSPSET1F(thickness);
  }

#if VTK_OSPRAY_VERSION >= 10401 // 1.4.1
  // Alloy added in 1.4.1
  else if (implname == "Alloy")
  {
    oMaterial = NewMaterial(orn, oRenderer, implname);
    OSPSET3F(color);
    OSPSET3F(edgeColor);
    OSPSET1F(roughness);
  }
#endif

#if VTK_OSPRAY_VERSION < 10600 // 1.6.0
  // Matte, Plastic, Velvet no longer listed in documentation in 1.6.0
  else if (implname == "Matte")
  {
    oMaterial = NewMaterial(orn, oRenderer, implname);
    OSPSET3F(reflectance);
  }
  else if (implname == "Plastic")
  {
    oMaterial = NewMaterial(orn, oRenderer, implname);
    OSPSET3F(pigmentColor);
    OSPSET1F(eta);
    OSPSET1F(roughness);
    OSPSET1F(thickness);
  }
  else if (implname == "Velvet")
  {
    oMaterial = NewMaterial(orn, oRenderer, implname);
    OSPSET3F(reflectance);
    OSPSET1F(backScattering);
    OSPSET3F(horizonScatteringColor);
    OSPSET1F(horizonScatteringFallOff);
  }
#else // OSPRay >= 1.6.0
  // Principled and CarPaint added in 1.6.0
  else if (implname == "Principled")
  {
    oMaterial = NewMaterial(orn, oRenderer, implname);
    OSPSET3F(baseColor);
    OSPSET3F(edgeColor);
    OSPSET1F(metallic);
    OSPSET1F(diffuse);
    OSPSET1F(specular);
    OSPSET1F(ior);
    OSPSET1F(transmission);
    OSPSET3F(transmissionColor);
    OSPSET1F(transmissionDepth);
    OSPSET1F(roughness);
    OSPSET1F(anisotropy);
    OSPSET1F(rotation);
    OSPSET1F(normal);
    OSPSET1F(backlight);
    OSPSET1F(coat);
    OSPSET1F(coatIor);
    OSPSET3F(coatColor);
    OSPSET1F(coatThickness);
    OSPSET1F(coatRoughness);
    OSPSET1F(coatNormal);
    OSPSET1F(sheen);
    OSPSET3F(sheenColor);
    OSPSET1F(sheenRoughness);

    OSPSETTEXTURE(baseColorMap);
    OSPSETTEXTURE(edgeColorMap);
    OSPSETTEXTURE(metallicMap);
    OSPSETTEXTURE(diffuseMap);
    OSPSETTEXTURE(specularMap);
    OSPSETTEXTURE(iorMap);
    OSPSETTEXTURE(transmissionMap);
    OSPSETTEXTURE(transmissionColorMap);
    OSPSETTEXTURE(transmissionDepthMap);
    OSPSETTEXTURE(roughnessMap);
    OSPSETTEXTURE(anisotropyMap);
    OSPSETTEXTURE(rotationMap);
    OSPSETTEXTURE(normalMap);
    OSPSETTEXTURE(backlightMap);
    OSPSETTEXTURE(coatMap);
    OSPSETTEXTURE(coatIorMap);
    OSPSETTEXTURE(coatColorMap);
    OSPSETTEXTURE(coatThicknessMap);
    OSPSETTEXTURE(coatRoughnessMap);
    OSPSETTEXTURE(coatNormalMap);
    OSPSETTEXTURE(sheenMap);
    OSPSETTEXTURE(sheenColorMap);
    OSPSETTEXTURE(sheenRoughnessMap);
  }
  else if (implname == "CarPaint")
  {
    oMaterial = NewMaterial(orn, oRenderer, implname);
    OSPSET3F(baseColor);
    OSPSET1F(roughness);
    OSPSET1F(normal);
    OSPSET1F(flakeDensity);
    OSPSET1F(flakeScale);
    OSPSET1F(flakeSpread);
    OSPSET1F(flakeJitter);
    OSPSET1F(flakeRoughness);
    OSPSET1F(coat);
    OSPSET1F(coatIor);
    OSPSET3F(coatColor);
    OSPSET1F(coatThickness);
    OSPSET1F(coatRoughness);
    OSPSET1F(coatNormal);
    OSPSET3F(flipflopColor);
    OSPSET1F(flipflopFalloff);
  }
#endif
  else
  {
    vtkGenericWarningMacro(
      "Warning: unrecognized material \"" << implname.c_str() <<
      "\" using OBJMaterial instead. ");
    oMaterial = NewMaterial(orn, oRenderer, "OBJMaterial");
  }
  return oMaterial;

}

//------------------------------------------------------------------------------
OSPMaterial vtkOSPRayMaterialHelpers::NewMaterial(vtkOSPRayRendererNode *orn,
                                                     OSPRenderer oRenderer,
                                                     std::string ospMatName)
{
  OSPMaterial result;

#if VTK_OSPRAY_VERSION >= 10500 // 1.5.0

  (void)oRenderer;
  const std::string rendererType = vtkOSPRayRendererNode::GetRendererType(orn->GetRenderer());
  result = ospNewMaterial2(rendererType.c_str(), ospMatName.c_str());

#else // ospray < 1.5.0

  (void)orn;
  result = ospNewMaterial(oRenderer, ospMatName.c_str());

#endif

  if (!result)
  {
    vtkGenericWarningMacro("OSPRay failed to create material: " << ospMatName
                           << ". Trying OBJMaterial instead.");
#if VTK_OSPRAY_VERSION >= 10500 // 1.5.0
  result = ospNewMaterial2(rendererType.c_str(), "OBJMaterial");
#else
  result = ospNewMaterial(oRenderer, "OBJMaterial");
#endif

  }

  return result;
}
