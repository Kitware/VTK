// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAnariGlyph3DMapperNode
 * @brief   A Glyph mapper node for ANARI (ANAlytic Rendering Interface).
 *
 *
 * ANARI provides cross-vendor portability to diverse rendering engines,
 * including those using state-of-the-art ray tracing. This is the Glyph
 * Mapper node class, which is the ANARI equivalent of the vtkGlyph3DMapper
 * for glyphs. It is built on top of the vtkAnariCompositePolyDataMapperNode
 * to reuse existing composite structure traversal and point/mesh rendering
 * capabilities of ANARI.
 *
 * @par Thanks:
 * Kees van Kooten kvankooten@nvidia.com for creating and contributing the class
 * and NVIDIA for supporting this work.
 *
 */

#include "vtkAnariGlyph3DMapperNode.h"

#include "vtkActor.h"
#include "vtkAlgorithmOutput.h"
#include "vtkAnariPolyDataMapperInheritInterface.h"
#include "vtkArrowSource.h"
#include "vtkBitArray.h"
#include "vtkConeSource.h"
#include "vtkCubeSource.h"
#include "vtkCylinderSource.h"
#include "vtkExecutive.h"
#include "vtkGlyph3DMapper.h"
#include "vtkGlyphSource2D.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkLineSource.h"
#include "vtkMapper.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkQuaternion.h"
#include "vtkSphereSource.h"

VTK_ABI_NAMESPACE_BEGIN

namespace vtkAnariMath
{
void RotationToQuaternion(float* rot, float* quat)
{
  vtkQuaternionf resultQuat;

  float angle = vtkMath::RadiansFromDegrees(rot[2]);
  vtkQuaternionf qz(cos(0.5f * angle), 0.0f, 0.0f, sin(0.5f * angle));

  angle = vtkMath::RadiansFromDegrees(rot[0]);
  vtkQuaternionf qx(cos(0.5f * angle), sin(0.5f * angle), 0.0f, 0.0f);

  angle = vtkMath::RadiansFromDegrees(rot[1]);
  vtkQuaternionf qy(cos(0.5f * angle), 0.0f, sin(0.5f * angle), 0.0f);

  resultQuat = qz * qx * qy;

  resultQuat.Get(quat);
}

void DirectionToQuaternionX(float* dir, float dirLength, float* quat)
{
  // Use X axis of glyph to orient along.

  if (dirLength == 0)
  { // identity rotation
    quat[0] = 1;
    quat[1] = 0;
    quat[2] = 0;
    quat[3] = 0;
    return;
  }

  float invDirLength = 1.0f / dirLength;
  float halfVec[3] = { dir[0] * invDirLength + 1.0f, dir[1] * invDirLength, dir[2] * invDirLength };
  float halfNorm = vtkMath::Normalize(halfVec);
  float sinAxis[3] = { 0.0, -halfVec[2], halfVec[1] };
  float cosAngle = halfVec[0];
  if (halfNorm == 0.0f)
  {
    sinAxis[2] = 1.0f; // sinAxis*sin(pi/2) = (0,0,1)*sin(pi/2) = (0,0,1)
    // cosAngle = cos(pi/2) = 0;
  }

  quat[0] = cosAngle;
  quat[1] = sinAxis[0];
  quat[2] = sinAxis[1];
  quat[3] = sinAxis[2];
}

void DirectionToQuaternionY(float* dir, float dirLength, float* quat)
{
  // Use Y axis of glyph to orient along.

  if (dirLength == 0)
  { // identity rotation
    quat[0] = 1;
    quat[1] = 0;
    quat[2] = 0;
    quat[3] = 0;
    return;
  }

  // (dot(|segDir|, yAxis), cross(|segDir|, yAxis)) gives (cos(th), axis*sin(th)),
  // but rotation is represented by cos(th/2), axis*sin(th/2), ie. half the amount of rotation.
  // So calculate (dot(|halfVec|, yAxis), cross(|halfVec|, yAxis)) instead, with halfvec =
  // |segDir|+yAxis.
  float invDirLength = 1.0f / dirLength;
  float halfVec[3] = { dir[0] * invDirLength, dir[1] * invDirLength + 1.0f, dir[2] * invDirLength };
  float halfNorm = vtkMath::Normalize(halfVec); // Normalizes halfVec and returns length

  // Cross yAxis (0,1,0) with halfVec (new Y axis) to get rotation axis * sin(angle/2)
  float sinAxis[3] = { halfVec[2], 0.0f, -halfVec[0] };
  // Dot for cos(angle/2)
  float cosAngle = halfVec[1];
  if (halfNorm == 0.0f) // In this case there is a 180 degree rotation (sinAxis==(0,0,0))
  {
    sinAxis[2] = 1.0f; // sinAxis*sin(pi/2) = (0,0,1)*sin(pi/2) = (0,0,1)
    // cosAngle = cos(pi/2) = 0;
  }

  quat[0] = cosAngle;
  quat[1] = sinAxis[0];
  quat[2] = sinAxis[1];
  quat[3] = sinAxis[2];
}
}

