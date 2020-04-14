/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGLTFImporter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkGLTFImporter.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkEventForwarderCommand.h"
#include "vtkFloatArray.h"
#include "vtkGLTFDocumentLoader.h"
#include "vtkImageAppendComponents.h"
#include "vtkImageData.h"
#include "vtkImageExtractComponents.h"
#include "vtkImageResize.h"
#include "vtkInformation.h"
#include "vtkLight.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataTangents.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkTexture.h"
#include "vtkTransform.h"
#include "vtksys/SystemTools.hxx"

#include <algorithm>
#include <array>
#include <stack>

vtkStandardNewMacro(vtkGLTFImporter);

namespace
{
// Desired attenuation value when distanceToLight == lightRange
static const float MIN_LIGHT_ATTENUATION = 0.01;

/**
 * Builds a new vtkCamera object with properties from a glTF Camera struct
 */
//----------------------------------------------------------------------------
vtkSmartPointer<vtkCamera> GLTFCameraToVTKCamera(const vtkGLTFDocumentLoader::Camera& gltfCam)
{
  vtkNew<vtkCamera> vtkCam;
  vtkCam->SetClippingRange(gltfCam.Znear, gltfCam.Zfar);
  if (gltfCam.IsPerspective)
  {
    vtkCam->SetParallelProjection(false);
    vtkCam->SetViewAngle(vtkMath::DegreesFromRadians(gltfCam.Yfov));
  }
  else
  {
    vtkCam->SetParallelProjection(true);
    vtkCam->SetParallelScale(gltfCam.Ymag);
  }
  return vtkCam;
}

/**
 * Create a vtkTexture object with a glTF texture as model. Sampling options are approximated.
 */
//----------------------------------------------------------------------------
vtkSmartPointer<vtkTexture> CreateVTKTextureFromGLTFTexture(
  std::shared_ptr<vtkGLTFDocumentLoader::Model> model, int textureIndex,
  std::map<int, vtkSmartPointer<vtkTexture> >& existingTextures)
{

  if (existingTextures.count(textureIndex))
  {
    return existingTextures[textureIndex];
  }

  const vtkGLTFDocumentLoader::Texture& glTFTex = model->Textures[textureIndex];
  if (glTFTex.Source < 0 || glTFTex.Source >= static_cast<int>(model->Images.size()))
  {
    vtkErrorWithObjectMacro(nullptr, "Image not found");
    return nullptr;
  }

  const vtkGLTFDocumentLoader::Image& image = model->Images[glTFTex.Source];

  vtkNew<vtkTexture> texture;
  texture->SetColorModeToDirectScalars();
  texture->SetBlendingMode(vtkTexture::VTK_TEXTURE_BLENDING_MODE_MODULATE);
  // Approximate filtering settings
  int nbSamplers = static_cast<int>(model->Samplers.size());
  if (glTFTex.Sampler >= 0 && glTFTex.Sampler < nbSamplers)
  {
    const vtkGLTFDocumentLoader::Sampler& sampler = model->Samplers[glTFTex.Sampler];
    if ((sampler.MinFilter == vtkGLTFDocumentLoader::Sampler::FilterType::NEAREST ||
          sampler.MinFilter == vtkGLTFDocumentLoader::Sampler::FilterType::LINEAR) &&
      (sampler.MagFilter == vtkGLTFDocumentLoader::Sampler::FilterType::NEAREST ||
        sampler.MagFilter == vtkGLTFDocumentLoader::Sampler::FilterType::LINEAR))
    {
      texture->MipmapOn();
    }
    else
    {
      texture->MipmapOff();
    }

    if (sampler.WrapS == vtkGLTFDocumentLoader::Sampler::WrapType::CLAMP_TO_EDGE ||
      sampler.WrapT == vtkGLTFDocumentLoader::Sampler::WrapType::CLAMP_TO_EDGE)
    {
      texture->RepeatOff();
      texture->EdgeClampOn();
    }
    else if (sampler.WrapS == vtkGLTFDocumentLoader::Sampler::WrapType::REPEAT ||
      sampler.WrapT == vtkGLTFDocumentLoader::Sampler::WrapType::REPEAT)
    {
      texture->RepeatOn();
      texture->EdgeClampOff();
    }
    else
    {
      vtkWarningWithObjectMacro(nullptr, "Mirrored texture wrapping is not supported!");
    }

    if (sampler.MinFilter == vtkGLTFDocumentLoader::Sampler::FilterType::LINEAR ||
      sampler.MinFilter == vtkGLTFDocumentLoader::Sampler::FilterType::LINEAR_MIPMAP_NEAREST ||
      sampler.MinFilter == vtkGLTFDocumentLoader::Sampler::FilterType::NEAREST_MIPMAP_LINEAR ||
      sampler.MinFilter == vtkGLTFDocumentLoader::Sampler::FilterType::LINEAR_MIPMAP_LINEAR ||
      sampler.MagFilter == vtkGLTFDocumentLoader::Sampler::FilterType::LINEAR ||
      sampler.MagFilter == vtkGLTFDocumentLoader::Sampler::FilterType::LINEAR_MIPMAP_NEAREST ||
      sampler.MagFilter == vtkGLTFDocumentLoader::Sampler::FilterType::NEAREST_MIPMAP_LINEAR ||
      sampler.MagFilter == vtkGLTFDocumentLoader::Sampler::FilterType::LINEAR_MIPMAP_LINEAR)
    {
      texture->InterpolateOn();
    }
  }
  else
  {
    texture->MipmapOn();
    texture->InterpolateOn();
    texture->EdgeClampOn();
  }

  vtkNew<vtkImageData> imageData;
  imageData->ShallowCopy(image.ImageData);

  texture->SetInputData(imageData);
  existingTextures[textureIndex] = texture;
  return texture;
}

//----------------------------------------------------------------------------
bool MaterialHasMultipleUVs(const vtkGLTFDocumentLoader::Material& material)
{
  int firstUV = material.PbrMetallicRoughness.BaseColorTexture.TexCoord;
  return (material.EmissiveTexture.Index >= 0 && material.EmissiveTexture.TexCoord != firstUV) ||
    (material.NormalTexture.Index >= 0 && material.NormalTexture.TexCoord != firstUV) ||
    (material.OcclusionTexture.Index >= 0 && material.OcclusionTexture.TexCoord != firstUV) ||
    (material.PbrMetallicRoughness.MetallicRoughnessTexture.Index >= 0 &&
      material.PbrMetallicRoughness.MetallicRoughnessTexture.TexCoord != firstUV);
}

//----------------------------------------------------------------------------
bool PrimitiveNeedsTangents(const std::shared_ptr<vtkGLTFDocumentLoader::Model> model,
  const vtkGLTFDocumentLoader::Primitive& primitive)
{
  // If no material is present, we don't need to generate tangents
  if (primitive.Material < 0 || primitive.Material >= static_cast<int>(model->Materials.size()))
  {
    return false;
  }
  vtkGLTFDocumentLoader::Material& material = model->Materials[primitive.Material];
  // If a normal map is present, we do need tangents
  int normalMapIndex = material.NormalTexture.Index;
  return normalMapIndex >= 0 && normalMapIndex < static_cast<int>(model->Textures.size());
}

//----------------------------------------------------------------------------
void ApplyGLTFMaterialToVTKActor(std::shared_ptr<vtkGLTFDocumentLoader::Model> model,
  vtkGLTFDocumentLoader::Primitive& primitive, vtkSmartPointer<vtkActor> actor,
  std::map<int, vtkSmartPointer<vtkTexture> >& existingTextures)
{
  vtkGLTFDocumentLoader::Material& material = model->Materials[primitive.Material];

  bool hasMultipleUVs = MaterialHasMultipleUVs(material);
  if (hasMultipleUVs)
  {
    vtkWarningWithObjectMacro(
      nullptr, "Using multiple texture coordinates for the same model is not supported.");
  }
  auto property = actor->GetProperty();
  property->SetInterpolationToPBR();
  if (!material.PbrMetallicRoughness.BaseColorFactor.empty())
  {
    // Apply base material color
    actor->GetProperty()->SetColor(material.PbrMetallicRoughness.BaseColorFactor.data());
    actor->GetProperty()->SetMetallic(material.PbrMetallicRoughness.MetallicFactor);
    actor->GetProperty()->SetRoughness(material.PbrMetallicRoughness.RoughnessFactor);
    actor->GetProperty()->SetEmissiveFactor(material.EmissiveFactor.data());
  }

  if (material.AlphaMode != vtkGLTFDocumentLoader::Material::AlphaModeType::OPAQUE)
  {
    actor->ForceTranslucentOn();
  }

  // flip texture coordinates
  if (actor->GetPropertyKeys() == nullptr)
  {
    vtkNew<vtkInformation> info;
    actor->SetPropertyKeys(info);
  }
  double mat[] = { 1, 0, 0, 0, 0, -1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1 };
  actor->GetPropertyKeys()->Set(vtkProp::GeneralTextureTransform(), mat, 16);

  if (!material.DoubleSided)
  {
    actor->GetProperty()->BackfaceCullingOn();
  }

  int texIndex = material.PbrMetallicRoughness.BaseColorTexture.Index;
  if (texIndex >= 0 && texIndex < static_cast<int>(model->Textures.size()))
  {
    // set albedo texture
    vtkSmartPointer<vtkTexture> baseColorTex;
    baseColorTex = CreateVTKTextureFromGLTFTexture(model, texIndex, existingTextures);
    baseColorTex->UseSRGBColorSpaceOn();
    property->SetBaseColorTexture(baseColorTex);

    // merge ambient occlusion and metallic/roughness, then set material texture
    int pbrTexIndex = material.PbrMetallicRoughness.MetallicRoughnessTexture.Index;
    if (pbrTexIndex >= 0 && pbrTexIndex < static_cast<int>(model->Textures.size()))
    {
      const vtkGLTFDocumentLoader::Texture& pbrTexture = model->Textures[pbrTexIndex];
      if (pbrTexture.Source >= 0 && pbrTexture.Source < static_cast<int>(model->Images.size()))
      {
        const vtkGLTFDocumentLoader::Image& pbrImage = model->Images[pbrTexture.Source];
        // While glTF 2.0 uses two different textures for Ambient Occlusion and Metallic/Roughness
        // values, VTK only uses one, so we merge both textures into one.
        // If an Ambient Occlusion texture is present, we merge its first channel into the
        // metallic/roughness texture (AO is r, Roughness g and Metallic b) If no Ambient
        // Occlusion texture is present, we need to fill the metallic/roughness texture's first
        // channel with 255
        int aoTexIndex = material.OcclusionTexture.Index;
        if (!hasMultipleUVs && aoTexIndex >= 0 &&
          aoTexIndex < static_cast<int>(model->Textures.size()))
        {
          actor->GetProperty()->SetOcclusionStrength(material.OcclusionTextureStrength);
          const vtkGLTFDocumentLoader::Texture& aoTexture = model->Textures[aoTexIndex];
          const vtkGLTFDocumentLoader::Image& aoImage = model->Images[aoTexture.Source];
          vtkNew<vtkImageExtractComponents> redAO;
          // If sizes are different, resize the AO texture to the R/M texture's size
          std::array<vtkIdType, 3> aoSize = { { 0 } };
          std::array<vtkIdType, 3> pbrSize = { { 0 } };
          aoImage.ImageData->GetDimensions(aoSize.data());
          pbrImage.ImageData->GetDimensions(pbrSize.data());
          // compare dimensions
          if (aoSize != pbrSize)
          {
            vtkNew<vtkImageResize> resize;
            resize->SetInputData(aoImage.ImageData);
            resize->SetOutputDimensions(pbrSize[0], pbrSize[1], pbrSize[2]);
            resize->Update();
            redAO->SetInputConnection(resize->GetOutputPort(0));
          }
          else
          {
            redAO->SetInputData(aoImage.ImageData);
          }
          redAO->SetComponents(0);
          vtkNew<vtkImageExtractComponents> gbPbr;
          gbPbr->SetInputData(pbrImage.ImageData);
          gbPbr->SetComponents(1, 2);
          vtkNew<vtkImageAppendComponents> append;
          append->AddInputConnection(redAO->GetOutputPort());
          append->AddInputConnection(gbPbr->GetOutputPort());
          append->SetOutput(pbrImage.ImageData);
          append->Update();
        }
        else
        {
          pbrImage.ImageData->GetPointData()->GetScalars()->FillComponent(0, 255);
        }
        auto materialTex = CreateVTKTextureFromGLTFTexture(model, pbrTexIndex, existingTextures);
        property->SetORMTexture(materialTex);
      }
    }

    // Set emissive texture
    int emissiveTexIndex = material.EmissiveTexture.Index;
    if (emissiveTexIndex >= 0 && emissiveTexIndex < static_cast<int>(model->Textures.size()))
    {
      auto emissiveTex = CreateVTKTextureFromGLTFTexture(model, emissiveTexIndex, existingTextures);
      emissiveTex->UseSRGBColorSpaceOn();
      property->SetEmissiveTexture(emissiveTex);
    }
    // Set normal map
    int normalMapIndex = material.NormalTexture.Index;
    if (normalMapIndex >= 0 && normalMapIndex < static_cast<int>(model->Textures.size()))
    {
      actor->GetProperty()->SetNormalScale(material.NormalTextureScale);
      auto normalTex = CreateVTKTextureFromGLTFTexture(model, normalMapIndex, existingTextures);
      property->SetNormalTexture(normalTex);
    }
  }
};

//----------------------------------------------------------------------------
void ApplyTransformToCamera(vtkSmartPointer<vtkCamera> cam, vtkSmartPointer<vtkTransform> transform)
{
  if (!cam || !transform)
  {
    return;
  }

  double position[3] = { 0.0 };
  double viewUp[3] = { 0.0 };
  double focus[3] = { 0.0 };

  transform->TransformPoint(cam->GetPosition(), position);
  transform->TransformVector(cam->GetViewUp(), viewUp);
  transform->TransformVector(cam->GetDirectionOfProjection(), focus);
  focus[0] -= position[0];
  focus[1] -= position[1];
  focus[2] -= position[2];

  cam->SetPosition(position);
  cam->SetFocalPoint(focus);
  cam->SetViewUp(viewUp);
}
}

