/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSPRayPolyDataMapperNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOSPRayPolyDataMapperNode.h"

#include "vtkActor.h"
#include "vtkDataArray.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkImageExtractComponents.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkMapper.h"
#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include "vtkOSPRayActorNode.h"
#include "vtkOSPRayMaterialHelpers.h"
#include "vtkOSPRayRendererNode.h"
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

#include "RTWrapper/RTWrapper.h"

#include <map>

//============================================================================

namespace vtkosp
{
void VToOPointNormals(vtkDataArray* vNormals, osp::vec3f*& normals, vtkMatrix3x3* matrix)
{
  int numNormals = vNormals->GetNumberOfTuples();
  normals = new osp::vec3f[numNormals];
  for (int i = 0; i < numNormals; i++)
  {
    double vNormal[3];
    double* vtmp = vNormals->GetTuple(i);
    matrix->MultiplyPoint(vtmp, vNormal);
    vtkMath::Normalize(vNormal);
    normals[i] = osp::vec3f{ static_cast<float>(vNormal[0]), static_cast<float>(vNormal[1]),
      static_cast<float>(vNormal[2]) };
  }
}

//------------------------------------------------------------------------------
void CellMaterials(vtkOSPRayRendererNode* orn, OSPRenderer oRenderer, vtkPolyData* poly,
  vtkMapper* mapper, vtkScalarsToColors* s2c, std::map<std::string, OSPMaterial> mats,
  std::vector<OSPMaterial>& ospMaterials, vtkUnsignedCharArray* vColors, float* specColor,
  float specPower, float opacity)
{
  RTW::Backend* backend = orn->GetBackend();
  if (backend == nullptr)
    return;
  vtkAbstractArray* scalars = nullptr;
  bool try_mats = s2c->GetIndexedLookup() && s2c->GetNumberOfAnnotatedValues() && mats.size() > 0;
  if (try_mats)
  {
    int cflag2 = -1;
    scalars = mapper->GetAbstractScalars(poly, mapper->GetScalarMode(),
      mapper->GetArrayAccessMode(), mapper->GetArrayId(), mapper->GetArrayName(), cflag2);
  }
  int numColors = vColors->GetNumberOfTuples();
  int width = vColors->GetNumberOfComponents();
  for (int i = 0; i < numColors; i++)
  {
    bool found = false;
    if (scalars)
    {
      vtkVariant v = scalars->GetVariantValue(i);
      vtkIdType idx = s2c->GetAnnotatedValueIndex(v);
      if (idx > -1)
      {
        std::string name(s2c->GetAnnotation(idx));
        if (mats.find(name) != mats.end())
        {
          OSPMaterial oMaterial = mats[name];
          ospCommit(oMaterial);
          ospMaterials.push_back(oMaterial);
          found = true;
        }
      }
    }
    if (!found)
    {
      double* color = vColors->GetTuple(i);
      OSPMaterial oMaterial;
      oMaterial = vtkOSPRayMaterialHelpers::NewMaterial(orn, oRenderer, "OBJMaterial");
      float diffusef[] = { static_cast<float>(color[0]) / (255.0f),
        static_cast<float>(color[1]) / (255.0f), static_cast<float>(color[2]) / (255.0f) };
      float localOpacity = 1.f;
      if (width >= 4)
      {
        localOpacity = static_cast<float>(color[3]) / (255.0f);
      }
      ospSet3fv(oMaterial, "Kd", diffusef);
      float specAdjust = 2.0f / (2.0f + specPower);
      float specularf[] = { specColor[0] * specAdjust, specColor[1] * specAdjust,
        specColor[2] * specAdjust };
      ospSet3fv(oMaterial, "Ks", specularf);
      ospSet1f(oMaterial, "Ns", specPower);
      ospSet1f(oMaterial, "d", opacity * localOpacity);
      ospCommit(oMaterial);
      ospMaterials.push_back(oMaterial);
    }
  }
}

//----------------------------------------------------------------------------
float MapThroughPWF(double in, vtkPiecewiseFunction* scaleFunction)
{
  double out = in;
  if (!scaleFunction)
  {
    out = in;
  }
  else
  {
    out = scaleFunction->GetValue(in);
  }
  return static_cast<float>(out);
}

//----------------------------------------------------------------------------
OSPGeometry RenderAsSpheres(osp::vec3f* vertices, std::vector<unsigned int>& indexArray,
  std::vector<unsigned int>& rIndexArray, double pointSize, vtkDataArray* scaleArray,
  vtkPiecewiseFunction* scaleFunction, bool useCustomMaterial, OSPMaterial actorMaterial,
  vtkImageData* vColorTextureMap, int numTextureCoordinates, float* textureCoordinates,
  int numCellMaterials, OSPData CellMaterials, int numPointColors, osp::vec4f* PointColors,
  int numPointValueTextureCoords, float* pointValueTextureCoords, RTW::Backend* backend)
{
  if (backend == nullptr)
    return OSPGeometry();
  OSPGeometry ospMesh = ospNewGeometry("spheres");
  int width = 4;
  int scaleOffset = -1;
  if (scaleArray != nullptr)
  {
    width = 5;
    scaleOffset = 4 * sizeof(float);
  }
  float* mdata = new float[width * indexArray.size()];
  int* idata = (int*)mdata;
  for (size_t i = 0; i < indexArray.size(); i++)
  {
    mdata[i * width + 0] = static_cast<float>(vertices[indexArray[i]].x);
    mdata[i * width + 1] = static_cast<float>(vertices[indexArray[i]].y);
    mdata[i * width + 2] = static_cast<float>(vertices[indexArray[i]].z);
    int mat = 0;
    if (numCellMaterials)
    {
      mat = rIndexArray[i];
    }
    else if (numPointColors)
    {
      mat = indexArray[i];
    }
    idata[i * width + 3] = mat;
    if (scaleArray != nullptr)
    {
      mdata[i * width + 4] = MapThroughPWF(*scaleArray->GetTuple(indexArray[i]), scaleFunction);
    }
  }
  OSPData _PointColors = nullptr;
  if (numPointColors)
  {
    _PointColors = ospNewData(numPointColors, OSP_FLOAT4, &PointColors[0]);
  }
  OSPData _mdata = ospNewData(indexArray.size() * width, OSP_FLOAT, mdata);
  ospSetObject(ospMesh, "spheres", _mdata);
  ospSet1i(ospMesh, "bytes_per_sphere", width * sizeof(float));
  ospSet1i(ospMesh, "offset_center", 0 * sizeof(float));
  ospSet1f(ospMesh, "radius", pointSize);
  ospSet1i(ospMesh, "offset_radius", scaleOffset);

  // send the texture map and texture coordinates over
  bool _hastm = false;
  osp::vec2f* tc = nullptr;
  if (numTextureCoordinates || numPointValueTextureCoords)
  {
    _hastm = true;

    if (numPointValueTextureCoords)
    {
      // using 1D texture for point value LUT
      tc = new osp::vec2f[indexArray.size()];
      for (size_t i = 0; i < indexArray.size(); i++)
      {
        float t1;
        int index1 = indexArray[i];
        t1 = pointValueTextureCoords[index1 + 0];
        tc[i] = osp::vec2f{ t1, 0 };
      }
      OSPData tcs = ospNewData(indexArray.size(), OSP_FLOAT2, &tc[0]);
      ospSetData(ospMesh, "texcoord", tcs);
    }
    else if (numTextureCoordinates)
    {
      // 2d texture mapping
      float* itc = textureCoordinates;
      tc = new osp::vec2f[indexArray.size()];
      for (size_t i = 0; i < indexArray.size(); i++)
      {
        float t1, t2;
        int index1 = indexArray[i];
        t1 = itc[index1 * 2 + 0];
        t2 = itc[index1 * 2 + 1];
        tc[i] = osp::vec2f{ t1, t2 };
      }
      OSPData tcs = ospNewData(indexArray.size(), OSP_FLOAT2, &tc[0]);
      ospSetData(ospMesh, "texcoord", tcs);
    }
  }
  delete[] tc;

  if (useCustomMaterial)
  {
    ospSetMaterial(ospMesh, actorMaterial);
  }
  else if (vColorTextureMap && _hastm)
  {
    OSPTexture t2d = vtkOSPRayMaterialHelpers::VTKToOSPTexture(backend, vColorTextureMap);
    ospSetObject(actorMaterial, "map_Kd", ((OSPTexture)(t2d)));
    ospRelease(t2d);
    ospCommit(actorMaterial);
    ospSetMaterial(ospMesh, actorMaterial);
  }
  else if (numCellMaterials)
  {
    // per cell color
    ospSet1i(ospMesh, "offset_materialID", 3 * sizeof(float));
    ospSetData(ospMesh, "materialList", CellMaterials);
  }
  else if (numPointColors)
  {
    // per point color
    ospSet1i(ospMesh, "offset_colorID", 3 * sizeof(float));
    ospSetData(ospMesh, "color", _PointColors);
  }
  else
  {
    // per actor color
    ospSetMaterial(ospMesh, actorMaterial);
  }

  ospCommit(ospMesh);
  ospRelease(_PointColors);
  ospRelease(_mdata);
  delete[] mdata;

  return ospMesh;
}

//----------------------------------------------------------------------------
OSPGeometry RenderAsCylinders(osp::vec3f* vertices, std::vector<unsigned int>& indexArray,
  std::vector<unsigned int>& rIndexArray, double lineWidth, vtkDataArray* scaleArray,
  vtkPiecewiseFunction* scaleFunction, bool useCustomMaterial, OSPMaterial actorMaterial,
  vtkImageData* vColorTextureMap, int numTextureCoordinates, float* textureCoordinates,
  int numCellMaterials, OSPData CellMaterials, int numPointColors, osp::vec4f* PointColors,
  int numPointValueTextureCoords, float* pointValueTextureCoords, RTW::Backend* backend)
{
  if (backend == nullptr)
    return OSPGeometry();
  OSPGeometry ospMesh = ospNewGeometry("cylinders");
  int width = 7;
  int scaleOffset = -1;
  if (scaleArray != nullptr)
  {
    width = 8;
    scaleOffset = 7 * sizeof(float);
  }
  float* mdata = new float[indexArray.size() / 2 * width];
  int* idata = (int*)mdata;
  for (size_t i = 0; i < indexArray.size() / 2; i++)
  {
    mdata[i * width + 0] = static_cast<float>(vertices[indexArray[i * 2 + 0]].x);
    mdata[i * width + 1] = static_cast<float>(vertices[indexArray[i * 2 + 0]].y);
    mdata[i * width + 2] = static_cast<float>(vertices[indexArray[i * 2 + 0]].z);
    mdata[i * width + 3] = static_cast<float>(vertices[indexArray[i * 2 + 1]].x);
    mdata[i * width + 4] = static_cast<float>(vertices[indexArray[i * 2 + 1]].y);
    mdata[i * width + 5] = static_cast<float>(vertices[indexArray[i * 2 + 1]].z);
    int mat = 0;
    if (numCellMaterials)
    {
      mat = rIndexArray[i * 2];
    }
    else if (numPointColors)
    {
      mat = indexArray[i * 2];
    }
    idata[i * width + 6] = mat;
    if (scaleArray != nullptr)
    {
      double avg = (*scaleArray->GetTuple(indexArray[i * 2 + 0]) +
                     *scaleArray->GetTuple(indexArray[i * 2 + 1])) *
        0.5;
      mdata[i * width + 7] = MapThroughPWF(avg, scaleFunction);
    }
  }
  OSPData _PointColors = nullptr;
  if (numPointColors)
  {
    _PointColors = ospNewData(numPointColors, OSP_FLOAT4, &PointColors[0]);
  }
  OSPData _mdata = ospNewData(indexArray.size() / 2 * width, OSP_FLOAT, mdata);
  ospSetData(ospMesh, "cylinders", _mdata);
  ospSet1i(ospMesh, "bytes_per_cylinder", width * sizeof(float));
  ospSet1i(ospMesh, "offset_v0", 0);
  ospSet1i(ospMesh, "offset_v1", 3 * sizeof(float));
  ospSet1f(ospMesh, "radius", lineWidth);
  ospSet1i(ospMesh, "offset_radius", scaleOffset);

  // send the texture map and texture coordinates over
  bool _hastm = false;
  osp::vec2f* tc = nullptr;
  if (numTextureCoordinates || numPointValueTextureCoords)
  {
    _hastm = true;

    if (numPointValueTextureCoords)
    {
      // using 1D texture for point value LUT
      tc = new osp::vec2f[indexArray.size()];
      for (size_t i = 0; i < indexArray.size(); i += 2)
      {
        float t1, t2;
        int index1 = indexArray[i + 0];
        t1 = pointValueTextureCoords[index1 + 0];
        tc[i] = osp::vec2f{ t1, 0 };
        int index2 = indexArray[i + 1];
        t2 = pointValueTextureCoords[index2 + 0];
        tc[i + 1] = osp::vec2f{ t2, 0 };
      }
      OSPData tcs = ospNewData(indexArray.size(), OSP_FLOAT2, &tc[0]);
      ospSetData(ospMesh, "texcoord", tcs);
      ospSetData(ospMesh, "vertex.texcoord", tcs);
    }
    else if (numTextureCoordinates)
    {
      // 2d texture mapping
      float* itc = textureCoordinates;
      tc = new osp::vec2f[indexArray.size()];
      for (size_t i = 0; i < indexArray.size(); i += 2)
      {
        float t1, t2;
        int index1 = indexArray[i + 0];
        t1 = itc[index1 * 2 + 0];
        t2 = itc[index1 * 2 + 1];
        tc[i] = osp::vec2f{ t1, t2 };
        int index2 = indexArray[i + 1];
        t1 = itc[index2 * 2 + 0];
        t2 = itc[index2 * 2 + 1];
        tc[i + 1] = osp::vec2f{ t1, t2 };
      }
      OSPData tcs = ospNewData(indexArray.size(), OSP_FLOAT2, &tc[0]);
      ospSetData(ospMesh, "texcoord", tcs);
    }
  }
  delete[] tc;

  if (useCustomMaterial)
  {
    ospSetMaterial(ospMesh, actorMaterial);
  }
  else if (vColorTextureMap && _hastm)
  {
    OSPTexture t2d = vtkOSPRayMaterialHelpers::VTKToOSPTexture(backend, vColorTextureMap);
    ospSetObject(actorMaterial, "map_Kd", ((OSPTexture)(t2d)));
    ospRelease(t2d);
    ospCommit(actorMaterial);
    ospSetMaterial(ospMesh, actorMaterial);
  }
  else if (numCellMaterials)
  {
    // per cell color
    ospSet1i(ospMesh, "offset_materialID", 6 * sizeof(float));
    ospSetData(ospMesh, "materialList", CellMaterials);
  }
  else if (numPointColors)
  {
    // per point color
    ospSet1i(ospMesh, "offset_colorID", 6 * sizeof(float));
    ospSetData(ospMesh, "color", _PointColors);
  }
  else
  {
    // per actor color
    ospSetMaterial(ospMesh, actorMaterial);
  }

  ospCommit(ospMesh);
  ospRelease(_PointColors);
  ospRelease(_mdata);
  delete[] mdata;

  return ospMesh;
}

//----------------------------------------------------------------------------
OSPGeometry RenderAsTriangles(OSPData vertices, std::vector<unsigned int>& indexArray,
  std::vector<unsigned int>& rIndexArray, bool useCustomMaterial, OSPMaterial actorMaterial,
  int numNormals, osp::vec3f* normals, int interpolationType, vtkImageData* vColorTextureMap,
  vtkImageData* vNormalTextureMap, vtkImageData* vMaterialTextureMap, int numTextureCoordinates,
  float* textureCoordinates, const osp::vec4f& textureTransform, int numCellMaterials,
  OSPData CellMaterials, int numPointColors, osp::vec4f* PointColors,
  int numPointValueTextureCoords, float* pointValueTextureCoords, RTW::Backend* backend)
{
  if (backend == nullptr)
    return OSPGeometry();
  OSPGeometry ospMesh = ospNewGeometry("trianglemesh");
  ospSetData(ospMesh, "position", vertices);

  size_t numTriangles = indexArray.size() / 3;
  osp::vec3i* triangles = new osp::vec3i[numTriangles];
  for (size_t i = 0, mi = 0; i < numTriangles; i++, mi += 3)
  {
    triangles[i] = osp::vec3i{ static_cast<int>(indexArray[mi + 0]),
      static_cast<int>(indexArray[mi + 1]), static_cast<int>(indexArray[mi + 2]) };
  }
  OSPData index = ospNewData(numTriangles, OSP_INT3, &triangles[0]);
  delete[] triangles;
  ospSetData(ospMesh, "index", index);

  OSPData _normals = nullptr;
  if (numNormals)
  {
    _normals = ospNewData(numNormals, OSP_FLOAT3, &normals[0], 0);
    ospSetData(ospMesh, "vertex.normal", _normals);
  }

  // send the texture map and texture coordinates over
  bool _hastm = false;
  OSPData tcs = nullptr;
  osp::vec2f* tc = nullptr;
  if (numTextureCoordinates || numPointValueTextureCoords)
  {
    _hastm = true;

    if (numPointValueTextureCoords)
    {
      // using 1D texture for point value LUT
      tc = new osp::vec2f[numPointValueTextureCoords];
      for (size_t i = 0; i < static_cast<size_t>(numPointValueTextureCoords); i++)
      {
        tc[i] = osp::vec2f{ pointValueTextureCoords[i], 0 };
      }
      tcs = ospNewData(numPointValueTextureCoords, OSP_FLOAT2, &tc[0]);
      ospSetData(ospMesh, "vertex.texcoord", tcs);
    }
    else if (numTextureCoordinates)
    {
      // 2d texture mapping
      tc = new osp::vec2f[numTextureCoordinates / 2];
      float* itc = textureCoordinates;
      for (size_t i = 0; i < static_cast<size_t>(numTextureCoordinates); i += 2)
      {
        float t1, t2;
        t1 = *itc;
        itc++;
        t2 = *itc;
        itc++;
        tc[i / 2] = osp::vec2f{ t1, t2 };
      }
      tcs = ospNewData(numTextureCoordinates / 2, OSP_FLOAT2, &tc[0]);
      ospSetData(ospMesh, "vertex.texcoord", tcs);
    }
  }
  delete[] tc;

  // send over cell colors, point colors or whole actor color
  OSPData _cmats = nullptr;
  OSPData _PointColors = nullptr;
  int* ids = nullptr;
  if (useCustomMaterial)
  {
    ospSetMaterial(ospMesh, actorMaterial);
  }
  else
  {
    if (vNormalTextureMap && _hastm)
    {
      OSPTexture t2d = vtkOSPRayMaterialHelpers::VTKToOSPTexture(backend, vNormalTextureMap);
      if (interpolationType == VTK_PBR)
      {
        ospSetObject(actorMaterial, "normalMap", t2d);
        ospSet4f(actorMaterial, "normalMap.transform", textureTransform.x, textureTransform.y,
          textureTransform.z, textureTransform.w);
      }
      else
      {
        ospSetObject(actorMaterial, "map_Bump", t2d);
        ospSet4f(actorMaterial, "map_Bump.transform", textureTransform.x, textureTransform.y,
          textureTransform.z, textureTransform.w);
      }
      ospRelease(t2d);
      ospCommit(actorMaterial);
    }

    if (interpolationType == VTK_PBR && vMaterialTextureMap && _hastm)
    {
      vtkNew<vtkImageExtractComponents> extractRoughness;
      extractRoughness->SetInputData(vMaterialTextureMap);
      extractRoughness->SetComponents(1);
      extractRoughness->Update();

      vtkNew<vtkImageExtractComponents> extractMetallic;
      extractMetallic->SetInputData(vMaterialTextureMap);
      extractMetallic->SetComponents(2);
      extractMetallic->Update();

      vtkImageData* vRoughnessTextureMap = extractRoughness->GetOutput();
      vtkImageData* vMetallicTextureMap = extractMetallic->GetOutput();

      OSPTexture t2dR = vtkOSPRayMaterialHelpers::VTKToOSPTexture(backend, vRoughnessTextureMap);
      ospSetObject(actorMaterial, "roughnessMap", t2dR);
      ospSet4f(actorMaterial, "roughnessMap.transform", textureTransform.x, textureTransform.y,
        textureTransform.z, textureTransform.w);
      ospRelease(t2dR);

      OSPTexture t2dM = vtkOSPRayMaterialHelpers::VTKToOSPTexture(backend, vMetallicTextureMap);
      ospSetObject(actorMaterial, "metallicMap", t2dM);
      ospSet4f(actorMaterial, "metallicMap.transform", textureTransform.x, textureTransform.y,
        textureTransform.z, textureTransform.w);
      ospRelease(t2dM);

      ospCommit(actorMaterial);
    }

    if (vColorTextureMap && _hastm)
    {
      // Note: this will only have an affect on OBJMaterials
      OSPTexture t2d = vtkOSPRayMaterialHelpers::VTKToOSPTexture(backend, vColorTextureMap);
      if (interpolationType == VTK_PBR)
      {
        ospSetObject(actorMaterial, "baseColorMap", ((OSPTexture)(t2d)));
        ospSet4f(actorMaterial, "baseColorMap.transform", textureTransform.x, textureTransform.y,
          textureTransform.z, textureTransform.w);
      }
      else
      {
        ospSetObject(actorMaterial, "map_Kd", ((OSPTexture)(t2d)));
        ospSet4f(actorMaterial, "map_Kd.transform", textureTransform.x, textureTransform.y,
          textureTransform.z, textureTransform.w);
      }
      ospRelease(t2d);
      ospCommit(actorMaterial);
      ospSetMaterial(ospMesh, actorMaterial);
    }
    else if (numCellMaterials)
    {
      ids = new int[numTriangles];
      for (size_t i = 0; i < numTriangles; i++)
      {
        ids[i] = rIndexArray[i * 3 + 0];
      }
      _cmats = ospNewData(numTriangles, OSP_INT, &ids[0]);
      ospSetData(ospMesh, "prim.materialID", _cmats);
      ospSetData(ospMesh, "materialList", CellMaterials);
    }
    else if (numPointColors)
    {
      _PointColors = ospNewData(numPointColors, OSP_FLOAT4, &PointColors[0]);
      ospSetData(ospMesh, "vertex.color", _PointColors);
    }
    else
    {
      ospSetMaterial(ospMesh, actorMaterial);
    }
  }

  ospCommit(ospMesh);
  ospRelease(index);
  ospRelease(_normals);
  ospRelease(_PointColors);
  ospRelease(_cmats);
  ospRelease(tcs);
  delete[] ids;
  return ospMesh;
}

//------------------------------------------------------------------------------
OSPMaterial MakeActorMaterial(vtkOSPRayRendererNode* orn, OSPRenderer oRenderer,
  vtkProperty* property, double* ambientColor, double* diffuseColor, float* specularf,
  double opacity, bool pt_avail, bool& useCustomMaterial, std::map<std::string, OSPMaterial>& mats,
  const std::string& materialName)
{
  RTW::Backend* backend = orn->GetBackend();
  useCustomMaterial = false;
  if (backend == nullptr)
  {
    return OSPMaterial();
  }

  float lum = static_cast<float>(vtkOSPRayActorNode::GetLuminosity(property));

  float diffusef[] = { static_cast<float>(diffuseColor[0] * property->GetDiffuse()),
    static_cast<float>(diffuseColor[1] * property->GetDiffuse()),
    static_cast<float>(diffuseColor[2] * property->GetDiffuse()) };
  if (lum > 0.0)
  {
    OSPMaterial oMaterial = vtkOSPRayMaterialHelpers::NewMaterial(orn, oRenderer, "Luminous");
    ospSet3fv(oMaterial, "color", diffusef);
    ospSetf(oMaterial, "intensity", lum);
    ospSetf(oMaterial, "transparency", 1.0 - opacity);
    return oMaterial;
  }

  if (pt_avail && property->GetMaterialName())
  {
    if (std::string("Value Indexed") == property->GetMaterialName())
    {
      vtkOSPRayMaterialHelpers::MakeMaterials(
        orn, oRenderer, mats); // todo: do an mtime check to avoid doing this when unchanged
      std::string requested_mat_name = materialName;
      if (requested_mat_name != "" && requested_mat_name != "Value Indexed")
      {
        useCustomMaterial = true;
        return vtkOSPRayMaterialHelpers::MakeMaterial(orn, oRenderer, requested_mat_name.c_str());
      }
    }
    else
    {
      useCustomMaterial = true;
      return vtkOSPRayMaterialHelpers::MakeMaterial(orn, oRenderer, property->GetMaterialName());
    }
  }

  OSPMaterial oMaterial;
  if (property->GetInterpolation() == VTK_PBR)
  {
    oMaterial = vtkOSPRayMaterialHelpers::NewMaterial(orn, oRenderer, "Principled");

    ospSet3fv(oMaterial, "baseColor", diffusef);
    ospSet1f(oMaterial, "metallic", static_cast<float>(property->GetMetallic()));
    ospSet1f(oMaterial, "roughness", static_cast<float>(property->GetRoughness()));
    ospSet1f(oMaterial, "opacity", static_cast<float>(opacity));
  }
  else
  {
    oMaterial = vtkOSPRayMaterialHelpers::NewMaterial(orn, oRenderer, "OBJMaterial");

    float ambientf[] = { static_cast<float>(ambientColor[0] * property->GetAmbient()),
      static_cast<float>(ambientColor[1] * property->GetAmbient()),
      static_cast<float>(ambientColor[2] * property->GetAmbient()) };

    float specPower = static_cast<float>(property->GetSpecularPower());
    float specAdjust = 2.0f / (2.0f + specPower);
    specularf[0] =
      static_cast<float>(property->GetSpecularColor()[0] * property->GetSpecular() * specAdjust);
    specularf[1] =
      static_cast<float>(property->GetSpecularColor()[1] * property->GetSpecular() * specAdjust);
    specularf[2] =
      static_cast<float>(property->GetSpecularColor()[2] * property->GetSpecular() * specAdjust);

    ospSet3fv(oMaterial, "Ka", ambientf);
    if (property->GetDiffuse() == 0.0)
    {
      // a workaround for ParaView, remove when ospray supports Ka
      ospSet3fv(oMaterial, "Kd", ambientf);
    }
    else
    {
      ospSet3fv(oMaterial, "Kd", diffusef);
    }
    ospSet3fv(oMaterial, "Ks", specularf);
    ospSet1f(oMaterial, "Ns", specPower);
    ospSet1f(oMaterial, "d", static_cast<float>(opacity));
  }

  return oMaterial;
}

//------------------------------------------------------------------------------
OSPMaterial MakeActorMaterial(vtkOSPRayRendererNode* orn, OSPRenderer oRenderer,
  vtkProperty* property, double* ambientColor, double* diffuseColor, float* specularf,
  double opacity)
{
  bool dontcare1;
  std::map<std::string, OSPMaterial> dontcare2;
  return MakeActorMaterial(orn, oRenderer, property, ambientColor, diffuseColor, specularf, opacity,
    false, dontcare1, dontcare2, "");
};

}