// Gain access to a few required protected functions
class vtkAnariGlyph3DMapper : public vtkGlyph3DMapper
{
public:
  vtkTypeMacro(vtkAnariGlyph3DMapper, vtkGlyph3DMapper);
  static vtkAnariGlyph3DMapper* New();
  void Render(vtkRenderer*, vtkActor*) override {}

  vtkDataArray* GetOrientations(vtkDataSet* input) { return this->GetOrientationArray(input); }
  vtkDataArray* GetScales(vtkDataSet* input) { return this->GetScaleArray(input); }
  vtkDataArray* GetMasks(vtkDataSet* input) { return this->GetMaskArray(input); }
};

vtkStandardNewMacro(vtkAnariGlyph3DMapper);

//============================================================================
class vtkAnariGlyph3DMapperNodeInternals
{
public:
  vtkAnariGlyph3DMapperNodeInternals(vtkAnariGlyph3DMapperNode* owner);
  ~vtkAnariGlyph3DMapperNodeInternals();

  void UpdateGlyphs();

  enum GlyphShape
  {
    SHAPE_SPHERE,
    SHAPE_CYLINDER,
    SHAPE_CONE,
    SHAPE_CUBE,
    SHAPE_ARROW,
    SHAPE_EXTERNAL
  };

  vtkAnariGlyph3DMapperNode* Owner{ nullptr };

  GlyphShape CurrentGlyphShape = SHAPE_SPHERE;
  double CurrentGlyphDims[3] = { 0.5, 0.5, 0.5 };
};

//----------------------------------------------------------------------------
vtkAnariGlyph3DMapperNodeInternals::vtkAnariGlyph3DMapperNodeInternals(
  vtkAnariGlyph3DMapperNode* owner)
  : Owner(owner)
{
}

//----------------------------------------------------------------------------
vtkAnariGlyph3DMapperNodeInternals::~vtkAnariGlyph3DMapperNodeInternals() {}