//----------------------------------------------------------------------------
vtkGLTFImporter::~vtkGLTFImporter()
{
  this->SetFileName(nullptr);
}

//----------------------------------------------------------------------------
int vtkGLTFImporter::ImportBegin()
{
  // Make sure we have a file to read.
  if (!this->FileName)
  {
    vtkErrorMacro("A FileName must be specified.");
    return 0;
  }

  this->Textures.clear();

  this->Loader = vtkSmartPointer<vtkGLTFDocumentLoader>::New();

  vtkNew<vtkEventForwarderCommand> forwarder;
  forwarder->SetTarget(this);
  this->Loader->AddObserver(vtkCommand::ProgressEvent, forwarder);

  // Check extension
  std::vector<char> glbBuffer;
  std::string extension = vtksys::SystemTools::GetFilenameLastExtension(this->FileName);
  if (extension == ".glb")
  {
    if (!this->Loader->LoadFileBuffer(this->FileName, glbBuffer))
    {
      vtkErrorMacro("Error loading binary data");
      return 0;
    }
  }

  if (!this->Loader->LoadModelMetaDataFromFile(this->FileName))
  {
    vtkErrorMacro("Error loading model metadata");
    return 0;
  }
  if (!this->Loader->LoadModelData(glbBuffer))
  {
    vtkErrorMacro("Error loading model data");
    return 0;
  }
  if (!this->Loader->BuildModelVTKGeometry())
  {
    vtkErrorMacro("Error building model vtk data");
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkGLTFImporter::ImportActors(vtkRenderer* renderer)
{
  auto model = this->Loader->GetInternalModel();

  int scene = model->DefaultScene;

  // List of nodes to import
  std::stack<int> nodeIdStack;

  // Add root nodes to the stack
  for (int nodeId : model->Scenes[scene].Nodes)
  {
    nodeIdStack.push(nodeId);
  }

  this->OutputsDescription = "";

  // Iterate over tree
  while (!nodeIdStack.empty())
  {
    // Get current node
    int nodeId = nodeIdStack.top();
    nodeIdStack.pop();
    vtkGLTFDocumentLoader::Node& node = model->Nodes[nodeId];

    // Import node's geometry
    if (node.Mesh >= 0)
    {
      auto mesh = model->Meshes[node.Mesh];
      for (auto primitive : mesh.Primitives)
      {
        auto pointData = primitive.Geometry->GetPointData();

        vtkNew<vtkActor> actor;
        vtkNew<vtkPolyDataMapper> mapper;
        mapper->SetColorModeToDirectScalars();
        mapper->SetInterpolateScalarsBeforeMapping(true);

        if (pointData->GetTangents() == nullptr && PrimitiveNeedsTangents(model, primitive))
        {
          // Generate tangents
          vtkNew<vtkPolyDataTangents> tangents;
          tangents->SetInputData(primitive.Geometry);
          tangents->Update();
          mapper->SetInputConnection(tangents->GetOutputPort(0));
        }
        else
        {
          mapper->SetInputData(primitive.Geometry);
        }

        actor->SetMapper(mapper);
        actor->SetUserTransform(node.GlobalTransform);

        if (!mesh.Name.empty())
        {
          this->OutputsDescription += mesh.Name + " ";
        }
        this->OutputsDescription += "Primitive Geometry:\n";
        this->OutputsDescription +=
          vtkImporter::GetDataSetDescription(primitive.Geometry, vtkIndent(1));

        if (primitive.Material >= 0 &&
          primitive.Material < static_cast<int>(model->Materials.size()))
        {
          ApplyGLTFMaterialToVTKActor(model, primitive, actor, this->Textures);
        }
        renderer->AddActor(actor);
      }
    }

    // Add node's children to stack
    for (int childNodeId : node.Children)
    {
      nodeIdStack.push(childNodeId);
    }
  }
}

//----------------------------------------------------------------------------
void vtkGLTFImporter::ImportCameras(vtkRenderer* renderer)
{
  auto model = this->Loader->GetInternalModel();

  int scene = model->DefaultScene;

  // List of nodes to import
  std::stack<int> nodeIdStack;

  // Add root nodes to the stack
  for (int nodeId : model->Scenes[scene].Nodes)
  {
    nodeIdStack.push(nodeId);
  }

  // Iterate over tree
  while (!nodeIdStack.empty())
  {
    // Get current node
    int nodeId = nodeIdStack.top();
    nodeIdStack.pop();
    const vtkGLTFDocumentLoader::Node& node = model->Nodes[nodeId];

    // Import node's camera
    if (node.Camera >= 0 && node.Camera < static_cast<int>(model->Cameras.size()))
    {
      vtkGLTFDocumentLoader::Camera const& camera = model->Cameras[node.Camera];
      auto vtkCam = GLTFCameraToVTKCamera(camera);
      ApplyTransformToCamera(vtkCam, node.GlobalTransform);
      renderer->SetActiveCamera(vtkCam);
      // Since the same glTF camera object can be used by multiple nodes (so with different
      // transforms), multiple vtkCameras are generated for the same glTF camera object, but with
      // different transforms.
      this->Cameras.push_back(vtkCam);
    }

    // Add node's children to stack
    for (int childNodeId : node.Children)
    {
      nodeIdStack.push(childNodeId);
    }
  }
}

//----------------------------------------------------------------------------
void vtkGLTFImporter::ImportLights(vtkRenderer* renderer)
{
  // Check that lights extension is enabled
  const auto& extensions = this->Loader->GetUsedExtensions();
  if (std::find(extensions.begin(), extensions.end(), "KHR_lights_punctual") == extensions.end())
  {
    return;
  }

  // List of nodes to import
  std::stack<int> nodeIdStack;

  const auto& model = this->Loader->GetInternalModel();
  const auto& lights = model->ExtensionMetaData.KHRLightsPunctualMetaData.Lights;

  // Add root nodes to the stack
  for (int nodeId : model->Scenes[model->DefaultScene].Nodes)
  {
    nodeIdStack.push(nodeId);
  }

  // Iterate over tree
  while (!nodeIdStack.empty())
  {
    // Get current node
    int nodeId = nodeIdStack.top();
    nodeIdStack.pop();

    const vtkGLTFDocumentLoader::Node& node = model->Nodes[nodeId];
    const auto lightId = node.ExtensionMetaData.KHRLightsPunctualMetaData.Light;
    if (lightId >= 0 && lightId < static_cast<int>(lights.size()))
    {
      const auto& glTFLight = lights[lightId];
      // Add light
      vtkNew<vtkLight> light;
      light->SetColor(glTFLight.Color.data());
      light->SetTransformMatrix(node.GlobalTransform->GetMatrix());
      // Handle range
      if (glTFLight.Range > 0)
      {
        // Set quadratic values to get attenuation(range) ~= MIN_LIGHT_ATTENUATION
        light->SetAttenuationValues(
          1, 0, 1.0 / (glTFLight.Range * glTFLight.Range * MIN_LIGHT_ATTENUATION));
      }
      light->SetIntensity(glTFLight.Intensity);
      switch (glTFLight.Type)
      {
        case vtkGLTFDocumentLoader::Extensions::KHRLightsPunctual::Light::LightType::DIRECTIONAL:
          light->SetPositional(false);
          break;
        case vtkGLTFDocumentLoader::Extensions::KHRLightsPunctual::Light::LightType::POINT:
          light->SetPositional(true);
          // Set as point light
          light->SetConeAngle(90);
          break;
        case vtkGLTFDocumentLoader::Extensions::KHRLightsPunctual::Light::LightType::SPOT:
          light->SetPositional(true);
          light->SetConeAngle(vtkMath::DegreesFromRadians(glTFLight.SpotOuterConeAngle));
          break;
      }
      renderer->AddLight(light);
    }

    // Add node's children to stack
    for (int childNodeId : node.Children)
    {
      nodeIdStack.push(childNodeId);
    }
  }
}

//----------------------------------------------------------------------------
void vtkGLTFImporter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "File Name: " << (this->FileName ? this->FileName : "(none)") << "\n";
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkCamera> vtkGLTFImporter::GetCamera(unsigned int id)
{
  if (id >= this->Cameras.size())
  {
    vtkErrorMacro("Out of range camera index");
    return nullptr;
  }
  return this->Cameras[id];
}

//----------------------------------------------------------------------------
size_t vtkGLTFImporter::GetNumberOfCameras()
{
  return this->Cameras.size();
}