//============================================================================
vtkStandardNewMacro(vtkOSPRayPolyDataMapperNode);

//----------------------------------------------------------------------------
vtkOSPRayPolyDataMapperNode::vtkOSPRayPolyDataMapperNode()
{
  this->UseInstanceCache = true;
  this->UseGeometryCache = true;
  this->GeometryCache = new vtkOSPRayCache<vtkOSPRayCacheItemGeometries>;
  this->InstanceCache = new vtkOSPRayCache<vtkOSPRayCacheItemObject>;
}

//----------------------------------------------------------------------------
vtkOSPRayPolyDataMapperNode::~vtkOSPRayPolyDataMapperNode()
{
  delete this->GeometryCache;
  delete this->InstanceCache;
}

//----------------------------------------------------------------------------
void vtkOSPRayPolyDataMapperNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkOSPRayPolyDataMapperNode::ORenderPoly(void* renderer, vtkOSPRayActorNode* aNode,
  vtkPolyData* poly, double* ambientColor, double* diffuseColor, double opacity,
  std::string materialName)
{
  vtkOSPRayRendererNode* orn =
    static_cast<vtkOSPRayRendererNode*>(this->GetFirstAncestorOfType("vtkOSPRayRendererNode"));
  RTW::Backend* backend = orn->GetBackend();
  if (backend == nullptr)
    return;

  OSPRenderer oRenderer = static_cast<OSPRenderer>(renderer);
  vtkActor* act = vtkActor::SafeDownCast(aNode->GetRenderable());
  vtkProperty* property = act->GetProperty();

  // get texture transform
  osp::vec4f texTransform{ 1.f, 0.f, 0.f, 1.f };
  vtkInformation* info = act->GetPropertyKeys();
  if (info && info->Has(vtkProp::GeneralTextureTransform()))
  {
    double* mat = info->Get(vtkProp::GeneralTextureTransform());
    texTransform.x = mat[0];
    texTransform.y = mat[1];
    texTransform.z = mat[4];
    texTransform.w = mat[5];
  }

  // make geometry
  std::vector<double> _vertices;
  vtkPolyDataMapperNode::TransformPoints(act, poly, _vertices);
  size_t numPositions = _vertices.size() / 3;
  osp::vec3f* vertices = new osp::vec3f[numPositions];
  for (size_t i = 0; i < numPositions; i++)
  {
    vertices[i] = osp::vec3f{ static_cast<float>(_vertices[i * 3 + 0]),
      static_cast<float>(_vertices[i * 3 + 1]), static_cast<float>(_vertices[i * 3 + 2]) };
  }
  OSPData position = ospNewData(numPositions, OSP_FLOAT3, &vertices[0]);
  ospCommit(position);
  _vertices.clear();

  // make connectivity
  vtkPolyDataMapperNode::vtkPDConnectivity conn;
  vtkPolyDataMapperNode::MakeConnectivity(poly, property->GetRepresentation(), conn);

  // choosing sphere and cylinder radii (for points and lines) that
  // approximate pointsize and linewidth
  vtkMapper* mapper = act->GetMapper();
  double length = 1.0;
  if (mapper)
  {
    length = mapper->GetLength();
  }
  double pointSize = length / 1000.0 * property->GetPointSize();
  double lineWidth = length / 1000.0 * property->GetLineWidth();
  // finer control over sphere and cylinders sizes
  int enable_scaling = vtkOSPRayActorNode::GetEnableScaling(act);
  vtkDataArray* scaleArray = nullptr;
  vtkPiecewiseFunction* scaleFunction = nullptr;
  if (enable_scaling && mapper)
  {
    vtkInformation* mapInfo = mapper->GetInformation();
    char* scaleArrayName = (char*)mapInfo->Get(vtkOSPRayActorNode::SCALE_ARRAY_NAME());
    scaleArray = poly->GetPointData()->GetArray(scaleArrayName);
    scaleFunction =
      vtkPiecewiseFunction::SafeDownCast(mapInfo->Get(vtkOSPRayActorNode::SCALE_FUNCTION()));
  }

  // per actor material
  float specularf[3];
  bool useCustomMaterial = false;
  std::map<std::string, OSPMaterial> mats;
  std::set<OSPMaterial> uniqueMats;
  const std::string rendererType =
    orn->GetRendererType(vtkRenderer::SafeDownCast(orn->GetRenderable()));
  bool pt_avail =
    rendererType == std::string("pathtracer") || rendererType == std::string("optix pathtracer");
  //}
  OSPMaterial oMaterial = vtkosp::MakeActorMaterial(orn, oRenderer, property, ambientColor,
    diffuseColor, specularf, opacity, pt_avail, useCustomMaterial, mats, materialName);
  ospCommit(oMaterial);
  uniqueMats.insert(oMaterial);

  // texture
  int numTextureCoordinates = 0;
  float* textureCoordinates = nullptr;
  if (vtkDataArray* da = poly->GetPointData()->GetTCoords())
  {
    numTextureCoordinates = da->GetNumberOfTuples();
    textureCoordinates = new float[numTextureCoordinates * 2];
    float* tp = textureCoordinates;
    for (int i = 0; i < numTextureCoordinates; i++)
    {
      *tp = static_cast<float>(da->GetTuple(i)[0]);
      tp++;
      *tp = static_cast<float>(da->GetTuple(i)[1]);
      tp++;
    }
    numTextureCoordinates = numTextureCoordinates * 2;
  }
  vtkTexture* texture = nullptr;
  if (property->GetInterpolation() == VTK_PBR)
  {
    texture = property->GetTexture("albedoTex");
  }
  else
  {
    texture = act->GetTexture();
  }
  vtkImageData* vColorTextureMap = nullptr;
  vtkImageData* vNormalTextureMap = nullptr;
  vtkImageData* vMaterialTextureMap = nullptr;

  if (texture)
  {
    vColorTextureMap = texture->GetInput();
  }

  // colors from point and cell arrays
  int numCellMaterials = 0;
  OSPData cellMaterials = nullptr;
  int numPointColors = 0;
  osp::vec4f* pointColors = nullptr;
  int numPointValueTextureCoords = 0;
  float* pointValueTextureCoords = nullptr;
  //
  // now ask mapper to do most of the work and provide us with
  // colors per cell and colors or texture coordinates per point
  vtkUnsignedCharArray* vColors = nullptr;
  vtkFloatArray* vColorCoordinates = nullptr;
  vtkImageData* pColorTextureMap = nullptr;
  int cellFlag = -1; // mapper tells us which
  if (mapper)
  {
    mapper->MapScalars(poly, 1.0, cellFlag);
    vColors = mapper->GetColorMapColors();
    vColorCoordinates = mapper->GetColorCoordinates();
    pColorTextureMap = mapper->GetColorTextureMap();
  }
  if (vColors)
  {
    if (cellFlag == 2 && mapper->GetFieldDataTupleId() > -1)
    {
      // color comes from field data entry
      bool use_material = false;
      // check if the field data content says to use a material lookup
      vtkScalarsToColors* s2c = mapper->GetLookupTable();
      bool try_mats =
        s2c->GetIndexedLookup() && s2c->GetNumberOfAnnotatedValues() && mats.size() > 0;
      if (try_mats)
      {
        int cflag2 = -1;
        vtkAbstractArray* scalars = mapper->GetAbstractScalars(poly, mapper->GetScalarMode(),
          mapper->GetArrayAccessMode(), mapper->GetArrayId(), mapper->GetArrayName(), cflag2);
        vtkVariant v = scalars->GetVariantValue(mapper->GetFieldDataTupleId());
        vtkIdType idx = s2c->GetAnnotatedValueIndex(v);
        if (idx > -1)
        {
          std::string name(s2c->GetAnnotation(idx));
          if (mats.find(name) != mats.end())
          {
            // yes it does!
            oMaterial = mats[name];
            ospCommit(oMaterial);
            use_material = true;
          }
        }
      }
      if (!use_material)
      {
        // just use the color for the field data value
        int numComp = vColors->GetNumberOfComponents();
        unsigned char* colorPtr = vColors->GetPointer(0);
        colorPtr = colorPtr + mapper->GetFieldDataTupleId() * numComp;
        // this setting (and all the other scalar colors)
        // really depends on mapper->ScalarMaterialMode
        // but I'm not sure Ka is working currently so leaving it on Kd
        float fdiffusef[] = { static_cast<float>(colorPtr[0] * property->GetDiffuse() / 255.0f),
          static_cast<float>(colorPtr[1] * property->GetDiffuse() / 255.0f),
          static_cast<float>(colorPtr[2] * property->GetDiffuse() / 255.0f) };
        ospSet3fv(oMaterial, "Kd", fdiffusef);
        ospCommit(oMaterial);
      }
    }
    else if (cellFlag == 1)
    {
      // color or material on cell
      vtkScalarsToColors* s2c = mapper->GetLookupTable();
      std::vector<OSPMaterial> cellColors;
      vtkosp::CellMaterials(orn, oRenderer, poly, mapper, s2c, mats, cellColors, vColors, specularf,
        float(property->GetSpecularPower()), opacity);
      numCellMaterials = static_cast<int>(cellColors.size());
      cellMaterials = ospNewData(cellColors.size(), OSP_OBJECT, &cellColors[0]);
      ospCommit(cellMaterials);

      for (OSPMaterial mat : cellColors)
      {
        uniqueMats.insert(mat);
      }
    }
    else if (cellFlag == 0)
    {
      // color on point interpolated RGB
      numPointColors = vColors->GetNumberOfTuples();
      pointColors = new osp::vec4f[numPointColors];
      for (int i = 0; i < numPointColors; i++)
      {
        unsigned char* color = vColors->GetPointer(4 * i);
        pointColors[i] = osp::vec4f{ color[0] / 255.0f, color[1] / 255.0f, color[2] / 255.0f,
          (color[3] / 255.0f) * static_cast<float>(opacity) };
      }
    }
  }
  else
  {
    if (vColorCoordinates && pColorTextureMap)
    {
      // color on point interpolated values (subsequently colormapped via 1D LUT)
      numPointValueTextureCoords = vColorCoordinates->GetNumberOfTuples();
      pointValueTextureCoords = new float[numPointValueTextureCoords];
      float* tc = vColorCoordinates->GetPointer(0);
      for (int i = 0; i < numPointValueTextureCoords; i++)
      {
        float v = *tc;
        v = ((v >= 1.0f) ? 0.99999f : ((v < 0.0f) ? 0.0f : v)); // clamp [0..1)
        pointValueTextureCoords[i] = v;
        tc += 2;
      }
      vColorTextureMap = pColorTextureMap;
    }
  }

  // create an ospray mesh for the vertex cells
  if (conn.vertex_index.size())
  {
    this->Geometries.emplace_back(vtkosp::RenderAsSpheres(vertices, conn.vertex_index,
      conn.vertex_reverse, pointSize, scaleArray, scaleFunction, useCustomMaterial, oMaterial,
      vColorTextureMap, numTextureCoordinates, textureCoordinates, numCellMaterials, cellMaterials,
      numPointColors, pointColors, numPointValueTextureCoords, pointValueTextureCoords, backend));
  }

  // create an ospray mesh for the line cells
  if (conn.line_index.size())
  {
    // format depends on representation style
    if (property->GetRepresentation() == VTK_POINTS)
    {
      this->Geometries.emplace_back(
        vtkosp::RenderAsSpheres(vertices, conn.line_index, conn.line_reverse, pointSize, scaleArray,
          scaleFunction, useCustomMaterial, oMaterial, vColorTextureMap, numTextureCoordinates,
          textureCoordinates, numCellMaterials, cellMaterials, numPointColors, pointColors,
          numPointValueTextureCoords, pointValueTextureCoords, backend));
    }
    else
    {
      this->Geometries.emplace_back(vtkosp::RenderAsCylinders(vertices, conn.line_index,
        conn.line_reverse, lineWidth, scaleArray, scaleFunction, useCustomMaterial, oMaterial,
        vColorTextureMap, numTextureCoordinates, textureCoordinates, numCellMaterials,
        cellMaterials, numPointColors, pointColors, numPointValueTextureCoords,
        pointValueTextureCoords, backend));
    }
  }

  // create an ospray mesh for the polygon cells
  if (conn.triangle_index.size())
  {
    // format depends on representation style
    switch (property->GetRepresentation())
    {
      case VTK_POINTS:
      {
        this->Geometries.emplace_back(vtkosp::RenderAsSpheres(vertices, conn.triangle_index,
          conn.triangle_reverse, pointSize, scaleArray, scaleFunction, useCustomMaterial, oMaterial,
          vColorTextureMap, numTextureCoordinates, textureCoordinates, numCellMaterials,
          cellMaterials, numPointColors, pointColors, numPointValueTextureCoords,
          pointValueTextureCoords, backend));
        break;
      }
      case VTK_WIREFRAME:
      {
        this->Geometries.emplace_back(vtkosp::RenderAsCylinders(vertices, conn.triangle_index,
          conn.triangle_reverse, lineWidth, scaleArray, scaleFunction, useCustomMaterial, oMaterial,
          vColorTextureMap, numTextureCoordinates, textureCoordinates, numCellMaterials,
          cellMaterials, numPointColors, pointColors, numPointValueTextureCoords,
          pointValueTextureCoords, backend));
        break;
      }
      default:
      {
        if (property->GetEdgeVisibility())
        {
          // edge mesh
          vtkPolyDataMapperNode::vtkPDConnectivity conn2;
          vtkPolyDataMapperNode::MakeConnectivity(poly, VTK_WIREFRAME, conn2);

          // edge material
          double* eColor = property->GetEdgeColor();
          OSPMaterial oMaterial2 =
            vtkosp::MakeActorMaterial(orn, oRenderer, property, eColor, eColor, specularf, opacity);
          ospCommit(oMaterial2);

          this->Geometries.emplace_back(vtkosp::RenderAsCylinders(vertices, conn2.triangle_index,
            conn2.triangle_reverse, lineWidth, scaleArray, scaleFunction, false, oMaterial2,
            vColorTextureMap, 0, textureCoordinates, numCellMaterials, cellMaterials,
            numPointColors, pointColors, 0, pointValueTextureCoords, backend));
          uniqueMats.insert(oMaterial2);
        }

        osp::vec3f* normals = nullptr;
        int numNormals = 0;
        if (property->GetInterpolation() != VTK_FLAT)
        {
          vtkDataArray* vNormals = poly->GetPointData()->GetNormals();
          if (vNormals)
          {
            vtkSmartPointer<vtkMatrix4x4> m = vtkSmartPointer<vtkMatrix4x4>::New();
            act->GetMatrix(m);
            vtkSmartPointer<vtkMatrix3x3> mat3 = vtkSmartPointer<vtkMatrix3x3>::New();
            for (int i = 0; i < 3; ++i)
            {
              for (int j = 0; j < 3; ++j)
              {
                mat3->SetElement(i, j, m->GetElement(i, j));
              }
            }
            mat3->Invert();
            mat3->Transpose();

            vtkosp::VToOPointNormals(vNormals, normals, mat3);
            numNormals = vNormals->GetNumberOfTuples();
          }
        }

        texture = property->GetTexture("normalTex");
        if (texture)
        {
          vNormalTextureMap = texture->GetInput();
        }

        if (property->GetInterpolation() == VTK_PBR)
        {
          texture = property->GetTexture("materialTex");
          if (texture)
          {
            vMaterialTextureMap = texture->GetInput();
          }
        }

        this->Geometries.emplace_back(
          vtkosp::RenderAsTriangles(position, conn.triangle_index, conn.triangle_reverse,
            useCustomMaterial, oMaterial, numNormals, normals, property->GetInterpolation(),
            vColorTextureMap, vNormalTextureMap, vMaterialTextureMap, numTextureCoordinates,
            textureCoordinates, texTransform, numCellMaterials, cellMaterials, numPointColors,
            pointColors, numPointValueTextureCoords, pointValueTextureCoords, backend));
        delete[] normals;
      }
    }
  }

  if (conn.strip_index.size())
  {
    switch (property->GetRepresentation())
    {
      case VTK_POINTS:
      {
        this->Geometries.emplace_back(vtkosp::RenderAsSpheres(vertices, conn.strip_index,
          conn.strip_reverse, pointSize, scaleArray, scaleFunction, useCustomMaterial, oMaterial,
          vColorTextureMap, numTextureCoordinates, textureCoordinates, numCellMaterials,
          cellMaterials, numPointColors, pointColors, numPointValueTextureCoords,
          pointValueTextureCoords, backend));
        break;
      }
      case VTK_WIREFRAME:
      {
        this->Geometries.emplace_back(vtkosp::RenderAsCylinders(vertices, conn.strip_index,
          conn.strip_reverse, lineWidth, scaleArray, scaleFunction, useCustomMaterial, oMaterial,
          vColorTextureMap, numTextureCoordinates, textureCoordinates, numCellMaterials,
          cellMaterials, numPointColors, pointColors, numPointValueTextureCoords,
          pointValueTextureCoords, backend));
        break;
      }
      default:
      {
        if (property->GetEdgeVisibility())
        {
          // edge mesh
          vtkPolyDataMapperNode::vtkPDConnectivity conn2;
          vtkPolyDataMapperNode::MakeConnectivity(poly, VTK_WIREFRAME, conn2);

          // edge material
          double* eColor = property->GetEdgeColor();
          OSPMaterial oMaterial2 =
            vtkosp::MakeActorMaterial(orn, oRenderer, property, eColor, eColor, specularf, opacity);
          ospCommit(oMaterial2);

          this->Geometries.emplace_back(vtkosp::RenderAsCylinders(vertices, conn2.strip_index,
            conn2.strip_reverse, lineWidth, scaleArray, scaleFunction, false, oMaterial2,
            vColorTextureMap, 0, textureCoordinates, numCellMaterials, cellMaterials,
            numPointColors, pointColors, 0, pointValueTextureCoords, backend));

          uniqueMats.insert(oMaterial2);
        }
        osp::vec3f* normals = nullptr;
        int numNormals = 0;
        if (property->GetInterpolation() != VTK_FLAT)
        {
          vtkDataArray* vNormals = poly->GetPointData()->GetNormals();
          if (vNormals)
          {
            vtkSmartPointer<vtkMatrix4x4> m = vtkSmartPointer<vtkMatrix4x4>::New();
            act->GetMatrix(m);
            vtkSmartPointer<vtkMatrix3x3> mat3 = vtkSmartPointer<vtkMatrix3x3>::New();
            for (int i = 0; i < 3; ++i)
            {
              for (int j = 0; j < 3; ++j)
              {
                mat3->SetElement(i, j, m->GetElement(i, j));
              }
            }
            mat3->Invert();
            mat3->Transpose();

            vtkosp::VToOPointNormals(vNormals, normals, mat3);
            numNormals = vNormals->GetNumberOfTuples();
          }
        }
        this->Geometries.emplace_back(
          vtkosp::RenderAsTriangles(position, conn.strip_index, conn.strip_reverse,
            useCustomMaterial, oMaterial, numNormals, normals, property->GetInterpolation(),
            vColorTextureMap, vNormalTextureMap, vMaterialTextureMap, numTextureCoordinates,
            textureCoordinates, texTransform, numCellMaterials, cellMaterials, numPointColors,
            pointColors, numPointValueTextureCoords, pointValueTextureCoords, backend));
        delete[] normals;
      }
    }
  }
  ospRelease(position);
  ospRelease(cellMaterials);

  for (auto it : mats)
  {
    uniqueMats.insert(it.second);
  }

  for (OSPMaterial mat : uniqueMats)
  {
    ospRelease(mat);
  }

  delete[] vertices;
  delete[] pointColors;
  delete[] pointValueTextureCoords;
  delete[] textureCoordinates;
}