//----------------------------------------------------------------------------
void vtkAnariGlyph3DMapperNodeInternals::UpdateGlyphs()
{
  // Set the glyph shape
  GlyphShape newGlyphShape = SHAPE_SPHERE;
  double scale[3] = { 0.5, 0.5, 0.5 };

  vtkGlyph3DMapper* mapper = vtkGlyph3DMapper::SafeDownCast(this->Owner->GetRenderable());
  if (mapper)
  {
    if (vtkAlgorithmOutput* algOut = mapper->GetInputConnection(1, 0))
    {
      vtkAlgorithm* sourceAlg = algOut->GetProducer();

      if (vtkConeSource* cone = vtkConeSource::SafeDownCast(sourceAlg))
      {
        newGlyphShape = SHAPE_CONE;

        scale[0] = cone->GetHeight() * 0.5;
        scale[1] = cone->GetRadius();
      }
      else if (vtkCylinderSource* cyl = vtkCylinderSource::SafeDownCast(sourceAlg))
      {
        newGlyphShape = SHAPE_CYLINDER;

        scale[0] = cyl->GetHeight() * 0.5;
        scale[1] = cyl->GetRadius();
      }
      else if (vtkCubeSource* cube = vtkCubeSource::SafeDownCast(sourceAlg))
      {
        newGlyphShape = SHAPE_CUBE;

        scale[0] = cube->GetXLength() * 0.5;
        scale[1] = cube->GetYLength() * 0.5;
        scale[2] = cube->GetZLength() * 0.5;
      }
      else if (vtkArrowSource* arrow = vtkArrowSource::SafeDownCast(sourceAlg))
      {
        newGlyphShape = SHAPE_ARROW;

        scale[0] = arrow->GetTipLength();
        scale[1] = arrow->GetShaftRadius();
        scale[2] = arrow->GetTipRadius();
      }
      else if (/*vtkLineSource* line =*/vtkLineSource::SafeDownCast(sourceAlg))
      {
        newGlyphShape = SHAPE_ARROW;

        scale[0] = 0;
        scale[1] = 0.01;
        scale[2] = 0.01;
      }
      else if (vtkGlyphSource2D* glyph = vtkGlyphSource2D::SafeDownCast(sourceAlg))
      {
        newGlyphShape = SHAPE_ARROW;

        scale[0] = glyph->GetTipLength();
        scale[1] = 0.01;
        scale[2] = 0.03;
      }
      else if (vtkSphereSource* sphere = vtkSphereSource::SafeDownCast(sourceAlg))
      {
        scale[0] = sphere->GetRadius();
      }
    }
  }

  if (this->CurrentGlyphShape != newGlyphShape)
  {
    this->CurrentGlyphShape = newGlyphShape;
  }

  if (memcmp(this->CurrentGlyphDims, scale, sizeof(scale)))
  {
    memcpy(this->CurrentGlyphDims, scale, sizeof(scale));
  }
}

//============================================================================
class vtkAnariGlyph3DMapperInheritInterface : public vtkAnariPolyDataMapperInheritInterface
{
public:
  vtkAnariGlyph3DMapperInheritInterface(vtkAnariGlyph3DMapperNodeInternals* mapperInternals)
    : Internal(mapperInternals)
  {
  }

  void SetDevice(anari::Device& device, anari::Extensions& extensions,
    const char* const* anariExtensionStrings) override;

  int GetSurfaceRepresentation(vtkProperty* property) const override;
  ParameterFlags GetBaseUpdateResponsibility() const override;

  anari::Geometry InitializeSpheres(vtkPolyData* polyData, vtkProperty* property,
    std::vector<vec3>& vertices, std::vector<uint32_t>& indexArray, double pointSize,
    vtkDataArray* radiusArray, vtkPiecewiseFunction* scaleFunction,
    std::vector<vec2>& textureCoords, std::vector<float>& pointValueTextureCoords,
    std::vector<vec4>& pointColors, int cellFlag) override;

  const char* GetSpheresPostfix() const override;

  void SetIndexArray(
    anari::Geometry& glyphGeometry, std::vector<uint32_t>& indexArray, size_t numPoints);
  void SetGlyphOrientArray(anari::Geometry& glyphGeometry, std::vector<float>& orients);
  void SetGlyphScaleArray(anari::Geometry& glyphGeometry, std::vector<float>& scales);
  void SetGlyphScale(anari::Geometry& glyphGeometry, float scaleFactor);
  void SetGlyphIdArray(anari::Geometry& glyphGeometry, std::vector<uint32_t>& pointIds);

  vtkAnariGlyph3DMapperNodeInternals* Internal;
  bool SupportsGlyphExtension = false;
};

//----------------------------------------------------------------------------
void vtkAnariGlyph3DMapperInheritInterface::SetDevice(
  anari::Device& device, anari::Extensions& extensions, const char* const* anariExtensionStrings)
{
  if (anariExtensionStrings != nullptr)
  {
    for (int i = 0; anariExtensionStrings[i] != nullptr; ++i)
    {
      if (strcmp(anariExtensionStrings[i], "ANARI_KHR_GEOMETRY_GLYPH") == 0)
      {
        this->SupportsGlyphExtension = true;
        break;
      }
    }
  }
  vtkAnariPolyDataMapperInheritInterface::SetDevice(device, extensions, anariExtensionStrings);
}

