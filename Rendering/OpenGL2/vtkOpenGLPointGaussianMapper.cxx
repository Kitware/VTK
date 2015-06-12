/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLPointGaussianMapper.h"

#include "vtkOpenGLHelper.h"

#include "vtkHardwareSelector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkMatrix4x4.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLIndexBufferObject.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkOpenGLVertexBufferObject.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkShaderProgram.h"

#include "vtkPointGaussianVS.h"
#include "vtkPolyDataFS.h"

#include "vtk_glew.h"



class vtkOpenGLPointGaussianMapperHelper : public vtkOpenGLPolyDataMapper
{
public:
  static vtkOpenGLPointGaussianMapperHelper* New();
  vtkTypeMacro(vtkOpenGLPointGaussianMapperHelper, vtkOpenGLPolyDataMapper)

  vtkPointGaussianMapper *Owner;

protected:
  vtkOpenGLPointGaussianMapperHelper();
  ~vtkOpenGLPointGaussianMapperHelper();

  // Description:
  // Create the basic shaders before replacement
  virtual void GetShaderTemplate(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *, vtkActor *);

  // Description:
  // Perform string replacments on the shader templates
  virtual void ReplaceShaderColor(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *, vtkActor *);
  virtual void ReplaceShaderPositionVC(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *, vtkActor *);

  // Description:
  // Set the shader parameters related to the Camera
  virtual void SetCameraShaderParameters(vtkOpenGLHelper &cellBO, vtkRenderer *ren, vtkActor *act);

  // Description:
  // Set the shader parameters related to the actor/mapper
  virtual void SetMapperShaderParameters(vtkOpenGLHelper &cellBO, vtkRenderer *ren, vtkActor *act);

  // Description:
  // Does the VBO/IBO need to be rebuilt
  virtual bool GetNeedToRebuildBufferObjects(vtkRenderer *ren, vtkActor *act);

  // Description:
  // Update the VBO to contain point based values
  virtual void BuildBufferObjects(vtkRenderer *ren, vtkActor *act);

  virtual void RenderPieceDraw(vtkRenderer *ren, vtkActor *act);

  // Description:
  // Does the shader source need to be recomputed
  virtual bool GetNeedToRebuildShaders(vtkOpenGLHelper &cellBO,
    vtkRenderer *ren, vtkActor *act);

  bool UsingPoints;

private:
  vtkOpenGLPointGaussianMapperHelper(const vtkOpenGLPointGaussianMapperHelper&); // Not implemented.
  void operator=(const vtkOpenGLPointGaussianMapperHelper&); // Not implemented.
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLPointGaussianMapperHelper)

//-----------------------------------------------------------------------------
vtkOpenGLPointGaussianMapperHelper::vtkOpenGLPointGaussianMapperHelper()
{
  this->Owner = NULL;
}


//-----------------------------------------------------------------------------
void vtkOpenGLPointGaussianMapperHelper::GetShaderTemplate(
  std::map<vtkShader::Type, vtkShader *> shaders,
  vtkRenderer *ren, vtkActor *actor)
{
  this->Superclass::GetShaderTemplate(shaders,ren,actor);

  vtkPolyData *poly = this->CurrentInput;
  bool hasScaleArray = this->Owner->GetScaleArray() != NULL &&
                       poly->GetPointData()->HasArray(this->Owner->GetScaleArray());
  if (!hasScaleArray && this->Owner->GetDefaultRadius() == 0.0)
    {
    this->UsingPoints = true;
    }
  else
    {
    this->UsingPoints = false;
    // for splats use a special shader than handles the offsets
    shaders[vtkShader::Vertex]->SetSource(vtkPointGaussianVS);
    }

}

void vtkOpenGLPointGaussianMapperHelper::ReplaceShaderPositionVC(
  std::map<vtkShader::Type, vtkShader *> shaders,
  vtkRenderer *ren, vtkActor *actor)
{
  if (!this->UsingPoints)
    {
    std::string VSSource = shaders[vtkShader::Vertex]->GetSource();
    std::string FSSource = shaders[vtkShader::Fragment]->GetSource();

    vtkShaderProgram::Substitute(FSSource,
      "//VTK::PositionVC::Dec",
      "varying vec2 offsetVC;");

    vtkShaderProgram::Substitute(VSSource,
      "//VTK::Camera::Dec",
      "uniform mat4 VCDCMatrix;\n"
      "uniform mat4 MCVCMatrix;");

    shaders[vtkShader::Vertex]->SetSource(VSSource);
    shaders[vtkShader::Fragment]->SetSource(FSSource);
    }

  this->Superclass::ReplaceShaderPositionVC(shaders,ren,actor);
}