//----------------------------------------------------------------------------
void vtkOSPRayPolyDataMapperNode::Invalidate(bool prepass)
{
  if (prepass)
  {
    this->RenderTime = 0;
  }
}

//----------------------------------------------------------------------------
void vtkOSPRayPolyDataMapperNode::Render(bool prepass)
{
  if (prepass)
  {
    // we use a lot of params from our parent
    vtkOSPRayActorNode* aNode = vtkOSPRayActorNode::SafeDownCast(this->Parent);
    vtkActor* act = vtkActor::SafeDownCast(aNode->GetRenderable());

    if (act->GetVisibility() == false)
    {
      return;
    }

    vtkOSPRayRendererNode* orn =
      static_cast<vtkOSPRayRendererNode*>(this->GetFirstAncestorOfType("vtkOSPRayRendererNode"));
    vtkRenderer* ren = vtkRenderer::SafeDownCast(orn->GetRenderable());
    this->GeometryCache->SetSize(vtkOSPRayRendererNode::GetTimeCacheSize(ren));
    this->InstanceCache->SetSize(vtkOSPRayRendererNode::GetTimeCacheSize(ren));

    double tstep = vtkOSPRayRendererNode::GetViewTime(orn->GetRenderer());

    // if there are no changes, just reuse last result
    vtkMTimeType inTime = aNode->GetMTime();
    if ((this->RenderTime >= inTime ||
          (this->UseInstanceCache && this->InstanceCache->Contains(tstep)) ||
          (this->UseGeometryCache && this->GeometryCache->Contains(tstep))))
    {
      // cout << "REUSE" << endl;
      this->RenderGeometries();
      return;
    }
    // cout << "DRAW" << endl;
    this->RenderTime = inTime;
    this->ClearGeometries();

    vtkPolyData* poly = nullptr;
    vtkPolyDataMapper* mapper = vtkPolyDataMapper::SafeDownCast(act->GetMapper());
    if (mapper && mapper->GetNumberOfInputPorts() > 0)
    {
      poly = mapper->GetInput();
    }
    if (poly)
    {
      vtkProperty* property = act->GetProperty();
      this->ORenderPoly(orn->GetORenderer(), aNode, poly, property->GetAmbientColor(),
        property->GetDiffuseColor(), property->GetOpacity(), "");
    }
    this->PopulateCache();
    this->RenderGeometries();
  }
}

