/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGLTFReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkGLTFReader.h"

#include "vtkCommand.h"
#include "vtkDataArraySelection.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkGLTFDocumentLoader.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkTransform.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkWeightedTransformFilter.h"
#include "vtksys/SystemTools.hxx"

#include <array>
#include <sstream>

namespace
{
//----------------------------------------------------------------------------
// Replacement for std::to_string as it is not supported by certain compilers
template<typename T>
std::string value_to_string(const T& val)
{
  std::ostringstream ss;
  ss << val;
  return ss.str();
}

//----------------------------------------------------------------------------
std::string MakeUniqueNonEmptyName(
  const std::string& name, std::map<std::string, unsigned int>& duplicateCounters)
{
  std::string newName = "Unnamed";
  if (!name.empty())
  {
    newName = name;
  }
  if (duplicateCounters.count(newName) > 0)
  {
    duplicateCounters[newName]++;
    newName += '_' + value_to_string(duplicateCounters[newName] - 1);
    duplicateCounters[newName] = 1;
  }
  else
  {
    duplicateCounters[newName] = 1;
  }
  return newName;
}

//----------------------------------------------------------------------------
void AddIntegerToFieldData(
  std::string arrayName, int value, vtkSmartPointer<vtkFieldData> fieldData)
{
  vtkNew<vtkIntArray> array;
  array->SetName(arrayName.c_str());
  array->SetNumberOfComponents(1);
  array->SetNumberOfValues(1);
  array->SetValue(0, value);
  fieldData->AddArray(array);
}

//----------------------------------------------------------------------------
void AddFloatToFieldData(
  std::string arrayName, float value, vtkSmartPointer<vtkFieldData> fieldData)
{
  vtkNew<vtkFloatArray> array;
  array->SetName(arrayName.c_str());
  array->SetNumberOfComponents(1);
  array->SetNumberOfValues(1);
  array->SetValue(0, value);
  fieldData->AddArray(array);
}

//----------------------------------------------------------------------------
void AddVecNfToFieldData(const std::string& arrayName, const std::vector<float>& multiplier,
  vtkSmartPointer<vtkFieldData> fieldData)
{
  vtkNew<vtkFloatArray> array;
  array->SetName(arrayName.c_str());
  array->SetNumberOfComponents(static_cast<int>(multiplier.size()));
  array->SetNumberOfTuples(1);
  array->SetTypedTuple(0, multiplier.data());
  fieldData->AddArray(array);
}

//----------------------------------------------------------------------------
void AddTextureInfoToFieldData(const std::string& prefix, int textureIndex, int textureCoordIndex,
  vtkSmartPointer<vtkFieldData> fieldData, std::vector<float> multiplier = std::vector<float>())
{
  AddIntegerToFieldData(prefix + "TextureIndex", textureIndex, fieldData);
  if (multiplier.size() == 3 || multiplier.size() == 4)
  {
    AddVecNfToFieldData(prefix + "Multiplier", multiplier, fieldData);
  }
  AddIntegerToFieldData(prefix + "TexCoordIndex", textureCoordIndex, fieldData);
}

//----------------------------------------------------------------------------
void AddMaterialToFieldData(int materialId, vtkSmartPointer<vtkFieldData> fieldData,
  const vtkGLTFDocumentLoader::Model& model)
{
  int nbTextures = static_cast<int>(model.Textures.size());

  // Append material information (multiplier, texture indices, and texture coordinate array name)
  if (materialId >= 0 && materialId < static_cast<int>(model.Materials.size()))
  {
    auto material = model.Materials[materialId];
    auto pbr = material.PbrMetallicRoughness;
    if (pbr.BaseColorTexture.Index >= 0 && pbr.BaseColorTexture.Index < nbTextures)
    {
      AddTextureInfoToFieldData(
        "BaseColor", pbr.BaseColorTexture.Index, pbr.BaseColorTexture.TexCoord, fieldData);
    }
    std::vector<float> multiplier(4, 1.0);
    if (pbr.BaseColorFactor.size() == 3 || pbr.BaseColorFactor.size() == 4)
    {
      multiplier = std::vector<float>{ pbr.BaseColorFactor.begin(), pbr.BaseColorFactor.end() };
    }
    AddVecNfToFieldData("BaseColorMultiplier", multiplier, fieldData);
    if (pbr.MetallicRoughnessTexture.Index >= 0 && pbr.MetallicRoughnessTexture.Index < nbTextures)
    {
      AddTextureInfoToFieldData("MetallicRoughness", pbr.MetallicRoughnessTexture.Index,
        pbr.MetallicRoughnessTexture.TexCoord, fieldData);
    }
    multiplier = std::vector<float>{ 0, pbr.MetallicFactor, pbr.RoughnessFactor };
    AddVecNfToFieldData("MetallicRoughness", multiplier, fieldData);
    if (material.NormalTexture.Index >= 0 && material.NormalTexture.Index < nbTextures)
    {
      AddTextureInfoToFieldData("Normal", material.NormalTexture.Index,
        material.NormalTexture.TexCoord, fieldData,
        std::vector<float>(3, material.NormalTextureScale));
    }
    if (material.OcclusionTexture.Index >= 0 && material.OcclusionTexture.Index < nbTextures)
    {
      AddTextureInfoToFieldData("Occlusion", material.OcclusionTexture.Index,
        material.OcclusionTexture.TexCoord, fieldData,
        std::vector<float>(3, material.OcclusionTextureStrength));
    }
    if (material.EmissiveTexture.Index >= 0 && material.EmissiveTexture.Index < nbTextures)
    {
      AddTextureInfoToFieldData("Emissive", material.EmissiveTexture.Index,
        material.EmissiveTexture.TexCoord, fieldData,
        std::vector<float>{ material.EmissiveFactor.begin(), material.EmissiveFactor.end() });
    }
    // Add alpha cutoff value, alpha mode
    if (material.AlphaMode == vtkGLTFDocumentLoader::Material::AlphaModeType::MASK)
    {
      AddFloatToFieldData("AlphaCutoff", material.AlphaCutoff, fieldData);
    }
    else if (material.AlphaMode == vtkGLTFDocumentLoader::Material::AlphaModeType::OPAQUE)
    {
      AddIntegerToFieldData("ForceOpaque", 1, fieldData);
    }
  }
  else
  {
    // Append default material information
    AddVecNfToFieldData("BaseColorMultiplier", std::vector<float>(4, 1.0f), fieldData);
    AddVecNfToFieldData("MetallicRoughness", std::vector<float>(3, 1.0f), fieldData);
    AddVecNfToFieldData("Emissive", std::vector<float>(3, 0.0f), fieldData);
    AddIntegerToFieldData("ForceOpaque", 1, fieldData);
  }
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> ApplyMorphingToDataArray(vtkSmartPointer<vtkDataArray> origin,
  const std::vector<float>& weights, const std::vector<vtkSmartPointer<vtkFloatArray> >& targets)
{
  if (origin == nullptr)
  {
    return nullptr;
  }

  vtkSmartPointer<vtkDataArray> result = vtkSmartPointer<vtkDataArray>::Take(origin->NewInstance());
  result->DeepCopy(origin);

  if (targets.empty() || weights.empty() || targets.size() != weights.size())
  {
    return origin;
  }
  std::vector<double> tuple(origin->GetNumberOfComponents(), 0);
  for (int tupleId = 0; tupleId < origin->GetNumberOfTuples(); tupleId++)
  {
    origin->GetTuple(tupleId, tuple.data());
    for (unsigned int targetId = 0; targetId < targets.size(); targetId++)
    {
      for (int component = 0; component < targets[targetId]->GetNumberOfComponents(); component++)
      {
        // Morphing:
        // P the resulting tuple, P0 the primitive's tuple, wi the weights, Ti the targets' tuples:
        // P = P0 + sum(wi * Ti)
        tuple[component] += weights[targetId] * targets[targetId]->GetTuple(tupleId)[component];
      }
    }
    result->SetTuple(tupleId, tuple.data());
  }
  return result;
}

//----------------------------------------------------------------------------
void SetupWeightedTransformFilterForGLTFSkinning(vtkSmartPointer<vtkWeightedTransformFilter> filter,
  const std::vector<vtkSmartPointer<vtkTransform> >& jointMats,
  const vtkSmartPointer<vtkPolyData> poly)
{
  filter->SetInputData(poly);

  // Add transforms to weightedTransformFilter (min. 4  transforms. If we have less than 4
  // transforms, complete with identity)
  size_t nbTransforms = vtkMath::Max<size_t>(jointMats.size(), 4);
  filter->SetNumberOfTransforms(static_cast<int>(nbTransforms));
  for (unsigned int i = 0; i < nbTransforms; i++)
  {
    if (i >= jointMats.size())
    {
      vtkNew<vtkTransform> transform;
      filter->SetTransform(transform, i);
    }
    else
    {
      filter->SetTransform(jointMats[i], i);
    }
  }

  // Add joint index and weight array information
  filter->SetTransformIndexArray("joints_0");
  filter->SetWeightArray("weights_0");
}

//----------------------------------------------------------------------------
void AddTransformToFieldData(const vtkSmartPointer<vtkTransform> transform,
  vtkSmartPointer<vtkFieldData> fieldData, const std::string& name)
{
  vtkSmartPointer<vtkDoubleArray> matrixArray;
  if (fieldData->HasArray(name.c_str()))
  {
    matrixArray = vtkDoubleArray::SafeDownCast(fieldData->GetArray(name.c_str()));
    matrixArray->Resize(0);
  }
  else
  {
    matrixArray = vtkSmartPointer<vtkDoubleArray>::New();
    matrixArray->SetName(name.c_str());
    fieldData->AddArray(matrixArray);
  }
  // Create array to store the matrix's values
  for (int i = 0; i < 16; i++)
  {
    matrixArray->InsertNextValue(transform->GetMatrix()->GetElement(i / 4, i % 4));
  }
}

//----------------------------------------------------------------------------
void AddJointMatricesToFieldData(const std::vector<vtkSmartPointer<vtkTransform> >& jointMats,
  vtkSmartPointer<vtkFieldData> fieldData)
{
  for (unsigned int matId = 0; matId < jointMats.size(); matId++)
  {
    AddTransformToFieldData(jointMats[matId], fieldData, "jointMatrix_" + value_to_string(matId));
  }
}

//----------------------------------------------------------------------------
void AddGlobalTransformToFieldData(
  const vtkSmartPointer<vtkTransform> globalTransform, vtkSmartPointer<vtkFieldData> fieldData)
{
  // Create array to store the matrix's values
  AddTransformToFieldData(globalTransform, fieldData, "globalTransform");
}

//----------------------------------------------------------------------------
void AddMorphingWeightsToFieldData(
  const std::vector<float>& weights, vtkSmartPointer<vtkFieldData> fieldData)
{
  vtkNew<vtkFloatArray> weightsArray;
  weightsArray->SetName("morphingWeights");
  weightsArray->SetNumberOfValues(static_cast<vtkIdType>(weights.size()));
  fieldData->AddArray(weightsArray);
  for (unsigned int weightId = 0; weightId < weights.size(); weightId++)
  {
    weightsArray->SetValue(weightId, weights[weightId]);
  }
}

//----------------------------------------------------------------------------
void AddInfoToFieldData(const std::vector<float>* morphingWeights,
  const std::vector<vtkSmartPointer<vtkTransform> >& jointMats,
  vtkSmartPointer<vtkTransform> globalTransform, vtkSmartPointer<vtkFieldData> fieldData)
{
  if (morphingWeights != nullptr && !morphingWeights->empty())
  {
    AddMorphingWeightsToFieldData(*morphingWeights, fieldData);
  }

  if (!jointMats.empty())
  {
    AddJointMatricesToFieldData(jointMats, fieldData);
  }

  AddGlobalTransformToFieldData(globalTransform, fieldData);
}

//----------------------------------------------------------------------------
void PrepareMorphingTargetArrays(std::vector<vtkSmartPointer<vtkFloatArray> >& positionArrays,
  std::vector<vtkSmartPointer<vtkFloatArray> >& normalArrays,
  std::vector<vtkSmartPointer<vtkFloatArray> >& tangentArrays,
  std::vector<vtkGLTFDocumentLoader::MorphTarget>& targets)
{
  for (auto& target : targets)
  {
    if (target.AttributeValues.count("POSITION"))
    {
      positionArrays.push_back(target.AttributeValues["POSITION"]);
    }
    if (target.AttributeValues.count("NORMAL"))
    {
      normalArrays.push_back(target.AttributeValues["NORMAL"]);
    }
    if (target.AttributeValues.count("TANGENT"))
    {
      tangentArrays.push_back(target.AttributeValues["TANGENT"]);
    }
  }
}

//----------------------------------------------------------------------------
void ApplyMorphingToPolyData(std::vector<vtkGLTFDocumentLoader::MorphTarget>& targets,
  std::vector<float>* morphingWeights, vtkSmartPointer<vtkPolyData> inputPolyData,
  vtkSmartPointer<vtkPolyData> outputPolyData)
{
  // Prepare target arrays
  std::vector<vtkSmartPointer<vtkFloatArray> > positionArrays;
  std::vector<vtkSmartPointer<vtkFloatArray> > normalArrays;
  std::vector<vtkSmartPointer<vtkFloatArray> > tangentArrays;
  PrepareMorphingTargetArrays(positionArrays, normalArrays, tangentArrays, targets);

  // Apply morphing with all targets
  auto points = ApplyMorphingToDataArray(
    inputPolyData->GetPoints()->GetData(), *morphingWeights, positionArrays);
  auto normals = ApplyMorphingToDataArray(
    inputPolyData->GetPointData()->GetNormals(), *morphingWeights, normalArrays);
  auto tangents = ApplyMorphingToDataArray(
    inputPolyData->GetPointData()->GetArray("tangents"), *morphingWeights, tangentArrays);

  // Add morphed arrays to current polydata
  if (points != nullptr)
  {
    outputPolyData->SetPoints(vtkSmartPointer<vtkPoints>::New());
    outputPolyData->GetPoints()->SetData(points);
  }
  if (normals != nullptr)
  {
    outputPolyData->GetPointData()->SetNormals(normals);
  }
  if (tangents != nullptr)
  {
    outputPolyData->GetPointData()->AddArray(tangents);
  }
}

//----------------------------------------------------------------------------
bool BuildMultiBlockDatasetFromMesh(vtkGLTFDocumentLoader::Model& m, unsigned int meshId,
  vtkSmartPointer<vtkMultiBlockDataSet> parentDataSet,
  vtkSmartPointer<vtkMultiBlockDataSet> meshDataSet, std::string& dataSetName,
  vtkSmartPointer<vtkTransform> globalTransform,
  const std::vector<vtkSmartPointer<vtkTransform> >& jointMats, bool applyDeformations,
  std::vector<float>* morphingWeights)
{
  if (meshId >= m.Meshes.size())
  {
    vtkErrorWithObjectMacro(nullptr, "Invalid mesh index " << meshId);
    return false;
  }
  bool createNewPolyData = false;
  // If meshDataSet is not defined, create it and append it to the parent dataset.
  if (!meshDataSet && !createNewPolyData)
  {
    createNewPolyData = true;
    meshDataSet = vtkSmartPointer<vtkMultiBlockDataSet>::New();
    parentDataSet->SetBlock(parentDataSet->GetNumberOfBlocks(), meshDataSet);
    parentDataSet->GetMetaData(parentDataSet->GetNumberOfBlocks() - 1)
      ->Set(vtkCompositeDataSet::NAME(), dataSetName);
  }

  vtkGLTFDocumentLoader::Mesh mesh = m.Meshes[meshId];

  int blockId = 0;
  for (auto& primitive : mesh.Primitives)
  {
    vtkSmartPointer<vtkPolyData> meshPolyData;

    // Even though no weights are defined in the node, meshes may contain default weights
    if ((morphingWeights == nullptr || morphingWeights->empty()) && !mesh.Weights.empty())
    {
      morphingWeights = &(mesh.Weights);
    }
    // Apply deformations (skins and morph targets to each primitive's geometry, then add the
    // resulting polydata to the parent dataSet)
    if (applyDeformations)
    {
      meshPolyData = vtkSmartPointer<vtkPolyData>::New();
      meshPolyData->ShallowCopy(primitive.Geometry);
      // Add material infomation to fieldData
      AddMaterialToFieldData(primitive.Material, meshPolyData->GetFieldData(), m);

      vtkNew<vtkTransformPolyDataFilter> filter;

      // Morphing
      if (morphingWeights != nullptr && !morphingWeights->empty())
      {
        // Number of weights should be equal to the number of targets
        if (morphingWeights->size() != primitive.Targets.size())
        {
          vtkErrorWithObjectMacro(nullptr, "Invalid number of morphing weights");
          return false;
        }
        ApplyMorphingToPolyData(
          primitive.Targets, morphingWeights, primitive.Geometry, meshPolyData);
      }

      // Skinning
      if (!jointMats.empty())
      {
        // Setup filter
        vtkNew<vtkWeightedTransformFilter> skinningFilter;
        SetupWeightedTransformFilterForGLTFSkinning(skinningFilter, jointMats, meshPolyData);
        // Connect to TransformPolyDataFilter
        filter->SetInputConnection(skinningFilter->GetOutputPort(0));
      }
      else
      {
        filter->SetInputData(meshPolyData);
      }

      // Node transform
      filter->SetTransform(globalTransform);
      if (createNewPolyData)
      {
        meshDataSet->SetBlock(meshDataSet->GetNumberOfBlocks(), (filter->GetOutputDataObject(0)));
      }
      else
      {
        filter->SetOutput(vtkPolyData::SafeDownCast(meshDataSet->GetBlock(blockId)));
      }
      filter->Update();
    }
    else
    {
      if (createNewPolyData)
      {
        meshPolyData = vtkSmartPointer<vtkPolyData>::New();
        meshPolyData->ShallowCopy(primitive.Geometry);
        // Add material infomation to fieldData
        AddMaterialToFieldData(primitive.Material, meshPolyData->GetFieldData(), m);
        meshDataSet->SetBlock(meshDataSet->GetNumberOfBlocks(), meshPolyData);
      }
      else
      {
        meshPolyData = vtkPolyData::SafeDownCast(meshDataSet->GetBlock(blockId));
      }
    }
    AddInfoToFieldData(morphingWeights, jointMats, globalTransform,
      vtkPolyData::SafeDownCast(meshDataSet->GetBlock(blockId))->GetFieldData());
  }
  return true;
}

//----------------------------------------------------------------------------
void ComputeJointMatrices(const vtkGLTFDocumentLoader::Model& m,
  const vtkGLTFDocumentLoader::Skin& skin, vtkGLTFDocumentLoader::Node& node,
  std::vector<vtkSmartPointer<vtkTransform> >& jointMats)
{
  jointMats.clear();
  jointMats.reserve(skin.Joints.size());

  for (unsigned int jointId = 0; jointId < skin.Joints.size(); jointId++)
  {
    const vtkGLTFDocumentLoader::Node& jointNode = m.Nodes[skin.Joints[jointId]];

    vtkNew<vtkTransform> jointGlobalTransform;
    vtkNew<vtkTransform> inverseMeshGlobalTransform;
    vtkNew<vtkTransform> inverseBindTransform;
    vtkNew<vtkTransform> jointTransform;

    /**
     * Joint matrices:
     * jointMatrix(j) =
     * globalTransformOfNodeThatTheMeshIsAttachedTo^-1 *
     * globalTransformOfJointNode(j) *
     * inverseBindMatrixForJoint(j);
     * The mesh will be transformed (using vtkWeightedTransformFilter) using this matrix:
     * mat4 skinMat =
     * weight.x * jointMatrix[joint.x] +
     * weight.y * jointMatrix[joint.y] +
     * weight.z * jointMatrix[joint.z] +
     * weight.w * jointMatrix[joint.w];
     */
    inverseMeshGlobalTransform->SetInput(node.GlobalTransform);
    inverseMeshGlobalTransform->Inverse();
    inverseBindTransform->SetMatrix(skin.InverseBindMatrices[jointId]);
    jointGlobalTransform->SetInput(jointNode.GlobalTransform);
    jointTransform->PostMultiply();
    jointTransform->Concatenate(inverseBindTransform);
    jointTransform->Concatenate(jointGlobalTransform);
    jointTransform->Concatenate(inverseMeshGlobalTransform);

    jointMats.push_back(jointTransform);
  }
}

//----------------------------------------------------------------------------
bool BuildMultiBlockDataSetFromNode(vtkGLTFDocumentLoader::Model& m, unsigned int nodeId,
  vtkSmartPointer<vtkMultiBlockDataSet> parentDataSet,
  vtkSmartPointer<vtkMultiBlockDataSet> nodeDataset, std::string nodeName, bool applyDeformations)
{
  if (nodeId >= m.Nodes.size())
  {
    vtkErrorWithObjectMacro(nullptr, "Invalid node index " << nodeId);
    return false;
  }
  bool createNewBlocks = false;
  // If nodeDataset is not defined, create it and append it to the parent dataset
  if (!nodeDataset)
  {
    createNewBlocks = true;
    nodeDataset = vtkSmartPointer<vtkMultiBlockDataSet>::New();
    parentDataSet->SetBlock(parentDataSet->GetNumberOfBlocks(), nodeDataset);
    parentDataSet->GetMetaData(parentDataSet->GetNumberOfBlocks() - 1)
      ->Set(vtkCompositeDataSet::NAME(), nodeName);
  }

  vtkGLTFDocumentLoader::Node node = m.Nodes[nodeId];
  int blockId = 0;
  if (node.Mesh >= 0)
  {
    std::vector<vtkSmartPointer<vtkTransform> > jointMats;

    if (node.Skin >= 0)
    {
      // Compute skinning matrices
      const vtkGLTFDocumentLoader::Skin& skin = m.Skins[node.Skin];
      ComputeJointMatrices(m, skin, node, jointMats);
    }

    std::vector<float>* morphingWeights = nullptr;
    if (!node.Weights.empty())
    {
      morphingWeights = &(node.Weights);
    }
    else if (!node.InitialWeights.empty())
    {
      morphingWeights = &(node.InitialWeights);
    }

    vtkSmartPointer<vtkMultiBlockDataSet> meshDataSet = nullptr;
    if (!createNewBlocks)
    {
      meshDataSet = vtkMultiBlockDataSet::SafeDownCast(nodeDataset->GetBlock(blockId));
    }
    std::string meshDatasetName = "Mesh_" + value_to_string(node.Mesh);
    if (!BuildMultiBlockDatasetFromMesh(m, node.Mesh, nodeDataset, meshDataSet, meshDatasetName,
          node.GlobalTransform, jointMats, applyDeformations, morphingWeights))
    {
      vtkErrorWithObjectMacro(
        nullptr, "Could not build vtkMultiBlockDataSet from mesh " << node.Mesh);
      return false;
    }
    blockId++;
  }
  for (int child : node.Children)
  {
    // look for existing dataset for this node
    vtkSmartPointer<vtkMultiBlockDataSet> childDataset;
    std::string childDatasetName = "Node_" + value_to_string(child);
    if (!createNewBlocks)
    {
      // find existing child dataset for this node
      childDataset = vtkMultiBlockDataSet::SafeDownCast(nodeDataset->GetBlock(blockId));
    }
    if (!BuildMultiBlockDataSetFromNode(
          m, child, nodeDataset, childDataset, childDatasetName, applyDeformations))
    {
      vtkErrorWithObjectMacro(nullptr, "Could not build vtkMultiBlockDataSet from node " << child);

      return false;
    }
    blockId++;
  }
  return true;
}

//----------------------------------------------------------------------------
bool BuildMultiBlockDataSetFromScene(vtkGLTFDocumentLoader::Model& m, vtkIdType sceneId,
  vtkSmartPointer<vtkMultiBlockDataSet> dataSet, bool applyDeformations)
{
  if (sceneId < 0 || sceneId >= static_cast<vtkIdType>(m.Scenes.size()))
  {
    vtkErrorWithObjectMacro(nullptr, "Invalid scene index " << sceneId);
    return false;
  }

  vtkGLTFDocumentLoader::Scene scene = m.Scenes[sceneId];

  bool createNewBlocks = (dataSet->GetNumberOfBlocks() == 0);

  int blockId = 0;
  for (int node : scene.Nodes)
  {
    std::string nodeDatasetName = "Node_" + value_to_string(node);
    vtkSmartPointer<vtkMultiBlockDataSet> nodeDataset = nullptr;
    if (!createNewBlocks)
    {
      // find existing child dataset for this node
      nodeDataset = vtkMultiBlockDataSet::SafeDownCast(dataSet->GetBlock(blockId));
    }
    if (!BuildMultiBlockDataSetFromNode(
          m, node, dataSet, nodeDataset, nodeDatasetName, applyDeformations))
    {
      vtkErrorWithObjectMacro(nullptr, "Could not build vtkMultiBlockDataSet from node " << node);
      return false;
    }
    blockId++;
  }
  return true;
}
}

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkGLTFReader);

