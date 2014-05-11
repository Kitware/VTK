/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLPolyDataMapper2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGL2PolyDataMapper2D.h"

#include "vtkglVBOHelper.h"

#include "vtkActor2D.h"
#include "vtkCellArray.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkOpenGLGL2PSHelper.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPlaneCollection.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty2D.h"
#include "vtkScalarsToColors.h"
#include "vtkUnsignedCharArray.h"
#include "vtkViewport.h"
#include "vtkWindow.h"
#include "vtkgluPickMatrix.h"
#include "vtkOpenGLError.h"

#include <cmath>

// Bring in our shader symbols.
#include "vtkglPolyData2DVS.h"
#include "vtkglPolyDataFS.h"

class vtkOpenGL2PolyDataMapper2D::Private
{
public:
  vtkgl::BufferObject vbo;
  vtkgl::VBOLayout layout;

  // Structures for the various cell types we render.
  vtkgl::CellBO points;
  vtkgl::CellBO lines;
  vtkgl::CellBO tris;
  vtkgl::CellBO triStrips;

  const char *vertexShaderFile;
  const char *fragmentShaderFile;

  vtkgl::Shader vertexShader;
  vtkgl::Shader fragmentShader;
  vtkgl::ShaderProgram program;

  // Array of colors, along with the number of components.
  std::vector<unsigned char> colors;
  unsigned char colorComponents;
  bool colorAttributes;

  vtkTimeStamp propertiesTime;
  vtkTimeStamp shaderBuildTime;

  Private() : colorAttributes(false)
  {
    this->vertexShader.SetType(vtkgl::Shader::Vertex);
    this->fragmentShader.SetType(vtkgl::Shader::Fragment);
  }
};

vtkStandardNewMacro(vtkOpenGL2PolyDataMapper2D);

//-----------------------------------------------------------------------------
vtkOpenGL2PolyDataMapper2D::vtkOpenGL2PolyDataMapper2D()
  : Internal(new Private), Initialized(false)
{
}

//-----------------------------------------------------------------------------
vtkOpenGL2PolyDataMapper2D::~vtkOpenGL2PolyDataMapper2D()
{
  delete this->Internal;
}