//----------------------------------------------------------------------------
int vtkAnariGlyph3DMapperInheritInterface::GetSurfaceRepresentation(vtkProperty*) const
{
  return VTK_POINTS;
}

//----------------------------------------------------------------------------
vtkAnariPolyDataMapperInheritInterface::ParameterFlags
vtkAnariGlyph3DMapperInheritInterface::GetBaseUpdateResponsibility() const
{
  ParameterFlags flags;
  flags.Indices = false;
  flags.Scales = false;

  return flags;
}

//----------------------------------------------------------------------------
anari::Geometry vtkAnariGlyph3DMapperInheritInterface::InitializeSpheres(vtkPolyData* polyData,
  vtkProperty* property, std::vector<vec3>& vertices, std::vector<uint32_t>& indexArray,
  double pointSize, vtkDataArray* radiusArray, vtkPiecewiseFunction* scaleFunction,
  std::vector<vec2>& textureCoords, std::vector<float>& pointValueTextureCoords,
  std::vector<vec4>& pointColors, int cellFlag)
{
  using MapperInternals = vtkAnariGlyph3DMapperNodeInternals;

  anari::Geometry glyphGeometry = nullptr;

  if (this->SupportsGlyphExtension)
  {
    glyphGeometry = anari::newObject<anari::Geometry>(this->AnariDevice, "glyph");

    const char* shapeType = "sphere";
    switch (this->Internal->CurrentGlyphShape)
    {
      case MapperInternals::SHAPE_CONE:
        shapeType = "cone";
        break;
      case MapperInternals::SHAPE_CYLINDER:
        shapeType = "cylinder";
        break;
      case MapperInternals::SHAPE_CUBE:
        break;
      case MapperInternals::SHAPE_ARROW:
        shapeType = "cone";
        break;
      default:
        break;
    }
    anari::setParameter(this->AnariDevice, glyphGeometry, "shapeType", shapeType);

    float scaleX = static_cast<float>(this->Internal->CurrentGlyphDims[0]);
    float scaleY = static_cast<float>(this->Internal->CurrentGlyphDims[1]);
    float scaleZ = static_cast<float>(this->Internal->CurrentGlyphDims[2]);
    // VTK shapes are aligned along the x axis, whereas ANARI aligns along z,
    // so a ccw 90 degree rotation along Y is required for ANARI to mimick VTK.
    float shapeTransform[16] = { 0.0, 0.0, -scaleZ, 0.0, 0.0, scaleY, 0.0, 0.0, scaleX, 0.0, 0.0,
      0.0, 0.0, 0.0, 0.0, 1.0 };
    anari::setParameter(
      this->AnariDevice, glyphGeometry, "shapeTransform", ANARI_FLOAT32_MAT4, shapeTransform);

    size_t numPoints = vertices.size();
    size_t numIndices = indexArray.size();

    vtkGlyph3DMapper* glm = vtkGlyph3DMapper::SafeDownCast(this->Internal->Owner->GetRenderable());
    vtkAnariGlyph3DMapper* glyphMapper = reinterpret_cast<vtkAnariGlyph3DMapper*>(glm);

    // Remove duplicate point entries from the index array
    SetIndexArray(glyphGeometry, indexArray, numPoints);

    if (glyphMapper->GetOrient())
    {
      vtkDataArray* orientArray = glyphMapper->GetOrientations(polyData);
      if (orientArray != nullptr &&
        static_cast<size_t>(orientArray->GetNumberOfTuples()) == numPoints)
      {
        std::vector<float> tempOrients(numPoints * 4);
        float* quatOut = tempOrients.data();

        int orientMode = glyphMapper->GetOrientationMode();
        if (orientMode == vtkGlyph3DMapper::DIRECTION && orientArray->GetNumberOfComponents() == 3)
        {
          auto dirToQuaternionF = vtkAnariMath::DirectionToQuaternionX;
          if (orientArray->GetDataType() == VTK_FLOAT)
          {
            float* dirIn = reinterpret_cast<float*>(orientArray->GetVoidPointer(0));
            for (uint64_t ptIdx = 0; ptIdx < numPoints; ++ptIdx, dirIn += 3)
            {
              float dirLength = vtkMath::Norm(dirIn);
              dirToQuaternionF(dirIn, dirLength, quatOut + ptIdx * 4);
            }
          }
          else
          {
            for (uint64_t ptIdx = 0; ptIdx < numPoints; ++ptIdx)
            {
              double dirInD[3];
              orientArray->GetTuple(ptIdx, dirInD);

              float dirIn[3] = { (float)dirInD[0], (float)dirInD[1], (float)dirInD[2] };
              float dirLength = vtkMath::Norm(dirIn);
              dirToQuaternionF(dirIn, dirLength, quatOut + ptIdx * 4);
            }
          }
        }
        else if (orientMode == vtkGlyph3DMapper::ROTATION &&
          orientArray->GetNumberOfComponents() == 3)
        {
          if (orientArray->GetDataType() == VTK_FLOAT)
          {
            float* rotIn = reinterpret_cast<float*>(orientArray->GetVoidPointer(0));
            for (uint64_t ptIdx = 0; ptIdx < numPoints; ++ptIdx, rotIn += 3)
            {
              vtkAnariMath::RotationToQuaternion(rotIn, quatOut + ptIdx * 4);
            }
          }
          else
          {
            for (uint64_t ptIdx = 0; ptIdx < numPoints; ++ptIdx)
            {
              double rotInD[3];
              orientArray->GetTuple(ptIdx, rotInD);

              float rotIn[3] = { (float)rotInD[0], (float)rotInD[1], (float)rotInD[2] };
              vtkAnariMath::RotationToQuaternion(rotIn, quatOut + ptIdx * 4);
            }
          }
        }
        else if (orientMode == vtkGlyph3DMapper::QUATERNION &&
          orientArray->GetNumberOfComponents() == 4)
        {
          if (orientArray->GetDataType() == VTK_FLOAT)
          {
            memcpy(quatOut, reinterpret_cast<float*>(orientArray->GetVoidPointer(0)),
              numPoints * 4 * sizeof(float));
          }
          else
          {
            for (uint64_t ptIdx = 0; ptIdx < numPoints; ++ptIdx)
            {
              vtkQuaterniond quat;
              orientArray->GetTuple(ptIdx, quat.GetData());

              vtkQuaternionf qRot(quat[0], quat[1], quat[2], quat[3]);

              qRot.Get(quatOut + ptIdx * 4);
            }
          }
        }

        SetGlyphOrientArray(glyphGeometry, tempOrients);
      }
    }

    if (glyphMapper->GetScaling())
    {
      vtkDataArray* scaleArray = glyphMapper->GetScales(polyData);

      bool scaleTypeCorrect = scaleArray != nullptr &&
        (glyphMapper->GetScaleMode() != vtkGlyph3DMapper::SCALE_BY_COMPONENTS ||
          scaleArray->GetNumberOfComponents() == 3);

      if (scaleTypeCorrect && static_cast<size_t>(scaleArray->GetNumberOfTuples()) == numPoints)
      {
        std::vector<float> tempScales(numPoints * 3);

        double* range = glyphMapper->GetRange();
        double den = range[1] - range[0];
        if (den == 0.0)
          den = 1.0;

        for (uint64_t ptIdx = 0; ptIdx < numPoints; ++ptIdx)
        {
          double scalex = 1.0f; // Scale of the Anari glyph at PV size 1
          double scaley = 1.0f;
          double scalez = 1.0f;

          double* tuple = scaleArray->GetTuple(ptIdx);
          switch (glyphMapper->GetScaleMode())
          {
            case vtkGlyph3DMapper::SCALE_BY_MAGNITUDE:
              scalex = scaley = scalez *= vtkMath::Norm(tuple, scaleArray->GetNumberOfComponents());
              break;
            case vtkGlyph3DMapper::SCALE_BY_COMPONENTS:
              scalex *= tuple[0];
              scaley *= tuple[1];
              scalez *= tuple[2];
              break;
            case vtkGlyph3DMapper::NO_DATA_SCALING:
            default:
              break;
          }

          // Clamp data scale if enabled
          if (glyphMapper->GetClamping() &&
            glyphMapper->GetScaleMode() != vtkGlyph3DMapper::NO_DATA_SCALING)
          {
            scalex = (scalex < range[0] ? range[0] : (scalex > range[1] ? range[1] : scalex));
            scalex = (scalex - range[0]) / den;
            scaley = (scaley < range[0] ? range[0] : (scaley > range[1] ? range[1] : scaley));
            scaley = (scaley - range[0]) / den;
            scalez = (scalez < range[0] ? range[0] : (scalez > range[1] ? range[1] : scalez));
            scalez = (scalez - range[0]) / den;
          }

          float scaleFac = glyphMapper->GetScaleFactor();
          scalex *= scaleFac;
          scaley *= scaleFac;
          scalez *= scaleFac;

          tempScales[ptIdx * 3] = scalex;
          tempScales[ptIdx * 3 + 1] = scaley;
          tempScales[ptIdx * 3 + 2] = scalez;
        }

        SetGlyphScaleArray(glyphGeometry, tempScales);
      }
      else if (glyphMapper->GetScaling())
      {
        float scaleFac = glyphMapper->GetScaleFactor();
        SetGlyphScale(glyphGeometry, scaleFac);
      }
    }

    if (glyphMapper->GetMasking())
    {
      vtkBitArray* maskArray = vtkArrayDownCast<vtkBitArray>(glyphMapper->GetMasks(polyData));
      if (maskArray && static_cast<size_t>(maskArray->GetNumberOfTuples()) == numPoints &&
        maskArray->GetNumberOfComponents() == 1)
      {
        std::vector<uint32_t> tempPointIds(numIndices);

        for (size_t indexIdx = 0; indexIdx < numIndices; ++indexIdx)
        {
          uint32_t ptIdx = indexArray[indexIdx];
          if (maskArray->GetValue(ptIdx) == 0)
          {
            tempPointIds[indexIdx] = static_cast<uint32_t>(-1);
          }
          else
          {
            tempPointIds[indexIdx] = ptIdx;
          }
        }

        SetGlyphIdArray(glyphGeometry, tempPointIds);
      }
    }
  }
  else
  {
    // Regular case not supported yet. A prototype geom has to be created with a list of transforms
    // incorporating the above scales and orientations with regular positions, which then has to be
    // set on an appropriate ANARI object.
    glyphGeometry = vtkAnariPolyDataMapperInheritInterface::InitializeSpheres(polyData, property,
      vertices, indexArray, pointSize, radiusArray, scaleFunction, textureCoords,
      pointValueTextureCoords, pointColors, cellFlag);
  }

  return glyphGeometry;
}