//----------------------------------------------------------------------------
vtkGLTFReader::vtkGLTFReader()
{
  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
vtkGLTFReader::~vtkGLTFReader()
{
  this->SetFileName(nullptr);
}

//----------------------------------------------------------------------------
void vtkGLTFReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << "\n";
  os << indent << "IsModelLoaded: " << (this->IsModelLoaded ? "On" : "Off") << "\n";
  os << indent << "IsMetaDataLoaded: " << (this->IsMetaDataLoaded ? "On" : "Off") << "\n";
  os << indent
     << "ApplyDeformationsToGeometry: " << (this->ApplyDeformationsToGeometry ? "On" : "Off")
     << "\n";
}

//----------------------------------------------------------------------------
void vtkGLTFReader::StoreTextureData()
{
  if (!this->Textures.empty())
  {
    this->Textures.clear();
  }
  if (this->Loader == nullptr || this->Loader->GetInternalModel()->Textures.empty())
  {
    return;
  }
  auto model = this->Loader->GetInternalModel();
  int nbTextures = static_cast<int>(model->Textures.size());
  int nbSamplers = static_cast<int>(model->Samplers.size());

  this->Textures.reserve(this->Loader->GetInternalModel()->Textures.size());
  for (auto loaderTexture : this->Loader->GetInternalModel()->Textures)
  {
    vtkGLTFReader::GLTFTexture readerTexture;
    if (loaderTexture.Source >= 0 && loaderTexture.Source < nbTextures)
    {
      readerTexture.Image = model->Images[loaderTexture.Source].ImageData;
    }
    else
    {
      vtkWarningMacro("Image index is out of range");
      continue;
    }
    if (loaderTexture.Sampler >= 0 && loaderTexture.Sampler < nbSamplers)
    {
      auto sampler = model->Samplers[loaderTexture.Sampler];
      readerTexture.MinFilterValue = sampler.MinFilter;
      readerTexture.MaxFilterValue = sampler.MagFilter;
      readerTexture.WrapSValue = sampler.WrapS;
      readerTexture.WrapTValue = sampler.WrapT;
    }
    this->Textures.emplace_back(std::move(readerTexture));
  }
}

