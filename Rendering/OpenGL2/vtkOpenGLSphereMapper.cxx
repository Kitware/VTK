// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOpenGLSphereMapper.h"

#include "vtkCellGraphicsPrimitiveMap.h"
#include "vtkDataArray.h"
#include "vtkFloatArray.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLCamera.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkShaderProgram.h"
#include "vtkStringToken.h"
#include "vtkTypeInt32Array.h"

#include <numeric>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOpenGLSphereMapper);

//------------------------------------------------------------------------------
vtkOpenGLSphereMapper::vtkOpenGLSphereMapper()
{
  // Every point is a sphere, regardless of the cells in the input. So, present all the points
  // as vertices to the vertices agent and skip the other cell types.
  this->Primitives[0].GeneratorFunction = [](vtkPolyData* mesh)
  {
    vtkCellGraphicsPrimitiveMap::PrimitiveDescriptor result;
    const vtkIdType numPoints = mesh->GetNumberOfPoints();
    result.VertexIDs = vtk::TakeSmartPointer(vtkTypeInt32Array::New());
    result.VertexIDs->SetNumberOfValues(numPoints);
    auto* ids = result.VertexIDs->GetPointer(0);
    std::iota(ids, ids + numPoints, 0);
    result.PrimitiveSize = 1;
    return result;
  };
  for (std::size_t i = 1; i < this->Primitives.size(); ++i)
  {
    this->Primitives[i].GeneratorFunction = [](vtkPolyData*)
    { return vtkCellGraphicsPrimitiveMap::PrimitiveDescriptor{}; };
  }
  // Draw each vertex as an instanced quad, whose corners are placed by ReplaceShaderPosition.
  this->DrawPointsAsQuads = true;

  // Ensure the token has a string in the dictionary so vtkStringToken::Data() can return it.
  vtkStringToken sphereRadii = "sphereRadii";
  (void)sphereRadii;
}

//------------------------------------------------------------------------------
vtkOpenGLSphereMapper::~vtkOpenGLSphereMapper()
{
  this->SetScaleArray(nullptr);
}

//------------------------------------------------------------------------------
void vtkOpenGLSphereMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ScaleArray: " << (this->ScaleArray ? this->ScaleArray : "(none)") << "\n";
  os << indent << "Radius: " << this->Radius << "\n";
}

//------------------------------------------------------------------------------
bool vtkOpenGLSphereMapper::BindArraysToTextureBuffers(vtkRenderer* renderer, vtkActor* actor,
  vtkCellGraphicsPrimitiveMap::CellTypeMapperOffsets& offsets)
{
  if (!this->Superclass::BindArraysToTextureBuffers(renderer, actor, offsets))
  {
    return false;
  }
  using namespace vtk::literals;
  vtkPolyData* mesh = this->CurrentInput;
  const bool hadScaleArray = this->HasScaleArray;
  this->HasScaleArray = false;
  vtkDataArray* scales =
    this->ScaleArray ? mesh->GetPointData()->GetArray(this->ScaleArray) : nullptr;
  if (scales && scales->GetNumberOfTuples() == mesh->GetNumberOfPoints())
  {
    if (scales->GetNumberOfComponents() != 1)
    {
      // only the first component is the radius.
      vtkNew<vtkFloatArray> radii;
      radii->SetNumberOfValues(scales->GetNumberOfTuples());
      radii->CopyComponent(0, scales, 0);
      this->AppendArrayToTexture("sphereRadii"_token, radii);
    }
    else
    {
      this->AppendArrayToTexture("sphereRadii"_token, scales);
    }
    this->HasScaleArray = true;
  }
  if (hadScaleArray != this->HasScaleArray)
  {
    // the radius is fetched from a different source now.
    this->ShaderProgram = nullptr;
  }
  return true;
}

