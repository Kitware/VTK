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
#include "vtkTexture.h"
#include "vtkTransform.h"
#include "vtkTriangleFilter.h"
#include "vtkTrivialProducer.h"
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
#include "pxr/usd/usdGeom/camera.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/primvarsAPI.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdLux/distantLight.h"
#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usdShade/materialBindingAPI.h"
#include "pxr/usd/usdShade/shader.h"

#if defined(undef_GLIBCXX_PERMIT_BACKWARD_HASH)
#undef _GLIBCXX_PERMIT_BACKWARD_HASH
#endif

PXR_NAMESPACE_USING_DIRECTIVE

VTK_ABI_NAMESPACE_BEGIN

namespace
{

void ApplyVtkActorTransformToUsdXform(vtkActor* actor, UsdGeomXform& xform)
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

  // Apply the transforms to the USD xform
  auto tOp = xform.AddTranslateOp(UsdGeomXformOp::PrecisionDouble);
  tOp.Set(usdTranslation);

  auto rOp = xform.AddOrientOp(UsdGeomXformOp::PrecisionDouble);
  rOp.Set(usdRotation);

  auto sOp = xform.AddScaleOp(UsdGeomXformOp::PrecisionDouble);
  sOp.Set(usdScale);
}

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

UsdGeomMesh WriteMesh(
  UsdStageRefPtr& stage, UsdGeomXform& xform, vtkPolyData* inputPd, vtkActor* actor, size_t index)
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

  // Vertex positions
  VtArray<GfVec3f> points(pd->GetNumberOfPoints());
  for (vtkIdType i = 0; i < pd->GetNumberOfPoints(); ++i)
  {
    double p[3];
    pd->GetPoint(i, p);
    points[i] =
      GfVec3f(static_cast<float>(p[0]), static_cast<float>(p[1]), static_cast<float>(p[2]));
  }
  mesh.GetPointsAttr().Set(points);

  // Face vertex counts
  VtArray<int> faceVertexCounts(pd->GetNumberOfCells());
  // Indices into the points array
  VtArray<int> faceVertexIndices;
  faceVertexIndices.reserve(pd->GetNumberOfCells() * 4); // rough estimate

  vtkCellArray* polys = pd->GetPolys();
  vtkIdType npts;
  const vtkIdType* pts;
  vtkIdType cellId = 0;
  for (vtkIdType cellIdx = 0; cellIdx < pd->GetNumberOfCells(); ++cellIdx)
  {
    polys->GetCellAtId(cellIdx, npts, pts);

    faceVertexCounts[cellId++] = static_cast<int>(npts);
    for (vtkIdType j = 0; j < npts; ++j)
    {
      faceVertexIndices.push_back(static_cast<int>(pts[j]));
    }
  }
  mesh.GetFaceVertexCountsAttr().Set(faceVertexCounts);
  mesh.GetFaceVertexIndicesAttr().Set(faceVertexIndices);

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
    mesh.GetNormalsAttr().Set(normals);
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
      mesh.GetNormalsAttr().Set(normals);
      mesh.SetNormalsInterpolation(UsdGeomTokens->faceVarying);
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
    mesh.GetNormalsAttr().Set(normals);
    mesh.SetNormalsInterpolation(UsdGeomTokens->faceVarying);
  }

  // if we have vertex colors then retrieve them
  vtkMapper* mapper = actor->GetMapper();
  if (NeedsTextureExport(actor))
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
    else
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
      stPrimvar.Set(uvs);
    }
  }

  return mesh;
}

void WriteMaterial(UsdStageRefPtr& stage, UsdGeomMesh& mesh, int meshIndex, vtkActor* actor,
  const std::string& textureFileName)
{
  // Material
  std::ostringstream strm;
  strm << "/Material" << meshIndex;
  SdfPath materialPath(strm.str());

  // Create a Material
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
    .Set(GfVec3f(diffuseColor[0], diffuseColor[1], diffuseColor[2]));

  double specularColor[3];
  property->GetSpecularColor(specularColor);
  shader.CreateInput(TfToken("specularColor"), SdfValueTypeNames->Color3f)
    .Set(GfVec3f(specularColor[0], specularColor[1], specularColor[2]));

  double opacity = property->GetOpacity();
  shader.CreateInput(TfToken("opacity"), SdfValueTypeNames->Float).Set(static_cast<float>(opacity));

  if (interpolation == VTK_PBR)
  {
    shader.CreateInput(TfToken("clearcoatRoughness"), SdfValueTypeNames->Float)
      .Set(static_cast<float>(property->GetCoatRoughness()));

    shader.CreateInput(TfToken("metallic"), SdfValueTypeNames->Float)
      .Set(static_cast<float>(property->GetMetallic()));

    shader.CreateInput(TfToken("ior"), SdfValueTypeNames->Float)
      .Set(static_cast<float>(property->GetCoatIOR()));
  }

  if (NeedsTextureExport(actor))
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
      .Set(SdfAssetPath(textureFileName));
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

std::string WriteTexture(vtkActor* actor, const char* fileName, size_t index)
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

  // figure out a filename - strip extension, add "_tex0.png"
  std::string filePath = vtksys::SystemTools::GetFilenamePath(fileName);
  std::string baseName = vtksys::SystemTools::GetFilenameWithoutLastExtension(fileName);
  std::ostringstream strm;
  strm << filePath << '/' << baseName << "_tex" << index << ".png";
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

  return textureFile;
}

} // end anonymous namespace

