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

#include "vtkOpenGLHelper.h"

#include "vtkFloatArray.h"
#include "vtkHardwareSelector.h"
#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLIndexBufferObject.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkOpenGLVertexBufferObject.h"
#include "vtkOpenGLVertexBufferObjectGroup.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkShaderProgram.h"
#include "vtkUnsignedCharArray.h"

#include "vtkStickMapperVS.h"

#include "vtk_glew.h"



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
void vtkOpenGLStickMapper::GetShaderTemplate(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *ren, vtkActor *actor)
{
  this->Superclass::GetShaderTemplate(shaders,ren,actor);
  shaders[vtkShader::Vertex]->SetSource(vtkStickMapperVS);
}

void vtkOpenGLStickMapper::ReplaceShaderValues(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *ren, vtkActor *actor)
{
  std::string VSSource = shaders[vtkShader::Vertex]->GetSource();
  std::string FSSource = shaders[vtkShader::Fragment]->GetSource();

  vtkShaderProgram::Substitute(VSSource,
    "//VTK::Camera::Dec",
    "uniform mat4 VCDCMatrix;\n"
    "uniform mat4 MCVCMatrix;");

  vtkShaderProgram::Substitute(FSSource,
    "//VTK::PositionVC::Dec",
    "varying vec4 vertexVCVSOutput;");

  // we create vertexVC below, so turn off the default
  // implementation
  vtkShaderProgram::Substitute(FSSource,
    "//VTK::PositionVC::Impl",
    "  vec4 vertexVC = vertexVCVSOutput;\n");

  // for lights kit and positional the VCDC matrix is already defined
  // so don't redefine it
  std::string replacement =
    "uniform int cameraParallel;\n"
    "varying float radiusVCVSOutput;\n"
    "varying vec3 orientVCVSOutput;\n"
    "varying float lengthVCVSOutput;\n"
    "varying vec3 centerVCVSOutput;\n"
    "uniform mat4 VCDCMatrix;\n";
  vtkShaderProgram::Substitute(FSSource,"//VTK::Normal::Dec",replacement);

  // see https://www.cl.cam.ac.uk/teaching/1999/AGraphHCI/SMAG/node2.html
  vtkShaderProgram::Substitute(FSSource,"//VTK::Depth::Impl",
    // compute the eye position and unit direction
    "  vec3 EyePos;\n"
    "  vec3 EyeDir;\n"
    "  if (cameraParallel != 0) {\n"
    "    EyePos = vec3(vertexVC.x, vertexVC.y, vertexVC.z + 3.0*radiusVCVSOutput);\n"
    "    EyeDir = vec3(0.0,0.0,-1.0); }\n"
    "  else {\n"
    "    EyeDir = vertexVC.xyz;\n"
    "    EyePos = vec3(0.0,0.0,0.0);\n"
    "    float lengthED = length(EyeDir);\n"
    "    EyeDir = normalize(EyeDir);\n"
    // we adjust the EyePos to be closer if it is too far away
    // to prevent floating point precision noise
    "    if (lengthED > radiusVCVSOutput*3.0) {\n"
    "      EyePos = vertexVC.xyz - EyeDir*3.0*radiusVCVSOutput; }\n"
    "    }\n"

    // translate to Cylinder center
    "  EyePos = EyePos - centerVCVSOutput;\n"

    // rotate to new basis
    // base1, base2, orientVC
    "  vec3 base1;\n"
    "  if (abs(orientVCVSOutput.z) < 0.99) {\n"
    "    base1 = normalize(cross(orientVCVSOutput,vec3(0.0,0.0,1.0))); }\n"
    "  else {\n"
    "    base1 = normalize(cross(orientVCVSOutput,vec3(0.0,1.0,0.0))); }\n"
    "  vec3 base2 = cross(orientVCVSOutput,base1);\n"
    "  EyePos = vec3(dot(EyePos,base1),dot(EyePos,base2),dot(EyePos,orientVCVSOutput));\n"
    "  EyeDir = vec3(dot(EyeDir,base1),dot(EyeDir,base2),dot(EyeDir,orientVCVSOutput));\n"

    // scale by radius
    "  EyePos = EyePos/radiusVCVSOutput;\n"

    // find the intersection
    "  float a = EyeDir.x*EyeDir.x + EyeDir.y*EyeDir.y;\n"
    "  float b = 2.0*(EyePos.x*EyeDir.x + EyePos.y*EyeDir.y);\n"
    "  float c = EyePos.x*EyePos.x + EyePos.y*EyePos.y - 1.0;\n"
    "  float d = b*b - 4.0*a*c;\n"
    "  vec3 normalVCVSOutput = vec3(0.0,0.0,1.0);\n"
    "  if (d < 0.0) { discard; }\n"
    "  else {\n"
    "    float t =  (-b - sqrt(d))/(2.0*a);\n"
    "    float tz = EyePos.z + t*EyeDir.z;\n"
    "    vec3 iPoint = EyePos + t*EyeDir;\n"
    "    if (abs(iPoint.z)*radiusVCVSOutput > lengthVCVSOutput*0.5) {\n"
    // test for end cap
    "      float t2 = (-b + sqrt(d))/(2.0*a);\n"
    "      float tz2 = EyePos.z + t2*EyeDir.z;\n"
    "      if (tz2*radiusVCVSOutput > lengthVCVSOutput*0.5 || tz*radiusVCVSOutput < -0.5*lengthVCVSOutput) { discard; }\n"
    "      else {\n"
    "        normalVCVSOutput = orientVCVSOutput;\n"
    "        float t3 = (lengthVCVSOutput*0.5/radiusVCVSOutput - EyePos.z)/EyeDir.z;\n"
    "        iPoint = EyePos + t3*EyeDir;\n"
    "        vertexVC.xyz = radiusVCVSOutput*(iPoint.x*base1 + iPoint.y*base2 + iPoint.z*orientVCVSOutput) + centerVCVSOutput;\n"
    "        }\n"
    "      }\n"
    "    else {\n"
    // The normal is the iPoint.xy rotated back into VC
    "      normalVCVSOutput = iPoint.x*base1 + iPoint.y*base2;\n"
    // rescale rerotate and translate
    "      vertexVC.xyz = radiusVCVSOutput*(normalVCVSOutput + iPoint.z*orientVCVSOutput) + centerVCVSOutput;\n"
    "      }\n"
    "    }\n"

//    "  vec3 normalVC = vec3(0.0,0.0,1.0);\n"
    // compute the pixel's depth
    "  vec4 pos = VCDCMatrix * vertexVC;\n"
    "  gl_FragDepth = (pos.z / pos.w + 1.0) / 2.0;\n"
    );

  // Strip out the normal line -- the normal is computed as part of the depth
  vtkShaderProgram::Substitute(FSSource,"//VTK::Normal::Impl", "");

  vtkHardwareSelector* selector = ren->GetSelector();
  bool picking = (ren->GetRenderWindow()->GetIsPicking() || selector != NULL);
  if (picking)
  {
    if (!selector ||
        (this->LastSelectionState >= vtkHardwareSelector::ID_LOW24))
    {
      vtkShaderProgram::Substitute(VSSource,
        "//VTK::Picking::Dec",
        "attribute vec4 selectionId;\n"
        "varying vec4 selectionIdVSOutput;");
      vtkShaderProgram::Substitute(VSSource,
        "//VTK::Picking::Impl",
        "selectionIdVSOutput = selectionId;");
      vtkShaderProgram::Substitute(FSSource,
        "//VTK::Picking::Dec",
        "varying vec4 selectionIdVSOutput;");
      vtkShaderProgram::Substitute(FSSource,
        "//VTK::Picking::Impl",
        "    gl_FragData[0] = vec4(selectionIdVSOutput.rgb, 1.0);\n"
        );
    }
    else
    {
      vtkShaderProgram::Substitute(FSSource,
        "//VTK::Picking::Dec",
        "uniform vec3 mapperIndex;");
      vtkShaderProgram::Substitute(FSSource,
        "//VTK::Picking::Impl",
        "  gl_FragData[0] = vec4(mapperIndex,1.0);\n"
        );
    }
  }

  shaders[vtkShader::Vertex]->SetSource(VSSource);
  shaders[vtkShader::Fragment]->SetSource(FSSource);

  this->Superclass::ReplaceShaderValues(shaders,ren,actor);
}

