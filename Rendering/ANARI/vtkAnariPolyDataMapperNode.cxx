// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAnariPolyDataMapperNode.h"
#include "vtkAnariActorNode.h"
#include "vtkAnariProfiling.h"
#include "vtkAnariRendererNode.h"

#include "vtkActor.h"
#include "vtkCommand.h"
#include "vtkDataArray.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkMapper.h"
#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkScalarsToColors.h"
#include "vtkSmartPointer.h"
#include "vtkTexture.h"
#include "vtkUnsignedCharArray.h"

#include <anari/anari_cpp.hpp>
#include <anari/anari_cpp/ext/std.h>

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <vector>

using uvec2 = anari::std_types::uvec2;
using uvec3 = anari::std_types::uvec3;
using vec2 = anari::std_types::vec2;
using vec3 = anari::std_types::vec3;
using vec4 = anari::std_types::vec4;
using mat4 = anari::std_types::mat4;

VTK_ABI_NAMESPACE_BEGIN

struct PolyDataMapperCallback : vtkCommand
{
  vtkTypeMacro(PolyDataMapperCallback, vtkCommand);

  static PolyDataMapperCallback* New() { return new PolyDataMapperCallback; }

  void Execute(
    vtkObject* vtkNotUsed(caller), unsigned long vtkNotUsed(eventId), void* vtkNotUsed(callData))
  {
    this->RendererNode->InvalidateSceneStructure();
  }

  vtkAnariRendererNode* RendererNode{ nullptr };
};

//============================================================================
class vtkAnariPolyDataMapperNodeInternals
{
public:
  vtkAnariPolyDataMapperNodeInternals(vtkAnariPolyDataMapperNode*);
  ~vtkAnariPolyDataMapperNodeInternals();

  /**
   * Create an ANARI surface based on edge visibility and representation type
   * (e.g., wireframe or points).
   */
  void RenderSurfaces(anari::Sampler, vtkActor*, vtkPolyData*, std::vector<vec3>&,
    std::vector<u_int>&, bool, double, double, vtkDataArray*, vtkPiecewiseFunction*,
    std::vector<vec2>&, std::vector<float>&, std::vector<vec4>&,
    vtkPolyDataMapperNode::vtkPDConnectivity& conn, int);

  /**
   * Create an ANARI surface with a geometry consisting of individual spheres, each
   * of which can have its own radius.
   */
  anari::Surface RenderAsSpheres(anari::Sampler, vtkProperty*, std::vector<vec3>&,
    std::vector<u_int>&, double, vtkDataArray*, vtkPiecewiseFunction*, std::vector<vec2>&,
    std::vector<float>&, std::vector<vec4>&, int);

  /**
   * Create an ANARI surface with a geometry consisting of individual cylinders, each
   * of which can have its own radius.
   */
  anari::Surface RenderAsCylinders(anari::Sampler, vtkProperty* property, std::vector<vec3>&,
    std::vector<u_int>&, double, vtkDataArray*, vtkPiecewiseFunction*, std::vector<vec2>&,
    std::vector<float>&, std::vector<vec4>&, int);

  /**
   * Create an ANARI surface with a geometry consisting of curves, each
   * of which can have its own radius.
   */
  anari::Surface RenderAsCurves(anari::Sampler, vtkProperty* property, std::vector<vec3>&,
    std::vector<u_int>&, double, vtkDataArray*, vtkPiecewiseFunction*, std::vector<vec2>&,
    std::vector<float>&, std::vector<vec4>&, int);

  /**
   * Create an ANARI surface with a geometry consisting of triangles.
   */
  anari::Surface RenderAsTriangles(anari::Sampler anariSampler, vtkProperty* property,
    std::vector<vec3>&, std::vector<u_int>& indexArray, std::vector<vec3>& normals,
    std::vector<vec2>& textureCoords, std::vector<float>&, std::vector<vec4>&, int);

  /**
   * Create an ANARI material from VTK properties
   */
  anari::Material MakeMaterial(vtkProperty*, float* df = nullptr, anari::Sampler sampler = nullptr,
    const char* colorStr = nullptr);

  //@{
  /**
   * Utility methods for setting the material parameters.
   */
  void SetPhysicallyBasedMaterialParameters(
    anari::Material, vtkProperty*, float*, anari::Sampler, const char*);

  void SetMatteMaterialParameters(
    anari::Material, vtkProperty*, float*, anari::Sampler, const char*);
  //@}

  /**
   * Utility function to get the VTK texture from a named property.
   */
  vtkImageData* GetTextureMap(vtkProperty*, const char*);

  /**
   * Return the result of applying the VTK piecewise function to an input value.
   */
  float MapThroughPWF(double, vtkPiecewiseFunction*);

  /**
   * Converts a 2D VTK texture to a 2D ANARI sampler.
   */
  anari::Sampler VTKToAnariSampler(std::string, std::string, mat4 inTransform, vtkImageData*, bool);

  /**
   * Extracts individual textures (occlusion, roughness, metallic) from the combined VTK
   * texture containing three RGB independent components corresponding to the occlusion,
   * roughness, and metallic values. textureIdx is used to indicate what texture to
   * extract where the occlusion texture index is 0, roughness texture index is 1, and the
   * metallic texture index is 2.
   */
  anari::Sampler ExtractORMFromVTK(std::string name, int textureIdx, std::string inAttribute,
    mat4 inTransform, vtkImageData* imageData, bool sRGB);

  /**
   * Utility function for applying a transform to VTK normals and placing in a container
   * for easy consumption by the ANARI API.
   */
  void VTKToAnariNormals(vtkDataArray*, std::vector<vec3>&, vtkMatrix3x3*);

  /**
   * Methods for setting/getting the ANARI library and device parameters
   */
  void SetAnariConfig(vtkAnariRendererNode*);

  /**
   * Converts the given string to lowercase.
   */
  std::string StrToLower(std::string s);

  /**
   * Send surfaces to the renderer.
   */
  void RenderSurfaceModels();

  /**
   * Remove the cached surfaces.
   */
  void ClearSurfaces();

  vtkAnariPolyDataMapperNode* Owner{ nullptr };
  vtkAnariRendererNode* AnariRendererNode{ nullptr };

  std::vector<anari::Surface> Surfaces;

  anari::Library AnariLibrary{ nullptr };
  anari::Device AnariDevice{ nullptr };
  std::string AnariDeviceName;
  anari::Extensions AnariDeviceExtensions{};
};

//----------------------------------------------------------------------------
vtkAnariPolyDataMapperNodeInternals::vtkAnariPolyDataMapperNodeInternals(
  vtkAnariPolyDataMapperNode* owner)
{
}

//----------------------------------------------------------------------------
vtkAnariPolyDataMapperNodeInternals::~vtkAnariPolyDataMapperNodeInternals()
{
  this->ClearSurfaces();
}

//----------------------------------------------------------------------------
void vtkAnariPolyDataMapperNodeInternals::RenderSurfaceModels()
{
  if (this->AnariRendererNode == nullptr)
  {
    return;
  }

  for (auto s : this->Surfaces)
  {
    this->AnariRendererNode->AddSurface(s);
  }
}

//----------------------------------------------------------------------------
void vtkAnariPolyDataMapperNodeInternals::ClearSurfaces()
{
  if (this->AnariRendererNode == nullptr)
  {
    return;
  }

  anari::Device anariDevice = this->AnariRendererNode->GetAnariDevice();

  if (anariDevice)
  {
    for (auto surface : this->Surfaces)
    {
      anari::release(anariDevice, surface);
    }
    this->Surfaces.clear();
  }
}

//----------------------------------------------------------------------------
float vtkAnariPolyDataMapperNodeInternals::MapThroughPWF(
  double in, vtkPiecewiseFunction* scaleFunction)
{
  vtkAnariProfiling startProfiling("VTKAPDMNInternals::MapThroughPWF", vtkAnariProfiling::LIME);
  double out = in;

  if (scaleFunction != nullptr)
  {
    out = scaleFunction->GetValue(in);
  }

  return static_cast<float>(out);
}

//----------------------------------------------------------------------------
void vtkAnariPolyDataMapperNodeInternals::VTKToAnariNormals(
  vtkDataArray* vtkNormals, std::vector<vec3>& vertexNormals, vtkMatrix3x3* matrix)
{
  vtkAnariProfiling startProfiling("VTKAPDMNInternals::VTKToAnariNormals", vtkAnariProfiling::LIME);
  const int numNormals = vtkNormals->GetNumberOfTuples();
  vertexNormals.resize(numNormals);
  double transformedNormal[3];

  for (int i = 0; i < numNormals; i++)
  {
    double* vtkNormal = vtkNormals->GetTuple(i);
    matrix->MultiplyPoint(vtkNormal, transformedNormal);
    vtkMath::Normalize(transformedNormal);
    vertexNormals[i] = { static_cast<float>(transformedNormal[0]),
      static_cast<float>(transformedNormal[1]), static_cast<float>(transformedNormal[2]) };
  }
}