//----------------------------------------------------------------------------
const char* vtkAnariGlyph3DMapperInheritInterface::GetSpheresPostfix() const
{
  return "_glyphs_";
}

//----------------------------------------------------------------------------
void vtkAnariGlyph3DMapperInheritInterface::SetIndexArray(
  anari::Geometry& glyphGeometry, std::vector<uint32_t>& indexArray, size_t numPoints)
{
  if (indexArray.size() > 0)
  {
    // Remove duplicate entries from the index array
    std::vector<bool> pointArray(numPoints, false);
    size_t newNumIndices = 0;
    for (uint32_t idx : indexArray)
    {
      pointArray[idx] = true;
    }
    for (bool point : pointArray)
    {
      if (point)
        ++newNumIndices;
    }

    auto indicesArray = anari::newArray1D(this->AnariDevice, ANARI_UINT32, newNumIndices);
    {
      auto indicesArrayPtr = anari::map<uint32_t>(this->AnariDevice, indicesArray);

      size_t currAnariIdx = 0;
      for (size_t i = 0; i < pointArray.size(); i++)
      {
        if (pointArray[i])
          indicesArrayPtr[currAnariIdx++] = static_cast<uint32_t>(i);
      }

      anari::unmap(this->AnariDevice, indicesArray);
    }

    anari::setAndReleaseParameter(
      this->AnariDevice, glyphGeometry, "primitive.index", indicesArray);
  }
}

