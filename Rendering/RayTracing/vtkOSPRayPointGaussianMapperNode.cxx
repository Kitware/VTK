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

namespace vtkosp
{
//----------------------------------------------------------------------------
double GetComponent(double* tuple, int nComp, int c)
{
  // If this is a single component array, make sure we do not compute a useless magnitude
  if (nComp == 1)
  {
    c = 0;
  }

  // If we request a non-existing component, return the magnitude of the tuple
  double value = 0.0;
  if (c < 0 || c >= nComp)
  {
    for (int i = 0; i < nComp; ++i)
    {
      double tmp = tuple[i];
      value += tmp * tmp;
    }
    value = sqrt(value);
  }
  else
  {
    value = tuple[c];
  }
  return value;
}

//----------------------------------------------------------------------------
/**
 * Generate an interpolated table of values from a piecewise linear function.
 * The method populates the table, scale and offset based on the function and its range.
 */
void BuildTableFromFunction(
  vtkPiecewiseFunction* pwf, float* table, int size, double& scale, double& offset)
{
  double range[2];
  if (pwf)
  {
    // build the interpolation table
    pwf->GetRange(range);
    pwf->GetTable(range[0], range[1], size, table);
    // duplicate the last value for bilinear interp edge case
    table[size] = table[size - 1];
    scale = (size - 1.0) / (range[1] - range[0]);
    offset = range[0];
    std::cout << "scale build: " << scale << " Offset: " << offset << std::endl;
  }
}

//----------------------------------------------------------------------------
float GetScaledRadius(double radius, float* scaleTable, int scaleTableSize, double scaleScale,
  double scaleOffset, double scaleFactor, double triangleScale)
{
  if (scaleTable)
  {
    double tindex = (radius - scaleOffset) * scaleScale;
    int itindex = static_cast<int>(tindex);
    if (itindex >= scaleTableSize - 1)
    {
      radius = scaleTable[scaleTableSize - 1];
    }
    else if (itindex < 0)
    {
      radius = scaleTable[0];
    }
    else
    {
      radius = (1.0 - tindex + itindex) * scaleTable[itindex] +
        (tindex - itindex) * scaleTable[itindex + 1];
    }
  }

  radius *= scaleFactor;
  radius *= triangleScale;
  if (radius < 1e-3)
  {
    radius *= 1e2;
  }
  radius = radius < 1e-6 ? 1e-3 : radius;

  return static_cast<float>(radius);
}

//----------------------------------------------------------------------------
OSPVolumetricModel RenderAsParticles(osp::vec3f* vertices, std::vector<unsigned int>& indexArray,
  double pointSize, double scaleFactor, double triangleScale, vtkDataArray* scaleArray,
  int scaleArrayComponent, float* scaleTable, int scaleTableSize, double scaleScale,
  double scaleOffset, vtkDataArray* opacityArray, float* opacityTable, int opacityTableSize,
  double opacityScale, double opacityOffset, vtkDataArray* scalarArray, int numPointColors,
  osp::vec4f* PointColors, RTW::Backend* backend)
{
  if (backend == nullptr)
    return OSPVolumetricModel();
  OSPVolume ospMesh = ospNewVolume("particle");
  OSPVolumetricModel ospVolModel = ospNewVolumetricModel(ospMesh);

  size_t numParticles = indexArray.size();
  std::vector<osp::vec3f> vdata;
  std::vector<float> radii;
  vdata.reserve(indexArray.size());
  if (scaleArray != nullptr)
  {
    radii.reserve(indexArray.size());
  }

  std::vector<float> weights;
  if (opacityArray != nullptr)
  {
    weights.reserve(indexArray.size());
  }

  double wRange[2] = { 0.0, 1.0 };
  if (scalarArray)
  {
    scalarArray->GetFiniteRange(wRange);
  }

  for (size_t i = 0; i < numParticles; i++)
  {
    vdata.emplace_back(vertices[indexArray[i]]);
    double rDouble = pointSize;
    if (scaleArray)
    {
      rDouble = vtkosp::GetComponent(scaleArray->GetTuple(indexArray[i]),
        scaleArray->GetNumberOfComponents(), scaleArrayComponent);
    }
    float r = vtkosp::GetScaledRadius(
      rDouble, scaleTable, scaleTableSize, scaleScale, scaleOffset, scaleFactor, triangleScale);
    radii.emplace_back(r);

    float weight = 1.f;
    double wscale = 1.0;
    if (scalarArray != nullptr)
    {
      wscale = *(scalarArray->GetTuple(indexArray[i]));
      // oscale = *(opacityArray->GetTuple(indexArray[i]));
      // if (opacityFunction != nullptr)
      // {
      //   oscale = opacityFunction->GetValue(oscale);
      // }
    }
    weights.emplace_back(static_cast<float>(weight * wscale));
    std::cout << "radius: " << r << " weight: " << weight * wscale << std::endl;
    // weights.emplace_back(static_cast<float>(1.f));
    // radii.emplace_back(0.01f);
    //    if (scaleArray != nullptr)
    //    {
    //      radii.emplace_back(MapThroughPWF(*scaleArray->GetTuple(indexArray[i]), scaleFunction));
    //    }

    // weights.emplace_back(1.0f);
  }

  OSPData positionData = ospNewCopyData1D(vdata.data(), OSP_VEC3F, vdata.size());
  ospCommit(positionData);
  ospSetObject(ospMesh, "particle.position", positionData);
  OSPData radiiData = ospNewCopyData1D(radii.data(), OSP_FLOAT, radii.size());
  ospCommit(radiiData);
  ospSetObject(ospMesh, "particle.radius", radiiData);
  OSPData weightsData = ospNewCopyData1D(weights.data(), OSP_FLOAT, weights.size());
  ospCommit(weightsData);
  ospSetObject(ospMesh, "particle.weight", weightsData);

  float clampMaxCumulativeValue = 0.0f;
  ospSetFloat(ospMesh, "clampMaxCumulativeValue", clampMaxCumulativeValue);

  float radiusSupportFactor = static_cast<float>(4.f);
  ospSetFloat(ospMesh, "radiusSupportFactor", radiusSupportFactor);

  // colors
  OSPData _PointColors = nullptr;
  std::vector<float> tfOVals;
  OSPData _AlphaData = nullptr;
  if (numPointColors)
  {
    std::vector<osp::vec3f> perPointColors;
    for (size_t i = 0; i < numParticles; i++)
    {
      unsigned int ii = indexArray[i];
      osp::vec3f p = { PointColors[ii].x, PointColors[ii].y, PointColors[ii].z };
      perPointColors.push_back(p);
      // tfOVals.push_back(PointColors[ii].w);
    }
    _PointColors = ospNewCopyData1D(&perPointColors[0], OSP_VEC3F, numParticles);
    // _AlphaData = ospNewCopyData1D(&tfOVals[0], OSP_FLOAT, numParticles);
    tfOVals.emplace_back(0.f);
    tfOVals.emplace_back(1.f);
    _AlphaData = ospNewCopyData1D(&tfOVals[0], OSP_FLOAT, 2);
  }

  if (numPointColors == 0)
  {
    std::vector<osp::vec3f> perPointColors;
    // perPointColors.emplace_back(osp::vec3f({ 0.0f, 0.0f, 0.0f }));
    // perPointColors.emplace_back(osp::vec3f({ 1.0f, 1.0f, 1.0f }));
    perPointColors.emplace_back(osp::vec3f({ 0, 0, 0.562493 }));
    perPointColors.emplace_back(osp::vec3f({ 0, 0, 1 }));
    perPointColors.emplace_back(osp::vec3f({ 0, 1, 1 }));
    perPointColors.emplace_back(osp::vec3f({ 0.500008, 1, 0.500008 }));
    perPointColors.emplace_back(osp::vec3f({ 1, 1, 0 }));
    perPointColors.emplace_back(osp::vec3f({ 1, 0, 0 }));
    perPointColors.emplace_back(osp::vec3f({ 0.500008, 0, 0 }));
    _PointColors = ospNewCopyData1D(&perPointColors[0], OSP_VEC3F, 7);
    tfOVals.emplace_back(0.f);
    tfOVals.emplace_back(1.f);
    _AlphaData = ospNewCopyData1D(&tfOVals[0], OSP_FLOAT, 2);
  }

  auto oTF = ospNewTransferFunction("piecewiseLinear");
  ospSetObject(oTF, "color", _PointColors);
  ospSetObject(oTF, "opacity", _AlphaData);
  ospSetVec2f(oTF, "valueRange", static_cast<float>(wRange[0]), static_cast<float>(wRange[1]));
  // oTF, "valueRange", 0.0f, 1.0f);
  ospCommit(oTF);
  ospSetObject(ospVolModel, "transferFunction", oTF);
  ospCommit(ospMesh);
  ospRelease(positionData);
  ospRelease(radiiData);
  ospRelease(weightsData);
  ospCommit(ospVolModel);
  ospRelease(ospMesh);
  ospRelease(oTF);

  return ospVolModel;
}
}