//----------------------------------------------------------------------------
int vtkGLTFReader::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (!this->Superclass::RequestInformation(request, inputVector, outputVector))
  {
    return 0;
  }

  // Read file metadata
  // Make sure we have a file to read.
  if (!this->FileName)
  {
    vtkErrorMacro("A FileName must be specified.");
    return 0;
  }

  // Check for filename change in case the loader was already created
  if (this->Loader != nullptr && this->Loader->GetInternalModel()->FileName != this->FileName)
  {
    this->IsMetaDataLoaded = false;
    this->IsModelLoaded = false;
    this->Textures.clear();
  }

  // Load model metadata if not done previously
  if (!this->IsMetaDataLoaded)
  {
    this->Loader = vtkSmartPointer<vtkGLTFDocumentLoader>::New();
    if (!this->Loader->LoadModelMetaDataFromFile(this->FileName))
    {
      vtkErrorMacro("Error loading model metadata from file " << this->FileName);
      return 0;
    }
    this->CreateAnimationSelection();
    this->CreateSceneNamesArray();
    this->SetCurrentScene(this->Loader->GetInternalModel()->DefaultScene);
    this->IsMetaDataLoaded = true;
  }

  // Get model information (numbers and names of animations and scenes, time range of animations)
  // Add this info to the output vtkInformation
  auto model = this->Loader->GetInternalModel();
  vtkInformation* info = outputVector->GetInformationObject(0);

  // Find maximum animation duration (for TIME_RANGE())
  double maxDuration = 0.0;
  if (this->AnimationSelection != nullptr)
  {
    for (vtkIdType i = 0; i < this->AnimationSelection->GetNumberOfArrays(); i++)
    {
      // Only use enabled animations to track maximum duration values
      if (this->AnimationSelection->ArrayIsEnabled(this->AnimationSelection->GetArrayName(i)))
      {
        float duration = model->Animations[i].Duration;
        if (maxDuration < duration)
        {
          maxDuration = duration;
        }
      }
    }
  }

  // Append TIME_STEPS
  if (this->GetFrameRate() > 0 && maxDuration > 0.0)
  {
    int maxFrameIndex = vtkMath::Floor(this->GetFrameRate() * maxDuration);
    if (info->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
    {
      info->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    }
    double period = 1.0 / this->GetFrameRate();
    // Append sampled time steps
    for (int i = 0; i <= maxFrameIndex; i++)
    {
      info->Append(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), i * period);
    }
    // Append the last step of the animation, if it doesn't match with the last sampled step
    if (maxDuration != maxFrameIndex * period)
    {
      info->Append(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), maxDuration);
    }

    // Add TIME_RANGE()
    std::array<double, 2> timeRange = { { 0.0, maxDuration } };
    info->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange.data(), 2);
  }
  else if (info->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    info->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  }

  this->NumberOfAnimations = static_cast<vtkIdType>(model->Animations.size());
  this->NumberOfScenes = static_cast<vtkIdType>(model->Scenes.size());
  return 1;
}