//----------------------------------------------------------------------------
void vtkAnariGlyph3DMapperInheritInterface::SetGlyphOrientArray(
  anari::Geometry& glyphGeometry, std::vector<float>& orients)
{
  size_t numOrients = orients.size() / 4;

  auto orientsArray = anari::newArray1D(this->AnariDevice, ANARI_FLOAT32_QUAT_IJKW, numOrients);
  {
    auto arrayPtr = anari::map<float>(this->AnariDevice, orientsArray);

    // ANARI expects the scalar in the third component
    for (size_t i = 0; i < numOrients; ++i)
    {
      arrayPtr[i * 4] = orients[i * 4 + 1];
      arrayPtr[i * 4 + 1] = orients[i * 4 + 2];
      arrayPtr[i * 4 + 2] = orients[i * 4 + 3];
      arrayPtr[i * 4 + 3] = orients[i * 4];
    }

    anari::unmap(this->AnariDevice, orientsArray);
  }

  anari::setAndReleaseParameter(
    this->AnariDevice, glyphGeometry, "vertex.orientation", orientsArray);
}

//----------------------------------------------------------------------------
void vtkAnariGlyph3DMapperInheritInterface::SetGlyphScaleArray(
  anari::Geometry& glyphGeometry, std::vector<float>& scales)
{
  size_t numScales = scales.size() / 3;

  auto scalesArray = anari::newArray1D(this->AnariDevice, ANARI_FLOAT32_VEC3, numScales);
  {
    auto arrayPtr = anari::map<float>(this->AnariDevice, scalesArray);

    memcpy(arrayPtr, scales.data(), scales.size() * sizeof(float));

    anari::unmap(this->AnariDevice, scalesArray);
  }

  anari::setAndReleaseParameter(this->AnariDevice, glyphGeometry, "vertex.scale", scalesArray);
}