//============================================================================
vtkStandardNewMacro(vtkOSPRayPointGaussianMapperNode);

//------------------------------------------------------------------------------
vtkOSPRayPointGaussianMapperNode::vtkOSPRayPointGaussianMapperNode() {}

//------------------------------------------------------------------------------
vtkOSPRayPointGaussianMapperNode::~vtkOSPRayPointGaussianMapperNode()
{
  if (this->ScaleTable)
  {
    delete[] this->ScaleTable;
    this->ScaleTable = nullptr;
  }

  if (this->OpacityTable)
  {
    delete[] this->OpacityTable;
    this->OpacityTable = nullptr;
  }
}

//------------------------------------------------------------------------------
void vtkOSPRayPointGaussianMapperNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkOSPRayPointGaussianMapperNode::InternalRender(void* vtkNotUsed(renderer),
  vtkOSPRayActorNode* aNode, vtkPolyData* poly, double* diffuseColor, double opacity,
  std::string materialName)
{
  vtkOSPRayRendererNode* orn =
    static_cast<vtkOSPRayRendererNode*>(this->GetFirstAncestorOfType("vtkOSPRayRendererNode"));
  RTW::Backend* backend = orn->GetBackend();
  if (backend == nullptr)
    return;

  vtkActor* act = vtkActor::SafeDownCast(aNode->GetRenderable());
  vtkProperty* property = act->GetProperty();

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

  // choosing particle radius that approximates pointsize
  vtkPointGaussianMapper* mapper = vtkPointGaussianMapper::SafeDownCast(act->GetMapper());
  if (!mapper)
  {
    // If not point gaussian mapper, no need to render particles.
    return;
  }

  // finer control over particle sizes
  double length = 1.0;
  if (mapper)
  {
    length = mapper->GetLength();
  }
  int scalingMode = vtkOSPRayActorNode::GetEnableScaling(act);
  double pointSize = (length / 1000.0) * property->GetPointSize();
  if (scalingMode == vtkOSPRayActorNode::ALL_EXACT)
  {
    pointSize = property->GetPointSize();
  }

  vtkDataArray* scaleArray = nullptr;
  if (mapper->GetScaleArray())
  {
    scaleArray = poly->GetPointData()->GetArray(mapper->GetScaleArray());
  }

  // update scale table
  if (mapper->GetScaleFunction() && scaleArray)
  {
    if (this->ScaleTableUpdateTime < mapper->GetScaleFunction()->GetMTime() ||
      this->ScaleTableUpdateTime < this->RenderTime)
    {
      if (this->ScaleTable)
      {
        delete[] this->ScaleTable;
        this->ScaleTable = nullptr;
      }
      this->ScaleTable = new float[this->ScaleTableSize + 1];
      std::cout << "this->ScaleScale: " << this->ScaleScale
                << " this->ScaleOffset: " << this->ScaleOffset << std::endl;
      vtkosp::BuildTableFromFunction(mapper->GetScaleFunction(), this->ScaleTable,
        this->ScaleTableSize, this->ScaleScale, this->ScaleOffset);
      std::cout << "this->ScaleScale: " << this->ScaleScale
                << " this->ScaleOffset: " << this->ScaleOffset << std::endl;
      this->ScaleTableUpdateTime.Modified();
    }
  }
  else
  {
    delete[] this->ScaleTable;
    this->ScaleTable = nullptr;
  }

  vtkDataArray* opacityArray = nullptr;
  if (mapper->GetOpacityArray())
  {
    opacityArray = poly->GetPointData()->GetArray(mapper->GetOpacityArray());
  }
  if (mapper->GetScalarOpacityFunction() && opacityArray)
  {
    if (this->OpacityTableUpdateTime < mapper->GetScalarOpacityFunction()->GetMTime() ||
      this->OpacityTableUpdateTime < this->RenderTime)
    {
      if (this->OpacityTable)
      {
        delete[] this->OpacityTable;
        this->OpacityTable = nullptr;
      }
      this->OpacityTable = new float[this->OpacityTableSize + 1];
      vtkosp::BuildTableFromFunction(mapper->GetScalarOpacityFunction(), this->OpacityTable,
        this->OpacityTableSize, this->OpacityScale, this->OpacityOffset);
      this->OpacityTableUpdateTime.Modified();
    }
  }
  else
  {
    delete[] this->OpacityTable;
    this->OpacityTable = nullptr;
  }

  int cflag = -1;
  vtkDataArray* scalarArray =
    vtkDataArray::SafeDownCast(mapper->GetAbstractScalars(poly, mapper->GetScalarMode(),
      mapper->GetArrayAccessMode(), mapper->GetArrayId(), mapper->GetArrayName(), cflag));

  // now ask mapper to do most of the work and provide us with
  // colors and/or texture coordinates per point
  vtkUnsignedCharArray* vColors = nullptr;
  int cellFlag = -1; // mapper tells us which
  if (mapper)
  {
    mapper->MapScalars(poly, 1.0, cellFlag);
    vColors = mapper->GetColorMapColors();
  }

  if (vColors)
  {
    // OSPRay scales the color mapping with the solid color but OpenGL backend does not do it.
    // set back to white to workaround this difference.
    std::fill(diffuseColor, diffuseColor + 3, 1.0);
  }

  // colors from point and cell arrays
  int numPointColors = 0;
  std::vector<osp::vec4f> pointColors;
  if (vColors)
  {
    if (cellFlag == 0)
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
    }
  }

  double scaleFactor = mapper->GetScaleFactor();
  double triangleScale = mapper->GetTriangleScale();
  std::cout << "Scale Factor: " << scaleFactor << " TriangleScale " << triangleScale << std::endl;
  //  if (scalingMode != vtkOSPRayActorNode::ALL_EXACT)
  //  {
  //    scaleFactor *= 500 * length;
  //  }
  // create an ospray mesh for the vertex cells
  // if (!conn.vertex_index.empty())
  // {
  this->VolumetricModels.emplace_back(vtkosp::RenderAsParticles(vertices.data(), conn.vertex_index,
    pointSize, scaleFactor, triangleScale, scaleArray, mapper->GetScaleArrayComponent(),
    this->ScaleTable, this->ScaleTableSize, this->ScaleScale, this->ScaleOffset, opacityArray,
    this->OpacityTable, this->OpacityTableSize, this->OpacityScale, this->OpacityOffset,
    scalarArray, numPointColors, pointColors.data(), backend));
  // }

  ospRelease(position);

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
      double diffuse[3];
      property->GetDiffuseColor(diffuse);
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        vtkPolyData* pd = vtkPolyData::SafeDownCast(iter->GetCurrentDataObject());
        if (!pd || !pd->GetPoints())
        {
          continue;
        }
        this->InternalRender(orn->GetORenderer(), aNode, pd, diffuse, property->GetOpacity(), "");
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
