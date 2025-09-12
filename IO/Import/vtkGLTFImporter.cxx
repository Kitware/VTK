// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkGLTFImporter.h"

#include "vtkActor.h"
#include "vtkActorCollection.h"
#include "vtkCamera.h"
#include "vtkCollection.h"
#include "vtkDataAssembly.h"
#include "vtkDoubleArray.h"
#include "vtkEventForwarderCommand.h"
#include "vtkFloatArray.h"
#include "vtkGLTFDocumentLoader.h"
#include "vtkImageAppendComponents.h"
#include "vtkImageData.h"
#include "vtkImageExtractComponents.h"
#include "vtkImageResize.h"
#include "vtkInformation.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataNormals.h"
#include "vtkPolyDataTangents.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkShaderProperty.h"
#include "vtkSmartPointer.h"
#include "vtkTexture.h"
#include "vtkTransform.h"
#include "vtkUniforms.h"
#include "vtksys/SystemTools.hxx"

#include <algorithm>
#include <array>
#include <set>
#include <stack>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkGLTFImporter);

namespace
{
// Desired attenuation value when distanceToLight == lightRange
const float MIN_LIGHT_ATTENUATION = 0.01;

/**
 * Builds a new vtkCamera object with properties from a glTF Camera struct
 */
//------------------------------------------------------------------------------
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
//------------------------------------------------------------------------------
vtkSmartPointer<vtkTexture> CreateVTKTextureFromGLTFTexture(
  std::shared_ptr<vtkGLTFDocumentLoader::Model> model, int textureIndex,
  std::map<int, vtkSmartPointer<vtkTexture>>& existingTextures, vtkGLTFImporter* parent)
{

  if (existingTextures.count(textureIndex))
  {
    return existingTextures[textureIndex];
  }

  const vtkGLTFDocumentLoader::Texture& glTFTex = model->Textures[textureIndex];
  if (glTFTex.Source < 0 || glTFTex.Source >= static_cast<int>(model->Images.size()))
  {
    vtkErrorWithObjectMacro(parent, "Image not found");
    return nullptr;
  }

  const vtkGLTFDocumentLoader::Image& image = model->Images[glTFTex.Source];
  if (image.ImageData == nullptr)
  {
    vtkErrorWithObjectMacro(parent, "Image mimeType not supported");
    return nullptr;
  }

  vtkNew<vtkTexture> texture;
  texture->SetColorModeToDirectScalars();
  texture->SetBlendingMode(vtkTexture::VTK_TEXTURE_BLENDING_MODE_MODULATE);
  // Approximate filtering settings
  int nbSamplers = static_cast<int>(model->Samplers.size());
  if (glTFTex.Sampler >= 0 && glTFTex.Sampler < nbSamplers)
  {
    const vtkGLTFDocumentLoader::Sampler& sampler = model->Samplers[glTFTex.Sampler];
    if (sampler.MinFilter == vtkGLTFDocumentLoader::Sampler::FilterType::NEAREST ||
      sampler.MinFilter == vtkGLTFDocumentLoader::Sampler::FilterType::LINEAR)
    {
      texture->MipmapOff();
    }
    else
    {
      texture->MipmapOn();
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

//------------------------------------------------------------------------------
bool MaterialHasMultipleUVs(
  const vtkGLTFDocumentLoader::Material& material, vtkGLTFImporter* parent)
{
  std::set<int> uv_sets;
  for (const vtkGLTFDocumentLoader::TextureInfo& info :
    { material.PbrMetallicRoughness.BaseColorTexture, material.EmissiveTexture,
      material.NormalTexture, material.OcclusionTexture,
      material.PbrMetallicRoughness.MetallicRoughnessTexture })
  {
    if (info.Index >= 0)
      uv_sets.insert(info.TexCoord);
  }
  if (uv_sets.find(-1) != uv_sets.end())
  {
    vtkErrorWithObjectMacro(parent,
      "A material defined a texture index without "
      "defining a texture coordinate set.");
  }
  return uv_sets.size() > 1;
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
bool ApplyGLTFMaterialToVTKActor(std::shared_ptr<vtkGLTFDocumentLoader::Model> model,
  vtkGLTFDocumentLoader::Primitive& primitive, vtkSmartPointer<vtkActor> actor,
  std::map<int, vtkSmartPointer<vtkTexture>>& existingTextures, vtkGLTFImporter* parent)
{
  vtkGLTFDocumentLoader::Material& material = model->Materials[primitive.Material];

  bool hasMultipleUVs = MaterialHasMultipleUVs(material, parent);
  if (hasMultipleUVs)
  {
    vtkWarningWithObjectMacro(
      parent, "Using multiple texture coordinates for the same model is not supported.");
  }
  auto property = actor->GetProperty();
  property->SetInterpolationToPBR();
  if (!material.PbrMetallicRoughness.BaseColorFactor.empty())
  {
    // Apply base material color
    actor->GetProperty()->SetColor(material.PbrMetallicRoughness.BaseColorFactor.data());
    actor->GetProperty()->SetOpacity(material.PbrMetallicRoughness.BaseColorFactor[3]);
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
    baseColorTex = CreateVTKTextureFromGLTFTexture(model, texIndex, existingTextures, parent);
    if (!baseColorTex)
    {
      return false;
    }
    baseColorTex->UseSRGBColorSpaceOn();
    property->SetBaseColorTexture(baseColorTex);
  }

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
      auto materialTex =
        CreateVTKTextureFromGLTFTexture(model, pbrTexIndex, existingTextures, parent);
      if (!materialTex)
      {
        return false;
      }
      property->SetORMTexture(materialTex);
    }
  }

  // Set emissive texture
  int emissiveTexIndex = material.EmissiveTexture.Index;
  if (emissiveTexIndex >= 0 && emissiveTexIndex < static_cast<int>(model->Textures.size()))
  {
    auto emissiveTex =
      CreateVTKTextureFromGLTFTexture(model, emissiveTexIndex, existingTextures, parent);
    if (!emissiveTex)
    {
      return false;
    }
    emissiveTex->UseSRGBColorSpaceOn();
    property->SetEmissiveTexture(emissiveTex);
  }
  // Set normal map
  int normalMapIndex = material.NormalTexture.Index;
  if (normalMapIndex >= 0 && normalMapIndex < static_cast<int>(model->Textures.size()))
  {
    actor->GetProperty()->SetNormalScale(material.NormalTextureScale);
    auto normalTex =
      CreateVTKTextureFromGLTFTexture(model, normalMapIndex, existingTextures, parent);
    if (!normalTex)
    {
      return false;
    }
    property->SetNormalTexture(normalTex);
  }

  // extension KHR_materials_unlit
  actor->GetProperty()->SetLighting(!material.Unlit);
  if (material.Unlit)
  {
    // the polydata mapper does not convert to sRGB when Unlit, so convert it there
    double r, g, b;
    actor->GetProperty()->GetColor(r, g, b);
    r = pow(r, 1.f / 2.2f);
    g = pow(g, 1.f / 2.2f);
    b = pow(b, 1.f / 2.2f);
    actor->GetProperty()->SetColor(r, g, b);
  }
  return true;
}

//------------------------------------------------------------------------------
void ApplyTransformToCamera(vtkSmartPointer<vtkCamera> cam, vtkSmartPointer<vtkMatrix4x4> transform)
{
  if (!cam || !transform)
  {
    return;
  }

  // At identity, camera position is origin, +y up, -z view direction
  double position[3] = { 0.0, 0.0, 0.0 };
  double viewUp[3] = { 0.0, 1.0, 0.0 };
  double focus[3] = { 0.0, 0.0, -1.0 };

  vtkNew<vtkTransform> t;
  t->SetMatrix(transform);

  t->TransformPoint(position, position);
  t->TransformVector(viewUp, viewUp);
  t->TransformVector(focus, focus);

  focus[0] += position[0];
  focus[1] += position[1];
  focus[2] += position[2];

  cam->SetPosition(position);
  cam->SetFocalPoint(focus);
  cam->SetViewUp(viewUp);
}
}

//------------------------------------------------------------------------------
vtkGLTFImporter::~vtkGLTFImporter()
{
  this->SetFileName(nullptr);
}

//------------------------------------------------------------------------------
void vtkGLTFImporter::InitializeLoader()
{
  this->Loader = vtkSmartPointer<vtkGLTFDocumentLoader>::New();
}

//------------------------------------------------------------------------------
int vtkGLTFImporter::ImportBegin()
{
  // Make sure we have a file to read.
  if (!this->Stream && !this->FileName)
  {
    vtkErrorMacro("Neither FileName nor Stream has been specified.");
    return 0;
  }

  this->Textures.clear();

  this->InitializeLoader();
  this->SceneHierarchy.TakeReference(vtkDataAssembly::New());

  vtkNew<vtkEventForwarderCommand> forwarder;
  forwarder->SetTarget(this);
  this->Loader->AddObserver(vtkCommand::ProgressEvent, forwarder);
  this->Loader->AddObserver(vtkCommand::WarningEvent, forwarder);
  this->Loader->AddObserver(vtkCommand::ErrorEvent, forwarder);

  // Check extension
  std::vector<char> glbBuffer;
  if (this->Stream != nullptr)
  {
    // this->Stream is defined.
    if (this->StreamIsBinary)
    {
      if (!this->Loader->LoadStreamBuffer(this->Stream, glbBuffer))
      {
        vtkErrorMacro("Error loading binary data");
        return 0;
      }
    }

    if (!this->Loader->LoadModelMetaDataFromStream(this->Stream, this->StreamURILoader))
    {
      vtkErrorMacro("Error loading model metadata");
      return 0;
    }
  }
  else
  {
    // this->FileName is defined.
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

  this->EnabledAnimations.resize(this->GetNumberOfAnimations());
  std::fill(std::begin(this->EnabledAnimations), std::end(this->EnabledAnimations), false);

  return 1;
}

//------------------------------------------------------------------------------
void vtkGLTFImporter::ImportActors(vtkRenderer* renderer)
{
  auto model = this->Loader->GetInternalModel();
  if (!model)
  {
    vtkErrorMacro("The GLTF model is nullptr, aborting.");
    this->SetUpdateStatus(vtkImporter::UpdateStatusEnum::FAILURE);
    return;
  }

  const int& scene = model->DefaultScene;

  // List of nodes to import
  std::stack<int> nodeIdStack;
  std::stack<int> dasmParents;
  int flatActorId = 0;

  // Add root nodes to the stack
  for (int nodeId : model->Scenes[scene].Nodes)
  {
    nodeIdStack.push(nodeId);
    dasmParents.push(0);
  }

  this->OutputsDescription = "";

  this->Actors.clear();
  this->ArmatureActors.clear();
  this->ActorCollection->RemoveAllItems();

  // Iterate over tree
  while (!nodeIdStack.empty())
  {
    // Get current node
    const int nodeId = nodeIdStack.top();
    nodeIdStack.pop();
    const auto& node = model->Nodes[nodeId];

    // Add this node into the scene hierarchy
    const int dasmParent = dasmParents.top();
    dasmParents.pop();
    std::string dasmNodeName;
    if (!node.Name.empty())
    {
      dasmNodeName = vtkDataAssembly::MakeValidNodeName(node.Name.c_str());
    }
    else
    {
      dasmNodeName = "node" + std::to_string(nodeId);
    }
    const int dasmNode = this->SceneHierarchy->AddNode(dasmNodeName.c_str(), dasmParent);

    // Import node's geometry
    if (node.Mesh >= 0)
    {
      auto mesh = model->Meshes[node.Mesh];
      int primitiveId = 0;
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

          if (pointData->GetNormals() == nullptr)
          {
            // Generate normals first
            vtkNew<vtkPolyDataNormals> normals;
            normals->SetInputData(primitive.Geometry);
            tangents->SetInputConnection(normals->GetOutputPort());
          }
          else
          {
            tangents->SetInputData(primitive.Geometry);
          }

          tangents->Update();
          mapper->SetInputConnection(tangents->GetOutputPort(0));
        }
        else
        {
          mapper->SetInputData(primitive.Geometry);
        }

        actor->SetMapper(mapper);
        actor->SetUserMatrix(node.GlobalTransform);
        std::string meshNodeName;
        if (!mesh.Name.empty())
        {
          std::string primitiveName = mesh.Name;
          if (mesh.Primitives.size() > 1)
          {
            primitiveName += "_primitive_" + std::to_string(primitiveId++);
          }
          this->OutputsDescription += primitiveName;
          meshNodeName = vtkDataAssembly::MakeValidNodeName(primitiveName.c_str());
        }
        else
        {
          meshNodeName = "primitive_" + std::to_string(primitiveId++);
        }
        this->OutputsDescription += "Primitive Geometry:\n";
        this->OutputsDescription +=
          vtkImporter::GetDataSetDescription(primitive.Geometry, vtkIndent(1));

        if (primitive.Material >= 0 &&
          primitive.Material < static_cast<int>(model->Materials.size()))
        {
          if (!ApplyGLTFMaterialToVTKActor(model, primitive, actor, this->Textures, this))
          {
            vtkErrorMacro("Could not apply GLTF material to VTK actor, aborting.");
            this->SetUpdateStatus(vtkImporter::UpdateStatusEnum::FAILURE);
            return;
          }
        }
        renderer->AddActor(actor);

        this->Actors[nodeId].emplace_back(actor);
        this->ActorCollection->AddItem(actor);
        const int actorNode =
          this->SceneHierarchy->AddNode(meshNodeName.c_str(), /*parent=*/dasmNode);
        this->SceneHierarchy->SetAttribute(actorNode, "parent_node_name", dasmNodeName.c_str());
        this->SceneHierarchy->SetAttribute(actorNode, "flat_actor_id", flatActorId++);

        this->InvokeEvent(vtkCommand::UpdateDataEvent);
      }
    }

    // Import node's armature
    if (this->GetImportArmature() && node.Skin >= 0)
    {
      auto skin = model->Skins[node.Skin];

      if (skin.Skeleton >= 0)
      {
        vtkNew<vtkActor> actor;

        vtkNew<vtkPolyDataMapper> mapper;
        mapper->SetInputData(skin.Armature);
        actor->SetMapper(mapper);

        this->ApplyArmatureProperties(actor);

        renderer->AddActor(actor);

        this->ArmatureActors[nodeId] = actor;
        this->ActorCollection->AddItem(actor);
      }
    }

    // Add node's children to stack
    for (int childNodeId : node.Children)
    {
      nodeIdStack.push(childNodeId);
      dasmParents.push(dasmNode);
    }
  }

  this->ApplySkinningMorphing();
}

//----------------------------------------------------------------------------
void vtkGLTFImporter::ApplyArmatureProperties(vtkActor* actor)
{
  actor->GetProperty()->RenderPointsAsSpheresOn();
  actor->GetProperty()->RenderLinesAsTubesOn();
}

//------------------------------------------------------------------------------
void vtkGLTFImporter::ImportCameras(vtkRenderer* renderer)
{
  auto model = this->Loader->GetInternalModel();
  if (!model)
  {
    vtkErrorMacro("The GLTF model is nullptr, aborting.");
    this->SetUpdateStatus(vtkImporter::UpdateStatusEnum::FAILURE);
    return;
  }

  int scene = model->DefaultScene;

  // List of nodes to import
  std::stack<int> nodeIdStack;

  // Add root nodes to the stack
  for (int nodeId : model->Scenes[scene].Nodes)
  {
    nodeIdStack.push(nodeId);
  }

  this->CameraCollection->RemoveAllItems();
  this->Cameras.clear();
  for (size_t i = 0; i < model->Cameras.size(); i++)
  {
    vtkGLTFDocumentLoader::Camera const& camera = model->Cameras[i];
    vtkSmartPointer<vtkCamera> vtkCam = GLTFCameraToVTKCamera(camera);
    this->Cameras[static_cast<int>(i)] = vtkCam;
    this->CameraCollection->AddItem(vtkCam);
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
      ApplyTransformToCamera(this->Cameras[node.Camera], node.GlobalTransform);
    }

    // Add node's children to stack
    for (int childNodeId : node.Children)
    {
      nodeIdStack.push(childNodeId);
    }
  }

