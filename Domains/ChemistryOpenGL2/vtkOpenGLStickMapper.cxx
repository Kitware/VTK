/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLStickMapper.h"

#include "vtkglVBOHelper.h"

#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLCamera.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkShaderProgram.h"

#include "vtkStickMapperVS.h"

using vtkgl::substitute;

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLStickMapper)

//-----------------------------------------------------------------------------
vtkOpenGLStickMapper::vtkOpenGLStickMapper()
{
  this->ScaleArray = 0;
  this->OrientationArray = 0;
  this->SelectionIdArray = 0;
}

//-----------------------------------------------------------------------------
void vtkOpenGLStickMapper::GetShaderTemplate(std::string &VSSource,
                                          std::string &FSSource,
                                          std::string &GSSource,
                                          int lightComplexity, vtkRenderer* ren, vtkActor *actor)
{
  this->Superclass::GetShaderTemplate(VSSource,FSSource,GSSource,lightComplexity,ren,actor);

  VSSource = vtkStickMapperVS;
}

void vtkOpenGLStickMapper::ReplaceShaderValues(std::string &VSSource,
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
    "uniform int cameraParallel;\n"
    "varying float radiusVC;\n"
    "varying vec3 orientVC;\n"
    "varying float lengthVC;\n"
    "varying vec3 centerVC;\n"
    "uniform mat4 VCDCMatrix;\n";
  substitute(FSSource,"//VTK::Normal::Dec",replacement);


  // see https://www.cl.cam.ac.uk/teaching/1999/AGraphHCI/SMAG/node2.html
  substitute(FSSource,"//VTK::Normal::Impl",
    // compute the eye position and unit direction
    "  vec4 vertexVC = vertexVCClose;\n"
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

    // translate to Cylinder center
    "  EyePos = EyePos - centerVC;\n"

    // rotate to new basis
    // base1, base2, orientVC
    "  vec3 base1;\n"
    "  if (abs(orientVC.z) < 0.99) {\n"
    "    base1 = normalize(cross(orientVC,vec3(0.0,0.0,1.0))); }\n"
    "  else {\n"
    "    base1 = normalize(cross(orientVC,vec3(0.0,1.0,0.0))); }\n"
    "  vec3 base2 = cross(orientVC,base1);\n"
    "  EyePos = vec3(dot(EyePos,base1),dot(EyePos,base2),dot(EyePos,orientVC));\n"
    "  EyeDir = vec3(dot(EyeDir,base1),dot(EyeDir,base2),dot(EyeDir,orientVC));\n"

    // scale by radius
    "  EyePos = EyePos/radiusVC;\n"

    // find the intersection
    "  float a = EyeDir.x*EyeDir.x + EyeDir.y*EyeDir.y;\n"
    "  float b = 2.0*(EyePos.x*EyeDir.x + EyePos.y*EyeDir.y);\n"
    "  float c = EyePos.x*EyePos.x + EyePos.y*EyePos.y - 1.0;\n"
    "  float d = b*b - 4.0*a*c;\n"
    "  vec3 normalVC = vec3(0.0,0.0,1.0);\n"
    "  if (d < 0.0) { discard; }\n"
    "  else {\n"
    "    float t =  (-b - sqrt(d))/(2.0*a);\n"
    "    float tz = EyePos.z + t*EyeDir.z;\n"
    "    vec3 iPoint = EyePos + t*EyeDir;\n"
    "    if (abs(iPoint.z)*radiusVC > lengthVC*0.5) {\n"
    // test for end cap
    "      float t2 = (-b + sqrt(d))/(2.0*a);\n"
    "      float tz2 = EyePos.z + t2*EyeDir.z;\n"
    "      if (tz2*radiusVC > lengthVC*0.5 || tz*radiusVC < -0.5*lengthVC) { discard; }\n"
    "      else {\n"
    "        normalVC = orientVC;\n"
    "        float t3 = (lengthVC*0.5/radiusVC - EyePos.z)/EyeDir.z;\n"
    "        iPoint = EyePos + t3*EyeDir;\n"
    "        vertexVC.xyz = radiusVC*(iPoint.x*base1 + iPoint.y*base2 + iPoint.z*orientVC) + centerVC;\n"
    "        }\n"
    "      }\n"
    "    else {\n"
    // The normal is the iPoint.xy rotated back into VC
    "      normalVC = iPoint.x*base1 + iPoint.y*base2;\n"
    // rescale rerotate and translate
    "      vertexVC.xyz = radiusVC*(normalVC + iPoint.z*orientVC) + centerVC;\n"
    "      }\n"
    "    }\n"

//    "  vec3 normalVC = vec3(0.0,0.0,1.0);\n"
    // compute the pixel's depth
    "  vec4 pos = VCDCMatrix * vertexVC;\n"
    "  gl_FragDepth = (pos.z / pos.w + 1.0) / 2.0;\n"
    );

  vtkHardwareSelector* selector = ren->GetSelector();
  bool picking = (ren->GetRenderWindow()->GetIsPicking() || selector != NULL);
  if (picking)
    {
    substitute(VSSource,
      "//VTK::Picking::Dec",
      "attribute vec4 selectionId;\n"
      "varying vec4 selectionIdFrag;");
    substitute(VSSource,
      "//VTK::Picking::Impl",
      "selectionIdFrag = selectionId;");
    substitute(FSSource,
      "//VTK::Picking::Dec",
      "uniform vec3 mapperIndex;\n"
      "varying vec4 selectionIdFrag;");
    substitute(FSSource,
      "//VTK::Picking::Impl",
      "if (mapperIndex == vec3(0.0,0.0,0.0))\n"
      "    {\n"
      "    gl_FragColor = vec4(selectionIdFrag.rgb, 1.0);\n"
      "    }\n"
      "  else\n"
      "    {\n"
      "    gl_FragColor = vec4(mapperIndex,1.0);\n"
      "    }"
      );
    }

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



  this->Superclass::ReplaceShaderValues(VSSource,FSSource,GSSource,lightComplexity,ren,actor);
}