//-----------------------------------------------------------------------------
vtkOpenGLStickMapper::~vtkOpenGLStickMapper()
{
  this->SetScaleArray(0);
  this->SetOrientationArray(0);
  this->SetSelectionIdArray(0);
}


//-----------------------------------------------------------------------------
void vtkOpenGLStickMapper::SetCameraShaderParameters(vtkOpenGLHelper &cellBO,
                                                    vtkRenderer* ren, vtkActor *actor)
{
  vtkShaderProgram *program = cellBO.Program;

  vtkOpenGLCamera *cam = (vtkOpenGLCamera *)(ren->GetActiveCamera());

  vtkMatrix4x4 *wcdc;
  vtkMatrix4x4 *wcvc;
  vtkMatrix3x3 *norms;
  vtkMatrix4x4 *vcdc;
  cam->GetKeyMatrices(ren,wcvc,norms,vcdc,wcdc);

  if (program->IsUniformUsed("VCDCMatrix"))
  {
    program->SetUniformMatrix("VCDCMatrix", vcdc);
  }

  if (!actor->GetIsIdentity())
  {
    vtkMatrix4x4 *mcwc;
    vtkMatrix3x3 *anorms;
    ((vtkOpenGLActor *)actor)->GetKeyMatrices(mcwc,anorms);
    if (program->IsUniformUsed("MCVCMatrix"))
    {
      vtkMatrix4x4::Multiply4x4(mcwc, wcvc, this->TempMatrix4);
      program->SetUniformMatrix("MCVCMatrix", this->TempMatrix4);
    }
    if (program->IsUniformUsed("normalMatrix"))
    {
      vtkMatrix3x3::Multiply3x3(anorms, norms, this->TempMatrix3);
      program->SetUniformMatrix("normalMatrix", this->TempMatrix3);
    }
  }
  else
  {
    if (program->IsUniformUsed("MCVCMatrix"))
    {
      program->SetUniformMatrix("MCVCMatrix", wcvc);
    }
    if (program->IsUniformUsed("normalMatrix"))
    {
      program->SetUniformMatrix("normalMatrix", norms);
    }
  }

  if (program->IsUniformUsed("cameraParallel"))
  {
    cellBO.Program->SetUniformi("cameraParallel", cam->GetParallelProjection());
  }
}