  // update enabled camera
  if (this->EnabledCamera >= this->GetNumberOfCameras())
  {
    vtkWarningMacro("Camera index "
      << this->EnabledCamera
      << "is invalid, cameras have been imported but none have been enabled.");
    this->EnabledCamera = -1;
    return;
  }

  if (this->EnabledCamera >= 0)
  {
    renderer->SetActiveCamera(this->Cameras[this->EnabledCamera]);
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkGLTFImporter::GetNumberOfCameras()
{
  auto model = this->Loader->GetInternalModel();
  if (!model)
  {
    vtkErrorMacro("The GLTF model is nullptr, aborting.");
    return 0;
  }

  return model->Cameras.size();
}

//------------------------------------------------------------------------------
std::string vtkGLTFImporter::GetCameraName(vtkIdType camIndex)
{
  if (camIndex < 0 || camIndex >= this->GetNumberOfCameras())
  {
    vtkErrorMacro("Camera index invalid");
    return "";
  }
  auto model = this->Loader->GetInternalModel();
  assert(model);
  return model->Cameras[camIndex].Name;
}

//------------------------------------------------------------------------------
void vtkGLTFImporter::SetCamera(vtkIdType camIndex)
{
  // if the user sets the camera before the import, we do not know how many
  // cameras there are, so do not check if the index if valid here
  this->EnabledCamera = camIndex;
}

//------------------------------------------------------------------------------
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
  if (!model)
  {
    vtkErrorMacro("The GLTF model is nullptr, aborting.");
    this->SetUpdateStatus(vtkImporter::UpdateStatusEnum::FAILURE);
    return;
  }

  this->LightCollection->RemoveAllItems();

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
      light->SetTransformMatrix(node.GlobalTransform);
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
      this->LightCollection->AddItem(light);
    }

