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
#include "vtkFloatArray.h"
#include "vtkGLTFDocumentLoader.h"
#include "vtkImageData.h"
#include "vtkLight.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkTexture.h"
#include "vtkTransform.h"
#include "vtksys/SystemTools.hxx"

#include <algorithm>
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

void ApplyGLTFMaterialToVTKActor(std::shared_ptr<vtkGLTFDocumentLoader::Model> model,
  vtkGLTFDocumentLoader::Primitive& primitive, vtkSmartPointer<vtkActor> actor,
  std::map<int, vtkSmartPointer<vtkTexture> >& existingTextures)
{
  vtkGLTFDocumentLoader::Material& material = model->Materials[primitive.Material];

  if (!material.PbrMetallicRoughness.BaseColorFactor.empty())
  {
    // Apply base material color
    actor->GetProperty()->SetColor(material.PbrMetallicRoughness.BaseColorFactor.data());
  }

  int texIndex = material.PbrMetallicRoughness.BaseColorTexture.Index;
  if (texIndex >= 0 && texIndex < static_cast<int>(model->Textures.size()))
  {
    const vtkGLTFDocumentLoader::Texture& texture = model->Textures[texIndex];

    // Apply texture
    if (texture.Source >= 0)
    {
      vtkSmartPointer<vtkTexture> vtkTex;
      if (existingTextures.count(primitive.Material))
      {
        vtkTex = existingTextures[primitive.Material];
      }
      else
      {
        const vtkGLTFDocumentLoader::Image& image = model->Images[texture.Source];
        vtkNew<vtkImageData> imageData;
        imageData->ShallowCopy(image.ImageData);
        // Create texture object
        vtkTex = vtkSmartPointer<vtkTexture>::New();

        if (!material.DoubleSided)
        {
          actor->GetProperty()->BackfaceCullingOn();
        }

        switch (material.AlphaMode)
        {
          case vtkGLTFDocumentLoader::Material::AlphaModeType::OPAQUE:
            actor->ForceOpaqueOn();
            break;
          case vtkGLTFDocumentLoader::Material::AlphaModeType::MASK:
            vtkWarningWithObjectMacro(nullptr, "Alpha masking is not supported");
            break;
          case vtkGLTFDocumentLoader::Material::AlphaModeType::BLEND:
            // default behavior
            break;
        }
        vtkTex->SetBlendingMode(vtkTexture::VTK_TEXTURE_BLENDING_MODE_MODULATE);

        int nbSamplers = static_cast<int>(model->Samplers.size());
        // Approximate filtering settings
        if (texture.Sampler >= 0 && texture.Sampler < nbSamplers)
        {
          const vtkGLTFDocumentLoader::Sampler& sampler = model->Samplers[texture.Sampler];
          if (sampler.MinFilter != vtkGLTFDocumentLoader::Sampler::FilterType::NEAREST &&
            sampler.MinFilter != vtkGLTFDocumentLoader::Sampler::FilterType::LINEAR &&
            sampler.MagFilter != vtkGLTFDocumentLoader::Sampler::FilterType::NEAREST &&
            sampler.MagFilter != vtkGLTFDocumentLoader::Sampler::FilterType::LINEAR)
          {
            vtkTex->SetMipmap(true);
          }
          else
          {
            vtkTex->SetMipmap(false);
          }

          if (sampler.WrapS == vtkGLTFDocumentLoader::Sampler::WrapType::CLAMP_TO_EDGE ||
            sampler.WrapT == vtkGLTFDocumentLoader::Sampler::WrapType::CLAMP_TO_EDGE)
          {
            vtkTex->SetRepeat(false);
            vtkTex->SetEdgeClamp(true);
          }
          else if (sampler.WrapS == vtkGLTFDocumentLoader::Sampler::WrapType::REPEAT ||
            sampler.WrapT == vtkGLTFDocumentLoader::Sampler::WrapType::REPEAT)
          {
            vtkTex->SetRepeat(true);
            vtkTex->SetEdgeClamp(false);
          }
          else
          {
            vtkWarningWithObjectMacro(nullptr, "Mirrored texture wrapping is not supported!");
          }
          if (sampler.MinFilter == vtkGLTFDocumentLoader::Sampler::FilterType::LINEAR)
          {
            vtkTex->SetInterpolate(true);
          }
        }

        // Multiply vertex color by base color factor, as per the specification
        const std::vector<double>& baseColorFactor = material.PbrMetallicRoughness.BaseColorFactor;
        if (!baseColorFactor.empty())
        {
          auto scalars = primitive.Geometry->GetPointData()->GetScalars();
          if (scalars)
          {
            std::vector<double> tuple(scalars->GetNumberOfComponents(), 0);
            for (int i = 0; i < scalars->GetNumberOfTuples(); i++)
            {
              scalars->GetTuple(i, tuple.data());
              for (int component = 0; component < scalars->GetNumberOfComponents(); component++)
              {
                tuple[component] *= baseColorFactor[component];
              }
              scalars->SetTuple(i, tuple.data());
            }
          }
          else
          {
            vtkNew<vtkFloatArray> baseColorFactorScalars;
            baseColorFactorScalars->SetNumberOfComponents(static_cast<int>(baseColorFactor.size()));
            baseColorFactorScalars->SetNumberOfTuples(primitive.Geometry->GetNumberOfPoints());
            for (int component = 0; component < baseColorFactorScalars->GetNumberOfComponents();
                 component++)
            {
              baseColorFactorScalars->FillComponent(component, baseColorFactor[component]);
            }
            primitive.Geometry->GetPointData()->SetScalars(baseColorFactorScalars);
          }
        }
        vtkTex->SetInputData(imageData);
        existingTextures[primitive.Material] = vtkTex;
      }
      actor->SetTexture(vtkTex);
    }
  }
}
};

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
        vtkNew<vtkActor> actor;
        vtkNew<vtkPolyDataMapper> mapper;
        mapper->SetColorModeToDirectScalars();
        mapper->SetInterpolateScalarsBeforeMapping(true);
        mapper->SetInputData(primitive.Geometry);
        actor->SetMapper(mapper);
        actor->SetUserTransform(node.GlobalTransform);

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

  bool appliedFirstCamera = false;

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
      if (!appliedFirstCamera)
      {
        vtkCam->ApplyTransform(node.GlobalTransform);
        appliedFirstCamera = true;
      }
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
          light->SetConeAngle(180);
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