//-----------------------------------------------------------------------------
void vtkOpenGLStickMapper::SetMapperShaderParameters(
  vtkOpenGLHelper &cellBO,
  vtkRenderer *ren,
  vtkActor *actor)
{
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
void vtkOpenGLStickMapperCreateVBO(float * points, vtkIdType numPts,
              unsigned char *colors, int colorComponents,
              float *orients,
              float *sizes,
              vtkIdType *selectionIds,
              vtkOpenGLVertexBufferObjectGroup *VBOs,
              vtkViewport *ren)
{
  vtkFloatArray *verts = vtkFloatArray::New();
  verts->SetNumberOfComponents(3);
  verts->SetNumberOfTuples(numPts*6);
  float *vPtr = static_cast<float *>(verts->GetVoidPointer(0));

  vtkFloatArray *orientDA = vtkFloatArray::New();
  orientDA->SetNumberOfComponents(3);
  orientDA->SetNumberOfTuples(numPts*6);
  float *orPtr = static_cast<float *>(orientDA->GetVoidPointer(0));

  vtkFloatArray *radiusDA = vtkFloatArray::New();
  radiusDA->SetNumberOfComponents(1);
  radiusDA->SetNumberOfTuples(numPts*6);
  float *radPtr = static_cast<float *>(radiusDA->GetVoidPointer(0));

  vtkUnsignedCharArray *offsets = vtkUnsignedCharArray::New();
  offsets->SetNumberOfComponents(4);
  offsets->SetNumberOfTuples(numPts*6);
  unsigned char *oPtr = static_cast<unsigned char *>(offsets->GetVoidPointer(0));

  vtkUnsignedCharArray *ucolors = vtkUnsignedCharArray::New();
  ucolors->SetNumberOfComponents(4);
  ucolors->SetNumberOfTuples(numPts*6);
  unsigned char *cPtr = static_cast<unsigned char *>(ucolors->GetVoidPointer(0));

  float *pointPtr;
  float *orientPtr;
  unsigned char *colorPtr;

  for (vtkIdType i = 0; i < numPts; ++i)
  {
    // Vertices
    pointPtr = points + i*3;
    vPtr[0] = pointPtr[0];
    vPtr[1] = pointPtr[1];
    vPtr[2] = pointPtr[2];
    // copy first point, now we have two
    memcpy((vPtr + 3), vPtr, sizeof(float)*3);
    // copy first two twice more
    memcpy((vPtr + 6), vPtr, sizeof(float)*6);
    memcpy((vPtr + 12), vPtr, sizeof(float)*6);
    vPtr += 18;

    // orientation
    float length = sizes[i*3];
    orientPtr = orients + i*3;
    orPtr[0] = orientPtr[0]*length;
    orPtr[1] = orientPtr[1]*length;
    orPtr[2] = orientPtr[2]*length;
    // copy first point, now we have two
    memcpy((orPtr + 3), orPtr, sizeof(float)*3);
    // copy first two twice more
    memcpy((orPtr + 6), orPtr, sizeof(float)*6);
    memcpy((orPtr + 12), orPtr, sizeof(float)*6);
    orPtr += 18;

    // offsets
    *(oPtr++) = 0; *(oPtr++) = 0; *(oPtr++) = 0; *(oPtr++) = 0;
    *(oPtr++) = 255; *(oPtr++) = 0; *(oPtr++) = 0; *(oPtr++) = 0;
    *(oPtr++) = 255; *(oPtr++) = 0; *(oPtr++) = 255; *(oPtr++) = 0;
    *(oPtr++) = 0; *(oPtr++) = 0; *(oPtr++) = 255; *(oPtr++) = 0;
    *(oPtr++) = 255; *(oPtr++) = 255; *(oPtr++) = 255; *(oPtr++) = 0;
    *(oPtr++) = 0; *(oPtr++) = 255; *(oPtr++) = 255; *(oPtr++) = 0;

    // colors or selection ids
    if (selectionIds)
    {
      vtkIdType thisId = selectionIds[i] + 1;
      cPtr[0] = thisId % 256;
      cPtr[1] = (thisId >> 8) % 256;
      cPtr[2] = (thisId >> 16) % 256;
      cPtr[3] = 0;
    }
    else
    {
      colorPtr = colors + i*colorComponents;
      cPtr[0] = colorPtr[0];
      cPtr[1] = colorPtr[1];
      cPtr[2] = colorPtr[2];
      cPtr[3] = (colorComponents == 4 ? colorPtr[3] : 255);
    }
    memcpy(cPtr + 4, cPtr, 4);
    memcpy(cPtr + 8, cPtr, 8);
    memcpy(cPtr + 16, cPtr, 8);
    cPtr += 24;

    float radius = sizes[i*3+1];
    *(radPtr++) = radius;
    *(radPtr++) = radius;
    *(radPtr++) = radius;
    *(radPtr++) = radius;
    *(radPtr++) = radius;
    *(radPtr++) = radius;
  }

  VBOs->CacheDataArray("vertexMC", verts, ren, VTK_FLOAT);
  verts->Delete();
  VBOs->CacheDataArray("orientMC", orientDA, ren, VTK_FLOAT);
  orientDA->Delete();
  VBOs->CacheDataArray("offsetMC", offsets, ren, VTK_UNSIGNED_CHAR);
  offsets->Delete();
  VBOs->CacheDataArray("radiusMC", radiusDA, ren, VTK_FLOAT);
  radiusDA->Delete();

  if (selectionIds)
  {
    VBOs->CacheDataArray("scalarColor", NULL, ren, VTK_UNSIGNED_CHAR);
    VBOs->CacheDataArray("selectionId", ucolors, ren, VTK_UNSIGNED_CHAR);
  }
  else
  {
    VBOs->CacheDataArray("scalarColor", ucolors, ren, VTK_UNSIGNED_CHAR);
    VBOs->CacheDataArray("selectionId", NULL, ren, VTK_UNSIGNED_CHAR);
  }
  ucolors->Delete();
  VBOs->BuildAllVBOs(ren);

  return;
}
}