void vtkOpenGLPointGaussianMapperHelper::ReplaceShaderColor(
  std::map<vtkShader::Type, vtkShader *> shaders,
  vtkRenderer *ren, vtkActor *actor)
{
  if (!this->UsingPoints)
    {
    std::string VSSource = shaders[vtkShader::Vertex]->GetSource();
    std::string FSSource = shaders[vtkShader::Fragment]->GetSource();

    vtkShaderProgram::Substitute(FSSource,"//VTK::Color::Impl",
      // compute the eye position and unit direction
      "//VTK::Color::Impl\n"
      "  float dist2 = dot(offsetVC.xy,offsetVC.xy);\n"
      "  if (dist2 > 9.0) { discard; }\n"
      "  float gaussian = exp(-0.5*dist2);\n"
      "  opacity = opacity*gaussian;"
      //  "  opacity = opacity*0.5;"
      , false);

    shaders[vtkShader::Vertex]->SetSource(VSSource);
    shaders[vtkShader::Fragment]->SetSource(FSSource);
    }

  this->Superclass::ReplaceShaderColor(shaders,ren,actor);
  //cerr << shaders[vtkShader::Fragment]->GetSource() << endl;
}

//-----------------------------------------------------------------------------
bool vtkOpenGLPointGaussianMapperHelper::GetNeedToRebuildShaders(
  vtkOpenGLHelper &cellBO, vtkRenderer* ren, vtkActor *actor)
{
  this->LastLightComplexity = 0;

  vtkHardwareSelector* selector = ren->GetSelector();
  int picking = selector ? selector->GetCurrentPass() : -1;
  if (this->LastSelectionState != picking)
    {
    this->SelectionStateChanged.Modified();
    this->LastSelectionState = picking;
    }

  // has something changed that would require us to recreate the shader?
  // candidates are
  // property modified (representation interpolation and lighting)
  // input modified
  // light complexity changed
  if (cellBO.Program == 0 ||
      cellBO.ShaderSourceTime < this->GetMTime() ||
      cellBO.ShaderSourceTime < actor->GetMTime() ||
      cellBO.ShaderSourceTime < this->CurrentInput->GetMTime() ||
      cellBO.ShaderSourceTime < this->SelectionStateChanged ||
      cellBO.ShaderSourceTime < this->DepthPeelingChanged ||
      cellBO.ShaderSourceTime < this->LightComplexityChanged)
    {
    return true;
    }

  return false;
}

//-----------------------------------------------------------------------------
vtkOpenGLPointGaussianMapperHelper::~vtkOpenGLPointGaussianMapperHelper()
{
}