//----------------------------------------------------------------------------
anari::Sampler vtkAnariPolyDataMapperNodeInternals::ExtractORMFromVTK(std::string name,
  int textureIdx, std::string inAttribute, mat4 inTransform, vtkImageData* imageData, bool sRGB)
{
  vtkAnariProfiling startProfiling("VTKAPDMNInternals::ExtractORMFromVTK", vtkAnariProfiling::LIME);

  if (imageData == nullptr)
  {
    return nullptr;
  }

  if (sRGB)
  {
    return nullptr;
  }

  auto anariSampler = anari::newObject<anari::Sampler>(this->AnariDevice, "image2D");

  anari::setParameter(this->AnariDevice, anariSampler, "name", ANARI_STRING, name.c_str());
  anari::setParameter(this->AnariDevice, anariSampler, "inAttribute", inAttribute);
  anari::setParameter(this->AnariDevice, anariSampler, "inTransform", inTransform);
  anari::setParameter(this->AnariDevice, anariSampler, "wrapMode1", "clampToEdge");
  anari::setParameter(this->AnariDevice, anariSampler, "wrapMode2", "clampToEdge");
  anari::setParameter(this->AnariDevice, anariSampler, "filter", "linear");

  // Get the needed image data attributes
  int xsize = (imageData->GetExtent()[1] - imageData->GetExtent()[0]) + 1;
  int ysize = (imageData->GetExtent()[3] - imageData->GetExtent()[2]) + 1;

  if (xsize <= 0 || ysize <= 0)
  {
    return nullptr;
  }

  std::vector<float> floatData;

  for (int i = 0; i < ysize; i++)
  {
    for (int j = 0; j < xsize; j++)
    {
      floatData.push_back(imageData->GetScalarComponentAsFloat(j, i, 0, textureIdx));
    }
  }

  anari::setParameterArray2D(
    this->AnariDevice, anariSampler, "image", floatData.data(), xsize, ysize);
  anari::commitParameters(this->AnariDevice, anariSampler);

  return anariSampler;
}

//----------------------------------------------------------------------------
anari::Sampler vtkAnariPolyDataMapperNodeInternals::VTKToAnariSampler(
  std::string name, std::string inAttribute, mat4 inTransform, vtkImageData* imageData, bool sRGB)
{
  vtkAnariProfiling startProfiling("VTKAPDMNInternals::VTKToAnariSampler", vtkAnariProfiling::LIME);

  if (imageData == nullptr)
  {
    return nullptr;
  }

  auto anariSampler = anari::newObject<anari::Sampler>(this->AnariDevice, "image2D");

  anari::setParameter(this->AnariDevice, anariSampler, "name", ANARI_STRING, name.c_str());
  anari::setParameter(this->AnariDevice, anariSampler, "inAttribute", inAttribute);
  anari::setParameter(this->AnariDevice, anariSampler, "inTransform", inTransform);
  anari::setParameter(this->AnariDevice, anariSampler, "wrapMode1", "clampToEdge");
  anari::setParameter(this->AnariDevice, anariSampler, "wrapMode2", "clampToEdge");
  anari::setParameter(this->AnariDevice, anariSampler, "filter", "linear");

  // Get the needed image data attributes
  int xsize = (imageData->GetExtent()[1] - imageData->GetExtent()[0]) + 1;
  int ysize = (imageData->GetExtent()[3] - imageData->GetExtent()[2]) + 1;

  if (xsize <= 0 || ysize <= 0)
  {
    return nullptr;
  }

  int scalarType = imageData->GetScalarType();
  int comps = imageData->GetNumberOfScalarComponents();

  switch (scalarType)
  {
    case VTK_UNSIGNED_CHAR:
    case VTK_CHAR:
    case VTK_SIGNED_CHAR:
    {
      anari::DataType anariColorFormats[4] = { ANARI_UFIXED8, ANARI_UFIXED8_VEC2,
        ANARI_UFIXED8_VEC3, ANARI_UFIXED8_VEC4 };

      anari::DataType anariLinearColorFormats[4] = { ANARI_UFIXED8_R_SRGB, ANARI_UFIXED8_RA_SRGB,
        ANARI_UFIXED8_RGB_SRGB, ANARI_UFIXED8_RGBA_SRGB };
      std::vector<u_char> charData;

      if (comps > 4)
      {
        u_char* imageDataPtr = (u_char*)imageData->GetScalarPointer(0, 0, 0);

        for (int i = 0; i < xsize; i++)
        {
          for (int j = 0; j < ysize; j++)
          {
            for (int k = 0; k < 3; k++)
            {
              charData.emplace_back(imageDataPtr[k]);
            }
          }

          imageDataPtr += comps;
        }

        comps = 3;
      }

      const auto* appMemory = charData.empty() ? imageData->GetScalarPointer() : charData.data();
      auto dataType = sRGB ? anariLinearColorFormats[comps - 1] : anariColorFormats[comps - 1];

      anari::setParameterArray2D(
        this->AnariDevice, anariSampler, "image", dataType, appMemory, xsize, ysize);
      break;
    }
    case VTK_FLOAT:
    {
      anari::DataType anariColorFormats[4] = { ANARI_FLOAT32, ANARI_FLOAT32_VEC2,
        ANARI_FLOAT32_VEC3, ANARI_FLOAT32_VEC4 };

      std::vector<float> floatData;

      if (comps > 4)
      {
        for (int i = 0; i < ysize; i++)
        {
          for (int j = 0; j < xsize; j++)
          {
            for (int k = 0; k < 3; k++)
            {
              floatData.emplace_back(imageData->GetScalarComponentAsFloat(j, i, 0, k));
            }
          }
        }

        comps = 3;
      }

      const auto* appMemory = floatData.empty() ? imageData->GetScalarPointer() : floatData.data();
      anari::setParameterArray2D(this->AnariDevice, anariSampler, "image",
        anariColorFormats[comps - 1], appMemory, xsize, ysize);
      break;
    }
    case VTK_SHORT:
    case VTK_UNSIGNED_SHORT:
    {
      anari::DataType anariColorFormats[4] = { ANARI_UFIXED16, ANARI_UFIXED16_VEC2,
        ANARI_UFIXED16_VEC3, ANARI_UFIXED16_VEC4 };

      std::vector<u_short> shortData;

      if (comps > 4)
      {
        u_short* imageDataPtr = reinterpret_cast<u_short*>(imageData->GetScalarPointer(0, 0, 0));

        for (int i = 0; i < xsize; i++)
        {
          for (int j = 0; j < ysize; j++)
          {
            for (int k = 0; k < 3; k++)
            {
              shortData.emplace_back(imageDataPtr[k]);
            }
          }

          imageDataPtr += comps;
        }

        comps = 3;
      }

      const auto* appMemory = shortData.empty() ? imageData->GetScalarPointer() : shortData.data();
      anari::setParameterArray2D(this->AnariDevice, anariSampler, "image",
        anariColorFormats[comps - 1], appMemory, xsize, ysize);
      break;
    }
    default: // All other types are converted to float
    {
      anari::DataType anariColorFormats[4] = { ANARI_FLOAT32, ANARI_FLOAT32_VEC2,
        ANARI_FLOAT32_VEC3, ANARI_FLOAT32_VEC4 };

      comps = comps > 4 ? 3 : comps;
      std::vector<float> floatData;

      for (int i = 0; i < ysize; i++)
      {
        for (int j = 0; j < xsize; j++)
        {
          for (int k = 0; k < comps; k++)
          {
            floatData.emplace_back(imageData->GetScalarComponentAsFloat(j, i, 0, k));
          }
        }
      }

      anari::setParameterArray2D(this->AnariDevice, anariSampler, "image",
        anariColorFormats[comps - 1], floatData.data(), xsize, ysize);
    }
  }

  anari::commitParameters(this->AnariDevice, anariSampler);
  return anariSampler;
}

//----------------------------------------------------------------------------
anari::Material vtkAnariPolyDataMapperNodeInternals::MakeMaterial(
  vtkProperty* property, float* color, anari::Sampler sampler, const char* colorStr)
{
  vtkAnariProfiling startProfiling("VTKAPDMNInternals::MakeMaterial", vtkAnariProfiling::LIME);

  std::string materialName = "matte";
  const char* vtkMaterialName = property->GetMaterialName();

  if (vtkMaterialName != nullptr)
  {
    materialName = vtkMaterialName;
  }

  anari::Material anariMaterial = nullptr;

  if (property->GetInterpolation() == VTK_PBR && this->StrToLower(materialName) != "matte")
  {
    if (this->AnariDeviceExtensions.ANARI_KHR_MATERIAL_PHYSICALLY_BASED)
    {
      anariMaterial = anari::newObject<anari::Material>(this->AnariDevice, "physicallyBased");
      this->SetPhysicallyBasedMaterialParameters(anariMaterial, property, color, sampler, colorStr);
    }
    else
    {
      vtkWarningWithObjectMacro(this->Owner,
        << "ANARI back-end " << this->AnariDeviceName
        << " doesn't support Physically Based Materials (KHR_MATERIAL_PHYSICALLY_BASED).");

      if (this->AnariDeviceExtensions.ANARI_KHR_MATERIAL_MATTE)
      {
        anariMaterial = anari::newObject<anari::Material>(this->AnariDevice, "matte");
        this->SetMatteMaterialParameters(anariMaterial, property, color, sampler, colorStr);
      }
      else
      {
        vtkErrorWithObjectMacro(
          this->Owner, << "ANARI back-end " << this->AnariDeviceName
                       << " doesn't support Matte Materials (KHR_MATERIAL_MATTE).");
      }
    }
  }
  else
  {
    if (this->AnariDeviceExtensions.ANARI_KHR_MATERIAL_MATTE)
    {
      anariMaterial = anari::newObject<anari::Material>(this->AnariDevice, "matte");
      this->SetMatteMaterialParameters(anariMaterial, property, color, sampler, colorStr);
    }
    else
    {
      vtkErrorWithObjectMacro(
        this->Owner, << "ANARI back-end " << this->AnariDeviceName
                     << " doesn't support Matte Materials (KHR_MATERIAL_MATTE).");
    }
  }

  if (anariMaterial != nullptr)
  {
    anari::setParameter(
      this->AnariDevice, anariMaterial, "name", ANARI_STRING, materialName.c_str());
    anari::commitParameters(this->AnariDevice, anariMaterial);
  }

  return anariMaterial;
}

