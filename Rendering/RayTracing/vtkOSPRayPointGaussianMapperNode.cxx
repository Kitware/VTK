/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSPRayPointGaussianMapperNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOSPRayPointGaussianMapperNode.h"

#include "vtkActor.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArray.h"
#include "vtkDataObjectTreeIterator.h"
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
#include "vtkPointGaussianMapper.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkScalarsToColors.h"
#include "vtkSmartPointer.h"
#include "vtkTexture.h"
#include "vtkUnsignedCharArray.h"

#include "RTWrapper/RTWrapper.h"

#include <map>

//============================================================================

namespace vtkospn
{
void VToOPointNormals(
  vtkDataArray* vNormals, std::vector<osp::vec3f>& normals, vtkMatrix3x3* matrix)
{
  int numNormals = vNormals->GetNumberOfTuples();
  normals.resize(numNormals);
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
void MakeCellMaterials(vtkOSPRayRendererNode* orn, OSPRenderer oRenderer, vtkPolyData* poly,
  vtkMapper* mapper, vtkScalarsToColors* s2c, std::map<std::string, OSPMaterial> mats,
  std::vector<OSPMaterial>& ospMaterials, vtkUnsignedCharArray* vColors, float* specColor,
  float specPower, float opacity)
{
  RTW::Backend* backend = orn->GetBackend();
  if (backend == nullptr)
    return;
  vtkAbstractArray* scalars = nullptr;
  bool try_mats = s2c->GetIndexedLookup() && s2c->GetNumberOfAnnotatedValues() && !mats.empty();
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
      oMaterial = vtkOSPRayMaterialHelpers::NewMaterial(orn, oRenderer, "obj");
      float diffusef[] = { static_cast<float>(color[0]) / (255.0f),
        static_cast<float>(color[1]) / (255.0f), static_cast<float>(color[2]) / (255.0f) };
      float localOpacity = 1.f;
      if (width >= 4)
      {
        localOpacity = static_cast<float>(color[3]) / (255.0f);
      }
      ospSetVec3f(oMaterial, "kd", diffusef[0], diffusef[1], diffusef[2]);
      float specAdjust = 2.0f / (2.0f + specPower);
      float specularf[] = { specColor[0] * specAdjust, specColor[1] * specAdjust,
        specColor[2] * specAdjust };
      ospSetVec3f(oMaterial, "ks", specularf[0], specularf[1], specularf[2]);
      ospSetFloat(oMaterial, "ns", specPower);
      ospSetFloat(oMaterial, "d", opacity * localOpacity);
      ospCommit(oMaterial);
      ospMaterials.push_back(oMaterial);
    }
  }
}

