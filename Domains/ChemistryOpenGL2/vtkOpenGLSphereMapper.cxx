/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLSphereMapper.h"

#include "vtkglVBOHelper.h"

#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLCamera.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkShaderProgram.h"

#include "vtkSphereMapperVS.h"

using vtkgl::substitute;

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLSphereMapper)

//-----------------------------------------------------------------------------
vtkOpenGLSphereMapper::vtkOpenGLSphereMapper()
{
  this->ScaleArray = 0;
  this->Invert = false;
}

//-----------------------------------------------------------------------------
void vtkOpenGLSphereMapper::GetShaderTemplate(std::string &VSSource,
  std::string &FSSource,
  std::string &GSSource,
  int lightComplexity, vtkRenderer* ren, vtkActor *actor)
{
  this->Superclass::GetShaderTemplate(VSSource,FSSource,GSSource,
                                      lightComplexity,ren,actor);

  VSSource = vtkSphereMapperVS;
}

void vtkOpenGLSphereMapper::ReplaceShaderValues(std::string &VSSource,
                                                 std::string &FSSource,
                                                 std::string &GSSource,
                                                 int lightComplexity,
                                                 vtkRenderer* ren,
                                                 vtkActor *actor)
{
  substitute(VSSource,
    "//VTK::Camera::Dec",
    "uniform mat4 VCDCMatrix;\n"
    "uniform mat4 MCVCMatrix;");

  substitute(FSSource,
    "//VTK::PositionVC::Dec",
    "varying vec4 vertexVCClose;");

  // for lights kit and positional the VCDC matrix is already defined
  // so don't redefine it
  std::string replacement =
    "uniform float invertedDepth;\n"
    "uniform int cameraParallel;\n"
    "varying float radiusVC;\n"
    "varying vec3 centerVC;\n"
    "uniform mat4 VCDCMatrix;\n";
  substitute(FSSource,"//VTK::Normal::Dec",replacement);

  substitute(FSSource,"//VTK::Normal::Impl",
    // compute the eye position and unit direction
    "vec4 vertexVC = vertexVCClose;\n"
    "  vec3 EyePos;\n"
    "  vec3 EyeDir;\n"
    "  if (cameraParallel != 0) {\n"
    "    EyePos = vec3(vertexVC.x, vertexVC.y, vertexVC.z + 3.0*radiusVC);\n"
    "    EyeDir = vec3(0.0,0.0,-1.0); }\n"
    "  else {\n"
    "    EyeDir = vertexVC.xyz;\n"
    "    EyePos = vec3(0.0,0.0,0.0);\n"
    "    float lengthED = length(EyeDir);\n"
    "    EyeDir = normalize(EyeDir);\n"
    // we adjust the EyePos to be closer if it is too far away
    // to prevent floating point precision noise
    "    if (lengthED > radiusVC*3.0) {\n"
    "      EyePos = vertexVC.xyz - EyeDir*3.0*radiusVC; }\n"
    "    }\n"

    // translate to Sphere center
    "  EyePos = EyePos - centerVC;\n"
    // scale to radius 1.0
    "  EyePos = EyePos/radiusVC;\n"
    // find the intersection
    "  float b = 2.0*dot(EyePos,EyeDir);\n"
    "  float c = dot(EyePos,EyePos) - 1.0;\n"
    "  float d = b*b - 4.0*c;\n"
    "  vec3 normalVC = vec3(0.0,0.0,1.0);\n"
    "  if (d < 0.0) { discard; }\n"
    "  else {\n"
    "    float t = (-b - invertedDepth*sqrt(d))*0.5;\n"

    // compute the normal, for unit sphere this is just
    // the intersection point
    "    normalVC = invertedDepth*normalize(EyePos + t*EyeDir);\n"
    // compute the intersection point in VC
    "    vertexVC.xyz = normalVC*radiusVC + centerVC;\n"
    "    }\n"
    // compute the pixel's depth
   // " normalVC = vec3(0,0,1);\n"
    "  vec4 pos = VCDCMatrix * vertexVC;\n"
    "  gl_FragDepth = (pos.z / pos.w + 1.0) / 2.0;\n"
    );

  if (ren->GetLastRenderingUsedDepthPeeling())
    {
    substitute(FSSource,
      "//VTK::DepthPeeling::Impl",
      "float odepth = texture2D(opaqueZTexture, gl_FragCoord.xy/screenSize).r;\n"
      "  if (gl_FragDepth >= odepth) { discard; }\n"
      "  float tdepth = texture2D(translucentZTexture, gl_FragCoord.xy/screenSize).r;\n"
      "  if (gl_FragDepth <= tdepth) { discard; }\n"
      );
    }

  this->Superclass::ReplaceShaderValues(VSSource,FSSource,GSSource,
                                        lightComplexity,ren,actor);
}