//----------------------------------------------------------------------------
void vtkAnariPolyDataMapperNodeInternals::SetPhysicallyBasedMaterialParameters(
  anari::Material anariMaterial, vtkProperty* vtkProperty, float* color,
  anari::Sampler baseColorSampler, const char* colorStr)
{
  vtkTexture* texture = nullptr;
  vtkTexture* ormTexture = vtkProperty->GetTexture("materialTex");

  mat4 inTransform = { vec4{ 1.0f, 0.0f, 0.0f, 0.0f }, vec4{ 0.0f, 1.0f, 0.0f, 0.0f },
    vec4{ 0.0f, 0.0f, 1.0f, 0.0f }, vec4{ 0.0f, 0.0f, 0.0f, 1.0f } };

  if (baseColorSampler != nullptr)
  {
    anari::setAndReleaseParameter(this->AnariDevice, anariMaterial, "baseColor", baseColorSampler);
  }
  else if (colorStr != nullptr)
  {
    anari::setParameter(this->AnariDevice, anariMaterial, "baseColor", colorStr);
  }
  else
  {
    // base color
    float materialColor[3] = { 0.0f, 0.0f, 0.0f };

    if (baseColorSampler == nullptr && colorStr == nullptr)
    {
      if (color != nullptr)
      {
        for (int i = 0; i < 3; i++)
        {
          materialColor[i] = color[i];
        }
      }
      else
      {
        double* actorColor = vtkProperty->GetColor();

        if (actorColor != nullptr)
        {
          for (int i = 0; i < 3; i++)
          {
            materialColor[i] = static_cast<float>(actorColor[i]);
          }
        }
      }
    }

    anari::setParameter(this->AnariDevice, anariMaterial, "baseColor", materialColor);
  }

  // opacity
  const float opacity = static_cast<float>(vtkProperty->GetOpacity());
  anari::setParameter(this->AnariDevice, anariMaterial, "opacity", opacity);

  // metalness
  if (ormTexture)
  {
    vtkImageData* ormImageData = texture->GetInput();
    auto metallicSampler =
      this->ExtractORMFromVTK("metallicTex", 2, "attribute0", inTransform, ormImageData, false);
    anari::setAndReleaseParameter(this->AnariDevice, anariMaterial, "metallic", metallicSampler);
  }
  else
  {
    const float metallic = static_cast<float>(vtkProperty->GetMetallic());
    anari::setParameter(this->AnariDevice, anariMaterial, "metallic", metallic);
  }

  // roughness
  if (ormTexture)
  {
    vtkImageData* ormImageData = texture->GetInput();
    auto roughnessSampler =
      this->ExtractORMFromVTK("roughnessTex", 1, "attribute0", inTransform, ormImageData, false);
    anari::setAndReleaseParameter(this->AnariDevice, anariMaterial, "roughness", roughnessSampler);
  }
  else
  {
    const float roughness = static_cast<float>(vtkProperty->GetRoughness());
    anari::setParameter(this->AnariDevice, anariMaterial, "roughness", roughness);
  }

  // normal map for the base layer
  texture = vtkProperty->GetTexture("normalTex");

  if (texture)
  {
    vtkImageData* normalImageData = texture->GetInput();
    auto normalSampler =
      this->VTKToAnariSampler("normalTex", "attribute0", inTransform, normalImageData, false);
    anari::setAndReleaseParameter(this->AnariDevice, anariMaterial, "normal", normalSampler);
  }

  // emissive
  texture = vtkProperty->GetTexture("emissiveTex");

  if (texture)
  {
    vtkImageData* emissiveImageData = texture->GetInput();
    auto emissiveSampler =
      this->VTKToAnariSampler("emissiveTex", "attribute0", inTransform, emissiveImageData, true);
    anari::setAndReleaseParameter(this->AnariDevice, anariMaterial, "emissive", emissiveSampler);
  }

  // occlusion map
  if (ormTexture)
  {
    vtkImageData* ormImageData = texture->GetInput();
    auto occlusionSampler =
      this->ExtractORMFromVTK("occlusionTex", 0, "attribute0", inTransform, ormImageData, false);
    anari::setAndReleaseParameter(this->AnariDevice, anariMaterial, "occlusion", occlusionSampler);
  }

  // strength of the specular reflection
  const float specular = static_cast<float>(vtkProperty->GetSpecular());
  anari::setParameter(this->AnariDevice, anariMaterial, "specular", specular);

  // color of the specular reflection at normal incidence
  double specularColor[3];
  vtkProperty->GetSpecularColor(specularColor);

  vec3 specularColorf = { static_cast<float>(specularColor[0]),
    static_cast<float>(specularColor[1]), static_cast<float>(specularColor[2]) };
  anari::setParameter(this->AnariDevice, anariMaterial, "specularColor", specularColorf);

  // strength of the clearcoat layer
  const float coatStrength = static_cast<float>(vtkProperty->GetCoatStrength());
  anari::setParameter(this->AnariDevice, anariMaterial, "clearcoat", coatStrength);

  // roughness of the clearcoat layer
  const float coatRoughness = static_cast<float>(vtkProperty->GetCoatRoughness());
  anari::setParameter(this->AnariDevice, anariMaterial, "clearcoatRoughness", coatRoughness);

  // normal map for the clearcoat layer
  texture = vtkProperty->GetTexture("coatNormalTex");

  if (texture)
  {
    vtkImageData* coatNormalImageData = texture->GetInput();
    auto coatNormalSampler = this->VTKToAnariSampler(
      "coatNormalTex", "attribute0", inTransform, coatNormalImageData, false);
    anari::setAndReleaseParameter(
      this->AnariDevice, anariMaterial, "clearCoatNormal", coatNormalSampler);
  }

  // index of refraction
  const float ior = static_cast<float>(vtkProperty->GetBaseIOR());
  anari::setParameter(this->AnariDevice, anariMaterial, "ior", ior);

  // Control cut-out transparency
  anari::setParameter(this->AnariDevice, anariMaterial, "alphaMode", "blend");
}

//----------------------------------------------------------------------------
void vtkAnariPolyDataMapperNodeInternals::SetMatteMaterialParameters(anari::Material anariMaterial,
  vtkProperty* vtkProperty, float* color, anari::Sampler sampler, const char* colorStr)
{
  if (sampler != nullptr)
  {
    anari::setAndReleaseParameter(this->AnariDevice, anariMaterial, "color", sampler);
  }
  else if (colorStr != nullptr)
  {
    anari::setParameter(this->AnariDevice, anariMaterial, "color", colorStr);
  }
  else
  {
    float materialColor[3] = { 0.0f, 0.0f, 0.0f };
    if (color != nullptr)
    {
      for (int i = 0; i < 3; i++)
      {
        materialColor[i] = color[i];
      }
    }
    else
    {
      double* actorColor = vtkProperty->GetDiffuseColor();

      if (actorColor != nullptr)
      {
        for (int i = 0; i < 3; i++)
        {
          materialColor[i] = static_cast<float>(actorColor[i]);
        }
      }
    }
    anari::setParameter(this->AnariDevice, anariMaterial, "color", materialColor);
  }

  // opacity
  const float opacity = static_cast<float>(vtkProperty->GetOpacity());
  anari::setParameter(this->AnariDevice, anariMaterial, "opacity", opacity);
  anari::setParameter(this->AnariDevice, anariMaterial, "alphaMode", "blend");
}

//----------------------------------------------------------------------------
void vtkAnariPolyDataMapperNodeInternals::SetAnariConfig(vtkAnariRendererNode* anariRendererNode)
{
  this->AnariRendererNode = anariRendererNode;
  this->AnariLibrary = anariRendererNode->GetAnariLibrary();
  this->AnariDevice = anariRendererNode->GetAnariDevice();
  this->AnariDeviceName = anariRendererNode->GetAnariDeviceName();
  this->AnariDeviceExtensions = anariRendererNode->GetAnariDeviceExtensions();
}

//----------------------------------------------------------------------------
std::string vtkAnariPolyDataMapperNodeInternals::StrToLower(std::string s)
{
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });

  return s;
}

//----------------------------------------------------------------------------
vtkImageData* vtkAnariPolyDataMapperNodeInternals::GetTextureMap(
  vtkProperty* property, const char* name)
{
  vtkAnariProfiling startProfiling("VTKAPDMNInternals::GetTextureMap", vtkAnariProfiling::LIME);
  vtkTexture* texture = property->GetTexture(name);

  if (texture)
  {
    return texture->GetInput();
  }

  return nullptr;
}