//-----------------------------------------------------------------------------
vtkOpenGLStickMapper::~vtkOpenGLStickMapper()
{
  this->SetScaleArray(0);
  this->SetOrientationArray(0);
  this->SetSelectionIdArray(0);
}


//-----------------------------------------------------------------------------
void vtkOpenGLStickMapper::SetCameraShaderParameters(vtkgl::CellBO &cellBO,
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
    vtkMatrix3x3::Multiply3x3(anorms, norms, this->TempMatrix3);
    program->SetUniformMatrix("normalMatrix", this->TempMatrix3);
    }
  else
    {
    program->SetUniformMatrix("MCVCMatrix", wcvc);
    program->SetUniformMatrix("normalMatrix", norms);
    }

  cellBO.Program->SetUniformi("cameraParallel", cam->GetParallelProjection());
}

//-----------------------------------------------------------------------------
void vtkOpenGLStickMapper::SetMapperShaderParameters(vtkgl::CellBO &cellBO,
                                                         vtkRenderer *ren, vtkActor *actor)
{
  if (cellBO.indexCount && (this->VBOBuildTime > cellBO.attributeUpdateTime ||
      cellBO.ShaderSourceTime > cellBO.attributeUpdateTime))
    {
    vtkHardwareSelector* selector = ren->GetSelector();
    bool picking = (ren->GetRenderWindow()->GetIsPicking() || selector != NULL);

    vtkgl::VBOLayout &layout = this->Layout;
    cellBO.vao.Bind();
    if (!cellBO.vao.AddAttributeArray(cellBO.Program, this->VBO,
                                    "orientMC", layout.ColorOffset+sizeof(float),
                                    layout.Stride, VTK_FLOAT, 3, false))
      {
      vtkErrorMacro(<< "Error setting 'orientMC' in shader VAO.");
      }
    if (!cellBO.vao.AddAttributeArray(cellBO.Program, this->VBO,
                                    "offsetMC", layout.ColorOffset+4*sizeof(float),
                                    layout.Stride, VTK_UNSIGNED_CHAR, 3, false))
      {
      vtkErrorMacro(<< "Error setting 'offsetMC' in shader VAO.");
      }
    if (!cellBO.vao.AddAttributeArray(cellBO.Program, this->VBO,
                                    "radiusMC", layout.ColorOffset+5*sizeof(float),
                                    layout.Stride, VTK_FLOAT, 1, false))
      {
      vtkErrorMacro(<< "Error setting 'radiusMC' in shader VAO.");
      }
    if (picking)
      {
      if (!cellBO.vao.AddAttributeArray(cellBO.Program, this->VBO,
                                      "selectionId", layout.ColorOffset+6*sizeof(float),
                                      layout.Stride, VTK_UNSIGNED_CHAR, 4, true))
        {
        vtkErrorMacro(<< "Error setting 'selectionId' in shader VAO.");
        }
      }
    else
      {
      cellBO.vao.RemoveAttributeArray("selectionId");
      }
    }

  this->Superclass::SetMapperShaderParameters(cellBO,ren,actor);
}


//-----------------------------------------------------------------------------
void vtkOpenGLStickMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

