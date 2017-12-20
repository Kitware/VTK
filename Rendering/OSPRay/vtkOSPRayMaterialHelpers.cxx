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

//------------------------------------------------------------------------------
osp::Texture2D *vtkOSPRayMaterialHelpers::VTKToOSPTexture
  (vtkImageData *vColorTextureMap)
{
  int xsize = vColorTextureMap->GetExtent()[1];
  int ysize = vColorTextureMap->GetExtent()[3];
  unsigned char *ichars =
    (unsigned char *)vColorTextureMap->GetScalarPointer();
  unsigned char *ochars = new unsigned char[(xsize+1)*(ysize+1)*3]; //LEAK?
  unsigned char *oc = ochars;
  int comps = vColorTextureMap->GetNumberOfScalarComponents();
  for (int i = 0; i <= xsize; ++i)
  {
    for (int j = 0; j <= ysize; ++j)
    {
      oc[0] = ichars[0];
      oc[1] = ichars[1];
      oc[2] = ichars[2];
      oc+=3;
      ichars+=comps;
    }
  }
  osp::Texture2D *t2d;
  t2d = (osp::Texture2D*)ospNewTexture2D
    (
     osp::vec2i{xsize+1,
         ysize+1},
     OSP_TEXTURE_RGB8,
     ochars,
     OSP_TEXTURE_FILTER_NEAREST);
  return t2d;
}

//------------------------------------------------------------------------------
void vtkOSPRayMaterialHelpers::MakeMaterials
  (vtkOSPRayRendererNode *orn,
   osp::Renderer *oRenderer,
   std::map<std::string, osp::Material*> &mats)
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
    osp::Material* newmat = vtkOSPRayMaterialHelpers::MakeMaterial
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
    osp::Texture2D *t2d = vtkOSPRayMaterialHelpers::VTKToOSPTexture(vColorTextureMap); \
    ospSetObject(oMaterial, #texname, ((OSPTexture2D)(t2d))); \
    ospCommit(t2d); \
  }

//------------------------------------------------------------------------------
osp::Material* vtkOSPRayMaterialHelpers::MakeMaterial
  (vtkOSPRayRendererNode *orn,
  osp::Renderer* oRenderer, std::string nickname)
{
  osp::Material* oMaterial;
  vtkOSPRayMaterialLibrary *ml = vtkOSPRayRendererNode::GetMaterialLibrary(orn->GetRenderer());
  if (!ml)
    {
    cout << "No material Library in this renderer. Using OBJMaterial by default." << endl;
    oMaterial = ospNewMaterial(oRenderer, "OBJMaterial");
    return oMaterial;
    }
  //todo: add a level of indirection and/or versioning so we aren't stuck with
  //these names forever
  std::string implname = ml->LookupImplName(nickname);
  if (implname == "Glass")
  {
    oMaterial = ospNewMaterial(oRenderer, implname.c_str());
    OSPSET3F(attenuationColor);
    OSPSET1F(etaInside);
    OSPSET1F(etaOutside);
    OSPSET3F(attenuationColorOutside);
    OSPSET1F(attenuationDistance);
  }
  else if (implname == "Matte")
  {
    oMaterial = ospNewMaterial(oRenderer, implname.c_str());
    OSPSET3F(reflectance);
  }
  else if (implname == "Metal")
  {
    oMaterial = ospNewMaterial(oRenderer, implname.c_str());
    OSPSET3F(eta);
    OSPSET3F(k);
    OSPSET1F(roughness);
    OSPSET3F(reflectance);
    OSPSETNF(ior);
  }
  else if (implname == "MetallicPaint")
  {
    oMaterial = ospNewMaterial(oRenderer, implname.c_str());
    OSPSET3F(baseColor)
    OSPSET3F(color)
    OSPSET1F(flakeAmount)
    OSPSET3F(flakeColor)
    OSPSET1F(flakeSpread)
    OSPSET1F(eta)
  }
  else if (implname == "OBJMaterial")
  {
    oMaterial = ospNewMaterial(oRenderer, implname.c_str());
    OSPSET1F(alpha);//aka "d", default 1.0
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
  else if (implname == "Plastic")
  {
    oMaterial = ospNewMaterial(oRenderer, implname.c_str());
    OSPSET3F(pigmentColor);
    OSPSET1F(eta);
    OSPSET1F(roughness);
    OSPSET1F(thickness);
  }
  else if (implname == "ThinGlass")
  {
    oMaterial = ospNewMaterial(oRenderer, implname.c_str());
    OSPSET3F(transmission);
    OSPSET3F(color);
    OSPSET3F(attenuationColor);
    OSPSET1F(attenuationDistance);
    OSPSET1F(eta);
    OSPSET1F(thickness);
  }
  else if (implname == "Velvet")
  {
    oMaterial = ospNewMaterial(oRenderer, implname.c_str());
    OSPSET3F(reflectance);
    OSPSET1F(backScattering);
    OSPSET3F(horizonScatteringColor);
    OSPSET1F(horizonScatteringFallOff);
  }
  else
  {
    cout <<
      "Warning: unrecognized material \"" << implname.c_str() <<
      "\" using OBJMaterial instead. " << endl;
    oMaterial = ospNewMaterial(oRenderer, "OBJMaterial");
  }
  return oMaterial;

}
