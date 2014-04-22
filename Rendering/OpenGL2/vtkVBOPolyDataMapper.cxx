/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVBOPolyDataMapper.h"

#include <GL/glew.h>

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"
#include "vtkPolyData.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkPointData.h"
#include "vtkCellArray.h"
#include "vtkVector.h"

#include "vtkglBufferObject.h"
#include "vtkglShader.h"
#include "vtkglShaderProgram.h"

#include <vector>

// Bring in our shader symbols.
extern const char * mesh_vs;
extern const char * mesh_fs;

class vtkVBOPolyDataMapper::Private
{
public:
  vtkgl::BufferObject vbo;
  vtkgl::BufferObject ibo;
  size_t numberOfVertices;
  size_t numberOfIndices;

  vtkgl::Shader vertexShader;
  vtkgl::Shader fragmentShader;
  vtkgl::ShaderProgram program;
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkVBOPolyDataMapper)

//-----------------------------------------------------------------------------
vtkVBOPolyDataMapper::vtkVBOPolyDataMapper()
  : Internal(new Private), Initialized(false)
{
}

//-----------------------------------------------------------------------------
vtkVBOPolyDataMapper::~vtkVBOPolyDataMapper()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void vtkVBOPolyDataMapper::ReleaseGraphicsResources(vtkWindow*)
{
  // FIXME: Implement resource release.
}

//-----------------------------------------------------------------------------
void vtkVBOPolyDataMapper::RenderPiece(vtkRenderer* ren, vtkActor*)
{
  vtkDataObject *input= this->GetInputDataObject(0, 0);

  // Make sure that we have been properly initialized.
  if (ren->GetRenderWindow()->CheckAbortStatus())
    {
    return;
    }

  if (input == NULL)
    {
    vtkErrorMacro(<< "No input!");
    return;
    }
  else
    {
    this->InvokeEvent(vtkCommand::StartEvent,NULL);
    if (!this->Static)
      {
      this->GetInputAlgorithm()->Update();
      }
    this->InvokeEvent(vtkCommand::EndEvent,NULL);
    }

  this->TimeToDraw = 0.0;

  // FIXME: This should be moved to the renderer, render window or similar.
  if (!this->Initialized)
    {
    GLenum result = glewInit();
    bool m_valid = (result == GLEW_OK);
    if (!m_valid)
      {
      vtkErrorMacro("GLEW could not be initialized.");
      return;
      }

    if (!GLEW_VERSION_2_1)
      {
      vtkErrorMacro("GL version 2.1 is not supported by your graphics driver.");
      //m_valid = false;
      return;
      }
    this->Initialized = true;
    }

  // Update the VBO if needed.
  if (this->VBOUpdateTime < this->GetMTime())
    {
    this->UpdateVBO();
    this->VBOUpdateTime.Modified();
    }

  if (!this->Internal->program.bind())
    {
    vtkErrorMacro(<< this->Internal->program.error());
    return;
    }

  this->Internal->vbo.bind();
  this->Internal->ibo.bind();

  this->Internal->program.enableAttributeArray("vertex");
  this->Internal->program.useAttributeArray("vertex", 0,
                                            sizeof(float) * 6,
                                            VTK_FLOAT, 3,
                                            vtkgl::ShaderProgram::NoNormalize);
  this->Internal->program.enableAttributeArray("normal");
  this->Internal->program.useAttributeArray("normal", sizeof(float) * 3,
                                            sizeof(float) * 6,
                                            VTK_FLOAT, 3,
                                            vtkgl::ShaderProgram::NoNormalize);

  Eigen::Affine3f mv, proj;
  glGetFloatv(GL_MODELVIEW_MATRIX, mv.matrix().data());
  glGetFloatv(GL_PROJECTION_MATRIX, proj.matrix().data());
  vtkgl::Matrix3f normalMatrix = mv.linear().inverse().transpose();

  this->Internal->program.setUniformValue("color",
                                          vtkgl::Vector3ub(0, 255, 0));
  this->Internal->program.setUniformValue("modelView", mv.matrix());
  this->Internal->program.setUniformValue("projection", proj.matrix());
  this->Internal->program.setUniformValue("normalMatrix", normalMatrix);

  // Render the loaded spheres using the shader and bound VBO.
  glDrawRangeElements(GL_TRIANGLES, 0,
                      static_cast<GLuint>(this->Internal->numberOfVertices - 1),
                      static_cast<GLsizei>(this->Internal->numberOfIndices),
                      GL_UNSIGNED_INT,
                      reinterpret_cast<const GLvoid *>(NULL));

  this->Internal->vbo.release();
  this->Internal->ibo.release();
  this->Internal->program.disableAttributeArray("vertex");
  this->Internal->program.disableAttributeArray("normal");
  this->Internal->program.release();

  // If the timer is not accurate enough, set it to a small
  // time so that it is not zero
  if (this->TimeToDraw == 0.0)
    {
    this->TimeToDraw = 0.0001;
    }

  this->UpdateProgress(1.0);
}

//-------------------------------------------------------------------------
void vtkVBOPolyDataMapper::ComputeBounds()
{
  if (!this->GetInput())
    {
    vtkMath::UninitializeBounds(this->Bounds);
    return;
    }
  this->GetInput()->GetBounds(this->Bounds);
}