//------------------------------------------------------------------------------
void vtkOpenGLSphereMapper::InstallArrayTextureShaderDeclarations()
{
  this->Superclass::InstallArrayTextureShaderDeclarations();
  if (this->HasScaleArray)
  {
    using namespace vtk::literals;
    this->ShaderDecls.emplace_back(
      /*qualifier=*/GLSLQualifierType::Uniform,
      /*precision=*/GLSLPrecisionType::High,
      /*dataType=*/GLSLDataType::Float,
      /*attributeType=*/GLSLAttributeType::SamplerBuffer,
      /*variableName=*/"sphereRadii"_token);
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLSphereMapper::ReplaceShaderPosition(
  vtkRenderer* renderer, vtkActor* actor, std::string& vsSource, std::string& fsSource)
{
  vtkShaderProgram::Substitute(vsSource, "//VTK::PositionVC::Dec",
    "//VTK::PositionVC::Dec\n"
    "uniform mat4 VCDCMatrix;\n"
    "uniform float sphereRadius;\n"
    "flat out float radiusVCVSOutput;\n"
    "flat out vec3 centerVCVSOutput;\n");
  std::string radiusImpl = this->HasScaleArray
    ? "  radiusVCVSOutput = texelFetchBuffer(sphereRadii, pointId).x;\n"
    : "  radiusVCVSOutput = sphereRadius;\n";
  // Substitute the position before the superclass does so that its default
  // implementation (a single vertex at the point) is not used.
  vtkShaderProgram::Substitute(vsSource, "//VTK::PositionVC::Impl", radiusImpl + R"(
  vec4 centerVC = MCVCMatrix * vertexMC;
  centerVCVSOutput = centerVC.xyz / centerVC.w;
  // Build a quad that faces the eye and covers the silhouette of the sphere.
  vec3 toEye = vec3(0.0, 0.0, 1.0);
  vec3 right = vec3(1.0, 0.0, 0.0);
  vec3 up = vec3(0.0, 1.0, 0.0);
  float halfSize = radiusVCVSOutput;
  if (cameraParallel == 0)
  {
    float distanceToEye = length(centerVCVSOutput);
    toEye = -centerVCVSOutput / max(distanceToEye, 1e-30);
    up = cross(toEye, vec3(1.0, 0.0, 0.0));
    if (dot(up, up) < 1e-12)
    {
      up = cross(toEye, vec3(0.0, 1.0, 0.0));
    }
    up = normalize(up);
    right = cross(up, toEye);
    // Under perspective, the silhouette is the circle where the cone from the eye touches the
    // sphere. In the plane through the center, that cone has a radius of r / cos(asin(r / d)).
    float sinHalfAngle = min(radiusVCVSOutput / max(distanceToEye, 1e-30), 0.999);
    halfSize = radiusVCVSOutput / sqrt(1.0 - sinHalfAngle * sinHalfAngle);
  }
  vertexVCVSOutput =
    vec4(centerVCVSOutput + halfSize * (quadCoord.x * right + quadCoord.y * up), 1.0);
  gl_Position = VCDCMatrix * vertexVCVSOutput;
)");
  this->Superclass::ReplaceShaderPosition(renderer, actor, vsSource, fsSource);
}

//------------------------------------------------------------------------------
void vtkOpenGLSphereMapper::ReplaceShaderNormal(
  vtkRenderer*, vtkActor*, std::string&, std::string& fsSource)
{
  vtkShaderProgram::Substitute(fsSource, "//VTK::Normal::Dec",
    "//VTK::Normal::Dec\n"
    "uniform float invertedDepth;\n"
    "uniform mat4 VCDCMatrix;\n"
    "flat in float radiusVCVSOutput;\n"
    "flat in vec3 centerVCVSOutput;\n"
    "flat in int hasTubeBasisVS;\n"); // base Light::Impl references hasTubeBasisVS;
                                      // re-declare because we own Normal::Dec,
  // Ray-cast the sphere. This consumes the depth tag, so that no other replacement
  // overwrites the depth of the sphere's surface.
  vtkShaderProgram::Substitute(fsSource, "//VTK::Depth::Impl", R"(
  // compute the eye position and unit direction
  vec3 EyePos;
  vec3 EyeDir;
  if (cameraParallel != 0)
  {
    EyePos = vec3(vertexVC.x, vertexVC.y, vertexVC.z + 3.0 * radiusVCVSOutput);
    EyeDir = vec3(0.0, 0.0, -1.0);
  }
  else
  {
    EyeDir = vertexVC.xyz;
    EyePos = vec3(0.0, 0.0, 0.0);
    float lengthED = length(EyeDir);
    EyeDir = normalize(EyeDir);
    // we adjust the EyePos to be closer if it is too far away
    // to prevent floating point precision noise
    if (lengthED > radiusVCVSOutput * 3.0)
    {
      EyePos = vertexVC.xyz - EyeDir * 3.0 * radiusVCVSOutput;
    }
  }
  // translate to Sphere center
  EyePos = EyePos - centerVCVSOutput;
  // scale to radius 1.0
  EyePos = EyePos / radiusVCVSOutput;
  // find the intersection
  float b = 2.0 * dot(EyePos, EyeDir);
  float c = dot(EyePos, EyePos) - 1.0;
  float d = b * b - 4.0 * c;
  if (d < 0.0)
  {
    discard;
  }
  float t = (-b - invertedDepth * sqrt(d)) * 0.5;
  // compute the normal, for unit sphere this is just the intersection point
  vec3 sphereNormalVC = normalize(EyePos + t * EyeDir);
  // compute the intersection point in VC
  vertexVC.xyz = sphereNormalVC * radiusVCVSOutput + centerVCVSOutput;
  sphereNormalVC *= invertedDepth;
  // compute the pixel's depth
  vec4 pos = VCDCMatrix * vertexVC;
  gl_FragDepth = (pos.z / pos.w + 1.0) / 2.0;
)");
  std::string normalImpl = "  vec3 normalVCVSOutput = sphereNormalVC;\n"
                           "  vec3 vertexNormalVCVS = sphereNormalVC;\n";
  if (this->HasClearCoat)
  {
    normalImpl += "  vec3 coatNormalVCVSOutput = sphereNormalVC;\n";
  }
  vtkShaderProgram::Substitute(fsSource, "//VTK::Normal::Impl", normalImpl + "//VTK::Normal::Impl");
}

//------------------------------------------------------------------------------
void vtkOpenGLSphereMapper::SetShaderParameters(vtkRenderer* renderer, vtkActor* actor)
{
  this->Superclass::SetShaderParameters(renderer, actor);
  vtkShaderProgram* program = this->ShaderProgram;
  if (!program)
  {
    return;
  }
  auto* cam = static_cast<vtkOpenGLCamera*>(renderer->GetActiveCamera());
  vtkMatrix4x4* wcdc;
  vtkMatrix4x4* wcvc;
  vtkMatrix3x3* norms;
  vtkMatrix4x4* vcdc;
  cam->GetKeyMatrices(renderer, wcvc, norms, vcdc, wcdc);
  if (program->IsUniformUsed("VCDCMatrix"))
  {
    program->SetUniformMatrix("VCDCMatrix", vcdc);
  }
  if (program->IsUniformUsed("invertedDepth"))
  {
    program->SetUniformf("invertedDepth", this->Invert ? -1.0f : 1.0f);
  }
  if (program->IsUniformUsed("sphereRadius"))
  {
    program->SetUniformf("sphereRadius", this->Radius);
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLSphereMapper::Render(vtkRenderer* ren, vtkActor* act)
{
  vtkProperty* prop = act->GetProperty();
  bool is_opaque = (prop->GetOpacity() >= 1.0);

  // if we are transparent (and not backface culling) we have to draw twice
  if (!is_opaque && !prop->GetBackfaceCulling())
  {
    this->Invert = true;
    this->Superclass::Render(ren, act);
    this->Invert = false;
  }
  this->Superclass::Render(ren, act);
}
VTK_ABI_NAMESPACE_END