//----------------------------------------------------------------------------
void vtkOSPRayPolyDataMapperNode::PopulateCache()
{

  vtkOSPRayRendererNode* orn =
    static_cast<vtkOSPRayRendererNode*>(this->GetFirstAncestorOfType("vtkOSPRayRendererNode"));
  double tstep = vtkOSPRayRendererNode::GetViewTime(orn->GetRenderer());

  RTW::Backend* backend = orn->GetBackend();

  if (this->UseGeometryCache)
  {
    auto cacheEntry = std::make_shared<vtkOSPRayCacheItemGeometries>(this->Geometries);
    this->GeometryCache->Set(tstep, cacheEntry);
  }
  if (this->UseInstanceCache && backend->IsSupported(RTW_INSTANCING))
  {
    OSPModel instanceModel = ospNewModel();
    for (auto g : this->Geometries)
    {
      ospAddGeometry(instanceModel, g);
    }
    ospCommit(instanceModel);
    osp::affine3f xfm{ 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0 };
    auto instance = ospNewInstance(instanceModel, xfm);
    ospCommit(instance);
    auto instanceCacheEntry =
      std::make_shared<vtkOSPRayCacheItemObject>(backend, (OSPObject)instance);
    instanceCacheEntry->size = this->Geometries.size();
    this->InstanceCache->Set(tstep, instanceCacheEntry);
    ospRelease(instanceModel);
  }
}