size_t vtkOpenGLStickMapperCreateTriangleIndexBuffer(
  vtkOpenGLBufferObject *indexBuffer,
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
  indexBuffer->Upload(indexArray, vtkOpenGLBufferObject::ElementArrayBuffer);
  return indexArray.size();
}

//-------------------------------------------------------------------------
bool vtkOpenGLStickMapper::GetNeedToRebuildBufferObjects(
  vtkRenderer *vtkNotUsed(ren),
  vtkActor *act)
{
  if (this->VBOBuildTime < this->GetMTime() ||
      this->VBOBuildTime < act->GetMTime() ||
      this->VBOBuildTime < this->CurrentInput->GetMTime() ||
      this->VBOBuildTime < this->SelectionStateChanged)
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
  vtkOpenGLStickMapperCreateVBO(
    static_cast<float *>(poly->GetPoints()->GetVoidPointer(0)),
    poly->GetPoints()->GetNumberOfPoints(),
    this->Colors ? (unsigned char *)this->Colors->GetVoidPointer(0) : NULL,
    this->Colors ? this->Colors->GetNumberOfComponents() : 0,
    static_cast<float *>(poly->GetPointData()->GetArray(this->OrientationArray)->GetVoidPointer(0)),
    static_cast<float *>(poly->GetPointData()->GetArray(this->ScaleArray)->GetVoidPointer(0)),
    picking ?
      static_cast<vtkIdType *>(poly->GetPointData()->GetArray(this->SelectionIdArray)->GetVoidPointer(0))
      : NULL,
    this->VBOs, ren);

  // create the IBO
  this->Primitives[PrimitivePoints].IBO->IndexCount = 0;
  this->Primitives[PrimitiveLines].IBO->IndexCount = 0;
  this->Primitives[PrimitiveTriStrips].IBO->IndexCount = 0;
  this->Primitives[PrimitiveTris].IBO->IndexCount =
    vtkOpenGLStickMapperCreateTriangleIndexBuffer(
      this->Primitives[PrimitiveTris].IBO,
      poly->GetPoints()->GetNumberOfPoints());
  this->VBOBuildTime.Modified();
}

//-----------------------------------------------------------------------------
void vtkOpenGLStickMapper::RenderPieceDraw(vtkRenderer* ren, vtkActor *actor)
{
  // draw polygons
  if (this->Primitives[PrimitiveTris].IBO->IndexCount)
  {
    // First we do the triangles, update the shader, set uniforms, etc.
    this->UpdateShaders(this->Primitives[PrimitiveTris], ren, actor);
    this->Primitives[PrimitiveTris].IBO->Bind();
    int numVerts = this->VBOs->GetNumberOfTuples("vertexMC");
    glDrawRangeElements(GL_TRIANGLES, 0,
                        static_cast<GLuint>(numVerts - 1),
                        static_cast<GLsizei>(this->Primitives[PrimitiveTris].IBO->IndexCount),
                        GL_UNSIGNED_INT,
                        reinterpret_cast<const GLvoid *>(NULL));
    this->Primitives[PrimitiveTris].IBO->Release();
  }
}