vtkStandardNewMacro(vtkUSDExporter);

vtkUSDExporter::vtkUSDExporter()
{
  this->FileName = nullptr;
}

vtkUSDExporter::~vtkUSDExporter()
{
  delete[] this->FileName;
}

void vtkUSDExporter::WriteData()
{
  if (this->FileName == nullptr)
  {
    vtkErrorMacro("Please specify FileName to use for exported USD file.");
    return;
  }

  UsdStageRefPtr stage = UsdStage::CreateNew(this->FileName);
  if (!stage)
  {
    vtkErrorMacro("Failed to create USD stage for file: " << this->FileName);
    return;
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
        cam->GetParallelProjection() ? UsdGeomTokens->orthographic : UsdGeomTokens->perspective);

      usdCam.CreateFocalLengthAttr().Set(static_cast<float>(cam->GetDistance()));
      usdCam.CreateClippingRangeAttr().Set(GfVec2f(static_cast<float>(cam->GetClippingRange()[0]),
        static_cast<float>(cam->GetClippingRange()[1])));

      // TODO - base the aperture on view angle and image aspect ratio
      usdCam.CreateHorizontalApertureAttr().Set(2.0f); // mm, placeholder
      usdCam.CreateVerticalApertureAttr().Set(2.0f);   // mm, placeholder

      // usdCam.CreateFocusDistanceAttr().Set(static_cast<float>(cam->GetDistance()));

      UsdGeomXformable xformable(usdCam.GetPrim());

      vtkTransform* trans = ren->GetActiveCamera()->GetModelViewTransformObject();
      vtkNew<vtkMatrix4x4> inv;
      trans->GetInverse(inv);

      // We need to convert row-major to column-major, with transpose.
      vtkNew<vtkMatrix4x4> transpose;
      vtkMatrix4x4::Transpose(inv, transpose);
      xformable.AddTransformOp().Set(GfMatrix4d(transpose->Element));
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
      usdLight.CreateColorAttr().Set(GfVec3f(
        static_cast<float>(color[0]), static_cast<float>(color[1]), static_cast<float>(color[2])));
      usdLight.CreateIntensityAttr().Set(
        static_cast<float>(light->GetIntensity() * 100.0)); // scale to USD

      // Set transform (rotation only, position is not used for distant lights)
      GfMatrix4d xform = GfMatrix4d(1.0);
      // Align -Z to direction
      GfVec3f zAxis(0, 0, -1);
      GfRotation rot = GfRotation(zAxis, direction);
      xform.SetRotateOnly(rot.GetQuat());
      UsdGeomXformable xformable(usdLight.GetPrim());
      xformable.AddTransformOp().Set(xform);
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
            pathStream << "/XForm" << xformCount++;
            UsdGeomXform xform = UsdGeomXform::Define(stage, SdfPath(pathStream.str()));
            ApplyVtkActorTransformToUsdXform(actor, xform);

            size_t previousMeshCount = meshCount;
            vtkCompositeDataSet* cpd = vtkCompositeDataSet::SafeDownCast(input);
            if (cpd)
            {
              int flatIndex = 0;
              vtkCompositePolyDataMapper* pdMapper =
                vtkCompositePolyDataMapper::SafeDownCast(mapper);
              using Opts = vtk::CompositeDataSetOptions;
              for (auto childDO : vtk::Range(cpd, Opts::SkipEmptyNodes))
              {
                if (pdMapper->GetBlockVisibility(flatIndex))
                {
                  vtkPolyData* pd = vtkPolyData::SafeDownCast(childDO);
                  if (pd && pd->GetNumberOfCells() > 0)
                  {
                    vtkMapper* partMapper = part->GetMapper();
                    // save and restore prop changed when generating texture coords
                    bool saveInterpScalars = partMapper->GetInterpolateScalarsBeforeMapping();

                    UsdGeomMesh mesh = WriteMesh(stage, xform, pd, part, meshCount);

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
                      textureFileName = WriteTexture(part, this->FileName, meshCount);
                    }

                    WriteMaterial(stage, mesh, meshCount, part, textureFileName);
                    partMapper->SetInterpolateScalarsBeforeMapping(saveInterpScalars);
                    ++meshCount;
                  }
                }
                ++flatIndex;
              }
            }

            vtkPolyData* pd = vtkPolyData::SafeDownCast(input);
            if (pd && pd->GetNumberOfCells() > 0)
            {
              // save and restore prop changed when generating texture coords
              bool saveInterpScalars = part->GetMapper()->GetInterpolateScalarsBeforeMapping();
              UsdGeomMesh mesh = WriteMesh(stage, xform, pd, part, meshCount);
              std::string textureFileName = WriteTexture(part, this->FileName, meshCount);
              WriteMaterial(stage, mesh, meshCount, part, textureFileName);
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

  stage->GetRootLayer()->Save();
}

void vtkUSDExporter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(null)") << std::endl;
}
VTK_ABI_NAMESPACE_END
