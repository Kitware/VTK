// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkUSDExporter.h"

#include "vtkActor.h"
#include "vtkActorCollection.h"
#include "vtkAssemblyPath.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCollectionRange.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositeDataSetRange.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkDataArray.h"
#include "vtkExtractVOI.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkMapper.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPNGWriter.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataNormals.h"
#include "vtkProp.h"
#include "vtkPropCollection.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkSmartPointer.h"
#include "vtkTexture.h"
#include "vtkTransform.h"
#include "vtkTriangleFilter.h"
#include "vtkTrivialProducer.h"
#include "vtkUnsignedCharArray.h"
#include <vtkStringFormatter.h>

// Avoid warning about deprecated hash_map header inclusion from
// pxr/base/tf/token.h in GCC 15
#if defined(__GLIBCXX__) && !defined(_GLIBCXX_PERMIT_BACKWARD_HASH)
#define _GLIBCXX_PERMIT_BACKWARD_HASH
#define undef_GLIBCXX_PERMIT_BACKWARD_HASH
#endif

#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/array.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usd/usdGeom/camera.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/primvarsAPI.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdGeom/xformOp.h"
#include "pxr/usd/usdLux/distantLight.h"
#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usdShade/materialBindingAPI.h"
#include "pxr/usd/usdShade/shader.h"

#include <algorithm>
#include <cstring>
#include <map>
#include <vector>

#if defined(undef_GLIBCXX_PERMIT_BACKWARD_HASH)
#undef _GLIBCXX_PERMIT_BACKWARD_HASH
#endif

PXR_NAMESPACE_USING_DIRECTIVE

VTK_ABI_NAMESPACE_BEGIN

// Holds state that must persist across multiple WriteData() calls when
// accumulating timesteps into a single stage (i.e. between Start() and
// Finish()). UsdGeomXformOp handles in particular must be created exactly
// once per prim and then reused: calling AddTranslateOp/AddOrientOp/
// AddScaleOp/AddTransformOp again on a later frame would append a
// duplicate op rather than update the existing one's time samples.
class vtkUSDExporter::vtkUSDExporterInternals
{
public:
  UsdStageRefPtr Stage;

  std::vector<UsdGeomXformOp> ActorTranslateOps;
  std::vector<UsdGeomXformOp> ActorOrientOps;
  std::vector<UsdGeomXformOp> ActorScaleOps;

  bool HasCameraTransformOp = false;
  UsdGeomXformOp CameraTransformOp;

  std::vector<UsdGeomXformOp> LightTransformOps;

  bool HasTime = false;
  double MinTime = 0.0;
  double MaxTime = 0.0;

  size_t FirstFrameXformCount = 0;
  size_t FirstFrameMeshCount = 0;

  // Counts WriteData() calls since Start(), used to give each frame's
  // texture images distinct filenames (see WriteTexture).
  size_t FrameIndex = 0;

  // Remembers, per mesh index, the pixel data and filename of the last
  // texture actually written to disk. This lets WriteTexture() detect that
  // a texture is unchanged (e.g. from one animation frame to the next) and
  // reuse the previous file instead of writing a duplicate.
  struct TextureRecord
  {
    vtkSmartPointer<vtkUnsignedCharArray> Data;
    std::string FileName;
  };
  std::map<size_t, TextureRecord> LastTextures;

  void Reset()
  {
    this->Stage = UsdStageRefPtr();
    this->ActorTranslateOps.clear();
    this->ActorOrientOps.clear();
    this->ActorScaleOps.clear();
    this->HasCameraTransformOp = false;
    this->CameraTransformOp = UsdGeomXformOp();
    this->LightTransformOps.clear();
    this->HasTime = false;
    this->MinTime = 0.0;
    this->MaxTime = 0.0;
    this->FirstFrameXformCount = 0;
    this->FirstFrameMeshCount = 0;
    this->FrameIndex = 0;
    this->LastTextures.clear();
  }

  void RecordTime(double time)
  {
    if (!this->HasTime)
    {
      this->MinTime = this->MaxTime = time;
      this->HasTime = true;
    }
    else
    {
      this->MinTime = std::min(this->MinTime, time);
      this->MaxTime = std::max(this->MaxTime, time);
    }
  }

  void ApplyVtkActorTransformToUsdXform(vtkActor* actor, UsdGeomXform& xform, size_t xformIndex,
    bool isFirstFrame, const UsdTimeCode& timeCode);