namespace
{
// internal function called by CreateVBO
vtkgl::VBOLayout vtkOpenGLStickMapperCreateVBO(float * points, vtkIdType numPts,
              unsigned char *colors, int colorComponents,
              float *orients,
              float *sizes,
              vtkIdType *selectionIds,
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

  // three more floats for orient + 2 for offset + 1 for radius
  blockSize += 5;
  if (selectionIds)
    {
    blockSize++;
    }
  layout.Stride = sizeof(float) * blockSize;

  // Create a buffer, and copy the data over.
  std::vector<float> packedVBO;
  packedVBO.resize(blockSize * numPts * 6);
  std::vector<float>::iterator it = packedVBO.begin();

  float *pointPtr;
  float *orientPtr;
  unsigned char *colorPtr;

  unsigned char offsets[4];
  offsets[3] = 0;
  unsigned int selId = 0;

  for (vtkIdType i = 0; i < numPts; ++i)
    {
    pointPtr = points + i*3;
    orientPtr = orients + i*3;
    colorPtr = colors + i*colorComponents;
    float radius = sizes[i*3+1];
    float length = sizes[i*3];
    if (selectionIds)
      {
      selId = static_cast<unsigned int>(selectionIds[i]) + 1;
      }

    // Vertices
    *(it++) = pointPtr[0];
    *(it++) = pointPtr[1];
    *(it++) = pointPtr[2];
    *(it++) = *reinterpret_cast<float *>(colorPtr);
    *(it++) = orientPtr[0]*length;
    *(it++) = orientPtr[1]*length;
    *(it++) = orientPtr[2]*length;
    offsets[0] = 0;
    offsets[1] = 0;
    offsets[2] = 0;
    *(it++) = *reinterpret_cast<float *>(offsets);
    *(it++) = radius;
    if (selectionIds)
      {
      *(it++) = *reinterpret_cast<float *>(&selId);
      }

    *(it++) = pointPtr[0];
    *(it++) = pointPtr[1];
    *(it++) = pointPtr[2];
    *(it++) = *reinterpret_cast<float *>(colorPtr);
    *(it++) = orientPtr[0]*length;
    *(it++) = orientPtr[1]*length;
    *(it++) = orientPtr[2]*length;
    offsets[0] = 1;
    offsets[1] = 0;
    offsets[2] = 0;
    *(it++) = *reinterpret_cast<float *>(offsets);
    *(it++) = radius;
    if (selectionIds)
      {
      *(it++) = *reinterpret_cast<float *>(&selId);
      }

    *(it++) = pointPtr[0];
    *(it++) = pointPtr[1];
    *(it++) = pointPtr[2];
    *(it++) = *reinterpret_cast<float *>(colorPtr);
    *(it++) = orientPtr[0]*length;
    *(it++) = orientPtr[1]*length;
    *(it++) = orientPtr[2]*length;
    offsets[0] = 1;
    offsets[1] = 0;
    offsets[2] = 1;
    *(it++) = *reinterpret_cast<float *>(offsets);
    *(it++) = radius;
    if (selectionIds)
      {
      *(it++) = *reinterpret_cast<float *>(&selId);
      }

    *(it++) = pointPtr[0];
    *(it++) = pointPtr[1];
    *(it++) = pointPtr[2];
    *(it++) = *reinterpret_cast<float *>(colorPtr);
    *(it++) = orientPtr[0]*length;
    *(it++) = orientPtr[1]*length;
    *(it++) = orientPtr[2]*length;
    offsets[0] = 0;
    offsets[1] = 0;
    offsets[2] = 1;
    *(it++) = *reinterpret_cast<float *>(offsets);
    *(it++) = radius;
    if (selectionIds)
      {
      *(it++) = *reinterpret_cast<float *>(&selId);
      }

    *(it++) = pointPtr[0];
    *(it++) = pointPtr[1];
    *(it++) = pointPtr[2];
    *(it++) = *reinterpret_cast<float *>(colorPtr);
    *(it++) = orientPtr[0]*length;
    *(it++) = orientPtr[1]*length;
    *(it++) = orientPtr[2]*length;
    offsets[0] = 1;
    offsets[1] = 1;
    offsets[2] = 1;
    *(it++) = *reinterpret_cast<float *>(offsets);
    *(it++) = radius;
    if (selectionIds)
      {
      *(it++) = *reinterpret_cast<float *>(&selId);
      }

    *(it++) = pointPtr[0];
    *(it++) = pointPtr[1];
    *(it++) = pointPtr[2];
    *(it++) = *reinterpret_cast<float *>(colorPtr);
    *(it++) = orientPtr[0]*length;
    *(it++) = orientPtr[1]*length;
    *(it++) = orientPtr[2]*length;
    offsets[0] = 0;
    offsets[1] = 1;
    offsets[2] = 1;
    *(it++) = *reinterpret_cast<float *>(offsets);
    *(it++) = radius;
    if (selectionIds)
      {
      *(it++) = *reinterpret_cast<float *>(&selId);
      }
    }
  vertexBuffer.Upload(packedVBO, vtkgl::BufferObject::ArrayBuffer);
  layout.VertexCount = numPts*6;
  return layout;
}
}