//----------------------------------------------------------------------------
void vtkAnariPolyDataMapperNodeInternals::RenderSurfaces(anari::Sampler anariSampler,
  vtkActor* actor, vtkPolyData* poly, std::vector<vec3>& vertices, std::vector<u_int>& indexArray,
  bool isTriangleIndex, double pointSize, double lineWidth, vtkDataArray* scaleArray,
  vtkPiecewiseFunction* scaleFunction, std::vector<vec2>& textureCoords,
  std::vector<float>& pointValueTextureCoords, std::vector<vec4>& pointColors,
  vtkPolyDataMapperNode::vtkPDConnectivity& conn, int cellFlag)
{
  vtkAnariProfiling startProfiling("VTKAPDMNInternals::RenderSurfaces", vtkAnariProfiling::LIME);
  vtkProperty* property = actor->GetProperty();

  switch (property->GetRepresentation())
  {
    case VTK_POINTS:
    {
      anari::Surface anariSurface = nullptr;

      if (this->AnariDeviceExtensions.ANARI_KHR_GEOMETRY_SPHERE)
      {
        anariSurface =
          this->RenderAsSpheres(anariSampler, property, vertices, indexArray, pointSize, scaleArray,
            scaleFunction, textureCoords, pointValueTextureCoords, pointColors, cellFlag);
      }

      this->Surfaces.emplace_back(anariSurface);
      break;
    }
    case VTK_WIREFRAME:
    {
      anari::Surface anariSurface = nullptr;

      if (this->AnariDeviceExtensions.ANARI_KHR_GEOMETRY_CYLINDER)
      {
        anariSurface = this->RenderAsCylinders(anariSampler, property, vertices, indexArray,
          lineWidth, scaleArray, scaleFunction, textureCoords, pointValueTextureCoords, pointColors,
          cellFlag);
      }
      else if (this->AnariDeviceExtensions.ANARI_KHR_GEOMETRY_CURVE)
      {
        anariSurface = this->RenderAsCurves(anariSampler, property, vertices, indexArray, lineWidth,
          scaleArray, scaleFunction, textureCoords, pointValueTextureCoords, pointColors, cellFlag);
      }

      this->Surfaces.emplace_back(anariSurface);
      break;
    }
    default:
    {
      if (property->GetEdgeVisibility())
      {
        // Edge material
        double edgeColor[3];
        property->GetEdgeColor(edgeColor);
        property->SetColor(edgeColor);

        std::vector<vec2> edgeTextureCoords;
        std::vector<float> edgePointValueTextureCoords;
        std::vector<vec4> edgePointColors;

        auto anariSurface = this->RenderAsCylinders(nullptr, property, vertices,
          isTriangleIndex ? conn.triangle_index : conn.strip_index, lineWidth, scaleArray,
          scaleFunction, edgeTextureCoords, edgePointValueTextureCoords, edgePointColors, cellFlag);
        this->Surfaces.emplace_back(anariSurface);
      }

      std::vector<vec3> vertexNormals;

      if (property->GetInterpolation() != VTK_FLAT)
      {
        vtkDataArray* vtkNormals = poly->GetPointData()->GetNormals();

        if (vtkNormals)
        {
          vtkNew<vtkMatrix4x4> vtkMat4;
          actor->GetMatrix(vtkMat4);
          vtkNew<vtkMatrix3x3> vtkMat3;

          for (int i = 0; i < 3; i++)
          {
            for (int j = 0; j < 3; j++)
            {
              vtkMat3->SetElement(i, j, vtkMat4->GetElement(i, j));
            }
          }
          // https://paroj.github.io/gltut/Illumination/Tut09%20Normal%20Transformation.html
          vtkMat3->Invert();
          vtkMat3->Transpose();
          this->VTKToAnariNormals(vtkNormals, vertexNormals, vtkMat3);
        }
      }

      auto anariSurface = this->RenderAsTriangles(anariSampler, property, vertices, indexArray,
        vertexNormals, textureCoords, pointValueTextureCoords, pointColors, cellFlag);
      this->Surfaces.emplace_back(anariSurface);
    }
  }
}

//----------------------------------------------------------------------------
anari::Surface vtkAnariPolyDataMapperNodeInternals::RenderAsTriangles(anari::Sampler anariSampler,
  vtkProperty* property, std::vector<vec3>& vertices, std::vector<u_int>& indexArray,
  std::vector<vec3>& normals, std::vector<vec2>& textureCoords,
  std::vector<float>& pointValueTextureCoords, std::vector<vec4>& pointColors, int cellFlag)
{
  vtkAnariProfiling startProfiling("VTKAPDMNInternals::RenderAsTriangles", vtkAnariProfiling::LIME);
  // Geometries in ANARI are objects that describe the spatial representation of a surface
  anari::Geometry triangleGeometry = nullptr;
  size_t numVertices = vertices.size();
  size_t numTriangles = indexArray.size() / 3;

  if (numVertices > 0)
  {
    triangleGeometry = anari::newObject<anari::Geometry>(this->AnariDevice, "triangle");
    std::string usdTriangleName("vtk_triangles_");

    anari::setParameter(
      this->AnariDevice, triangleGeometry, "name", ANARI_STRING, usdTriangleName.c_str());

    // Vertices
    auto positionArray = anari::newArray1D(this->AnariDevice, ANARI_FLOAT32_VEC3, numVertices);
    {
      auto positionArrayPtr = anari::map<vec3>(this->AnariDevice, positionArray);

      for (size_t i = 0; i < numVertices; i++)
      {
        positionArrayPtr[i] = vertices[i];
      }

      anari::unmap(this->AnariDevice, positionArray);
    }

    anari::setAndReleaseParameter(
      this->AnariDevice, triangleGeometry, "vertex.position", positionArray);
  }
  else
  {
    char buffer[50];
    snprintf(buffer, 50, "[RenderAsTriangles] numVertices = %ld", numVertices);
    vtkDebugWithObjectMacro(this->Owner, << buffer);
    return nullptr;
  }

  // Optional indices
  if (numTriangles > 0)
  {
    auto indicesArray = anari::newArray1D(this->AnariDevice, ANARI_UINT32_VEC3, numTriangles);
    {
      auto indicesArrayPtr = anari::map<uvec3>(this->AnariDevice, indicesArray);

      for (size_t i = 0, j = 0; i < numTriangles; i++, j += 3)
      {
        indicesArrayPtr[i] = uvec3{ indexArray[j], indexArray[j + 1], indexArray[j + 2] };
      }

      anari::unmap(this->AnariDevice, indicesArray);
    }

    anari::setAndReleaseParameter(
      this->AnariDevice, triangleGeometry, "primitive.index", indicesArray);
  }

  // Vertex normals
  size_t numNormals = normals.size();

  if (numNormals > 0)
  {
    auto normalArray = anari::newArray1D(this->AnariDevice, ANARI_FLOAT32_VEC3, numNormals);
    {
      auto normalArrayPtr = anari::map<vec3>(this->AnariDevice, normalArray);

      for (size_t i = 0; i < numNormals; i++)
      {
        normalArrayPtr[i] = normals[i];
      }

      anari::unmap(this->AnariDevice, normalArray);
    }

    anari::setAndReleaseParameter(
      this->AnariDevice, triangleGeometry, "vertex.normal", normalArray);
  }

  // Texture Coordinates
  size_t numPointValueTextureCoords = pointValueTextureCoords.size();
  size_t numTextureCoords = textureCoords.size();

  if (numTextureCoords > 0 || numPointValueTextureCoords > 0)
  {
    std::vector<vec2> tcoords;
    anari::Array1D tcoordsArray = nullptr;

    if (numPointValueTextureCoords > 0)
    {
      tcoordsArray =
        anari::newArray1D(this->AnariDevice, ANARI_FLOAT32_VEC2, numPointValueTextureCoords);
      {
        auto tcoordsArrayPtr = anari::map<vec2>(this->AnariDevice, tcoordsArray);

        for (size_t i = 0; i < numPointValueTextureCoords; i++)
        {
          tcoordsArrayPtr[i] = vec2{ pointValueTextureCoords[i], 0.0f };
        }

        anari::unmap(this->AnariDevice, tcoordsArray);
      }
    }
    else
    {
      tcoordsArray = anari::newArray1D(this->AnariDevice, ANARI_FLOAT32_VEC2, numTextureCoords);
      {
        auto tcoordsArrayPtr = anari::map<vec2>(this->AnariDevice, tcoordsArray);

        for (size_t i = 0; i < numTextureCoords; i++)
        {
          tcoordsArrayPtr[i] = textureCoords[i];
        }

        anari::unmap(this->AnariDevice, tcoordsArray);
      }
    }

    anari::setAndReleaseParameter(
      this->AnariDevice, triangleGeometry, "vertex.attribute0", tcoordsArray);
  }

  // Per point color
  size_t numPointColors = pointColors.size();

  if (numPointColors > 0)
  {
    if (cellFlag == 0)
    {
      anari::Array1D colorArray =
        anari::newArray1D(this->AnariDevice, ANARI_FLOAT32_VEC4, numPointColors);
      {
        auto colorArrayPtr = anari::map<vec4>(this->AnariDevice, colorArray);

        for (size_t i = 0; i < numPointColors; i++)
        {
          colorArrayPtr[i] = pointColors[i];
        }

        anari::unmap(this->AnariDevice, colorArray);
      }

      anari::setAndReleaseParameter(
        this->AnariDevice, triangleGeometry, "vertex.color", colorArray);
    }
    else
    {
      int colorRepeatCount = numTriangles / numPointColors;
      colorRepeatCount = colorRepeatCount <= 0 ? 1 : colorRepeatCount;

      anari::Array1D colorArray =
        anari::newArray1D(this->AnariDevice, ANARI_FLOAT32_VEC4, numTriangles);
      {
        auto colorArrayPtr = anari::map<vec4>(this->AnariDevice, colorArray);

        for (size_t i = 0; i < numPointColors; i++)
        {
          for (int j = 0; j < colorRepeatCount; j++)
          {
            size_t idx = (colorRepeatCount * i) + j;

            if (idx < numTriangles)
            {
              colorArrayPtr[idx] = pointColors[i];
            }
          }
        }

        anari::unmap(this->AnariDevice, colorArray);
      }

      anari::setAndReleaseParameter(
        this->AnariDevice, triangleGeometry, "primitive.color", colorArray);
    }
  }

  anari::commitParameters(this->AnariDevice, triangleGeometry);
  // Geometries are matched with appearance information through Surfaces.
  // These take a geometry, which defines the spatial representation, and
  // applies either full-object or per-primitive color and material information
  auto anariSurface = anari::newObject<anari::Surface>(this->AnariDevice);
  // anari::setParameter(anariDevice, anariSurface, "name", ANARI_STRING, "vtk_surface");
  anari::setAndReleaseParameter(this->AnariDevice, anariSurface, "geometry", triangleGeometry);

  anari::Material anariMaterial = nullptr;

  if (((numTextureCoords > 0) || (numPointValueTextureCoords > 0)) && anariSampler != nullptr)
  {
    anariMaterial = this->MakeMaterial(property, nullptr, anariSampler);
  }
  else
  {
    if (anariSampler != nullptr)
    {
      anari::release(this->AnariDevice, anariSampler);
    }

    const char* colorStr = pointColors.size() > 0 ? "color" : nullptr;
    anariMaterial = this->MakeMaterial(property, nullptr, nullptr, colorStr);
  }

  if (anariMaterial != nullptr)
  {
    anari::setAndReleaseParameter(this->AnariDevice, anariSurface, "material", anariMaterial);
  }

  anari::commitParameters(this->AnariDevice, anariSurface);
  return anariSurface;
}