  std::string WriteTexture(
    vtkActor* actor, const char* fileName, size_t index, bool timeVarying, size_t frameIndex);
};

void vtkUSDExporter::vtkUSDExporterInternals::ApplyVtkActorTransformToUsdXform(vtkActor* actor,
  UsdGeomXform& xform, size_t xformIndex, bool isFirstFrame, const UsdTimeCode& timeCode)
{
  if (!actor)
  {
    return;
  }

  // Get the actor's transformation matrix
  vtkMatrix4x4* vtkMat = actor->GetMatrix();

  // Use vtkTransform to decompose the matrix
  vtkNew<vtkTransform> transform;
  transform->SetMatrix(vtkMat);
  double translation[3], scale[3], orientation[3];
  transform->GetPosition(translation);
  transform->GetScale(scale);
  transform->GetOrientation(orientation);

  // Convert VTK results to USD types
  GfVec3d usdTranslation(translation[0], translation[1], translation[2]);
  GfVec3d usdScale(scale[0], scale[1], scale[2]);

  // Convert Euler angles to quaternion
  GfRotation rotation;
  rotation = GfRotation(GfVec3d(1, 0, 0), orientation[0]) *
    GfRotation(GfVec3d(0, 1, 0), orientation[1]) * GfRotation(GfVec3d(0, 0, 1), orientation[2]);
  GfQuatd usdRotation = rotation.GetQuat();

  // Add operations to the USD xform.
  bool needNewOps = isFirstFrame || this->ActorTranslateOps.size() <= xformIndex ||
    !this->ActorTranslateOps[xformIndex].GetAttr().IsValid();
  if (needNewOps)
  {
    if (this->ActorTranslateOps.size() <= xformIndex)
    {
      this->ActorTranslateOps.resize(xformIndex + 1);
      this->ActorOrientOps.resize(xformIndex + 1);
      this->ActorScaleOps.resize(xformIndex + 1);
    }
    this->ActorTranslateOps[xformIndex] = xform.AddTranslateOp(UsdGeomXformOp::PrecisionDouble);
    this->ActorOrientOps[xformIndex] = xform.AddOrientOp(UsdGeomXformOp::PrecisionDouble);
    this->ActorScaleOps[xformIndex] = xform.AddScaleOp(UsdGeomXformOp::PrecisionDouble);
  }

  this->ActorTranslateOps[xformIndex].Set(usdTranslation, timeCode);
  this->ActorOrientOps[xformIndex].Set(usdRotation, timeCode);
  this->ActorScaleOps[xformIndex].Set(usdScale, timeCode);
}