//-----------------------------------------------------------------------------
void vtkOpenGL2PolyDataMapper2D::UpdateShader(vtkViewport* vtkNotUsed(viewport), vtkActor2D *vtkNotUsed(actor))
{
  this->Internal->fragmentShaderFile = vtkglPolyDataFS;
  this->Internal->vertexShaderFile = vtkglPolyData2DVS;

  // compile and link the shader program if it has changed
  // eventually use some sort of caching here
  if (this->Internal->vertexShader.GetType() == vtkgl::Shader::Unknown ||
      this->Internal->propertiesTime > this->Internal->shaderBuildTime)
    {
    // Build our shader if necessary.
    std::string vertexShaderSource = this->Internal->vertexShaderFile;
    if (this->Internal->colorAttributes)
      {
      vertexShaderSource = vtkgl::replace(vertexShaderSource,
                                   "//VTK::Color::Dec",
                                   "attribute vec4 diffuseColor;");
      }
    else
      {
      vertexShaderSource = vtkgl::replace(vertexShaderSource,
                                   "//VTK::Color::Dec",
                                   "uniform vec3 diffuseColor;");
      }
    cout << "VS: " << vertexShaderSource << endl;

    this->Internal->vertexShader.SetSource(vertexShaderSource);
    this->Internal->fragmentShader.SetSource(this->Internal->fragmentShaderFile);
    if (!this->Internal->vertexShader.Compile())
      {
      vtkErrorMacro(<< this->Internal->vertexShader.GetError());
      }
    if (!this->Internal->fragmentShader.Compile())
      {
      vtkErrorMacro(<< this->Internal->fragmentShader.GetError());
      }
    if (!this->Internal->program.attachShader(this->Internal->vertexShader))
      {
      vtkErrorMacro(<< this->Internal->program.error());
      }
    if (!this->Internal->program.attachShader(this->Internal->fragmentShader))
      {
      vtkErrorMacro(<< this->Internal->program.error());
      }
    if (!this->Internal->program.link())
      {
      vtkErrorMacro(<< this->Internal->program.error());
      }
    this->Internal->shaderBuildTime.Modified();
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGL2PolyDataMapper2D::SetPropertyShaderParameters(vtkViewport*,
                                                       vtkActor2D *actor)
{
  // Query the actor for some of the properties that can be applied.
  float opacity = static_cast<float>(actor->GetProperty()->GetOpacity());
  double *dColor = actor->GetProperty()->GetColor();
  vtkgl::Vector3ub diffuseColor(static_cast<unsigned char>(dColor[0] * 255.0),
                         static_cast<unsigned char>(dColor[1] * 255.0),
                         static_cast<unsigned char>(dColor[2] * 255.0));

  this->Internal->program.setUniformValue("opacity", opacity);
  this->Internal->program.setUniformValue("diffuseColor", diffuseColor);
}

//-----------------------------------------------------------------------------
void vtkOpenGL2PolyDataMapper2D::SetCameraShaderParameters(vtkViewport* viewport, vtkActor2D *actor)
{
  // Get the position of the actor
  int size[2];
  size[0] = viewport->GetSize()[0];
  size[1] = viewport->GetSize()[1];

  double *vport = viewport->GetViewport();
  int* actorPos =
    actor->GetPositionCoordinate()->GetComputedViewportValue(viewport);

  // get window info
  double *tileViewPort = viewport->GetVTKWindow()->GetTileViewport();
  double visVP[4];
  visVP[0] = (vport[0] >= tileViewPort[0]) ? vport[0] : tileViewPort[0];
  visVP[1] = (vport[1] >= tileViewPort[1]) ? vport[1] : tileViewPort[1];
  visVP[2] = (vport[2] <= tileViewPort[2]) ? vport[2] : tileViewPort[2];
  visVP[3] = (vport[3] <= tileViewPort[3]) ? vport[3] : tileViewPort[3];
  if (visVP[0] >= visVP[2])
    {
    return;
    }
  if (visVP[1] >= visVP[3])
    {
    return;
    }
  size[0] =
    vtkMath::Round(size[0]*(visVP[2] - visVP[0])/(vport[2] - vport[0]));
  size[1] =
    vtkMath::Round(size[1]*(visVP[3] - visVP[1])/(vport[3] - vport[1]));

  int *winSize = viewport->GetVTKWindow()->GetSize();

  int xoff = static_cast<int>(actorPos[0] - (visVP[0] - vport[0])*
                              winSize[0]);
  int yoff = static_cast<int>(actorPos[1] - (visVP[1] - vport[1])*
                              winSize[1]);

  // set ortho projection
  float left = -xoff;
  float right = -xoff + size[0];
  float bottom = -yoff;
  float top = -yoff + size[1];

  // it's an error to call glOrtho with
  // either left==right or top==bottom
  if (left==right)
    {
    right = left + 1.0;
    }
  if (bottom==top)
    {
    top = bottom + 1.0;
    }

  float nearV = 0;
  float farV = 1;
  if (actor->GetProperty()->GetDisplayLocation() !=
       VTK_FOREGROUND_LOCATION)
    {
    nearV = -1;
    farV = 0;
    }

  // compute the combined ModelView matrix and send it down to save time in the shader
  vtkMatrix4x4 *tmpMat = vtkMatrix4x4::New();
  tmpMat->SetElement(0,0,2.0/(right - left));
  tmpMat->SetElement(1,1,2.0/(top - bottom));
  tmpMat->SetElement(2,2,-2.0/(farV - nearV));
  tmpMat->SetElement(3,3,1.0);
  tmpMat->SetElement(0,3,-1.0*(right+left)/(right-left));
  tmpMat->SetElement(1,3,-1.0*(top+bottom)/(top-bottom));
  tmpMat->SetElement(2,3,-1.0*(farV+nearV)/(farV-nearV));
  tmpMat->Transpose();
  this->Internal->program.setUniformValue("WCVCMatrix", tmpMat);

  tmpMat->Delete();
}


//-------------------------------------------------------------------------
void vtkOpenGL2PolyDataMapper2D::UpdateVBO(vtkActor2D *act)
{
  vtkPolyData *poly = this->GetInput();
  if (poly == NULL)
    {
    return;
    }

  // Mark our properties as updated.
  this->Internal->propertiesTime.Modified();

  this->Internal->colorAttributes = false;
  if (this->ScalarVisibility)
    {
    // We must figure out how the scalars should be mapped to the polydata.
    this->MapScalars(act->GetProperty()->GetOpacity());
    if (this->Colors && (this->Colors->GetNumberOfComponents() == 3 ||
        this->Colors->GetNumberOfComponents() == 4))
      {
      this->Internal->colorAttributes = true;
      }
    }


  // Iterate through all of the different types in the polydata, building VBOs
  // and IBOs as appropriate for each type.
  this->Internal->layout =
    CreateVBO(poly->GetPoints(),
              NULL,
              this->Internal->colorAttributes ? this->Colors->GetPointer(0) : NULL,
              this->Colors ? this->Colors->GetNumberOfComponents() : 0,
              this->Internal->vbo);


  this->Internal->triStrips.indexCount = CreateMultiIndexBuffer(poly->GetStrips(),
                         this->Internal->triStrips.ibo,
                         this->Internal->triStrips.offsetArray,
                         this->Internal->triStrips.elementsArray);
  this->Internal->lines.indexCount = CreateMultiIndexBuffer(poly->GetLines(),
                         this->Internal->lines.ibo,
                         this->Internal->lines.offsetArray,
                         this->Internal->lines.elementsArray);
  this->Internal->points.indexCount = CreatePointIndexBuffer(poly->GetVerts(),
                                                    this->Internal->points.ibo);
  this->Internal->tris.indexCount = CreateTriangleIndexBuffer(poly->GetPolys(),
                                                              this->Internal->tris.ibo,
                                                              poly->GetPoints());
}


void vtkOpenGL2PolyDataMapper2D::RenderOverlay(vtkViewport* viewport,
                                              vtkActor2D* actor)
{
  vtkOpenGLClearErrorMacro();
  int            numPts;
  vtkPolyData    *input=static_cast<vtkPolyData *>(this->GetInput());
  int            j;
  vtkPoints      *p, *displayPts;
  vtkDataArray*  t = 0;

  vtkDebugMacro (<< "vtkOpenGL2PolyDataMapper2D::Render");

  if ( input == NULL )
    {
    vtkErrorMacro(<< "No input!");
    return;
    }
  else
    {
    this->GetInputAlgorithm()->Update();
    numPts = input->GetNumberOfPoints();
    }

  if (numPts == 0)
    {
    vtkDebugMacro(<< "No points!");
    return;
    }

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

  if ( this->LookupTable == NULL )
    {
    this->CreateDefaultLookupTable();
    }

  // Texture and color by texture
  t = input->GetPointData()->GetTCoords();
  if ( t )
    {
    int tDim = t->GetNumberOfComponents();
    if (tDim != 2)
      {
      vtkDebugMacro(<< "Currently only 2d textures are supported.\n");
      t = 0;
      }
    }

  // Transform the points, if necessary
  p = input->GetPoints();
  if ( this->TransformCoordinate )
    {
    numPts = p->GetNumberOfPoints();
    displayPts = vtkPoints::New();
    displayPts->SetNumberOfPoints(numPts);
    for ( j=0; j < numPts; j++ )
      {
      this->TransformCoordinate->SetValue(p->GetPoint(j));
      if (this->TransformCoordinateUseDouble)
        {
        double* dtmp = this->TransformCoordinate->GetComputedDoubleViewportValue(viewport);
        displayPts->SetPoint(j,dtmp[0], dtmp[1], 0.0);
        }
      else
        {
        int* itmp = this->TransformCoordinate->GetComputedViewportValue(viewport);
        displayPts->SetPoint(j,itmp[0], itmp[1], 0.0);
        }
      }
    p = displayPts;
    }
  if ( this->TransformCoordinate )
    {
    p->Delete();
    }

  // push a 2D matrix on the stack
  if(viewport->GetIsPicking())
    {
    vtkgluPickMatrix(viewport->GetPickX(), viewport->GetPickY(),
                     viewport->GetPickWidth(),
                     viewport->GetPickHeight(),
                     viewport->GetOrigin(), viewport->GetSize());
    }

  if (!t)
    {
    glDisable(GL_TEXTURE_2D);
    }

  // Assume we want to do Zbuffering for now.
  // we may turn this off later
  glDepthMask(GL_TRUE);

  // Update the VBO if needed.
  if (this->VBOUpdateTime < this->GetMTime())
    {
    this->UpdateVBO(actor);
    this->VBOUpdateTime.Modified();
    }

  // Figure out and build the appropriate shader for the mapped geometry.
  this->UpdateShader(viewport, actor);

  if (!this->Internal->program.bind())
    {
    vtkErrorMacro(<< this->Internal->program.error());
    return;
    }

  this->SetCameraShaderParameters(viewport, actor);
  this->SetPropertyShaderParameters(viewport, actor);

  this->Internal->vbo.bind();

  size_t stride = sizeof(float) * 3;
  if (this->Internal->colorAttributes)
    {
    stride += sizeof(float);
    }

  this->Internal->program.enableAttributeArray("vertexWC");
  this->Internal->program.useAttributeArray("vertexWC", 0,
                                            stride,
                                            VTK_FLOAT, 3,
                                            vtkgl::ShaderProgram::NoNormalize);
  if (this->Internal->colorAttributes)
    {
    if (!this->Internal->program.enableAttributeArray("diffuseColor"))
      {
      vtkErrorMacro(<< this->Internal->program.error());
      }
    this->Internal->program.useAttributeArray("diffuseColor", sizeof(float) * 6,
                                              stride,
                                              VTK_UNSIGNED_CHAR,
                                              this->Colors->GetNumberOfComponents(),
                                              vtkgl::ShaderProgram::Normalize);
    }

  if (this->Internal->points.indexCount)
    {
    // Set the PointSize
    glPointSize(actor->GetProperty()->GetPointSize());
    //vtkOpenGLGL2PSHelper::SetPointSize(actor->GetProperty()->GetPointSize());

    this->Internal->points.ibo.bind();
    glDrawRangeElements(GL_POINTS, 0,
                        static_cast<GLuint>(this->Internal->layout.VertexCount - 1),
                        static_cast<GLsizei>(this->Internal->points.indexCount),
                        GL_UNSIGNED_INT,
                        reinterpret_cast<const GLvoid *>(NULL));
    this->Internal->points.ibo.release();
    }

  if (this->Internal->tris.indexCount)
    {
    this->Internal->tris.ibo.bind();
    glDrawRangeElements(GL_TRIANGLES, 0,
                        static_cast<GLuint>(this->Internal->layout.VertexCount - 1),
                        static_cast<GLsizei>(this->Internal->tris.indexCount),
                        GL_UNSIGNED_INT,
                        reinterpret_cast<const GLvoid *>(NULL));
    this->Internal->tris.ibo.release();
    }

  if (this->Internal->lines.offsetArray.size() > 0)
    {
    this->Internal->lines.ibo.bind();
    for (int eCount = 0; eCount < this->Internal->lines.offsetArray.size(); ++eCount)
      {
      glDrawElements(GL_LINE_STRIP,
        this->Internal->lines.elementsArray[eCount],
        GL_UNSIGNED_INT,
        (GLvoid *)(this->Internal->lines.offsetArray[eCount]));
      }
    this->Internal->lines.ibo.release();
    }

  if (this->Internal->triStrips.offsetArray.size() > 0)
    {
    this->Internal->triStrips.ibo.bind();
    for (int eCount = 0; eCount < this->Internal->triStrips.offsetArray.size(); ++eCount)
      {
      glDrawElements(GL_LINE_STRIP,
        this->Internal->triStrips.elementsArray[eCount],
        GL_UNSIGNED_INT,
        (GLvoid *)(this->Internal->triStrips.offsetArray[eCount]));
      }
    this->Internal->triStrips.ibo.release();
    }

  this->Internal->vbo.release();
  this->Internal->program.disableAttributeArray("vertexWC");
  if (this->Internal->colorAttributes)
    {
    this->Internal->program.disableAttributeArray("diffuseColor");
    }
  this->Internal->program.release();

  vtkOpenGLCheckErrorMacro("failed after RenderOverlay");
}

//----------------------------------------------------------------------------
void vtkOpenGL2PolyDataMapper2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