//----------------------------------------------------------------------------
anari::Surface vtkAnariPolyDataMapperNodeInternals::RenderAsCylinders(anari::Sampler anariSampler,
  vtkProperty* property, std::vector<vec3>& vertices, std::vector<u_int>& indexArray,
  double lineWidth, vtkDataArray* scaleArray, vtkPiecewiseFunction* scaleFunction,
  std::vector<vec2>& textureCoords, std::vector<float>& pointValueTextureCoords,
  std::vector<vec4>& pointColors, int cellFlag)
{
  vtkAnariProfiling startProfiling("VTKAPDMNInternals::RenderAsCylinders", vtkAnariProfiling::LIME);

  // Geometries in ANARI are objects that describe the spatial representation of a surface
  anari::Geometry cylinderGeometry = nullptr;
  size_t numVertices = vertices.size();
  size_t numCylinders = indexArray.size() / 2;

  if (numVertices > 0)
  {
    cylinderGeometry = anari::newObject<anari::Geometry>(this->AnariDevice, "cylinder");
    std::string usdCylinderName("vtk_cylinders_");

    anari::setParameter(
      this->AnariDevice, cylinderGeometry, "name", ANARI_STRING, usdCylinderName.c_str());

    // Vertex positions
    auto positionArray = anari::newArray1D(this->AnariDevice, ANARI_FLOAT32_VEC3, numVertices);
    {
      auto positionArrayPtr = anari::map<vec3>(this->AnariDevice, positionArray);

      for (size_t i = 0; i < numVertices; i++)
      {
        positionArrayPtr[i] = vertices[i];
      }

      anari::unmap(this->AnariDevice, positionArray);
    }

    anari::setAndReleaseParameter(
      this->AnariDevice, cylinderGeometry, "vertex.position", positionArray);
  }
  else
  {
    char buffer[50];
    snprintf(buffer, 50, "[RenderAsCylinders] numVertices = %ld", numVertices);
    vtkDebugWithObjectMacro(this->Owner, << buffer);
    return nullptr;
  }

  // Optional indices
  if (numCylinders > 0)
  {
    auto indicesArray = anari::newArray1D(this->AnariDevice, ANARI_UINT32_VEC2, numCylinders);
    {
      auto indicesArrayPtr = anari::map<uvec2>(this->AnariDevice, indicesArray);

      for (size_t i = 0, j = 0; i < numCylinders; i++, j += 2)
      {
        indicesArrayPtr[i] = uvec2{ indexArray[j], indexArray[j + 1] };
      }

      anari::unmap(this->AnariDevice, indicesArray);
    }

    anari::setAndReleaseParameter(
      this->AnariDevice, cylinderGeometry, "primitive.index", indicesArray);
  }

  // Radius
  if (scaleArray != nullptr) // per cylinder radius
  {
    auto radiusArray = anari::newArray1D(this->AnariDevice, ANARI_FLOAT32, numCylinders);
    {
      auto radiusArrayPtr = anari::map<float>(this->AnariDevice, radiusArray);

      for (size_t i = 0; i < numCylinders; i++)
      {
        float avgRadius = (*scaleArray->GetTuple(indexArray[i * 2]) +
                            *scaleArray->GetTuple(indexArray[i * 2 + 1])) *
          0.5f;
        radiusArrayPtr[i] = MapThroughPWF(avgRadius, scaleFunction);
      }

      anari::unmap(this->AnariDevice, radiusArray);
    }

    anari::setAndReleaseParameter(
      this->AnariDevice, cylinderGeometry, "primitive.radius", radiusArray);
  }
  else
  {
    anari::setParameter(
      this->AnariDevice, cylinderGeometry, "radius", static_cast<float>(lineWidth));
  }

  // Texture Coordinates
  size_t numTextureCoords = textureCoords.size();
  size_t numPointValueTextureCoords = pointValueTextureCoords.size();

  if (numTextureCoords > 0 || numPointValueTextureCoords > 0)
  {
    anari::Array1D tcoordsArray = nullptr;

    if (numPointValueTextureCoords > 0)
    {
      tcoordsArray =
        anari::newArray1D(this->AnariDevice, ANARI_FLOAT32_VEC2, numPointValueTextureCoords);
      {
        auto tcoordsArrayPtr = anari::map<vec2>(this->AnariDevice, tcoordsArray);

        for (size_t i = 0; i < numPointValueTextureCoords; i++)
        {
          tcoordsArrayPtr[i] = vec2{ pointValueTextureCoords[i], 0.0f };
        }

        anari::unmap(this->AnariDevice, tcoordsArray);
      }
    }
    else
    {
      tcoordsArray = anari::newArray1D(this->AnariDevice, ANARI_FLOAT32_VEC2, numTextureCoords);
      {
        auto tcoordsArrayPtr = anari::map<vec2>(this->AnariDevice, tcoordsArray);

        for (size_t i = 0; i < numTextureCoords; i++)
        {
          tcoordsArrayPtr[i] = textureCoords[i];
        }

        anari::unmap(this->AnariDevice, tcoordsArray);
      }
    }

    anari::setAndReleaseParameter(
      this->AnariDevice, cylinderGeometry, "vertex.attribute0", tcoordsArray);
  }

  // Per point color
  size_t numPointColors = pointColors.size();

  if (numPointColors > 0)
  {
    // if(cellFlag == 0)
    // {
    anari::Array1D colorArray =
      anari::newArray1D(this->AnariDevice, ANARI_FLOAT32_VEC4, numPointColors);
    {
      auto colorArrayPtr = anari::map<vec4>(this->AnariDevice, colorArray);

      for (size_t i = 0; i < numPointColors; i++)
      {
        colorArrayPtr[i] = pointColors[i];
      }

      anari::unmap(this->AnariDevice, colorArray);
    }

    if (cellFlag == 0)
    {
      anari::setAndReleaseParameter(
        this->AnariDevice, cylinderGeometry, "vertex.color", colorArray);
    }
    else
    {
      anari::setAndReleaseParameter(
        this->AnariDevice, cylinderGeometry, "primitive.color", colorArray);
    }
  }

  anari::commitParameters(this->AnariDevice, cylinderGeometry);
  // Geometries are matched with appearance information through Surfaces.
  // These take a geometry, which defines the spatial representation, and
  // applies either full-object or per-primitive color and material information
  auto anariSurface = anari::newObject<anari::Surface>(this->AnariDevice);
  // anari::setParameter(anariDevice, anariSurface, "name", ANARI_STRING, "vtk_surface");
  anari::setAndReleaseParameter(this->AnariDevice, anariSurface, "geometry", cylinderGeometry);

  anari::Material anariMaterial = nullptr;

  if (((numTextureCoords > 0) || (numPointValueTextureCoords > 0)) && anariSampler != nullptr)
  {
    anariMaterial = this->MakeMaterial(property, nullptr, anariSampler);
  }
  else
  {
    if (anariSampler != nullptr)
    {
      anari::release(this->AnariDevice, anariSampler);
    }

    const char* colorStr = pointColors.size() > 0 ? "color" : nullptr;
    anariMaterial = this->MakeMaterial(property, nullptr, nullptr, colorStr);
  }

  if (anariMaterial != nullptr)
  {
    anari::setAndReleaseParameter(this->AnariDevice, anariSurface, "material", anariMaterial);
  }

  anari::commitParameters(this->AnariDevice, anariSurface);
  return anariSurface;
}