//-------------------------------------------------------------------------
void vtkVBOPolyDataMapper::UpdateVBO()
{
  vtkPolyData *poly = this->GetInput();
  if (poly == NULL || !poly->GetPointData()->GetNormals())
    {
    return;
    }

  // Due to the requirement to use derived classes rather than typedefs for
  // the vtkVector types, it is simpler to add a few convenience typedefs here
  // than use the classes which are then harder for the compiler to interpret.
  typedef vtkVector<int,    3> Vector3i;
  typedef vtkVector<float,  3> Vector3f;
  typedef vtkVector<double, 3> Vector3d;

  // Create a mesh packed with two vector 3f for vertex then normal.
  vtkDataArray* normals = poly->GetPointData()->GetNormals();
  if (normals->GetNumberOfTuples() != poly->GetNumberOfPoints())
    {
    vtkErrorMacro(<< "Polydata with bad normals.");
    }

  std::vector<Vector3f> packedMesh;
  packedMesh.reserve(poly->GetNumberOfPoints() * 2);
  Vector3d tmp;
  for (vtkIdType i = 0; i < poly->GetNumberOfPoints(); ++i)
    {
    poly->GetPoint(i, tmp.GetData());
    packedMesh.push_back(tmp.Cast<float>());
    normals->GetTuple(i, tmp.GetData());
    packedMesh.push_back(tmp.Cast<float>());
    }

  vtkCellArray* polys = poly->GetPolys();
  std::vector<unsigned int> indexArray;
  indexArray.reserve(polys->GetNumberOfCells() * 3);
  for (vtkIdType i = 0; i < polys->GetNumberOfCells(); ++i)
    {
    vtkIdType* indices(NULL);
    vtkIdType num(0);
    // Each triangle cell is of length 4 (count, tri1, tri2, tri3).
    polys->GetCell(4 * i, num, indices);
    assert(num == 3);
    indexArray.push_back(static_cast<unsigned int>(indices[0]));
    indexArray.push_back(static_cast<unsigned int>(indices[1]));
    indexArray.push_back(static_cast<unsigned int>(indices[2]));
    }

  // Now we need to upload the two arrays to the GPU.
  this->Internal->vbo.upload(packedMesh, vtkgl::BufferObject::ArrayBuffer);
  this->Internal->ibo.upload(indexArray, vtkgl::BufferObject::ElementArrayBuffer);
  this->Internal->numberOfVertices = packedMesh.size() / 2;
  this->Internal->numberOfIndices = indexArray.size();

  // Check if the shader has been initialized.
  if (this->Internal->vertexShader.type() == vtkgl::Shader::Unknown)
    {
    this->Internal->vertexShader.setType(vtkgl::Shader::Vertex);
    this->Internal->vertexShader.setSource(mesh_vs);
    this->Internal->fragmentShader.setType(vtkgl::Shader::Fragment);
    this->Internal->fragmentShader.setSource(mesh_fs);
    if (!this->Internal->vertexShader.compile())
      {
      vtkErrorMacro(<< this->Internal->vertexShader.error());
      }
    if (!this->Internal->fragmentShader.compile())
      {
      vtkErrorMacro(<< this->Internal->fragmentShader.error());
      }
    this->Internal->program.attachShader(this->Internal->vertexShader);
    this->Internal->program.attachShader(this->Internal->fragmentShader);
    if (!this->Internal->program.link())
      {
      vtkErrorMacro(<< this->Internal->program.error());
      }
    }
}

//-----------------------------------------------------------------------------
bool vtkVBOPolyDataMapper::GetIsOpaque()
{
  // Straight copy of what the vtkPainterPolyDataMapper was doing.
  if (this->ScalarVisibility &&
    this->ColorMode == VTK_COLOR_MODE_DEFAULT)
    {
    vtkPolyData* input =
      vtkPolyData::SafeDownCast(this->GetInputDataObject(0, 0));
    if (input)
      {
      int cellFlag;
      vtkDataArray* scalars = this->GetScalars(input,
        this->ScalarMode, this->ArrayAccessMode, this->ArrayId,
        this->ArrayName, cellFlag);
      if (scalars && scalars->IsA("vtkUnsignedCharArray") &&
        (scalars->GetNumberOfComponents() ==  4 /*(RGBA)*/ ||
         scalars->GetNumberOfComponents() == 2 /*(LuminanceAlpha)*/))
        {
        vtkUnsignedCharArray* colors =
          static_cast<vtkUnsignedCharArray*>(scalars);
        if ((colors->GetNumberOfComponents() == 4 && colors->GetValueRange(3)[0] < 255) ||
          (colors->GetNumberOfComponents() == 2 && colors->GetValueRange(1)[0] < 255))
          {
          // If the opacity is 255, despite the fact that the user specified
          // RGBA, we know that the Alpha is 100% opaque. So treat as opaque.
          return false;
          }
        }
      }
    }
  return this->Superclass::GetIsOpaque();
}

//-----------------------------------------------------------------------------
void vtkVBOPolyDataMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