//-----------------------------------------------------------------------------
vtkOpenGLSphereMapper::~vtkOpenGLSphereMapper()
{
  this->SetScaleArray(0);
}


//-----------------------------------------------------------------------------
void vtkOpenGLSphereMapper::SetCameraShaderParameters(
  vtkgl::CellBO &cellBO,
  vtkRenderer* ren, vtkActor *actor)
{
  vtkShaderProgram *program = cellBO.Program;

  vtkOpenGLCamera *cam = (vtkOpenGLCamera *)(ren->GetActiveCamera());

  vtkMatrix4x4 *wcdc;
  vtkMatrix4x4 *wcvc;
  vtkMatrix3x3 *norms;
  vtkMatrix4x4 *vcdc;
  cam->GetKeyMatrices(ren,wcvc,norms,vcdc,wcdc);
  program->SetUniformMatrix("VCDCMatrix", vcdc);

  if (!actor->GetIsIdentity())
    {
    vtkMatrix4x4 *mcwc;
    vtkMatrix3x3 *anorms;
    ((vtkOpenGLActor *)actor)->GetKeyMatrices(mcwc,anorms);
    vtkMatrix4x4::Multiply4x4(mcwc, wcvc, this->TempMatrix4);
    program->SetUniformMatrix("MCVCMatrix", this->TempMatrix4);
    }
  else
    {
    program->SetUniformMatrix("MCVCMatrix", wcvc);
    }

  cellBO.Program->SetUniformi("cameraParallel", cam->GetParallelProjection());
}

//-----------------------------------------------------------------------------
void vtkOpenGLSphereMapper::SetMapperShaderParameters(vtkgl::CellBO &cellBO,
                                                         vtkRenderer *ren, vtkActor *actor)
{
  if (cellBO.indexCount && (this->VBOBuildTime > cellBO.attributeUpdateTime ||
      cellBO.ShaderSourceTime > cellBO.attributeUpdateTime))
    {
    vtkgl::VBOLayout &layout = this->Layout;
    cellBO.vao.Bind();
    if (!cellBO.vao.AddAttributeArray(cellBO.Program, this->VBO,
                                    "offsetMC", layout.ColorOffset+sizeof(float),
                                    layout.Stride, VTK_FLOAT, 2, false))
      {
      vtkErrorMacro(<< "Error setting 'offsetMC' in shader VAO.");
      }
    }

  cellBO.Program->SetUniformf("invertedDepth", this->Invert ? -1.0 : 1.0);
  this->Superclass::SetMapperShaderParameters(cellBO,ren,actor);
}


//-----------------------------------------------------------------------------
void vtkOpenGLSphereMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