    // Add node's children to stack
    for (int childNodeId : node.Children)
    {
      nodeIdStack.push(childNodeId);
    }
  }
}

//----------------------------------------------------------------------------
bool vtkGLTFImporter::UpdateAtTimeValue(double timeValue)
{
  for (int animationId = 0; animationId < this->GetNumberOfAnimations(); animationId++)
  {
    if (this->EnabledAnimations[animationId])
    {
      if (!this->Loader->ApplyAnimation(static_cast<float>(timeValue), animationId))
      {
        vtkErrorMacro("Error applying animation");
        return false;
      }
    }
  }
  this->Loader->BuildGlobalTransforms();

  this->ImportCameras(this->Renderer);

  this->ApplySkinningMorphing();

  return this->GetUpdateStatus() == vtkImporter::UpdateStatusEnum::SUCCESS;
}

//----------------------------------------------------------------------------
void vtkGLTFImporter::ApplySkinningMorphing()
{
  const auto& model = this->Loader->GetInternalModel();
  if (!model)
  {
    vtkErrorMacro("The GLTF model is nullptr, aborting.");
    this->SetUpdateStatus(vtkImporter::UpdateStatusEnum::FAILURE);
    return;
  }

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
    vtkGLTFDocumentLoader::Node& node = model->Nodes[nodeId];

    std::vector<vtkSmartPointer<vtkMatrix4x4>> jointMats;
    if (node.Skin >= 0)
    {
      vtkGLTFDocumentLoader::ComputeJointMatrices(*model, model->Skins[node.Skin], node, jointMats);
    }

    for (const auto& actor : this->Actors[nodeId])
    {
      actor->SetUserMatrix(node.GlobalTransform);

      vtkShaderProperty* shaderProp = actor->GetShaderProperty();
      vtkUniforms* uniforms = shaderProp->GetVertexCustomUniforms();
      uniforms->RemoveAllUniforms();

      if (!jointMats.empty())
      {
        std::vector<float> vec;
        vec.reserve(16 * jointMats.size());
        for (size_t i = 0; i < jointMats.size(); i++)
        {
          vtkMatrix4x4* mat = jointMats[i];
          for (int j = 0; j < 4; j++)
          {
            for (int k = 0; k < 4; k++)
            {
              vec.push_back(static_cast<float>(mat->GetElement(k, j)));
            }
          }
        }
        uniforms->SetUniformMatrix4x4v(
          "jointMatrices", static_cast<int>(jointMats.size()), vec.data());
      }

      if (!node.Weights.empty())
      {
        size_t nbWeights = vtkMath::Min<size_t>(node.Weights.size(), 4);
        uniforms->SetUniform1fv("morphWeights", static_cast<int>(nbWeights), node.Weights.data());
      }
      else
      {
        vtkGLTFDocumentLoader::Mesh& mesh = model->Meshes[node.Mesh];
        if (!mesh.Weights.empty())
        {
          size_t nbWeights = vtkMath::Min<size_t>(mesh.Weights.size(), 4);
          uniforms->SetUniform1fv("morphWeights", static_cast<int>(nbWeights), mesh.Weights.data());
        }
      }
    }

    // armature
    if (this->GetImportArmature())
    {
      auto& actor = this->ArmatureActors[nodeId];
      if (actor)
      {
        vtkGLTFDocumentLoader::Skin& skin = model->Skins[node.Skin];

        vtkNew<vtkPoints> points;
        points->SetNumberOfPoints(skin.Armature->GetPoints()->GetNumberOfPoints());

        for (size_t i = 0; i < skin.Joints.size(); i++)
        {
          int jointId = skin.Joints[i];
          vtkGLTFDocumentLoader::Node& joint = model->Nodes[jointId];

          double p[3];
          p[0] = joint.GlobalTransform->GetElement(0, 3);
          p[1] = joint.GlobalTransform->GetElement(1, 3);
          p[2] = joint.GlobalTransform->GetElement(2, 3);

          points->SetPoint(i, p);
        }

        skin.Armature->SetPoints(points);
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
vtkIdType vtkGLTFImporter::GetNumberOfAnimations()
{
  const auto& model = this->Loader->GetInternalModel();
  if (!model)
  {
    vtkErrorMacro("The GLTF model is nullptr, aborting.");
    return 0;
  }

  return static_cast<vtkIdType>(model->Animations.size());
}

//----------------------------------------------------------------------------
std::string vtkGLTFImporter::GetAnimationName(vtkIdType animationIndex)
{
  if (animationIndex >= 0 && animationIndex < this->GetNumberOfAnimations())
  {
    const auto& model = this->Loader->GetInternalModel();
    assert(model);
    return model->Animations[animationIndex].Name;
  }
  return "";
}

//----------------------------------------------------------------------------
void vtkGLTFImporter::EnableAnimation(vtkIdType animationIndex)
{
  if (animationIndex >= 0 && animationIndex < this->GetNumberOfAnimations())
  {
    this->EnabledAnimations[animationIndex] = true;
  }
}

//----------------------------------------------------------------------------
void vtkGLTFImporter::DisableAnimation(vtkIdType animationIndex)
{
  if (animationIndex >= 0 && animationIndex < this->GetNumberOfAnimations())
  {
    this->EnabledAnimations[animationIndex] = false;
  }
}

//----------------------------------------------------------------------------
bool vtkGLTFImporter::IsAnimationEnabled(vtkIdType animationIndex)
{
  return this->EnabledAnimations[animationIndex];
}

//----------------------------------------------------------------------------
bool vtkGLTFImporter::GetTemporalInformation(vtkIdType animationIndex, double frameRate,
  int& nbTimeSteps, double timeRange[2], vtkDoubleArray* timeSteps)
{
  if (animationIndex < this->GetNumberOfAnimations())
  {
    const auto& model = this->Loader->GetInternalModel();
    assert(model);

    timeRange[0] = 0;
    timeRange[1] = model->Animations[animationIndex].Duration;

    if (frameRate > 0)
    {
      nbTimeSteps = 0;
      timeSteps->SetNumberOfComponents(1);
      timeSteps->SetNumberOfTuples(0);

      std::vector<double> ts;
      double period = (1.0 / frameRate);
      for (double i = timeRange[0]; i < timeRange[1]; i += period)
      {
        timeSteps->InsertNextTuple(&i);
        nbTimeSteps++;
      }
    }
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
void vtkGLTFImporter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent;
  if (this->Stream != nullptr)
  {
    os << "Stream (" << (this->StreamIsBinary ? "binary" : "ascii") << ")";
  }
  else
  {
    os << "File Name: " << (this->FileName ? this->FileName : "(none)");
  }
  os << "\n";
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkCamera> vtkGLTFImporter::GetCamera(unsigned int id)
{
  auto it = this->Cameras.find(id);
  if (it == this->Cameras.end())
  {
    vtkErrorMacro("Out of range camera index");
    return nullptr;
  }
  return it->second;
}

VTK_ABI_NAMESPACE_END