//----------------------------------------------------------------------------
int vtkGLTFReader::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  // Get the output
  vtkMultiBlockDataSet* output = vtkMultiBlockDataSet::GetData(outputVector);

  auto model = this->Loader->GetInternalModel();
  if (!this->IsModelLoaded)
  {
    // Make sure we have a file to read.
    if (!this->FileName)
    {
      vtkErrorMacro("A FileName must be specified.");
      return 0;
    }
    // Attempt to load binary buffer in case the file is binary-glTF
    // Check extension
    std::string extension = vtksys::SystemTools::GetFilenameLastExtension(this->FileName);
    std::vector<char> glbBuffer;
    if (extension == ".glb")
    {
      if (!this->Loader->LoadFileBuffer(this->FileName, glbBuffer))
      {
        vtkErrorMacro("Error loading binary data");
        return 0;
      }
    }

    // Load buffer data
    if (!this->Loader->LoadModelData(glbBuffer))
    {
      vtkErrorMacro("Error loading model data");
      return 0;
    }
    // Build polydata and transforms
    if (!this->Loader->BuildModelVTKGeometry())
    {
      vtkErrorMacro("Error building model vtk data");
      return 0;
    }
    this->StoreTextureData();
    this->IsModelLoaded = true;
  }

  if (this->OutputDataSet == nullptr)
  {
    this->OutputDataSet = vtkSmartPointer<vtkMultiBlockDataSet>::New();
  }

  // Apply selected animations on specified time step to the model's transforms
  vtkInformation* info = outputVector->GetInformationObject(0);

  if (this->FrameRate > 0)
  {
    double time = info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    for (vtkIdType i = 0; i < this->NumberOfAnimations; i++)
    {
      if (this->AnimationSelection->GetArraySetting(i))
      {
        this->Loader->ApplyAnimation(time, i);
      }
      else if (this->PreviousAnimationSelection->GetArraySetting(i))
      {
        // Reset transforms and weights
        this->Loader->ResetAnimation(i);
      }
    }
  }

  vtkIdType selectedScene = this->CurrentScene;
  if (selectedScene < 0 || selectedScene >= static_cast<vtkIdType>(model->Scenes.size()))
  {
    selectedScene = model->DefaultScene;
  }

  if (!BuildMultiBlockDataSetFromScene(
        *(model), selectedScene, this->OutputDataSet, this->ApplyDeformationsToGeometry))
  {
    vtkErrorMacro("Error building MultiBlockDataSet object");
    return 0;
  }

  // Save current animations
  this->PreviousAnimationSelection->CopySelections(this->AnimationSelection);

  output->ShallowCopy(this->OutputDataSet);
  return 1;
}