namespace
{
// internal function called by CreateVBO
vtkgl::VBOLayout vtkOpenGLSphereMapperCreateVBO(float * points, vtkIdType numPts,
              unsigned char *colors, int colorComponents,
              float *sizes,
              vtkgl::BufferObject &vertexBuffer)
{
  vtkgl::VBOLayout layout;
  // Figure out how big each block will be, currently 6 or 7 floats.
  int blockSize = 3;
  layout.VertexOffset = 0;
  layout.NormalOffset = 0;
  layout.TCoordOffset = 0;
  layout.TCoordComponents = 0;
  layout.ColorComponents = colorComponents;
  layout.ColorOffset = sizeof(float) * blockSize;
  ++blockSize;

  // two more floats
  blockSize += 2;
  layout.Stride = sizeof(float) * blockSize;

  // Create a buffer, and copy the data over.
  std::vector<float> packedVBO;
  packedVBO.resize(blockSize * numPts*3);
  std::vector<float>::iterator it = packedVBO.begin();

  float *pointPtr;
  unsigned char *colorPtr;

  float cos30 = cos(vtkMath::RadiansFromDegrees(30.0));

  for (vtkIdType i = 0; i < numPts; ++i)
    {
    pointPtr = points + i*3;
    colorPtr = colors + i*colorComponents;
    float radius = sizes[i];

    // Vertices
    *(it++) = pointPtr[0];
    *(it++) = pointPtr[1];
    *(it++) = pointPtr[2];
    *(it++) = *reinterpret_cast<float *>(colorPtr);
    *(it++) = -2.0f*radius*cos30;
    *(it++) = -radius;

    *(it++) = pointPtr[0];
    *(it++) = pointPtr[1];
    *(it++) = pointPtr[2];
    *(it++) = *reinterpret_cast<float *>(colorPtr);
    *(it++) = 2.0f*radius*cos30;
    *(it++) = -radius;

    *(it++) = pointPtr[0];
    *(it++) = pointPtr[1];
    *(it++) = pointPtr[2];
    *(it++) = *reinterpret_cast<float *>(colorPtr);
    *(it++) = 0.0f;
    *(it++) = 2.0f*radius;
    }
  vertexBuffer.Upload(packedVBO, vtkgl::BufferObject::ArrayBuffer);
  layout.VertexCount = numPts*3;
  return layout;
}
}

//-------------------------------------------------------------------------
bool vtkOpenGLSphereMapper::GetNeedToRebuildBufferObjects(vtkRenderer *vtkNotUsed(ren), vtkActor *act)
{
  // picking state does not require a rebuild, unlike our parent
  if (this->VBOBuildTime < this->GetMTime() ||
      this->VBOBuildTime < act->GetMTime() ||
      this->VBOBuildTime < this->CurrentInput->GetMTime())
    {
    return true;
    }
  return false;
}

//-------------------------------------------------------------------------
void vtkOpenGLSphereMapper::BuildBufferObjects(
  vtkRenderer *vtkNotUsed(ren), vtkActor *vtkNotUsed(act))
{
  vtkPolyData *poly = this->CurrentInput;

  if (poly == NULL)// || !poly->GetPointData()->GetNormals())
    {
    return;
    }

  // For vertex coloring, this sets this->Colors as side effect.
  // For texture map coloring, this sets ColorCoordinates
  // and ColorTextureMap as a side effect.
  // I moved this out of the conditional because it is fast.
  // Color arrays are cached. If nothing has changed,
  // then the scalars do not have to be regenerted.
  this->MapScalars(1.0);

  // Iterate through all of the different types in the polydata, building OpenGLs
  // and IBOs as appropriate for each type.
  this->Layout =
    vtkOpenGLSphereMapperCreateVBO(static_cast<float *>(poly->GetPoints()->GetVoidPointer(0)),
              poly->GetPoints()->GetNumberOfPoints(),
              this->Colors ? (unsigned char *)this->Colors->GetVoidPointer(0) : NULL,
              this->Colors ? this->Colors->GetNumberOfComponents() : 0,
              static_cast<float *>(poly->GetPointData()->GetArray(this->ScaleArray)->GetVoidPointer(0)),
              this->VBO);

  // create the IBO
  this->Points.indexCount = 0;
  this->Lines.indexCount = 0;
  this->TriStrips.indexCount = 0;
  this->Tris.indexCount = this->Layout.VertexCount;
}


//----------------------------------------------------------------------------
void vtkOpenGLSphereMapper::Render(vtkRenderer *ren, vtkActor *act)
{
  vtkProperty *prop = act->GetProperty();
  bool is_opaque = (prop->GetOpacity() >= 1.0);

  // if we are transparent (and not backface culling) we have to draw twice
  if (!is_opaque && !prop->GetBackfaceCulling())
    {
    this->Invert = true;
    this->Superclass::Render(ren,act);
    this->Invert = false;
    }
  this->Superclass::Render(ren,act);
}

//-----------------------------------------------------------------------------
void vtkOpenGLSphereMapper::RenderPieceDraw(vtkRenderer* ren, vtkActor *actor)
{
  vtkgl::VBOLayout &layout = this->Layout;

  // draw polygons
  if (this->Tris.indexCount)
    {
    // First we do the triangles, update the shader, set uniforms, etc.
    this->UpdateShader(this->Tris, ren, actor);
    glDrawArrays(GL_TRIANGLES, 0,
                static_cast<GLuint>(layout.VertexCount));
    }
}