//------------------------------------------------------------------------------
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
OSPGeometricModel RenderAsSpheres(osp::vec3f* vertices, std::vector<unsigned int>& indexArray,
  std::vector<unsigned int>& rIndexArray, double pointSize, vtkDataArray* scaleArray,
  vtkPiecewiseFunction* scaleFunction, bool useCustomMaterial, OSPMaterial actorMaterial,
  vtkImageData* vColorTextureMap, bool sRGB, int numTextureCoordinates, float* textureCoordinates,
  int numCellMaterials, std::vector<OSPMaterial>& CellMaterials, int numPointColors,
  osp::vec4f* PointColors, int numPointValueTextureCoords, float* pointValueTextureCoords,
  RTW::Backend* backend)
{
  if (backend == nullptr)
    return OSPGeometry();
  OSPVolume ospMesh = ospNewVolume("particle");
  OSPVolumetricModel ospVolModel = ospNewVolumetricModel(ospMesh);

  size_t numSpheres = indexArray.size();
  std::vector<osp::vec3f> vdata;
  std::vector<float> radii;
  vdata.reserve(indexArray.size());
  if (scaleArray != nullptr)
  {
    radii.reserve(indexArray.size());
  }

  std::vector<float> weights;
  weights.reserve(indexArray.size());

  for (size_t i = 0; i < indexArray.size(); i++)
  {
    vdata.emplace_back(vertices[indexArray[i]]);
    radii.emplace_back(0.01f);
    //    if (scaleArray != nullptr)
    //    {
    //      radii.emplace_back(MapThroughPWF(*scaleArray->GetTuple(indexArray[i]), scaleFunction));
    //    }

    weights.emplace_back(1.0f);
  }

  OSPData positionData = ospNewCopyData1D(vdata.data(), OSP_VEC3F, vdata.size());
  ospCommit(positionData);
  ospSetObject(ospMesh, "particle.position", positionData);
  //  if (scaleArray != nullptr)
  //  {
  OSPData radiiData = ospNewCopyData1D(radii.data(), OSP_FLOAT, radii.size());
  ospCommit(radiiData);
  ospSetObject(ospMesh, "particle.radius", radiiData);
  //  }
  //  else
  //  {
  //    ospSetFloat(ospMesh, "radius", pointSize);
  //  }
  OSPData weightsData = ospNewCopyData1D(weights.data(), OSP_FLOAT, weights.size());
  ospCommit(weightsData);
  ospSetObject(ospMesh, "particle.weight", weightsData);

  float clampMaxCumulativeValue = 0.0f;
  ospSetFloat(ospMesh, "clampMaxCumulativeValue", clampMaxCumulativeValue);

  float radiusSupportFactor = 4.f;
  ospSetFloat(ospMesh, "radiusSupportFactor", radiusSupportFactor);

  //  // send the texture map and texture coordinates over
  //  bool _hastm = false;
  //  if (numTextureCoordinates || numPointValueTextureCoords)
  //  {
  //    _hastm = true;
  //
  //    if (numPointValueTextureCoords)
  //    {
  //      // using 1D texture for point value LUT
  //      std::vector<osp::vec2f> tc(indexArray.size());
  //      for (size_t i = 0; i < indexArray.size(); i++)
  //      {
  //        float t1;
  //        int index1 = indexArray[i];
  //        t1 = pointValueTextureCoords[index1 + 0];
  //        tc[i] = osp::vec2f{ t1, 0 };
  //      }
  //      OSPData tcs = ospNewCopyData1D(tc.data(), OSP_VEC2F, indexArray.size());
  //      ospCommit(tcs);
  //      ospSetObject(ospMesh, "sphere.texcoord", tcs);
  //    }
  //    else if (numTextureCoordinates)
  //    {
  //      // 2d texture mapping
  //      float* itc = textureCoordinates;
  //      std::vector<osp::vec2f> tc(indexArray.size());
  //      for (size_t i = 0; i < indexArray.size(); i++)
  //      {
  //        float t1, t2;
  //        int index1 = indexArray[i];
  //        t1 = itc[index1 * 2 + 0];
  //        t2 = itc[index1 * 2 + 1];
  //        tc[i] = osp::vec2f{ t1, t2 };
  //      }
  //      OSPData tcs = ospNewCopyData1D(tc.data(), OSP_VEC2F, indexArray.size());
  //      ospCommit(tcs);
  //      ospSetObject(ospMesh, "sphere.texcoord", tcs);
  //    }
  //  }
  //
  // OSPData _cmats = nullptr;
  OSPData _PointColors = nullptr;
  // bool perCellColor = false;
  // bool perPointColor = false;
  std::vector<float> tfOVals;
  if (!useCustomMaterial)
  {
    //    if (vColorTextureMap && _hastm)
    //    {
    //      OSPTexture t2d = vtkOSPRayMaterialHelpers::VTKToOSPTexture(backend, vColorTextureMap,
    //      sRGB); ospSetObject(actorMaterial, "map_kd", ((OSPTexture)(t2d)));
    //      ospCommit(actorMaterial);
    //      ospRelease(t2d);
    //    }
    //    else if (numCellMaterials)
    //    {
    //      // per cell color
    //      perCellColor = true;
    //      std::vector<OSPMaterial> perCellMats;
    //      for (size_t i = 0; i < numSpheres; i++)
    //      {
    //        perCellMats.push_back(CellMaterials[rIndexArray[i]]);
    //      }
    //      _cmats = ospNewCopyData1D(&perCellMats[0], OSP_MATERIAL, numSpheres);
    //      ospCommit(_cmats);
    //      ospSetObject(ospVolModel, "material", _cmats);
    //    }
    //    else if (numPointColors)
    if (numPointColors)
    {
      // per point color
      // perPointColor = true;
      std::vector<osp::vec3f> perPointColors;
      for (size_t i = 0; i < numSpheres; i++)
      {
        unsigned int ii = indexArray[i];
        osp::vec3f p = { PointColors[ii].x, PointColors[ii].y, PointColors[ii].z };
        perPointColors.push_back(p);
        tfOVals.push_back(PointColors[ii].w);
      }
      _PointColors = ospNewCopyData1D(&perPointColors[0], OSP_VEC3F, numSpheres);
      // ospCommit(_PointColors);
      // ospSetObject(ospVolModel, "color", _PointColors);
    }
  }

  //  if (actorMaterial && !perCellColor && !perPointColor)
  //  {
  //    ospCommit(actorMaterial);
  //    ospSetObjectAsData(ospVolModel, "material", OSP_MATERIAL, actorMaterial);
  //  }

  if (numPointColors == 0)
  {
    std::vector<osp::vec3f> perPointColors;
    perPointColors.emplace_back(osp::vec3f({ 0.0f, 0.0f, 0.0f }));
    perPointColors.emplace_back(osp::vec3f({ 1.0f, 1.0f, 1.0f }));
    _PointColors = ospNewCopyData1D(&perPointColors[0], OSP_VEC3F, 2);
    tfOVals.emplace_back(1.f);
    // for (size_t i = 0; i < numSpheres; i++)
    // {
    //   tfOVals.emplace_back(1.f);
    // }
  }

  auto oTF = ospNewTransferFunction("piecewiseLinear");
  ospSetObject(oTF, "color", _PointColors);
  OSPData tfAlphaData = ospNewCopyData1D(&tfOVals[0], OSP_FLOAT, 1);
  ospSetObject(oTF, "opacity", tfAlphaData);
  ospSetVec2f(oTF, "valueRange", 0.f, 1.5f);
  ospCommit(oTF);
  ospSetObject(ospVolModel, "transferFunction", oTF);
  ospCommit(ospMesh);
  ospCommit(ospVolModel);
  ospRelease(ospMesh);
  // ospRelease(_cmats);
  // ospRelease(_PointColors);

  return ospVolModel;
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
    OSPMaterial oMaterial = vtkOSPRayMaterialHelpers::NewMaterial(orn, oRenderer, "luminous");
    ospSetVec3f(oMaterial, "color", diffusef[0], diffusef[1], diffusef[2]);
    ospSetFloat(oMaterial, "intensity", lum);
    return oMaterial;
  }

  if (pt_avail && property->GetMaterialName())
  {
    if (std::string("Value Indexed") == property->GetMaterialName())
    {
      vtkOSPRayMaterialHelpers::MakeMaterials(
        orn, oRenderer, mats); // todo: do an mtime check to avoid doing this when unchanged
      std::string requested_mat_name = materialName;
      if (!requested_mat_name.empty() && requested_mat_name != "Value Indexed")
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
  if (pt_avail && property->GetInterpolation() == VTK_PBR)
  {
    oMaterial = vtkOSPRayMaterialHelpers::NewMaterial(orn, oRenderer, "principled");

    ospSetVec3f(oMaterial, "baseColor", diffusef[0], diffusef[1], diffusef[2]);
    ospSetFloat(oMaterial, "metallic", static_cast<float>(property->GetMetallic()));
    ospSetFloat(oMaterial, "roughness", static_cast<float>(property->GetRoughness()));
    ospSetFloat(oMaterial, "opacity", static_cast<float>(opacity));
    // As OSPRay seems to not recalculate the refractive index of the base layer
    // we need to recalculate, from the effective reflectance of the base layer (with the
    // coat), the ior of the base that will produce the same reflectance but with the air
    // with an ior of 1.0
    double baseF0 = property->ComputeReflectanceOfBaseLayer();
    const double exteriorIor = 1.0;
    double baseIor = vtkProperty::ComputeIORFromReflectance(baseF0, exteriorIor);
    ospSetFloat(oMaterial, "ior", static_cast<float>(baseIor));
    float edgeColor[3] = { static_cast<float>(property->GetEdgeTint()[0]),
      static_cast<float>(property->GetEdgeTint()[1]),
      static_cast<float>(property->GetEdgeTint()[2]) };
    ospSetVec3f(oMaterial, "edgeColor", edgeColor[0], edgeColor[1], edgeColor[2]);
    ospSetFloat(oMaterial, "anisotropy", static_cast<float>(property->GetAnisotropy()));
    ospSetFloat(oMaterial, "rotation", static_cast<float>(property->GetAnisotropyRotation()));
    ospSetFloat(oMaterial, "baseNormalScale", static_cast<float>(property->GetNormalScale()));
    ospSetFloat(oMaterial, "coat", static_cast<float>(property->GetCoatStrength()));
    ospSetFloat(oMaterial, "coatIor", static_cast<float>(property->GetCoatIOR()));
    ospSetFloat(oMaterial, "coatRoughness", static_cast<float>(property->GetCoatRoughness()));
    float coatColor[] = { static_cast<float>(property->GetCoatColor()[0]),
      static_cast<float>(property->GetCoatColor()[1]),
      static_cast<float>(property->GetCoatColor()[2]) };
    ospSetVec3f(oMaterial, "coatColor", coatColor[0], coatColor[1], coatColor[2]);
    ospSetFloat(oMaterial, "coatNormal", static_cast<float>(property->GetCoatNormalScale()));
  }
  else
  {
    oMaterial = vtkOSPRayMaterialHelpers::NewMaterial(orn, oRenderer, "obj");

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

    ospSetVec3f(oMaterial, "ka", ambientf[0], ambientf[1], ambientf[2]);
    if (property->GetDiffuse() == 0.0)
    {
      // a workaround for ParaView, remove when ospray supports Ka
      ospSetVec3f(oMaterial, "kd", ambientf[0], ambientf[1], ambientf[2]);
    }
    else
    {
      ospSetVec3f(oMaterial, "kd", diffusef[0], diffusef[1], diffusef[2]);
    }
    ospSetVec3f(oMaterial, "ks", specularf[0], specularf[1], specularf[2]);
    ospSetFloat(oMaterial, "ns", specPower);
    ospSetFloat(oMaterial, "d", static_cast<float>(opacity));
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
vtkStandardNewMacro(vtkOSPRayPointGaussianMapperNode);

//------------------------------------------------------------------------------
vtkOSPRayPointGaussianMapperNode::vtkOSPRayPointGaussianMapperNode() {}

//------------------------------------------------------------------------------
vtkOSPRayPointGaussianMapperNode::~vtkOSPRayPointGaussianMapperNode() {}

//------------------------------------------------------------------------------
void vtkOSPRayPointGaussianMapperNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkOSPRayPointGaussianMapperNode::ORenderPoly(void* renderer, vtkOSPRayActorNode* aNode,
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
  if (numPositions == 0)
  {
    return;
  }
  std::vector<osp::vec3f> vertices(numPositions);
  for (size_t i = 0; i < numPositions; i++)
  {
    vertices[i] = osp::vec3f{ static_cast<float>(_vertices[i * 3 + 0]),
      static_cast<float>(_vertices[i * 3 + 1]), static_cast<float>(_vertices[i * 3 + 2]) };
  }
  OSPData position = ospNewCopyData1D(&vertices[0], OSP_VEC3F, numPositions);
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
  int scalingMode = vtkOSPRayActorNode::GetEnableScaling(act);
  double pointSize = length / 1000.0 * property->GetPointSize();
  double lineWidth = length / 1000.0 * property->GetLineWidth();
  if (scalingMode == vtkOSPRayActorNode::ALL_EXACT)
  {
    pointSize = property->GetPointSize();
    lineWidth = property->GetLineWidth();
  }
  // finer control over sphere and cylinders sizes
  vtkDataArray* scaleArray = nullptr;
  vtkPiecewiseFunction* scaleFunction = nullptr;
  if (mapper && scalingMode > vtkOSPRayActorNode::ALL_APPROXIMATE)
  {
    vtkInformation* mapInfo = mapper->GetInformation();
    char* scaleArrayName = (char*)mapInfo->Get(vtkOSPRayActorNode::SCALE_ARRAY_NAME());
    scaleArray = poly->GetPointData()->GetArray(scaleArrayName);
    if (scalingMode != vtkOSPRayActorNode::EACH_EXACT)
    {
      scaleFunction =
        vtkPiecewiseFunction::SafeDownCast(mapInfo->Get(vtkOSPRayActorNode::SCALE_FUNCTION()));
    }
  }

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

  if (vColors || (vColorCoordinates && pColorTextureMap))
  {
    // OSPRay scales the color mapping with the solid color but OpenGL backend does not do it.
    // set back to white to workaround this difference.
    std::fill(diffuseColor, diffuseColor + 3, 1.0);
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
  OSPMaterial oMaterial = vtkospn::MakeActorMaterial(orn, oRenderer, property, ambientColor,
    diffuseColor, specularf, opacity, pt_avail, useCustomMaterial, mats, materialName);
  ospCommit(oMaterial);
  uniqueMats.insert(oMaterial);

  // texture
  int numTextureCoordinates = 0;
  std::vector<osp::vec2f> textureCoordinates;
  if (vtkDataArray* da = poly->GetPointData()->GetTCoords())
  {
    numTextureCoordinates = da->GetNumberOfTuples();
    textureCoordinates.resize(numTextureCoordinates);
    for (int i = 0; i < numTextureCoordinates; i++)
    {
      textureCoordinates[i] = osp::vec2f(
        { static_cast<float>(da->GetTuple(i)[0]), static_cast<float>(da->GetTuple(i)[1]) });
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
  vtkImageData* vAnisotropyTextureMap = nullptr;
  vtkImageData* vCoatNormalTextureMap = nullptr;

  bool sRGB = false;

  if (texture)
  {
    sRGB = texture->GetUseSRGBColorSpace();
    vColorTextureMap = texture->GetInput();
    ospSetVec3f(oMaterial, "kd", 1.0f, 1.0f, 1.0f);
    ospCommit(oMaterial);
  }

  // colors from point and cell arrays
  int numCellMaterials = 0;
  std::vector<OSPMaterial> cellMaterials;
  int numPointColors = 0;
  std::vector<osp::vec4f> pointColors;
  int numPointValueTextureCoords = 0;
  std::vector<float> pointValueTextureCoords;
  if (vColors)
  {
    if (cellFlag == 2 && mapper->GetFieldDataTupleId() > -1)
    {
      // color comes from field data entry
      bool use_material = false;
      // check if the field data content says to use a material lookup
      vtkScalarsToColors* s2c = mapper->GetLookupTable();
      bool try_mats = s2c->GetIndexedLookup() && s2c->GetNumberOfAnnotatedValues() && !mats.empty();
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
        ospSetVec3f(oMaterial, "kd", fdiffusef[0], fdiffusef[1], fdiffusef[2]);
        ospCommit(oMaterial);
      }
    }
    else if (cellFlag == 1)
    {
      // color or material on cell
      vtkScalarsToColors* s2c = mapper->GetLookupTable();
      vtkospn::MakeCellMaterials(orn, oRenderer, poly, mapper, s2c, mats, cellMaterials, vColors,
        specularf, float(property->GetSpecularPower()), opacity);
      numCellMaterials = static_cast<int>(cellMaterials.size());
      for (OSPMaterial mat : cellMaterials)
      {
        uniqueMats.insert(mat);
      }
    }
    else if (cellFlag == 0)
    {
      // color on point interpolated RGB
      numPointColors = vColors->GetNumberOfTuples();
      pointColors.resize(numPointColors);
      for (int i = 0; i < numPointColors; i++)
      {
        unsigned char* color = vColors->GetPointer(4 * i);
        pointColors[i] = osp::vec4f{ color[0] / 255.0f, color[1] / 255.0f, color[2] / 255.0f,
          (color[3] / 255.0f) * static_cast<float>(opacity) };
      }
      ospSetVec3f(oMaterial, "kd", 1.0f, 1.0f, 1.0f);
      ospCommit(oMaterial);
    }
  }
  else
  {
    if (vColorCoordinates && pColorTextureMap)
    {
      // color on point interpolated values (subsequently colormapped via 1D LUT)
      numPointValueTextureCoords = vColorCoordinates->GetNumberOfTuples();
      pointValueTextureCoords.resize(numPointValueTextureCoords);
      float* tc = vColorCoordinates->GetPointer(0);
      for (int i = 0; i < numPointValueTextureCoords; i++)
      {
        float v = *tc;
        v = ((v >= 1.0f) ? 0.99999f : ((v < 0.0f) ? 0.0f : v)); // clamp [0..1)
        pointValueTextureCoords[i] = v;
        tc += 2;
      }
      vColorTextureMap = pColorTextureMap;
      ospSetVec3f(oMaterial, "kd", 1.0f, 1.0f, 1.0f);
      ospCommit(oMaterial);
    }
  }

  // create an ospray mesh for the vertex cells
  if (!conn.vertex_index.empty())
  {
    this->VolumetricModels.emplace_back(vtkospn::RenderAsSpheres(vertices.data(), conn.vertex_index,
      conn.vertex_reverse, pointSize, scaleArray, scaleFunction, useCustomMaterial, oMaterial,
      vColorTextureMap, sRGB, numTextureCoordinates, (float*)textureCoordinates.data(),
      numCellMaterials, cellMaterials, numPointColors, pointColors.data(),
      numPointValueTextureCoords, (float*)pointValueTextureCoords.data(), backend));
  }

  ospRelease(position);

  for (auto it : mats)
  {
    uniqueMats.insert(it.second);
  }

  for (OSPMaterial mat : uniqueMats)
  {
    ospRelease(mat);
  }

  for (auto g : this->VolumetricModels)
  {
    OSPGroup group = ospNewGroup();
    OSPData data = ospNewCopyData1D(&g, OSP_VOLUMETRIC_MODEL, 1);
    ospCommit(data);
    ospRelease(g);
    ospSetObject(group, "volume", data);
    ospCommit(group);
    ospRelease(data);
    OSPInstance instance = ospNewInstance(group); // valgrind reports instance is lost
    ospCommit(instance);
    ospRelease(group);
    this->Instances.emplace_back(instance);
  }

  this->VolumetricModels.clear();
}

//------------------------------------------------------------------------------
void vtkOSPRayPointGaussianMapperNode::Invalidate(bool prepass)
{
  if (prepass)
  {
    this->RenderTime = 0;
  }
}

//------------------------------------------------------------------------------
void vtkOSPRayPointGaussianMapperNode::Render(bool prepass)
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

    // if there are no changes, just reuse last result
    vtkMTimeType inTime = aNode->GetMTime();
    if (this->RenderTime >= inTime)
    {
      this->RenderVolumetricModels();
      return;
    }
    this->RenderTime = inTime;
    this->ClearVolumetricModels();

    vtkCompositeDataSet* input = nullptr;
    vtkPointGaussianMapper* mapper = vtkPointGaussianMapper::SafeDownCast(act->GetMapper());
    if (mapper && mapper->GetNumberOfInputPorts() > 0)
    {
      input = vtkCompositeDataSet::SafeDownCast(mapper->GetInputDataObject(0, 0));
    }
    if (input)
    {
      vtkNew<vtkDataObjectTreeIterator> iter;
      iter->SetDataSet(input);
      iter->SkipEmptyNodesOn();
      iter->VisitOnlyLeavesOn();
      vtkProperty* property = act->GetProperty();
      double ambient[3];
      double diffuse[3];
      property->GetAmbientColor(ambient);
      property->GetDiffuseColor(diffuse);
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        vtkPolyData* pd = vtkPolyData::SafeDownCast(iter->GetCurrentDataObject());
        if (!pd || !pd->GetPoints())
        {
          continue;
        }
        this->ORenderPoly(
          orn->GetORenderer(), aNode, pd, ambient, diffuse, property->GetOpacity(), "");
      }
    }
    this->RenderVolumetricModels();
  }
}

//----------------------------------------------------------------------------
void vtkOSPRayPointGaussianMapperNode::RenderVolumetricModels()
{
  vtkOSPRayRendererNode* orn =
    static_cast<vtkOSPRayRendererNode*>(this->GetFirstAncestorOfType("vtkOSPRayRendererNode"));

  for (auto instance : this->Instances)
  {
    orn->Instances.emplace_back(instance);
  }
}

//----------------------------------------------------------------------------
void vtkOSPRayPointGaussianMapperNode::ClearVolumetricModels()
{
  vtkOSPRayRendererNode* orn =
    static_cast<vtkOSPRayRendererNode*>(this->GetFirstAncestorOfType("vtkOSPRayRendererNode"));

  RTW::Backend* backend = orn->GetBackend();

  for (auto instance : this->Instances)
  {
    ospRelease(instance);
  }
  this->Instances.clear();
}