//-----------------------------------------------------------------------------
void vtkOpenGLPointGaussianMapperHelper::SetCameraShaderParameters(vtkOpenGLHelper &cellBO,
                                                    vtkRenderer* ren, vtkActor *actor)
{
  if (this->UsingPoints)
    {
    this->Superclass::SetCameraShaderParameters(cellBO,ren,actor);
    }
  else
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

    // add in uniforms for parallel and distance
    cellBO.Program->SetUniformi("cameraParallel", cam->GetParallelProjection());
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLPointGaussianMapperHelper::SetMapperShaderParameters(vtkOpenGLHelper &cellBO,
                                                         vtkRenderer *ren, vtkActor *actor)
{
  if (!this->UsingPoints)
    {
    if (cellBO.IBO->IndexCount && (this->VBOBuildTime > cellBO.AttributeUpdateTime ||
        cellBO.ShaderSourceTime > cellBO.AttributeUpdateTime))
      {
      cellBO.VAO->Bind();
      if (!cellBO.VAO->AddAttributeArray(cellBO.Program, this->VBO,
                                      "offsetMC", this->VBO->ColorOffset+sizeof(float),
                                      this->VBO->Stride, VTK_FLOAT, 2, false))
        {
        vtkErrorMacro(<< "Error setting 'offsetMC' in shader VAO.");
        }
      }
    }

  this->Superclass::SetMapperShaderParameters(cellBO,ren,actor);
}


namespace
{
// internal function called by CreateVBO
template< typename PointDataType, typename SizeDataType >
void vtkOpenGLPointGaussianMapperHelperPackVBOTemplate2(
              std::vector< float >::iterator& it,
              PointDataType* points, vtkIdType numPts,
              unsigned char *colors, int colorComponents,
              SizeDataType* sizes, float defaultSize)
{
  PointDataType *pointPtr;
  unsigned char *colorPtr;

  // if there are no per point sizes and the default size is zero
  // then just render points, saving memory and speed
  if (!sizes && defaultSize == 0.0)
    {
    unsigned char white[4] = {255, 255, 255, 255};

    for (vtkIdType i = 0; i < numPts; ++i)
      {
      pointPtr = points + i*3;
      colorPtr = colors ? (colors + i*colorComponents) : white;

      // Vertices
      *(it++) = pointPtr[0];
      *(it++) = pointPtr[1];
      *(it++) = pointPtr[2];
      *(it++) = *reinterpret_cast<float *>(colorPtr);
      }
    }
  else // otherwise splats
    {
    float cos30 = cos(vtkMath::RadiansFromDegrees(30.0));

    unsigned char white[4] = {255, 255, 255, 255};

    for (vtkIdType i = 0; i < numPts; ++i)
      {
      pointPtr = points + i*3;
      colorPtr = colors ? (colors + i*colorComponents) : white;
      float radius = sizes ? sizes[i] : defaultSize;
      radius *= 3.0;

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
    }
}

template< typename PointDataType >
void vtkOpenGLPointGaussianMapperHelperPackVBOTemplate(
    std::vector< float >::iterator& it,
    PointDataType* points, vtkIdType numPts,
    unsigned char *colors, int colorComponents,
    vtkDataArray* sizes, float defaultSize)
{
  if (sizes)
    {
    switch (sizes->GetDataType())
      {
    vtkTemplateMacro(
          vtkOpenGLPointGaussianMapperHelperPackVBOTemplate2(
            it, points, numPts, colors, colorComponents,
            static_cast<VTK_TT*>(sizes->GetVoidPointer(0)),
            defaultSize)
          );
      }
    }
  else
    {
    vtkOpenGLPointGaussianMapperHelperPackVBOTemplate2(
          it, points, numPts, colors, colorComponents,
          static_cast<float*>(NULL), defaultSize);
    }
}

void vtkOpenGLPointGaussianMapperHelperCreateVBO(
    vtkPoints* points, unsigned char* colors, int colorComponents,
    vtkDataArray* sizes, float defaultSize,
    vtkOpenGLVertexBufferObject *VBO,
    bool usingPoints)
{
  // Figure out how big each block will be, currently 6 floats.
  int blockSize = 3;  // x y z
  VBO->VertexOffset = 0;
  VBO->NormalOffset = 0;
  VBO->TCoordOffset = 0;
  VBO->TCoordComponents = 0;
  VBO->ColorComponents = colorComponents;
  VBO->ColorOffset = sizeof(float) * blockSize;
  ++blockSize; // color

  if (usingPoints)
    {
    VBO->Stride = sizeof(float) * blockSize;

    // Create a buffer, and copy the data over.
    VBO->PackedVBO.resize(blockSize * points->GetNumberOfPoints());
    std::vector<float>::iterator it = VBO->PackedVBO.begin();

    switch(points->GetDataType())
      {
      vtkTemplateMacro(
            vtkOpenGLPointGaussianMapperHelperPackVBOTemplate(
              it, static_cast<VTK_TT*>(points->GetVoidPointer(0)),
              points->GetNumberOfPoints(),colors,colorComponents,
              sizes,defaultSize));
      }
    VBO->Upload(VBO->PackedVBO, vtkOpenGLBufferObject::ArrayBuffer);
    VBO->VertexCount = points->GetNumberOfPoints();
    }
  else
    {
    // two more floats
    blockSize += 2;  // offset
    VBO->Stride = sizeof(float) * blockSize;

    // Create a buffer, and copy the data over.
    VBO->PackedVBO.resize(blockSize * points->GetNumberOfPoints() * 3);
    std::vector<float>::iterator it = VBO->PackedVBO.begin();

    switch(points->GetDataType())
      {
      vtkTemplateMacro(
            vtkOpenGLPointGaussianMapperHelperPackVBOTemplate(
              it, static_cast<VTK_TT*>(points->GetVoidPointer(0)),
              points->GetNumberOfPoints(),colors,colorComponents,
              sizes,defaultSize));
      }
    VBO->Upload(VBO->PackedVBO, vtkOpenGLBufferObject::ArrayBuffer);
    VBO->VertexCount = points->GetNumberOfPoints() * 3;
    }
  return;
}
}

//-------------------------------------------------------------------------
bool vtkOpenGLPointGaussianMapperHelper::GetNeedToRebuildBufferObjects(
  vtkRenderer *vtkNotUsed(ren),
  vtkActor *act)
{
  // picking state does not require a rebuild, unlike our parent
  if (this->VBOBuildTime < this->GetMTime() ||
      this->VBOBuildTime < act->GetMTime() ||
      this->VBOBuildTime < this->CurrentInput->GetMTime() ||
      this->VBOBuildTime < this->Owner->GetMTime())
    {
    return true;
    }
  return false;
}

//-------------------------------------------------------------------------
void vtkOpenGLPointGaussianMapperHelper::BuildBufferObjects(
  vtkRenderer *vtkNotUsed(ren), vtkActor *vtkNotUsed(act))
{
  vtkPolyData *poly = this->CurrentInput;

  if (poly == NULL)// || !poly->GetPointData()->GetNormals())
    {
    return;
    }

  bool hasScaleArray = this->Owner->GetScaleArray() != NULL &&
                       poly->GetPointData()->HasArray(this->Owner->GetScaleArray());
  if (!hasScaleArray && this->Owner->GetDefaultRadius() == 0.0)
    {
    this->UsingPoints = true;
    }
  else
    {
    this->UsingPoints = false;
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
  vtkOpenGLPointGaussianMapperHelperCreateVBO(
      poly->GetPoints(),
      this->Colors ? (unsigned char *)this->Colors->GetVoidPointer(0) : (unsigned char*)NULL,
      this->Colors ? this->Colors->GetNumberOfComponents() : 0,
      hasScaleArray ? poly->GetPointData()->GetArray(
        this->Owner->GetScaleArray()) : (vtkDataArray*)NULL,
      this->Owner->GetDefaultRadius(),
      this->VBO, this->UsingPoints);

  // we use no IBO
  this->Points.IBO->IndexCount = 0;
  this->Lines.IBO->IndexCount = 0;
  this->TriStrips.IBO->IndexCount = 0;
  this->Tris.IBO->IndexCount = this->VBO->VertexCount;
}

//-----------------------------------------------------------------------------
void vtkOpenGLPointGaussianMapperHelper::RenderPieceDraw(vtkRenderer* ren, vtkActor *actor)
{
  // draw polygons
  if (this->Owner->GetEmissive() != 0)
    {
    glBlendFunc( GL_SRC_ALPHA, GL_ONE);  // additive for emissive sources
    }
  if (this->VBO->VertexCount)
    {
    // First we do the triangles or points, update the shader, set uniforms, etc.
    this->UpdateShaders(this->Tris, ren, actor);
    if (this->UsingPoints)
      {
      glDrawArrays(GL_POINTS, 0,
        static_cast<GLuint>(this->VBO->VertexCount));
      }
    else
      {
      glDrawArrays(GL_TRIANGLES, 0,
        static_cast<GLuint>(this->VBO->VertexCount));
      }
    }
}


//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLPointGaussianMapper)

//-----------------------------------------------------------------------------
vtkOpenGLPointGaussianMapper::vtkOpenGLPointGaussianMapper()
{
  this->Helper = vtkOpenGLPointGaussianMapperHelper::New();
  this->Helper->Owner = this;
}

vtkOpenGLPointGaussianMapper::~vtkOpenGLPointGaussianMapper()
{
  this->Helper->Delete();
  this->Helper = 0;
}

void vtkOpenGLPointGaussianMapper::RenderPiece(vtkRenderer *ren, vtkActor *act)
{
  if (this->GetMTime() > this->HelperUpdateTime)
    {
    this->Helper->vtkPolyDataMapper::ShallowCopy(this);
    this->HelperUpdateTime.Modified();
    }
  this->Helper->RenderPiece(ren,act);
}

//-----------------------------------------------------------------------------
void vtkOpenGLPointGaussianMapper::ReleaseGraphicsResources(vtkWindow* win)
{
  this->Helper->ReleaseGraphicsResources(win);
  this->Helper->SetInputData(0);
  this->Modified();
}

//-----------------------------------------------------------------------------
bool vtkOpenGLPointGaussianMapper::GetIsOpaque()
{
  if (this->Emissive)
    {
    return false;
    }
  return this->Superclass::GetIsOpaque();
}

//-----------------------------------------------------------------------------
void vtkOpenGLPointGaussianMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