namespace
{

// Determine if the actor needs texture export. This is true if either
// scalar visibility with ColorMode set to VTK_COLOR_MODE_MAP_SCALARS,
// or if the actor has a texture.
bool NeedsTextureExport(vtkActor* actor)
{
  if (!actor || !actor->GetMapper())
  {
    return false;
  }

  vtkMapper* mapper = actor->GetMapper();
  return (mapper->GetScalarVisibility() && mapper->GetColorMode() == VTK_COLOR_MODE_MAP_SCALARS) ||
    actor->GetTexture() != nullptr;
}

UsdGeomMesh WriteMesh(UsdStageRefPtr& stage, UsdGeomXform& xform, vtkPolyData* inputPd,
  vtkActor* actor, size_t index, const UsdTimeCode& timeCode)
{
  // Define a Mesh prim under the xform
  SdfPath xformPath = xform.GetPath();

  UsdGeomMesh mesh =
    UsdGeomMesh::Define(stage, xformPath.AppendChild(TfToken("Mesh" + vtk::to_string(index))));

  vtkNew<vtkTriangleFilter> triangle;
  triangle->SetInputData(inputPd);
  triangle->Update();
  vtkNew<vtkPolyData> pd;
  pd->ShallowCopy(triangle->GetOutput());

  // Vertex positions. These may change over time, so always author a time
  // sample for the current frame.
  VtArray<GfVec3f> points(pd->GetNumberOfPoints());
  for (vtkIdType i = 0; i < pd->GetNumberOfPoints(); ++i)
  {
    double p[3];
    pd->GetPoint(i, p);
    points[i] =
      GfVec3f(static_cast<float>(p[0]), static_cast<float>(p[1]), static_cast<float>(p[2]));
  }
  mesh.GetPointsAttr().Set(points, timeCode);

  // Face vertex counts from polys only because any triangle strips will be converted
  // to polys by the triangle filter above.
  vtkCellArray* polys = pd->GetPolys();
  VtArray<int> faceVertexCounts(polys->GetNumberOfCells());
  // Indices into the points array
  VtArray<int> faceVertexIndices;
  faceVertexIndices.reserve(polys->GetNumberOfCells() * 4); // rough estimate

  for (vtkIdType cellIdx = 0; cellIdx < polys->GetNumberOfCells(); ++cellIdx)
  {
    vtkIdType npts;
    const vtkIdType* pts;

    polys->GetCellAtId(cellIdx, npts, pts);

    faceVertexCounts[cellIdx] = static_cast<int>(npts);
    for (vtkIdType j = 0; j < npts; ++j)
    {
      faceVertexIndices.push_back(static_cast<int>(pts[j]));
    }
  }
  mesh.GetFaceVertexCountsAttr().Set(faceVertexCounts, timeCode);
  mesh.GetFaceVertexIndicesAttr().Set(faceVertexIndices, timeCode);

  // Normals (per-vertex if available, otherwise per-face)
  vtkDataArray* normalsArray = pd->GetPointData()->GetNormals();
  if (normalsArray && normalsArray->GetNumberOfTuples() == pd->GetNumberOfPoints())
  {
    VtArray<GfVec3f> normals(pd->GetNumberOfPoints());
    for (vtkIdType i = 0; i < pd->GetNumberOfPoints(); ++i)
    {
      double n[3];
      normalsArray->GetTuple(i, n);
      normals[i] =
        GfVec3f(static_cast<float>(n[0]), static_cast<float>(n[1]), static_cast<float>(n[2]));
    }
    mesh.GetNormalsAttr().Set(normals, timeCode);
    mesh.SetNormalsInterpolation(UsdGeomTokens->vertex);
  }
  else if ((normalsArray = pd->GetCellData()->GetNormals()))
  {
    if (normalsArray && normalsArray->GetNumberOfTuples() == pd->GetNumberOfCells())
    {
      VtArray<GfVec3f> normals(pd->GetNumberOfCells());
      for (vtkIdType i = 0; i < pd->GetNumberOfCells(); ++i)
      {
        double n[3];
        normalsArray->GetTuple(i, n);
        normals[i] =
          GfVec3f(static_cast<float>(n[0]), static_cast<float>(n[1]), static_cast<float>(n[2]));
      }
      mesh.GetNormalsAttr().Set(normals, timeCode);
      mesh.SetNormalsInterpolation(UsdGeomTokens->vertex);
    }
  }
  else
  {
    // Compute per-face normals for each triangle
    VtArray<GfVec3f> normals;
    // Use vtkPolyDataNormals to compute cell normals only
    vtkNew<vtkPolyDataNormals> normalsFilter;
    normalsFilter->SetInputData(pd);
    normalsFilter->ComputeCellNormalsOn();
    normalsFilter->ComputePointNormalsOff();
    normalsFilter->SplittingOff();
    normalsFilter->ConsistencyOff();
    normalsFilter->Update();

    vtkPolyData* normalsPd = normalsFilter->GetOutput();
    vtkDataArray* cellNormals = normalsPd->GetCellData()->GetNormals();
    if (cellNormals && cellNormals->GetNumberOfTuples() == pd->GetNumberOfCells())
    {
      normals.resize(pd->GetNumberOfCells());
      for (vtkIdType i = 0; i < pd->GetNumberOfCells(); ++i)
      {
        double n[3];
        cellNormals->GetTuple(i, n);
        normals[i] =
          GfVec3f(static_cast<float>(n[0]), static_cast<float>(n[1]), static_cast<float>(n[2]));
      }
    }
    mesh.GetNormalsAttr().Set(normals, timeCode);
    mesh.SetNormalsInterpolation(UsdGeomTokens->vertex);
  }

  // Texture coordinates may change from frame to frame (e.g. along with
  // deforming topology or a scalar field driving color-mapped tcoords), so
  // they are authored as a time sample on every frame, just like points and
  // topology.
  // if we have vertex colors then retrieve them
  vtkMapper* mapper = actor->GetMapper();
  if (::NeedsTextureExport(actor))
  {
    // Generate tcoord by changing mapper settings:
    mapper->SetInterpolateScalarsBeforeMapping(true);
    mapper->MapScalars(pd, 1.0);

    // If we have tcoords from either color coordinates or explicit texture coordinates
    // then write them out.
    vtkFloatArray* tcoords = mapper->GetColorCoordinates();
    if (!tcoords && actor->GetTexture())
    {
      // No color coordinates, try explicit tcoords
      tcoords = vtkFloatArray::SafeDownCast(pd->GetPointData()->GetTCoords());
    }

    // Check that tcoords exists and has 2 components
    if (tcoords && tcoords->GetNumberOfComponents() != 2)
    {
      vtkGenericWarningMacro("Ignoring texture coords without 2 components.");
      tcoords = nullptr;
    }
    if (tcoords)
    {
      // Write out texture coordinates
      VtArray<GfVec2f> uvs(tcoords->GetNumberOfTuples());
      for (vtkIdType i = 0; i < tcoords->GetNumberOfTuples(); ++i)
      {
        double uv[2];
        tcoords->GetTuple(i, uv);
        uvs[i] = GfVec2f(static_cast<float>(uv[0]), 0.0f);
      }
      UsdGeomPrimvarsAPI primvarsAPI(mesh);
      UsdGeomPrimvar stPrimvar = primvarsAPI.CreatePrimvar(
        TfToken("st"), SdfValueTypeNames->TexCoord2fArray, UsdGeomTokens->vertex);
      stPrimvar.Set(uvs, timeCode);
    }
  }

  return mesh;
}

void WriteMaterial(UsdStageRefPtr& stage, UsdGeomMesh& mesh, int meshIndex, vtkActor* actor,
  const std::string& textureFileName, const UsdTimeCode& timeCode)
{
  // Material
  std::ostringstream strm;
  strm << "/Material" << meshIndex;
  SdfPath materialPath(strm.str());

  // Define()/CreateInput()/CreateIdAttr() etc. are idempotent, so it is safe
  // to call this once per frame: prims and attributes are created on the
  // first call and simply reused on later ones. The color/opacity/texture
  // values themselves may vary with the actor's appearance or active scalar
  // field over time, so they are authored as time samples on every frame.
  UsdShadeMaterial material = UsdShadeMaterial::Define(stage, materialPath);
  UsdShadeShader shader =
    UsdShadeShader::Define(stage, materialPath.AppendChild(TfToken("PreviewSurface")));

  // Connect the shader to the material’s surface output
  shader.CreateIdAttr().Set(TfToken("UsdPreviewSurface"));
  material.CreateSurfaceOutput().ConnectToSource(shader.ConnectableAPI(), TfToken("surface"));
  UsdShadeMaterialBindingAPI(mesh).Bind(material);

  vtkProperty* property = actor->GetProperty();
  int interpolation = property->GetInterpolation();

  // Some common properties for all interpolation types
  double diffuseColor[3];
  property->GetDiffuseColor(diffuseColor);
  shader.CreateInput(TfToken("diffuseColor"), SdfValueTypeNames->Color3f)
    .Set(GfVec3f(diffuseColor[0], diffuseColor[1], diffuseColor[2]), timeCode);

  double specularColor[3];
  property->GetSpecularColor(specularColor);
  shader.CreateInput(TfToken("specularColor"), SdfValueTypeNames->Color3f)
    .Set(GfVec3f(specularColor[0], specularColor[1], specularColor[2]), timeCode);

  double opacity = property->GetOpacity();
  shader.CreateInput(TfToken("opacity"), SdfValueTypeNames->Float)
    .Set(static_cast<float>(opacity), timeCode);

  if (interpolation == VTK_PBR)
  {
    shader.CreateInput(TfToken("clearcoatRoughness"), SdfValueTypeNames->Float)
      .Set(static_cast<float>(property->GetCoatRoughness()), timeCode);

    shader.CreateInput(TfToken("metallic"), SdfValueTypeNames->Float)
      .Set(static_cast<float>(property->GetMetallic()), timeCode);

    shader.CreateInput(TfToken("ior"), SdfValueTypeNames->Float)
      .Set(static_cast<float>(property->GetCoatIOR()), timeCode);
  }

  if (::NeedsTextureExport(actor))
  {
    material.CreateInput(TfToken("stPrimvarName"), SdfValueTypeNames->Token).Set(TfToken("st"));

    // Create stReader for 'st'
    UsdShadeShader stReader =
      UsdShadeShader::Define(stage, materialPath.AppendChild(TfToken("stReader")));
    stReader.CreateIdAttr().Set(TfToken("UsdPrimvarReader_float2"));
    stReader.CreateInput(TfToken("varname"), SdfValueTypeNames->Token);
    stReader.CreateOutput(TfToken("result"), SdfValueTypeNames->Float2);

    UsdShadeConnectableAPI::ConnectToSource(
      stReader.GetInput(TfToken("varname")), material.GetInput(TfToken("stPrimvarName")));

    // Create diffuse texture shader
    UsdShadeShader diffuseTexture =
      UsdShadeShader::Define(stage, materialPath.AppendChild(TfToken("diffuseTexture")));
    diffuseTexture.CreateIdAttr().Set(TfToken("UsdUVTexture"));
    diffuseTexture.CreateInput(TfToken("file"), SdfValueTypeNames->Asset)
      .Set(SdfAssetPath(textureFileName), timeCode);
    diffuseTexture.CreateInput(TfToken("sourceColorSpace"), SdfValueTypeNames->Token)
      .Set(TfToken("auto"));
    diffuseTexture.CreateInput(TfToken("st"), SdfValueTypeNames->Token);
    diffuseTexture.CreateOutput(TfToken("rgb"), SdfValueTypeNames->Float3);

    UsdShadeConnectableAPI::ConnectToSource(
      shader.GetInput(TfToken("diffuseColor")), diffuseTexture.GetOutput(TfToken("rgb")));

    UsdShadeConnectableAPI::ConnectToSource(
      diffuseTexture.GetInput(TfToken("st")), stReader.GetOutput(TfToken("result")));
  }
}

// Returns true if two texture pixel arrays have identical dimensions and
// content.
bool TextureDataEqual(vtkUnsignedCharArray* a, vtkUnsignedCharArray* b)
{
  if (!a || !b)
  {
    return false;
  }
  if (a->GetNumberOfTuples() != b->GetNumberOfTuples() ||
    a->GetNumberOfComponents() != b->GetNumberOfComponents())
  {
    return false;
  }
  size_t numValues =
    static_cast<size_t>(a->GetNumberOfTuples()) * static_cast<size_t>(a->GetNumberOfComponents());
  return numValues == 0 || std::memcmp(a->GetPointer(0), b->GetPointer(0), numValues) == 0;
}

} // end anonymous namespace