//----------------------------------------------------------------------------
anari::Surface vtkAnariPolyDataMapperNodeInternals::RenderAsCurves(anari::Sampler anariSampler,
  vtkProperty* property, std::vector<vec3>& vertices, std::vector<u_int>& indexArray,
  double lineWidth, vtkDataArray* scaleArray, vtkPiecewiseFunction* scaleFunction,
  std::vector<vec2>& textureCoords, std::vector<float>& pointValueTextureCoords,
  std::vector<vec4>& pointColors, int cellFlag)
{
  vtkAnariProfiling startProfiling("VTKAPDMNInternals::RenderAsCurves", vtkAnariProfiling::LIME);
  // Geometries in ANARI are objects that describe the spatial representation of a surface
  anari::Geometry curveGeometry = nullptr;

  size_t numVertices = vertices.size();
  size_t numIndices = indexArray.size();

  if (numVertices > 0)
  {
    curveGeometry = anari::newObject<anari::Geometry>(this->AnariDevice, "curve");
    std::string usdCurveName("vtk_curves_");

    anari::setParameter(
      this->AnariDevice, curveGeometry, "name", ANARI_STRING, usdCurveName.c_str());

    // Vertex positions
    auto positionArray = anari::newArray1D(this->AnariDevice, ANARI_FLOAT32_VEC3, numVertices);
    {
      auto positionArrayPtr = anari::map<vec3>(this->AnariDevice, positionArray);

      for (size_t i = 0; i < numVertices; i++)
      {
        positionArrayPtr[i] = vertices[i];
      }

      anari::unmap(this->AnariDevice, positionArray);
    }

    anari::setAndReleaseParameter(
      this->AnariDevice, curveGeometry, "vertex.position", positionArray);
  }
  else
  {
    char buffer[50];
    snprintf(buffer, 50, "[RenderAsCurves] numVertices = %ld", numVertices);
    vtkDebugWithObjectMacro(this->Owner, << buffer);
    return nullptr;
  }

  // Optional indices
  if (numIndices > 0)
  {
    auto indicesArray = anari::newArray1D(this->AnariDevice, ANARI_UINT32, numIndices);
    {
      auto indicesArrayPtr = anari::map<u_int>(this->AnariDevice, indicesArray);

      for (size_t i = 0; i < numIndices; i++)
      {
        indicesArrayPtr[i] = indexArray[i];
      }

      anari::unmap(this->AnariDevice, indicesArray);
    }

    anari::setAndReleaseParameter(
      this->AnariDevice, curveGeometry, "primitive.index", indicesArray);
  }

  // Radius
  if (scaleArray != nullptr)
  {
    size_t numRadius = scaleArray->GetNumberOfTuples();

    auto radiusArray = anari::newArray1D(this->AnariDevice, ANARI_FLOAT32, numRadius);
    {
      auto radiusArrayPtr = anari::map<float>(this->AnariDevice, radiusArray);

      for (size_t i = 0; i < numRadius; i++)
      {
        radiusArrayPtr[i] = MapThroughPWF(*scaleArray->GetTuple(i), scaleFunction);
      }

      anari::unmap(this->AnariDevice, radiusArray);
    }

    anari::setAndReleaseParameter(this->AnariDevice, curveGeometry, "vertex.radius", radiusArray);
  }
  else
  {
    anari::setParameter(this->AnariDevice, curveGeometry, "radius", static_cast<float>(lineWidth));
  }

  // Texture Coordinates
  size_t numTextureCoords = textureCoords.size();
  size_t numPointValueTextureCoords = pointValueTextureCoords.size();

  if (numTextureCoords > 0 || numPointValueTextureCoords > 0)
  {
    anari::Array1D tcoordsArray = nullptr;

    if (numPointValueTextureCoords > 0)
    {
      tcoordsArray =
        anari::newArray1D(this->AnariDevice, ANARI_FLOAT32_VEC2, numPointValueTextureCoords);
      {
        auto tcoordsArrayPtr = anari::map<vec2>(this->AnariDevice, tcoordsArray);

        for (size_t i = 0; i < numPointValueTextureCoords; i++)
        {
          tcoordsArrayPtr[i] = vec2{ pointValueTextureCoords[i], 0.0f };
        }

        anari::unmap(this->AnariDevice, tcoordsArray);
      }
    }
    else
    {
      tcoordsArray = anari::newArray1D(this->AnariDevice, ANARI_FLOAT32_VEC2, numTextureCoords);
      {
        auto tcoordsArrayPtr = anari::map<vec2>(this->AnariDevice, tcoordsArray);

        for (size_t i = 0; i < numTextureCoords; i++)
        {
          tcoordsArrayPtr[i] = textureCoords[i];
        }

        anari::unmap(this->AnariDevice, tcoordsArray);
      }
    }

    anari::setAndReleaseParameter(
      this->AnariDevice, curveGeometry, "vertex.attribute0", tcoordsArray);
  }

  // Per point color
  size_t numPointColors = pointColors.size();

  if (numPointColors > 0)
  {
    // if(cellFlag == 0)
    // {
    anari::Array1D colorArray =
      anari::newArray1D(this->AnariDevice, ANARI_FLOAT32_VEC4, numPointColors);
    {
      auto colorArrayPtr = anari::map<vec4>(this->AnariDevice, colorArray);

      for (size_t i = 0; i < numPointColors; i++)
      {
        colorArrayPtr[i] = pointColors[i];
      }

      anari::unmap(this->AnariDevice, colorArray);
    }

    if (cellFlag == 0)
    {
      anari::setAndReleaseParameter(this->AnariDevice, curveGeometry, "vertex.color", colorArray);
    }
    else
    {
      anari::setAndReleaseParameter(
        this->AnariDevice, curveGeometry, "primitive.color", colorArray);
    }
  }

  anari::commitParameters(this->AnariDevice, curveGeometry);
  // Geometries are matched with appearance information through Surfaces.
  // These take a geometry, which defines the spatial representation, and
  // applies either full-object or per-primitive color and material information
  auto anariSurface = anari::newObject<anari::Surface>(this->AnariDevice);
  // anari::setParameter(anariDevice, anariSurface, "name", ANARI_STRING, "vtk_surface");
  anari::setAndReleaseParameter(this->AnariDevice, anariSurface, "geometry", curveGeometry);

  anari::Material anariMaterial = nullptr;

  if (((numTextureCoords > 0) || (numPointValueTextureCoords > 0)) && anariSampler != nullptr)
  {
    anariMaterial = this->MakeMaterial(property, nullptr, anariSampler);
  }
  else
  {
    if (anariSampler != nullptr)
    {
      anari::release(this->AnariDevice, anariSampler);
    }

    const char* colorStr = pointColors.size() > 0 ? "color" : nullptr;
    anariMaterial = this->MakeMaterial(property, nullptr, nullptr, colorStr);
  }

  if (anariMaterial != nullptr)
  {
    anari::setAndReleaseParameter(this->AnariDevice, anariSurface, "material", anariMaterial);
  }

  anari::commitParameters(this->AnariDevice, anariSurface);
  return anariSurface;
}

//----------------------------------------------------------------------------
anari::Surface vtkAnariPolyDataMapperNodeInternals::RenderAsSpheres(anari::Sampler anariSampler,
  vtkProperty* property, std::vector<vec3>& vertices, std::vector<u_int>& indexArray,
  double pointSize, vtkDataArray* scaleArray, vtkPiecewiseFunction* scaleFunction,
  std::vector<vec2>& textureCoords, std::vector<float>& pointValueTextureCoords,
  std::vector<vec4>& pointColors, int cellFlag)
{
  vtkAnariProfiling startProfiling("VTKAPDMNInternals::RenderAsSpheres", vtkAnariProfiling::LIME);

  // Ignore cellFlag, as spheres have only one point
  (void)cellFlag;

  // Geometries in ANARI are objects that describe the spatial representation of a surface
  anari::Geometry sphereGeometry = nullptr;
  size_t numPositions = indexArray.size();
  size_t numVertices = vertices.size();

  if (numVertices > 0)
  {
    sphereGeometry = anari::newObject<anari::Geometry>(this->AnariDevice, "sphere");
    std::string usdSphereName("vtk_spheres_");

    anari::setParameter(
      this->AnariDevice, sphereGeometry, "name", ANARI_STRING, usdSphereName.c_str());

    // Vertex positions
    auto positionArray = anari::newArray1D(this->AnariDevice, ANARI_FLOAT32_VEC3, numVertices);
    {
      auto positionArrayPtr = anari::map<vec3>(this->AnariDevice, positionArray);

      for (size_t i = 0; i < numVertices; i++)
      {
        positionArrayPtr[i] = vertices[i];
      }

      anari::unmap(this->AnariDevice, positionArray);
    }

    anari::setAndReleaseParameter(
      this->AnariDevice, sphereGeometry, "vertex.position", positionArray);
  }
  else
  {
    char buffer[50];
    snprintf(buffer, 50, "[RenderAsSpheres] numVertices = %ld", numVertices);
    vtkDebugWithObjectMacro(this->Owner, << buffer);
    return nullptr;
  }

  // Optional indices
  if (numPositions > 0)
  {
    auto indicesArray = anari::newArray1D(this->AnariDevice, ANARI_UINT32, numPositions);
    {
      auto indicesArrayPtr = anari::map<u_int32_t>(this->AnariDevice, indicesArray);

      for (size_t i = 0; i < numPositions; i++)
      {
        indicesArrayPtr[i] = indexArray[i];
      }

      anari::unmap(this->AnariDevice, indicesArray);
    }

    anari::setAndReleaseParameter(
      this->AnariDevice, sphereGeometry, "primitive.index", indicesArray);
  }

  // Radius
  if (scaleArray != nullptr) // per sphere radius
  {
    size_t numRadius = scaleArray->GetNumberOfTuples();

    auto radiusArray = anari::newArray1D(this->AnariDevice, ANARI_FLOAT32, numRadius);
    {
      auto radiusArrayPtr = anari::map<float>(this->AnariDevice, radiusArray);

      for (size_t i = 0; i < numRadius; i++)
      {
        // radiusArrayPtr[i] = MapThroughPWF(*scaleArray->GetTuple(indexArray[i]), scaleFunction);
        radiusArrayPtr[i] = MapThroughPWF(*scaleArray->GetTuple(i), scaleFunction);
      }

      anari::unmap(this->AnariDevice, radiusArray);
    }

    anari::setAndReleaseParameter(this->AnariDevice, sphereGeometry, "vertex.radius", radiusArray);
  }
  else
  {
    anari::setParameter(this->AnariDevice, sphereGeometry, "radius", static_cast<float>(pointSize));
  }

  // Texture Coordinates
  size_t numTextureCoords = textureCoords.size();
  size_t numPointValueTextureCoords = pointValueTextureCoords.size();

  if (numTextureCoords > 0 || numPointValueTextureCoords > 0)
  {
    anari::Array1D tcoordsArray = nullptr;

    if (numPointValueTextureCoords > 0)
    {
      tcoordsArray =
        anari::newArray1D(this->AnariDevice, ANARI_FLOAT32_VEC2, numPointValueTextureCoords);
      {
        auto tcoordsArrayPtr = anari::map<vec2>(this->AnariDevice, tcoordsArray);

        for (size_t i = 0; i < numPointValueTextureCoords; i++)
        {
          tcoordsArrayPtr[i] = vec2{ pointValueTextureCoords[i], 0.0f };
        }

        anari::unmap(this->AnariDevice, tcoordsArray);
      }
    }
    else
    {
      tcoordsArray = anari::newArray1D(this->AnariDevice, ANARI_FLOAT32_VEC2, numTextureCoords);
      {
        auto tcoordsArrayPtr = anari::map<vec2>(this->AnariDevice, tcoordsArray);

        for (size_t i = 0; i < numTextureCoords; i++)
        {
          tcoordsArrayPtr[i] = textureCoords[i];
        }

        anari::unmap(this->AnariDevice, tcoordsArray);
      }
    }

    anari::setAndReleaseParameter(
      this->AnariDevice, sphereGeometry, "vertex.attribute0", tcoordsArray);
  }

  // Per point color
  size_t numPointColors = pointColors.size();

  if (numPointColors > 0)
  {
    anari::Array1D colorArray =
      anari::newArray1D(this->AnariDevice, ANARI_FLOAT32_VEC4, numPointColors);
    {
      auto colorArrayPtr = anari::map<vec4>(this->AnariDevice, colorArray);

      for (size_t i = 0; i < numPointColors; i++)
      {
        colorArrayPtr[i] = pointColors[i];
      }

      anari::unmap(this->AnariDevice, colorArray);
    }

    anari::setAndReleaseParameter(this->AnariDevice, sphereGeometry, "vertex.color", colorArray);
  }

  anari::commitParameters(this->AnariDevice, sphereGeometry);
  // Geometries are matched with appearance information through Surfaces.
  // These take a geometry, which defines the spatial representation, and
  // applies either full-object or per-primitive color and material information
  auto anariSurface = anari::newObject<anari::Surface>(this->AnariDevice);
  anari::setAndReleaseParameter(this->AnariDevice, anariSurface, "geometry", sphereGeometry);

  // Get the PBR textures in makeMaterial method
  anari::Material anariMaterial = nullptr; // material;

  if (((numTextureCoords > 0) || (numPointValueTextureCoords > 0)) && anariSampler != nullptr)
  {
    anariMaterial = this->MakeMaterial(property, nullptr, anariSampler);
  }
  else
  {
    if (anariSampler != nullptr)
    {
      anari::release(this->AnariDevice, anariSampler);
    }

    const char* colorStr = pointColors.size() > 0 ? "color" : nullptr;
    anariMaterial = this->MakeMaterial(property, nullptr, nullptr, colorStr);
  }

  if (anariMaterial != nullptr)
  {
    anari::setAndReleaseParameter(this->AnariDevice, anariSurface, "material", anariMaterial);
  }

  anari::commitParameters(this->AnariDevice, anariSurface);
  return anariSurface;
}