void vtkAnariGlyph3DMapperInheritInterface::SetGlyphScale(
  anari::Geometry& glyphGeometry, float scaleFactor)
{
  anari::setParameter(this->AnariDevice, glyphGeometry, "scale", scaleFactor);
}

//----------------------------------------------------------------------------
void vtkAnariGlyph3DMapperInheritInterface::SetGlyphIdArray(
  anari::Geometry& glyphGeometry, std::vector<uint32_t>& pointIds)
{
  size_t numIds = pointIds.size();

  auto idsArray = anari::newArray1D(this->AnariDevice, ANARI_UINT32, numIds);
  {
    auto arrayPtr = anari::map<float>(this->AnariDevice, idsArray);

    memcpy(arrayPtr, pointIds.data(), pointIds.size() * sizeof(uint32_t));

    anari::unmap(this->AnariDevice, idsArray);
  }

  anari::setAndReleaseParameter(this->AnariDevice, glyphGeometry, "primitive.id", idsArray);
}

//============================================================================
vtkStandardNewMacro(vtkAnariGlyph3DMapperNode);

//----------------------------------------------------------------------------
vtkAnariGlyph3DMapperNode::vtkAnariGlyph3DMapperNode()
{
  this->Internal = new vtkAnariGlyph3DMapperNodeInternals(this);
  this->SetInheritInterface(new vtkAnariGlyph3DMapperInheritInterface(this->Internal));
}

//----------------------------------------------------------------------------
vtkAnariGlyph3DMapperNode::~vtkAnariGlyph3DMapperNode()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkAnariGlyph3DMapperNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkCompositeDataDisplayAttributes* vtkAnariGlyph3DMapperNode::GetCompositeDisplayAttributes()
{
  vtkGlyph3DMapper* glm = vtkGlyph3DMapper::SafeDownCast(this->GetRenderable());
  return glm ? glm->GetBlockAttributes() : nullptr;
}

//----------------------------------------------------------------------------
void vtkAnariGlyph3DMapperNode::Synchronize(bool prepass)
{
  this->Internal->UpdateGlyphs();

  this->Superclass::Synchronize(prepass);
}

VTK_ABI_NAMESPACE_END