//----------------------------------------------------------------------------
void vtkGLTFReader::EnableAnimation(vtkIdType animationIndex)
{
  if (this->AnimationSelection == nullptr)
  {
    vtkErrorMacro("Error accessing animations: model is not loaded yet");
    return;
  }
  if (animationIndex < 0 || animationIndex >= this->AnimationSelection->GetNumberOfArrays())
  {
    vtkErrorMacro("Out of range animation index");
    return;
  }
  auto name = this->AnimationSelection->GetArrayName(animationIndex);
  this->AnimationSelection->EnableArray(name);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkGLTFReader::DisableAnimation(vtkIdType animationIndex)
{
  if (this->AnimationSelection == nullptr)
  {
    vtkErrorMacro("Error accessing animations: model is not loaded yet");
    return;
  }
  if (animationIndex < 0 || animationIndex >= this->AnimationSelection->GetNumberOfArrays())
  {
    vtkErrorMacro("Out of range animation index");
    return;
  }
  auto name = this->AnimationSelection->GetArrayName(animationIndex);
  this->AnimationSelection->DisableArray(name);
  this->Modified();
}

//----------------------------------------------------------------------------
bool vtkGLTFReader::IsAnimationEnabled(vtkIdType animationIndex)
{
  if (this->AnimationSelection == nullptr)
  {
    vtkErrorMacro("Error accessing animations: model is not loaded yet");
    return false;
  }
  if (animationIndex < 0 || animationIndex >= this->AnimationSelection->GetNumberOfArrays())
  {
    vtkErrorMacro("Out of range animation index");
    return false;
  }
  auto name = this->AnimationSelection->GetArrayName(animationIndex);
  return this->AnimationSelection->ArrayIsEnabled(name) != 0;
}

//----------------------------------------------------------------------------
std::string vtkGLTFReader::GetAnimationName(vtkIdType animationIndex)
{
  if (this->Loader == nullptr || this->Loader->GetInternalModel() == nullptr)
  {
    vtkErrorMacro("Error while accessing animations: model is not loaded");
    return "";
  }
  if (animationIndex < 0 ||
    animationIndex >= static_cast<vtkIdType>(this->Loader->GetInternalModel()->Animations.size()))
  {
    vtkErrorMacro("Out of range animation index");
    return "";
  }
  return this->Loader->GetInternalModel()->Animations[animationIndex].Name;
}

//----------------------------------------------------------------------------
float vtkGLTFReader::GetAnimationDuration(vtkIdType animationIndex)
{
  if (this->Loader == nullptr || this->Loader->GetInternalModel() == nullptr)
  {
    vtkErrorMacro("Error while accessing animations: model is not loaded");
    return 0.0;
  }
  if (animationIndex < 0 ||
    animationIndex >= static_cast<vtkIdType>(this->Loader->GetInternalModel()->Animations.size()))
  {
    vtkErrorMacro("Out of range animation index");
    return 0.0;
  }
  return this->Loader->GetInternalModel()->Animations[animationIndex].Duration;
}

//----------------------------------------------------------------------------
std::string vtkGLTFReader::GetSceneName(vtkIdType sceneIndex)
{
  if (this->Loader == nullptr || this->Loader->GetInternalModel() == nullptr)
  {
    vtkErrorMacro("Error while accessing scenes: model is not loaded");
    return "";
  }
  if (sceneIndex < 0 ||
    sceneIndex >= static_cast<vtkIdType>(this->Loader->GetInternalModel()->Scenes.size()))
  {
    vtkErrorMacro("Out of range scene index");
    return "";
  }
  return this->Loader->GetInternalModel()->Scenes[sceneIndex].Name;
}

//----------------------------------------------------------------------------
vtkIdType vtkGLTFReader::GetNumberOfTextures()
{
  return static_cast<vtkIdType>(this->Textures.size());
}

//----------------------------------------------------------------------------
vtkGLTFReader::GLTFTexture vtkGLTFReader::GetGLTFTexture(vtkIdType textureIndex)
{
  if (textureIndex < 0 || textureIndex >= static_cast<vtkIdType>(this->Textures.size()))
  {
    vtkErrorMacro("Out of range texture index");
    return vtkGLTFReader::GLTFTexture{ nullptr, 0, 0, 0, 0 };
  }
  return this->Textures[textureIndex];
}

//----------------------------------------------------------------------------
void vtkGLTFReader::SetScene(const std::string& scene)
{
  if (this->SceneNames == nullptr)
  {
    this->CurrentScene = 0;
    return;
  }
  for (vtkIdType i = 0; i < this->SceneNames->GetNumberOfValues(); i++)
  {
    if (scene == this->SceneNames->GetValue(i))
    {
      this->SetCurrentScene(i);
      this->OutputDataSet = nullptr;
      return;
    }
  }
  vtkWarningMacro("Scene '" << scene << "' does not exist.");
}

//----------------------------------------------------------------------------
void vtkGLTFReader::CreateSceneNamesArray()
{
  if (this->Loader == nullptr || this->Loader->GetInternalModel() == nullptr)
  {
    vtkErrorMacro("Error while accessing scenes: model is not loaded");
    return;
  }
  this->SceneNames = vtkSmartPointer<vtkStringArray>::New();
  this->SceneNames->SetNumberOfComponents(1);

  std::map<std::string, unsigned int> duplicateNameCounters;

  for (const auto& scene : this->Loader->GetInternalModel()->Scenes)
  {
    this->SceneNames->InsertNextValue(MakeUniqueNonEmptyName(scene.Name, duplicateNameCounters));
  }
}

//----------------------------------------------------------------------------
vtkStringArray* vtkGLTFReader::GetAllSceneNames()
{
  if (this->Loader == nullptr || this->Loader->GetInternalModel() == nullptr)
  {
    vtkErrorMacro("Error while accessing scenes: model is not loaded");
    return nullptr;
  }
  return this->SceneNames;
}

//----------------------------------------------------------------------------
vtkDataArraySelection* vtkGLTFReader::GetAnimationSelection()
{
  return this->AnimationSelection;
}

//----------------------------------------------------------------------------
void vtkGLTFReader::CreateAnimationSelection()
{
  if (this->Loader == nullptr || this->Loader->GetInternalModel() == nullptr)
  {
    vtkErrorMacro("Error while accessing animations: model is not loaded");
    return;
  }
  this->AnimationSelection = vtkSmartPointer<vtkDataArraySelection>::New();
  std::map<std::string, unsigned int> duplicateNameCounters;
  for (const auto& animation : this->Loader->GetInternalModel()->Animations)
  {
    this->AnimationSelection->AddArray(
      MakeUniqueNonEmptyName(animation.Name, duplicateNameCounters).c_str(), false);
  }
  this->PreviousAnimationSelection = vtkSmartPointer<vtkDataArraySelection>::New();
  this->PreviousAnimationSelection->CopySelections(this->AnimationSelection);
  this->AnimationSelection->AddObserver(vtkCommand::ModifiedEvent, this, &vtkGLTFReader::Modified);
}

//---------------------------------------------------------------------------
void vtkGLTFReader::SetApplyDeformationsToGeometry(bool flag)
{
  if (this->ApplyDeformationsToGeometry != flag)
  {
    this->OutputDataSet = nullptr;
    this->Modified();
  }
  this->ApplyDeformationsToGeometry = flag;
}