//============================================================================
vtkStandardNewMacro(vtkAnariPolyDataMapperNode);

//----------------------------------------------------------------------------
vtkAnariPolyDataMapperNode::vtkAnariPolyDataMapperNode()
{
  this->Internal = new vtkAnariPolyDataMapperNodeInternals(this);
}

//----------------------------------------------------------------------------
vtkAnariPolyDataMapperNode::~vtkAnariPolyDataMapperNode()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkAnariPolyDataMapperNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkAnariPolyDataMapperNode::Invalidate(bool prepass)
{
  if (prepass)
  {
    this->RenderTime = 0;
  }
}

//----------------------------------------------------------------------------
void vtkAnariPolyDataMapperNode::AnariRenderPoly(vtkAnariActorNode* anariActorNode,
  vtkPolyData* poly, double* diffuse, double opacity, const std::string& materialName)
{
  vtkAnariProfiling startProfiling("VTKAPDMN::AnariRenderPoly", vtkAnariProfiling::GREEN);

  vtkActor* actor = vtkActor::SafeDownCast(anariActorNode->GetRenderable());
  vtkProperty* property = actor->GetProperty();

  if (diffuse)
  {
    property->SetColor(diffuse[0], diffuse[1], diffuse[2]);
  }
  else
  {
    property->SetColor(1, 1, 1);
  }

  property->SetOpacity(opacity);
  property->SetMaterialName(materialName.c_str());

  // Geometry
  std::vector<double> outTransformedVertices;
  vtkPolyDataMapperNode::TransformPoints(actor, poly, outTransformedVertices);
  size_t numPositions = outTransformedVertices.size() / 3;

  if (numPositions == 0)
  {
    return;
  }

  std::vector<vec3> vertices;

  for (size_t i = 0; i < numPositions; i++)
  {
    vertices.emplace_back(vec3{ static_cast<float>(outTransformedVertices[i * 3]),
      static_cast<float>(outTransformedVertices[i * 3 + 1]),
      static_cast<float>(outTransformedVertices[i * 3 + 2]) });
  }
  // vector::clear doesn't guarantee a reallocation, this way
  // we can force a reallocation
  outTransformedVertices.clear();
  std::vector<double>().swap(outTransformedVertices);

  // ANARI Sampler Transform
  mat4 anariSamplerInTransform = { vec4{ 1.0f, 0.0f, 0.0f, 0.0f }, vec4{ 0.0f, 1.0f, 0.0f, 0.0f },
    vec4{ 0.0f, 0.0f, 1.0f, 0.0f }, vec4{ 0.0f, 0.0f, 0.0f, 1.0f } };
  vtkInformation* info = actor->GetPropertyKeys();

  if (info && info->Has(vtkProp::GeneralTextureTransform()))
  {
    double* transform = info->Get(vtkProp::GeneralTextureTransform());
    int length = info->Length(vtkProp::GeneralTextureTransform());

    if (length == 16)
    {
      anariSamplerInTransform[0] =
        vec4{ static_cast<float>(transform[0]), static_cast<float>(transform[1]),
          static_cast<float>(transform[2]), static_cast<float>(transform[3]) };
      anariSamplerInTransform[1] =
        vec4{ static_cast<float>(transform[4]), static_cast<float>(transform[5]),
          static_cast<float>(transform[6]), static_cast<float>(transform[7]) };
      anariSamplerInTransform[2] =
        vec4{ static_cast<float>(transform[8]), static_cast<float>(transform[9]),
          static_cast<float>(transform[10]), static_cast<float>(transform[11]) };
      anariSamplerInTransform[3] =
        vec4{ static_cast<float>(transform[12]), static_cast<float>(transform[13]),
          static_cast<float>(transform[14]), static_cast<float>(transform[15]) };
    }
  }

  // cellFlag == 0 => PointData - length must equal the number of points
  // cellFlag == 1 => CellData - length must equal number of cells
  // cellFlag == 2 => FieldData - global properties of the data - no length restrictions

  // Geometry
  int cellFlag = -1;
  vtkUnsignedCharArray* mapperColors = nullptr;  // vColors
  vtkFloatArray* mapperColorCoords = nullptr;    // vColorCoordinates
  vtkImageData* mapperColorTextureMap = nullptr; // pColorTextureMap

  vtkMapper* mapper = actor->GetMapper();
  if (mapper)
  {
    // Geometry
    mapper->MapScalars(poly, 1.0, cellFlag);
    mapperColors = mapper->GetColorMapColors();

    // Material
    mapperColorCoords = mapper->GetColorCoordinates();
    mapperColorTextureMap = mapper->GetColorTextureMap();
  }

  // texture
  int numTextureCoordinates = 0;
  std::vector<vec2> textureCoords;
  vtkDataArray* da = poly->GetPointData()->GetTCoords();

  if (da != nullptr)
  {
    numTextureCoordinates = da->GetNumberOfTuples();

    for (int i = 0; i < numTextureCoordinates; i++)
    {
      textureCoords.emplace_back(
        vec2{ static_cast<float>(da->GetTuple(i)[0]), static_cast<float>(da->GetTuple(i)[1]) });
    }

    numTextureCoordinates *= 2;
  }

  bool sRGB = false;
  vtkImageData* albedoTextureMap = nullptr; // vColorTextureMap
  vtkTexture* texture = nullptr;

  if (property->GetInterpolation() == VTK_PBR)
  {
    texture = property->GetTexture("albedoTex");
  }
  else
  {
    texture = actor->GetTexture();
  }

  if (texture != nullptr)
  {
    sRGB = texture->GetUseSRGBColorSpace();
    albedoTextureMap = texture->GetInput();
  }

  // Setup Material or Colors
  std::vector<float> pointValueTextureCoords;
  std::vector<vec4> pointColors;

  if (mapperColors)
  {
    if (cellFlag == 2 && mapper->GetFieldDataTupleId() > -1)
    {
      // Color comes from field data entry
      bool useMaterial = false;
      // check if field data content says to use a material lookup
      vtkScalarsToColors* s2c = mapper->GetLookupTable();
      bool tryMats = s2c->GetIndexedLookup() && s2c->GetNumberOfAnnotatedValues();

      if (tryMats)
      {
        int cflag2 = -1;
        vtkAbstractArray* scalars = mapper->GetAbstractScalars(poly, mapper->GetScalarMode(),
          mapper->GetArrayAccessMode(), mapper->GetArrayId(), mapper->GetArrayName(), cflag2);
        vtkVariant v = scalars->GetVariantValue(mapper->GetFieldDataTupleId());
        vtkIdType idx = s2c->GetAnnotatedValueIndex(v);

        if (idx > -1)
        {
          std::string name(s2c->GetAnnotation(idx));
          property->SetMaterialName(name.c_str());
          useMaterial = true;
        }
      }

      if (!useMaterial)
      {
        // Use the color for the field data value
        int ncomps = mapperColors->GetNumberOfComponents();
        u_char* mapperColorsPtr = mapperColors->GetPointer(0);
        mapperColorsPtr = mapperColorsPtr + mapper->GetFieldDataTupleId() * ncomps;
        double diffuseColor[3] = { mapperColorsPtr[0] * property->GetDiffuse() / 255.0,
          mapperColorsPtr[1] * property->GetDiffuse() / 255.0,
          mapperColorsPtr[2] * property->GetDiffuse() / 255.0f };
        property->SetDiffuseColor(diffuseColor);
      }
    }
    else if (cellFlag == 0 || cellFlag == 1)
    {
      int numPointColors = mapperColors->GetNumberOfTuples();
      pointColors.resize(numPointColors);
      int ncomps = mapperColors->GetNumberOfComponents();

      for (int i = 0; i < numPointColors; i++)
      {
        u_char* color = mapperColors->GetPointer(ncomps * i);
        float alpha = ncomps == 3 ? 1.0f : (color[3] / 255.0f);

        pointColors[i] = { color[0] / 255.0f, color[1] / 255.0f, color[2] / 255.0f, alpha };
      }
    }
  }
  else
  {
    if (mapperColorCoords && mapperColorTextureMap)
    {
      // color on point interpolated values (subsequently colormapped via 1D LUT)
      int numPointValueTextureCoords = mapperColorCoords->GetNumberOfTuples();
      pointValueTextureCoords.resize(numPointValueTextureCoords);
      float* tc = mapperColorCoords->GetPointer(0);

      for (int i = 0; i < numPointValueTextureCoords; i++)
      {
        pointValueTextureCoords[i] = *tc;
        tc += 2;
      }

      albedoTextureMap = mapperColorTextureMap;
    }
  }

  double length = 1.0;
  if (mapper)
  {
    length = mapper->GetLength();
  }

  int scalingMode = vtkAnariActorNode::GetEnableScaling(actor);
  double pointSize = length / 1000.0 * property->GetPointSize();
  double lineWidth = length / 1000.0 * property->GetLineWidth();

  if (scalingMode == static_cast<int>(vtkAnariActorNode::ScalingMode::ALL_EXACT))
  {
    pointSize = property->GetPointSize();
    lineWidth = property->GetLineWidth();
  }

  // finer control over sphere and cylinders sizes
  vtkDataArray* scaleArray = nullptr;
  vtkPiecewiseFunction* scaleFunction = nullptr;

  if (mapper && (scalingMode > static_cast<int>(vtkAnariActorNode::ScalingMode::ALL_APPROXIMATE)))
  {
    const char* scaleArrayName = vtkAnariActorNode::GetScaleArrayName(actor);
    scaleArray = poly->GetPointData()->GetArray(scaleArrayName);

    if (scalingMode != static_cast<int>(vtkAnariActorNode::ScalingMode::EACH_EXACT))
    {
      scaleFunction = vtkAnariActorNode::GetScaleFunction(actor);
    }
  }

  vtkPolyDataMapperNode::vtkPDConnectivity conn;
  vtkPolyDataMapperNode::MakeConnectivity(poly, property->GetRepresentation(), conn);
  anari::Extensions anariDeviceExtensions =
    this->Internal->AnariRendererNode->GetAnariDeviceExtensions();

  if (!conn.vertex_index.empty())
  {
    anari::Surface anariSurface = nullptr;

    if (anariDeviceExtensions.ANARI_KHR_GEOMETRY_SPHERE)
    {
      auto anariSampler = this->Internal->VTKToAnariSampler(
        "albedoTex", "attribute0", anariSamplerInTransform, albedoTextureMap, sRGB);
      anariSurface = this->Internal->RenderAsSpheres(anariSampler, property, vertices,
        conn.vertex_index, pointSize, scaleArray, scaleFunction, textureCoords,
        pointValueTextureCoords, pointColors, cellFlag);
    }

    this->Internal->Surfaces.emplace_back(anariSurface);
  }

  if (!conn.line_index.empty())
  {
    anari::Surface anariSurface = nullptr;

    if (property->GetRepresentation() == VTK_POINTS)
    {
      if (anariDeviceExtensions.ANARI_KHR_GEOMETRY_SPHERE)
      {
        auto anariSampler = this->Internal->VTKToAnariSampler(
          "albedoTex", "attribute0", anariSamplerInTransform, albedoTextureMap, sRGB);
        anariSurface = this->Internal->RenderAsSpheres(anariSampler, property, vertices,
          conn.line_index, pointSize, scaleArray, scaleFunction, textureCoords,
          pointValueTextureCoords, pointColors, cellFlag);
      }
    }
    else
    {
      if (anariDeviceExtensions.ANARI_KHR_GEOMETRY_CYLINDER)
      {
        auto anariSampler = this->Internal->VTKToAnariSampler(
          "albedoTex", "attribute0", anariSamplerInTransform, albedoTextureMap, sRGB);
        anariSurface = this->Internal->RenderAsCylinders(anariSampler, property, vertices,
          conn.line_index, lineWidth, scaleArray, scaleFunction, textureCoords,
          pointValueTextureCoords, pointColors, cellFlag);
      }
      else if (anariDeviceExtensions.ANARI_KHR_GEOMETRY_CURVE)
      {
        auto anariSampler = this->Internal->VTKToAnariSampler(
          "albedoTex", "attribute0", anariSamplerInTransform, albedoTextureMap, sRGB);
        anariSurface = this->Internal->RenderAsCurves(anariSampler, property, vertices,
          conn.line_index, lineWidth, scaleArray, scaleFunction, textureCoords,
          pointValueTextureCoords, pointColors, cellFlag);
      }
    }

    this->Internal->Surfaces.emplace_back(anariSurface);
  }

  vtkPolyDataMapperNode::vtkPDConnectivity conn2;
  vtkPolyDataMapperNode::MakeConnectivity(poly, VTK_WIREFRAME, conn2);

  if (!conn.triangle_index.empty())
  {
    auto anariSampler = this->Internal->VTKToAnariSampler(
      "albedoTex", "attribute0", anariSamplerInTransform, albedoTextureMap, sRGB);
    this->Internal->RenderSurfaces(anariSampler, actor, poly, vertices, conn.triangle_index, true,
      pointSize, lineWidth, scaleArray, scaleFunction, textureCoords, pointValueTextureCoords,
      pointColors, conn2, cellFlag);
  }

  if (!conn.strip_index.empty())
  {
    auto anariSampler = this->Internal->VTKToAnariSampler(
      "albedoTex", "attribute0", anariSamplerInTransform, albedoTextureMap, sRGB);
    this->Internal->RenderSurfaces(anariSampler, actor, poly, vertices, conn.strip_index, false,
      pointSize, lineWidth, scaleArray, scaleFunction, textureCoords, pointValueTextureCoords,
      pointColors, conn2, cellFlag);
  }
}