size_t vtkOpenGLStickMapperCreateTriangleIndexBuffer(
  vtkgl::BufferObject &indexBuffer,
  int numPts)
{
  std::vector<unsigned int> indexArray;
  indexArray.reserve(numPts * 12);

  for (int i = 0; i < numPts; i++)
    {
    indexArray.push_back(i*6);
    indexArray.push_back(i*6+1);
    indexArray.push_back(i*6+2);
    indexArray.push_back(i*6);
    indexArray.push_back(i*6+2);
    indexArray.push_back(i*6+3);
    indexArray.push_back(i*6+3);
    indexArray.push_back(i*6+2);
    indexArray.push_back(i*6+4);
    indexArray.push_back(i*6+3);
    indexArray.push_back(i*6+4);
    indexArray.push_back(i*6+5);
    }
  indexBuffer.Upload(indexArray, vtkgl::BufferObject::ElementArrayBuffer);
  return indexArray.size();
}

//-------------------------------------------------------------------------
bool vtkOpenGLStickMapper::GetNeedToRebuildBufferObjects(vtkRenderer *ren, vtkActor *act)
{
  // picking state changing always requires a rebuild
  vtkHardwareSelector* selector = ren->GetSelector();
  bool picking = (ren->GetIsPicking() || selector != NULL);

  if (this->VBOBuildTime < this->GetMTime() ||
      this->VBOBuildTime < act->GetMTime() ||
      this->VBOBuildTime < this->CurrentInput->GetMTime() ||
      this->LastSelectionState || picking)
    {
    return true;
    }
  return false;
}

//-------------------------------------------------------------------------
void vtkOpenGLStickMapper::BuildBufferObjects(vtkRenderer *ren,
  vtkActor *vtkNotUsed(act))
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

  vtkHardwareSelector* selector = ren->GetSelector();
  bool picking = (ren->GetRenderWindow()->GetIsPicking() || selector != NULL);

  // Iterate through all of the different types in the polydata, building OpenGLs
  // and IBOs as appropriate for each type.
  this->Layout =
    vtkOpenGLStickMapperCreateVBO(static_cast<float *>(poly->GetPoints()->GetVoidPointer(0)),
              poly->GetPoints()->GetNumberOfPoints(),
              this->Colors ? (unsigned char *)this->Colors->GetVoidPointer(0) : NULL,
              this->Colors ? this->Colors->GetNumberOfComponents() : 0,
              static_cast<float *>(poly->GetPointData()->GetArray(this->OrientationArray)->GetVoidPointer(0)),
              static_cast<float *>(poly->GetPointData()->GetArray(this->ScaleArray)->GetVoidPointer(0)),
              picking ?
                static_cast<vtkIdType *>(poly->GetPointData()->GetArray(this->SelectionIdArray)->GetVoidPointer(0))
                : NULL,
              this->VBO);

  // create the IBO
  this->Points.indexCount = 0;
  this->Lines.indexCount = 0;
  this->TriStrips.indexCount = 0;
  this->Tris.indexCount =
    vtkOpenGLStickMapperCreateTriangleIndexBuffer(this->Tris.ibo,
      poly->GetPoints()->GetNumberOfPoints());
}

//-----------------------------------------------------------------------------
void vtkOpenGLStickMapper::RenderPieceDraw(vtkRenderer* ren, vtkActor *actor)
{
  vtkgl::VBOLayout &layout = this->Layout;

  // draw polygons
  if (this->Tris.indexCount)
    {
    // First we do the triangles, update the shader, set uniforms, etc.
    this->UpdateShader(this->Tris, ren, actor);
    this->Tris.ibo.Bind();
    glDrawRangeElements(GL_TRIANGLES, 0,
                        static_cast<GLuint>(layout.VertexCount - 1),
                        static_cast<GLsizei>(this->Tris.indexCount),
                        GL_UNSIGNED_INT,
                        reinterpret_cast<const GLvoid *>(NULL));
    this->Tris.ibo.Release();
    }
}