//----------------------------------------------------------------------------
void vtkOSPRayPolyDataMapperNode::RenderGeometries()
{
  vtkOSPRayRendererNode* orn =
    static_cast<vtkOSPRayRendererNode*>(this->GetFirstAncestorOfType("vtkOSPRayRendererNode"));
  double tstep = vtkOSPRayRendererNode::GetViewTime(orn->GetRenderer());
  auto oModel = orn->GetOModel();

  RTW::Backend* backend = orn->GetBackend();

  if (this->UseInstanceCache && backend->IsSupported(RTW_INSTANCING) &&
    this->InstanceCache->Contains(tstep))
  {
    auto cacheEntry = this->InstanceCache->Get(tstep);
    if (cacheEntry->size > 0)
    {
      OSPGeometry instance = static_cast<OSPGeometry>(cacheEntry->object);
      ospAddGeometry(oModel, instance);
    }
    return;
  }
  else if (this->UseGeometryCache && this->GeometryCache->Contains(tstep))
  {
    auto& geometriesCache = this->GeometryCache->Get(tstep)->GeometriesAtTime;
    for (auto g : geometriesCache)
    {
      ospAddGeometry(oModel, g);
    }
    return;
  }
  for (auto g : this->Geometries)
  {
    ospAddGeometry(oModel, g);
  }
}

//----------------------------------------------------------------------------
void vtkOSPRayPolyDataMapperNode::ClearGeometries()
{
  vtkOSPRayRendererNode* orn =
    static_cast<vtkOSPRayRendererNode*>(this->GetFirstAncestorOfType("vtkOSPRayRendererNode"));

  RTW::Backend* backend = orn->GetBackend();

  // We can't delete if we may reuse.
  if (vtkOSPRayRendererNode::GetTimeCacheSize(orn->GetRenderer()) == 0)
  {
    for (auto g : this->Geometries)
    {
      ospRelease(g);
    }
  }
  this->Geometries.clear();
}