//----------------------------------------------------------------------------
void vtkAnariPolyDataMapperNode::Build(bool prepass)
{
  vtkAnariProfiling startProfiling("VTKAPDMN::Build", vtkAnariProfiling::GREEN);
  if (!prepass || !ActorWasModified())
  {
    return;
  }

  if (!this->RendererNode)
  {
    this->RendererNode =
      static_cast<vtkAnariRendererNode*>(this->GetFirstAncestorOfType("vtkAnariRendererNode"));
  }

  if (!this->Internal->AnariDevice)
  {
    this->Internal->SetAnariConfig(this->RendererNode);
  }

  auto* actor = GetVtkActor();
  if (!actor->HasObserver(vtkCommand::ModifiedEvent))
  {
    vtkNew<PolyDataMapperCallback> cc;
    cc->RendererNode = this->RendererNode;
    actor->AddObserver(vtkCommand::ModifiedEvent, cc);
    cc->Execute(nullptr, vtkCommand::ModifiedEvent, nullptr);
  }
}

//----------------------------------------------------------------------------
void vtkAnariPolyDataMapperNode::Synchronize(bool prepass)
{
  vtkAnariProfiling startProfiling("VTKAPDMN::Synchronize", vtkAnariProfiling::GREEN);

  if (!prepass || !ActorWasModified())
  {
    return;
  }

  this->RenderTime = this->GetVtkActor()->GetMTime();
  this->ClearSurfaces();

  auto* actor = GetVtkActor();
  if (!actor->GetVisibility())
  {
    return;
  }

  vtkSmartPointer<vtkPolyData> poly;
  vtkPolyDataMapper* polyDataMapper = vtkPolyDataMapper::SafeDownCast(actor->GetMapper());

  if (polyDataMapper && polyDataMapper->GetNumberOfInputPorts() > 0)
  {
    poly = polyDataMapper->GetInput();
  }
  else
  {
    vtkNew<vtkDataSetSurfaceFilter> geometryExtractor;

    geometryExtractor->SetInputData(actor->GetMapper()->GetInput());
    geometryExtractor->Update();

    poly = static_cast<vtkPolyData*>(geometryExtractor->GetOutput());
  }

  if (!poly)
  {
    return;
  }

  vtkProperty* property = actor->GetProperty();
  std::string materialName = "matte";

  if (property->GetMaterialName() != nullptr)
  {
    materialName = property->GetMaterialName();
  }

  this->AnariRenderPoly(
    this->GetAnariActorNode(), poly, property->GetColor(), property->GetOpacity(), materialName);
}

//----------------------------------------------------------------------------
void vtkAnariPolyDataMapperNode::Render(bool prepass)
{
  vtkAnariProfiling startProfiling("VTKAPDMN::Render", vtkAnariProfiling::GREEN);

  if (!prepass)
  {
    return;
  }

  this->RenderSurfaceModels();
}

//----------------------------------------------------------------------------
vtkActor* vtkAnariPolyDataMapperNode::GetVtkActor() const
{
  return vtkActor::SafeDownCast(this->GetAnariActorNode()->GetRenderable());
}

//----------------------------------------------------------------------------
vtkAnariActorNode* vtkAnariPolyDataMapperNode::GetAnariActorNode() const
{
  return vtkAnariActorNode::SafeDownCast(this->Parent);
}

//----------------------------------------------------------------------------
bool vtkAnariPolyDataMapperNode::ActorWasModified() const
{
  return this->RenderTime < this->GetVtkActor()->GetMTime();
}

//----------------------------------------------------------------------------
void vtkAnariPolyDataMapperNode::RenderSurfaceModels()
{
  vtkAnariProfiling startProfiling("VTKAPDMN::RenderSurfaceModels", vtkAnariProfiling::GREEN);
  this->Internal->RenderSurfaceModels();
}

//----------------------------------------------------------------------------
void vtkAnariPolyDataMapperNode::ClearSurfaces()
{
  vtkAnariProfiling startProfiling("VTKAPDMN::ClearSurfaces", vtkAnariProfiling::GREEN);
  this->Internal->ClearSurfaces();
  this->RendererNode->InvalidateSceneStructure();
}

VTK_ABI_NAMESPACE_END