std::string vtkUSDExporter::vtkUSDExporterInternals::WriteTexture(
  vtkActor* actor, const char* fileName, size_t index, bool timeVarying, size_t frameIndex)
{
  // do we have a texture?
  vtkImageData* id = actor->GetMapper()->GetColorTextureMap();
  vtkTexture* t = nullptr;
  if (!id && actor->GetTexture())
  {
    t = actor->GetTexture();
    id = t->GetInput();
  }

  vtkUnsignedCharArray* da = nullptr;
  if (id && id->GetPointData()->GetScalars()) // scalars
  {
    da = vtkUnsignedCharArray::FastDownCast(id->GetPointData()->GetScalars());
  }
  if (!da)
  {
    return {};
  }

  // If the texture at this mesh index is identical to the last one we wrote
  // (e.g. an unchanging texture across animation frames), reuse that file
  // instead of writing a duplicate.
  auto lastIt = this->LastTextures.find(index);
  if (lastIt != this->LastTextures.end() && ::TextureDataEqual(da, lastIt->second.Data))
  {
    return lastIt->second.FileName;
  }

  // figure out a filename - strip extension, add "_tex0.png". When
  // accumulating multiple timesteps into a single stage, the texture image
  // itself may differ each frame (e.g. it is derived from a time-varying
  // scalar field), so each frame's texture is written to its own file,
  // named "_tex0_frame0.png", "_tex0_frame1.png", etc., and referenced via
  // a time-sampled asset path in the material (see WriteMaterial).
  std::string filePath = vtksys::SystemTools::GetFilenamePath(fileName);
  std::string baseName = vtksys::SystemTools::GetFilenameWithoutLastExtension(fileName);
  std::ostringstream strm;
  strm << filePath << '/' << baseName << "_tex" << index;
  if (timeVarying)
  {
    strm << "_frame" << frameIndex;
  }
  strm << ".png";
  std::string textureFile = strm.str();

  // we don't want the NaN color in the texture file
  vtkNew<vtkTrivialProducer> triv;
  triv->SetOutput(id);

  vtkNew<vtkExtractVOI> extractVOI;
  extractVOI->SetInputConnection(triv->GetOutputPort());
  int extent[6];
  id->GetExtent(extent);
  extent[3] = 0;
  extractVOI->SetVOI(extent);

  // WRite a separate PNG to store the texture
  vtkNew<vtkPNGWriter> png;
  png->SetFileName(textureFile.c_str());
  png->SetCompressionLevel(5);
  png->SetInputConnection(extractVOI->GetOutputPort());
  png->Write();

  vtkNew<vtkUnsignedCharArray> dataCopy;
  dataCopy->ShallowCopy(da);
  this->LastTextures[index] = { dataCopy, textureFile };

  return textureFile;
}

vtkStandardNewMacro(vtkUSDExporter);

vtkUSDExporter::vtkUSDExporter()
{
  this->FileName = nullptr;
  this->Internal = new vtkUSDExporterInternals;
}

vtkUSDExporter::~vtkUSDExporter()
{
  delete[] this->FileName;
  delete this->Internal;
}

void vtkUSDExporter::Start()
{
  this->Internal->Reset();
  this->Started = true;
}

void vtkUSDExporter::Finish()
{
  if (!this->Started)
  {
    return;
  }

  if (this->Internal->Stage)
  {
    if (this->Internal->HasTime)
    {
      this->Internal->Stage->SetStartTimeCode(this->Internal->MinTime);
      this->Internal->Stage->SetEndTimeCode(this->Internal->MaxTime);
    }
    this->Internal->Stage->GetRootLayer()->Save();
  }

  this->Internal->Reset();
  this->Started = false;
}

void vtkUSDExporter::WriteData()
{
  if (this->FileName == nullptr)
  {
    vtkErrorMacro("Please specify FileName to use for exported USD file.");
    return;
  }

  // Determine whether this call is part of a multi-frame, single-file
  // export (this->Started), and if so, whether it is the first frame of
  // that sequence, i.e. whether the shared stage still needs to be created.
  bool isFirstFrame = true;
  UsdStageRefPtr stage;
  if (this->Started)
  {
    if (this->Internal->Stage)
    {
      stage = this->Internal->Stage;
      isFirstFrame = false;
    }
    else
    {
      stage = UsdStage::CreateNew(this->FileName);
      if (stage)
      {
        stage->SetTimeCodesPerSecond(1.0);
      }
      this->Internal->Stage = stage;
      isFirstFrame = true;
    }
  }
  else
  {
    // Legacy, single-shot behavior: always a fresh, self-contained stage.
    stage = UsdStage::CreateNew(this->FileName);
    isFirstFrame = true;
    // Each independent single-shot export starts a new stage, so texture
    // reuse tracking must not carry over from a previous, unrelated export.
    this->Internal->LastTextures.clear();
  }

  if (!stage)
  {
    vtkErrorMacro("Failed to create USD stage for file: " << this->FileName);
    return;
  }

  UsdTimeCode timeCode = this->Started ? UsdTimeCode(this->TimeValue) : UsdTimeCode::Default();
  size_t frameIndex = 0;
  if (this->Started)
  {
    this->Internal->RecordTime(this->TimeValue);
    frameIndex = this->Internal->FrameIndex++;
  }

  size_t xformCount = 0, meshCount = 0;
  for (auto ren : vtk::Range(this->RenderWindow->GetRenderers()))
  {
    if (this->ActiveRenderer && ren != this->ActiveRenderer)
    {
      // If ActiveRenderer is specified then ignore all other renderers
      continue;
    }
    if (!ren->GetDraw())
    {
      continue;
    }

    if (ren && ren->GetActiveCamera())
    {
      vtkCamera* cam = ren->GetActiveCamera();
      std::string camPath = "/Camera";
      UsdGeomCamera usdCam = UsdGeomCamera::Define(stage, SdfPath(camPath));

      double pos[3], focal[3], up[3];
      cam->GetPosition(pos);
      cam->GetFocalPoint(focal);
      cam->GetViewUp(up);

      usdCam.CreateProjectionAttr().Set(
        cam->GetParallelProjection() ? UsdGeomTokens->orthographic : UsdGeomTokens->perspective,
        timeCode);

      usdCam.CreateFocalLengthAttr().Set(static_cast<float>(cam->GetDistance()), timeCode);
      usdCam.CreateClippingRangeAttr().Set(GfVec2f(static_cast<float>(cam->GetClippingRange()[0]),
                                             static_cast<float>(cam->GetClippingRange()[1])),
        timeCode);

      // TODO - base the aperture on view angle and image aspect ratio
      usdCam.CreateHorizontalApertureAttr().Set(2.0f, timeCode); // mm, placeholder
      usdCam.CreateVerticalApertureAttr().Set(2.0f, timeCode);   // mm, placeholder

      // usdCam.CreateFocusDistanceAttr().Set(static_cast<float>(cam->GetDistance()));

      UsdGeomXformable xformable(usdCam.GetPrim());

      vtkTransform* trans = ren->GetActiveCamera()->GetModelViewTransformObject();
      vtkNew<vtkMatrix4x4> inv;
      trans->GetInverse(inv);

      // We need to convert row-major to column-major, with transpose.
      vtkNew<vtkMatrix4x4> transpose;
      vtkMatrix4x4::Transpose(inv, transpose);
      if (isFirstFrame)
      {
        this->Internal->CameraTransformOp = xformable.AddTransformOp();
        this->Internal->HasCameraTransformOp = true;
      }
      if (this->Internal->HasCameraTransformOp)
      {
        this->Internal->CameraTransformOp.Set(GfMatrix4d(transpose->Element), timeCode);
      }
    }

    // Export lights from the renderer to USD
    unsigned int lightIndex = 0;
    for (auto light : vtk::Range(ren->GetLights()))
    {
      if (!light || !light->GetSwitch())
      {
        continue;
      }

      std::ostringstream lightPathStrm;
      lightPathStrm << "/Light" << lightIndex;
      ++lightIndex;
      SdfPath lightPath(lightPathStrm.str());
      UsdLuxDistantLight usdLight = UsdLuxDistantLight::Define(stage, lightPath);

      // Compute direction
      GfVec3d position, focal;
      ;
      light->GetPosition(position.data());
      light->GetFocalPoint(focal.data());
      GfVec3d direction3d = focal - position;
      direction3d.Normalize();
      GfVec3f direction(direction3d);

      double color[3];
      light->GetDiffuseColor(color);

      // Set color and intensity
      usdLight.CreateColorAttr().Set(GfVec3f(static_cast<float>(color[0]),
                                       static_cast<float>(color[1]), static_cast<float>(color[2])),
        timeCode);
      usdLight.CreateIntensityAttr().Set(
        static_cast<float>(light->GetIntensity() * 100.0), timeCode); // scale to USD

      // Set transform (rotation only, position is not used for distant lights)
      GfMatrix4d xform = GfMatrix4d(1.0);
      // Align -Z to direction
      GfVec3f zAxis(0, 0, -1);
      GfRotation rot = GfRotation(zAxis, direction);
      xform.SetRotateOnly(rot.GetQuat());
      UsdGeomXformable xformable(usdLight.GetPrim());
      size_t thisLightIndex = lightIndex - 1;
      if (isFirstFrame)
      {
        if (this->Internal->LightTransformOps.size() <= thisLightIndex)
        {
          this->Internal->LightTransformOps.resize(thisLightIndex + 1);
        }
        this->Internal->LightTransformOps[thisLightIndex] = xformable.AddTransformOp();
      }
      this->Internal->LightTransformOps[thisLightIndex].Set(xform, timeCode);
    }

    // Iterate over all the visible props in the renderer and export their geometry.
    // Skip widget representations under the assumption that they are used mainly for
    // interaction and are not part of the scene.
    for (auto propObject : vtk::Range(ren->GetViewProps()))
    {
      // vtkPropCollection does not provide a GetNextItem method that returns vtkProp* directly,
      // so vtk::Range returns props as vtkObjects, hence the downcast here.
      vtkProp* prop = vtkProp::SafeDownCast(propObject);

      // skip writing actors that are hidden as well as widget representations
      if (!prop->GetVisibility() || prop->IsA("vtkWidgetRepresentation"))
      {
        continue;
      }

      vtkNew<vtkActorCollection> ac;
      prop->GetActors(ac);
      for (auto actor : vtk::Range(ac.Get()))
      {
        vtkAssemblyPath* assemblyPath;
        for (actor->InitPathTraversal(); (assemblyPath = actor->GetNextPath());)
        {
          vtkActor* part = static_cast<vtkActor*>(assemblyPath->GetLastNode()->GetViewProp());
          if (part->GetVisibility() && part->GetMapper() && part->GetMapper()->GetInputAlgorithm())
          {
            vtkMapper* mapper = part->GetMapper();
            mapper->GetInputAlgorithm()->Update();

            vtkDataObject* input = mapper->GetInputDataObject(0, 0);

            // Create a transform for the actor
            std::ostringstream pathStream;
            size_t currentXformIndex = xformCount++;
            pathStream << "/XForm" << currentXformIndex;
            UsdGeomXform xform = UsdGeomXform::Define(stage, SdfPath(pathStream.str()));
            this->Internal->ApplyVtkActorTransformToUsdXform(
              actor, xform, currentXformIndex, isFirstFrame, timeCode);

            size_t previousMeshCount = meshCount;
            vtkCompositeDataSet* cpd = vtkCompositeDataSet::SafeDownCast(input);
            if (cpd)
            {
              vtkCompositePolyDataMapper* pdMapper =
                vtkCompositePolyDataMapper::SafeDownCast(mapper);
              vtkSmartPointer<vtkCompositeDataIterator> cpdIter;
              cpdIter.TakeReference(cpd->NewIterator());
              for (cpdIter->InitTraversal(); !cpdIter->IsDoneWithTraversal();
                   cpdIter->GoToNextItem())
              {
                auto childDO = cpdIter->GetCurrentDataObject();
                int flatIndex = cpdIter->GetCurrentFlatIndex();

                if (pdMapper->GetBlockVisibility(flatIndex))
                {
                  vtkPolyData* pd = vtkPolyData::SafeDownCast(childDO);
                  if (pd &&
                    (pd->GetPolys()->GetNumberOfCells() > 0 ||
                      pd->GetStrips()->GetNumberOfCells() > 0))
                  {
                    vtkMapper* partMapper = part->GetMapper();
                    // save and restore prop changed when generating texture coords
                    bool saveInterpScalars = partMapper->GetInterpolateScalarsBeforeMapping();

                    UsdGeomMesh mesh = ::WriteMesh(stage, xform, pd, part, meshCount, timeCode);

                    std::string textureFileName;
                    if (mapper->GetScalarVisibility() &&
                      mapper->GetColorMode() != VTK_COLOR_MODE_MAP_SCALARS && !actor->GetTexture())
                    {
                      vtkWarningMacro(
                        "Cannot export color textures when scalar visibility is on and "
                        "mapper's ColorMode is not set to VTK_COLOR_MODE_MAP_SCALARS.");
                    }
                    else
                    {
                      textureFileName = this->Internal->WriteTexture(
                        part, this->FileName, meshCount, this->Started, frameIndex);
                    }

                    ::WriteMaterial(stage, mesh, meshCount, part, textureFileName, timeCode);
                    partMapper->SetInterpolateScalarsBeforeMapping(saveInterpScalars);
                    ++meshCount;
                  }
                }
              }
            }

            vtkPolyData* pd = vtkPolyData::SafeDownCast(input);
            if (pd &&
              (pd->GetPolys()->GetNumberOfCells() > 0 || pd->GetStrips()->GetNumberOfCells() > 0))
            {
              // save and restore prop changed when generating texture coords
              bool saveInterpScalars = part->GetMapper()->GetInterpolateScalarsBeforeMapping();
              UsdGeomMesh mesh = ::WriteMesh(stage, xform, pd, part, meshCount, timeCode);
              std::string textureFileName = this->Internal->WriteTexture(
                part, this->FileName, meshCount, this->Started, frameIndex);
              ::WriteMaterial(stage, mesh, meshCount, part, textureFileName, timeCode);
              part->GetMapper()->SetInterpolateScalarsBeforeMapping(saveInterpScalars);
              ++meshCount;
            }

            if (meshCount == previousMeshCount)
            {
              // we did not write any meshes, so remove the xform
              stage->RemovePrim(xform.GetPath());
              --xformCount;
            }
          }
        }
      }
    }
  }

  if (this->Started)
  {
    if (isFirstFrame)
    {
      this->Internal->FirstFrameXformCount = xformCount;
      this->Internal->FirstFrameMeshCount = meshCount;
    }
    else if (xformCount != this->Internal->FirstFrameXformCount ||
      meshCount != this->Internal->FirstFrameMeshCount)
    {
      vtkWarningMacro("The number of actors/meshes changed between frames of a single-file USD "
                      "export. Each mesh's own topology may vary over time, but prims cannot "
                      "be added or removed once the stage is created; results may be "
                      "incorrect.");
    }
    // Saving is deferred to Finish() so all timesteps land in one file.
  }
  else
  {
    stage->GetRootLayer()->Save();
  }
}

void vtkUSDExporter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(null)") << std::endl;
}
VTK_ABI_NAMESPACE_END
