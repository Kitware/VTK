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
#include "vtkTexturedActor2D.h"
#include "vtkCellArray.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkOpenGLGL2PSHelper.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGL2Texture.h"
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

#include "vtkOpenGL2RenderWindow.h"
#include "vtkOpenGL2ShaderCache.h"

#include <cmath>

// Bring in our shader symbols.
#include "vtkglPolyData2DVS.h"
#include "vtkglPolyData2DFS.h"

class vtkOpenGL2PolyDataMapper2D::Private
{
public:
  vtkgl::BufferObject vbo;
  vtkgl::VBOLayout layout;

  // Structures for the various cell types we render.
  // we keep the shader in points
  vtkgl::CellBO points;
  vtkgl::CellBO lines;
  vtkgl::CellBO tris;
  vtkgl::CellBO triStrips;

  Private()  { }
};

vtkStandardNewMacro(vtkOpenGL2PolyDataMapper2D);

//-----------------------------------------------------------------------------
vtkOpenGL2PolyDataMapper2D::vtkOpenGL2PolyDataMapper2D()
  : Internal(new Private)
{
}

//-----------------------------------------------------------------------------
vtkOpenGL2PolyDataMapper2D::~vtkOpenGL2PolyDataMapper2D()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void vtkOpenGL2PolyDataMapper2D::BuildShader(std::string &VSSource, std::string &FSSource, vtkViewport* ren, vtkActor2D *actor)
{
  VSSource = vtkglPolyData2DVS;
  FSSource = vtkglPolyData2DFS;

  // Build our shader if necessary.
  if (this->Colors && this->Colors->GetNumberOfComponents())
    {
    VSSource = vtkgl::replace(VSSource,
                                 "//VTK::Color::Dec",
                                 "attribute vec4 diffuseColor;");
    }
  else
    {
    VSSource = vtkgl::replace(VSSource,
                                 "//VTK::Color::Dec",
                                 "uniform vec4 diffuseColor;");
    }
  if (this->Internal->layout.TCoordComponents)
    {
    if (this->Internal->layout.TCoordComponents == 1)
      {
      VSSource = vtkgl::replace(VSSource,
                                   "//VTK::TCoord::Dec",
                                   "attribute float tcoordMC; varying float tcoordVC;");
      VSSource = vtkgl::replace(VSSource,
                                   "//VTK::TCoord::Impl",
                                   "tcoordVC = tcoordMC;");
      FSSource = vtkgl::replace(FSSource,
                                   "//VTK::TCoord::Dec",
                                   "varying float tcoordVC; uniform sampler2D texture1;");
      FSSource = vtkgl::replace(FSSource,
                                   "//VTK::TCoord::Impl",
                                   "gl_FragColor = gl_FragColor*texture2D(texture1, vec2(tcoordVC,0));");
      }
    else
      {
      VSSource = vtkgl::replace(VSSource,
                                   "//VTK::TCoord::Dec",
                                   "attribute vec2 tcoordMC; varying vec2 tcoordVC;");
      VSSource = vtkgl::replace(VSSource,
                                   "//VTK::TCoord::Impl",
                                   "tcoordVC = tcoordMC;");
      FSSource = vtkgl::replace(FSSource,
                                   "//VTK::TCoord::Dec",
                                   "varying vec2 tcoordVC; uniform sampler2D texture1;");
      FSSource = vtkgl::replace(FSSource,
                                   "//VTK::TCoord::Impl",
                                   "gl_FragColor = gl_FragColor*texture2D(texture1, tcoordVC.st);");
      }
    }
  else
    {
    VSSource = vtkgl::replace(VSSource,
                                 "//VTK::TCoord::Dec","");
    VSSource = vtkgl::replace(VSSource,
                                 "//VTK::TCoord::Impl","");
    FSSource = vtkgl::replace(FSSource,
                                 "//VTK::TCoord::Dec","");
    FSSource = vtkgl::replace(FSSource,
                                 "//VTK::TCoord::Impl","");
    }
  //cout << "VS: " << VSSource << endl;
}


//-----------------------------------------------------------------------------
void vtkOpenGL2PolyDataMapper2D::UpdateShader(vtkgl::CellBO &cellBO,
    vtkViewport* viewport, vtkActor2D *actor)
{
  vtkOpenGL2RenderWindow *renWin = vtkOpenGL2RenderWindow::SafeDownCast(viewport->GetVTKWindow());

  // has something changed that would require us to recreate the shader?
  // candidates are
  // property modified (representation interpolation and lighting)
  // input modified
  // light complexity changed
  if (cellBO.ShaderSourceTime < this->GetMTime() ||
      cellBO.ShaderSourceTime < actor->GetMTime() ||
      cellBO.ShaderSourceTime < this->GetInput()->GetMTime())
    {
    std::string VSSource;
    std::string FSSource;
    this->BuildShader(VSSource,FSSource,viewport,actor);
    vtkOpenGL2ShaderCache::CachedShaderProgram *newShader =
      renWin->GetShaderCache()->ReadyShader(VSSource.c_str(), FSSource.c_str());
    cellBO.ShaderSourceTime.Modified();
    // if the shader changed reinitialize the VAO
    if (newShader != cellBO.CachedProgram)
      {
      cellBO.CachedProgram = newShader;
      cellBO.vao.Initialize(); // reset the VAO as the shader has changed
      }
    }
    else
    {
    renWin->GetShaderCache()->ReadyShader(cellBO.CachedProgram);
    }

  // Now to update the VAO too, if necessary.
  vtkgl::VBOLayout &layout = this->Internal->layout;
  if (this->VBOUpdateTime > cellBO.attributeUpdateTime)
    {
    cellBO.vao.Bind();
    if (!cellBO.vao.AddAttributeArray(cellBO.CachedProgram->Program, this->Internal->vbo,
                                    "vertexWC", layout.VertexOffset,
                                    layout.Stride, VTK_FLOAT, 3, false))
      {
      vtkErrorMacro(<< "Error setting 'vertexWC' in shader program.");
      }
    if (layout.TCoordComponents)
      {
      if (!cellBO.vao.AddAttributeArray(cellBO.CachedProgram->Program, this->Internal->vbo,
                                      "tcoordMC", layout.TCoordOffset,
                                      layout.Stride, VTK_FLOAT, layout.TCoordComponents, false))
        {
        vtkErrorMacro(<< "Error setting 'tcoordMC' in shader VAO.");
        }
      }
    if (layout.ColorComponents != 0)
      {
      if (!cellBO.vao.AddAttributeArray(cellBO.CachedProgram->Program, this->Internal->vbo,
                                      "diffuseColor", layout.ColorOffset,
                                      layout.Stride, VTK_UNSIGNED_CHAR,
                                      layout.ColorComponents, true))
        {
        vtkErrorMacro(<< "Error setting 'diffuseColor' in shader program.");
        }
      }
    cellBO.attributeUpdateTime.Modified();
    }

  if (layout.TCoordComponents)
    {
    cellBO.CachedProgram->Program.SetUniformValue("texture1", 0);
    }

  this->SetPropertyShaderParameters(cellBO, viewport, actor);
  this->SetCameraShaderParameters(cellBO, viewport, actor);
  cellBO.vao.Bind();
}

//-----------------------------------------------------------------------------
void vtkOpenGL2PolyDataMapper2D::SetPropertyShaderParameters(
  vtkgl::CellBO &cellBO, vtkViewport*, vtkActor2D *actor)
{
  vtkgl::ShaderProgram &program = cellBO.CachedProgram->Program;

  // Query the actor for some of the properties that can be applied.
  float opacity = static_cast<float>(actor->GetProperty()->GetOpacity());
  double *dColor = actor->GetProperty()->GetColor();
  vtkgl::Vector4ub diffuseColor(static_cast<unsigned char>(dColor[0] * 255.0),
                         static_cast<unsigned char>(dColor[1] * 255.0),
                         static_cast<unsigned char>(dColor[2] * 255.0),
                         static_cast<unsigned char>(opacity * 255.0));

  program.SetUniformValue("diffuseColor", diffuseColor);
}

//-----------------------------------------------------------------------------
void vtkOpenGL2PolyDataMapper2D::SetCameraShaderParameters(
  vtkgl::CellBO &cellBO, vtkViewport* viewport, vtkActor2D *actor)
{
  vtkgl::ShaderProgram &program = cellBO.CachedProgram->Program;

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
  program.SetUniformValue("WCVCMatrix", tmpMat);

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

  bool cellScalars = false;
  if (this->ScalarVisibility)
    {
    // We must figure out how the scalars should be mapped to the polydata.
    this->MapScalars(act->GetProperty()->GetOpacity());
    if ( (this->ScalarMode == VTK_SCALAR_MODE_USE_CELL_DATA ||
          this->ScalarMode == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA ||
          this->ScalarMode == VTK_SCALAR_MODE_USE_FIELD_DATA ||
          !poly->GetPointData()->GetScalars() )
         && this->ScalarMode != VTK_SCALAR_MODE_USE_POINT_FIELD_DATA
         && this->Colors)
      {
      cellScalars = true;
      }
    }

  // if we have cell scalars then we have to
  // explode the data
  vtkCellArray *prims[4];
  prims[0] =  poly->GetVerts();
  prims[1] =  poly->GetLines();
  prims[2] =  poly->GetPolys();
  prims[3] =  poly->GetStrips();
  std::vector<unsigned int> cellPointMap;
  std::vector<unsigned int> pointCellMap;
  if (cellScalars)
    {
    vtkgl::CreateCellSupportArrays(poly, prims, cellPointMap, pointCellMap);
    }

  // do we have texture maps?
  bool haveTextures = false;
  vtkTexturedActor2D *ta =
      vtkTexturedActor2D::SafeDownCast(act);
  if (ta)
    {
    haveTextures = (ta->GetTexture() != NULL);
    }

  // Iterate through all of the different types in the polydata, building VBOs
  // and IBOs as appropriate for each type.
  this->Internal->layout =
    CreateVBO(poly->GetPoints(),
              cellPointMap.size() > 0 ? cellPointMap.size() : poly->GetPoints()->GetNumberOfPoints(),
              NULL,
              haveTextures ? poly->GetPointData()->GetTCoords() : NULL,
              this->Colors ? (unsigned char *)this->Colors->GetVoidPointer(0) : NULL,
              this->Colors ? this->Colors->GetNumberOfComponents() : 0,
              this->Internal->vbo,
              cellPointMap.size() > 0 ? &cellPointMap.front() : NULL,
              pointCellMap.size() > 0 ? &pointCellMap.front() : NULL);


  this->Internal->points.indexCount = CreatePointIndexBuffer(prims[0],
                                                    this->Internal->points.ibo);
  this->Internal->lines.indexCount = CreateMultiIndexBuffer(prims[1],
                         this->Internal->lines.ibo,
                         this->Internal->lines.offsetArray,
                         this->Internal->lines.elementsArray);
  this->Internal->tris.indexCount = CreateTriangleIndexBuffer(prims[2],
                                                              this->Internal->tris.ibo,
                                                              poly->GetPoints());
  this->Internal->triStrips.indexCount = CreateMultiIndexBuffer(prims[3],
                         this->Internal->triStrips.ibo,
                         this->Internal->triStrips.offsetArray,
                         this->Internal->triStrips.elementsArray);

  // free up new cell arrays
  if (cellScalars)
    {
    for (int primType = 0; primType < 4; primType++)
      {
      prims[primType]->UnRegister(this);
      }
    }

}


void vtkOpenGL2PolyDataMapper2D::RenderOverlay(vtkViewport* viewport,
                                              vtkActor2D* actor)
{
  vtkOpenGLClearErrorMacro();
  int            numPts;
  vtkPolyData    *input=static_cast<vtkPolyData *>(this->GetInput());
  int            j;
  vtkPoints      *p, *displayPts;

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

  if ( this->LookupTable == NULL )
    {
    this->CreateDefaultLookupTable();
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

  // Assume we want to do Zbuffering for now.
  // we may turn this off later
  glDepthMask(GL_TRUE);

  // Update the VBO if needed.
  if (this->VBOUpdateTime < this->GetMTime() ||
      this->VBOUpdateTime < actor->GetMTime() ||
      this->VBOUpdateTime < input->GetMTime() )
    {
    this->UpdateVBO(actor);
    this->VBOUpdateTime.Modified();
    }

  this->Internal->vbo.Bind();
  vtkgl::VBOLayout &layout = this->Internal->layout;

  // Figure out and build the appropriate shader for the mapped geometry.
  this->UpdateShader(this->Internal->points, viewport, actor);

  if (this->Internal->points.indexCount)
    {
    // Set the PointSize
    glPointSize(actor->GetProperty()->GetPointSize());
    //vtkOpenGLGL2PSHelper::SetPointSize(actor->GetProperty()->GetPointSize());

    this->Internal->points.ibo.Bind();
    glDrawRangeElements(GL_POINTS, 0,
                        static_cast<GLuint>(layout.VertexCount - 1),
                        static_cast<GLsizei>(this->Internal->points.indexCount),
                        GL_UNSIGNED_INT,
                        reinterpret_cast<const GLvoid *>(NULL));
    this->Internal->points.ibo.Release();
    }

  if (this->Internal->lines.indexCount)
    {
    // Set the LineWidth
    glLineWidth(actor->GetProperty()->GetLineWidth()); // supported by all OpenGL versions
    vtkOpenGLGL2PSHelper::SetLineWidth(actor->GetProperty()->GetLineWidth());

    this->Internal->lines.ibo.Bind();
    for (int eCount = 0; eCount < this->Internal->lines.offsetArray.size(); ++eCount)
      {
      glDrawElements(GL_LINE_STRIP,
        this->Internal->lines.elementsArray[eCount],
        GL_UNSIGNED_INT,
        (GLvoid *)(this->Internal->lines.offsetArray[eCount]));
      }
    this->Internal->lines.ibo.Release();
    }

  // now handle lit primatives
  if (this->Internal->tris.indexCount)
    {
    this->Internal->tris.ibo.Bind();
    glDrawRangeElements(GL_TRIANGLES, 0,
                        static_cast<GLuint>(layout.VertexCount - 1),
                        static_cast<GLsizei>(this->Internal->tris.indexCount),
                        GL_UNSIGNED_INT,
                        reinterpret_cast<const GLvoid *>(NULL));
    this->Internal->tris.ibo.Release();
    }

  if (this->Internal->triStrips.indexCount)
    {
    this->Internal->triStrips.ibo.Bind();
    for (int eCount = 0; eCount < this->Internal->triStrips.offsetArray.size(); ++eCount)
      {
      glDrawElements(GL_TRIANGLE_STRIP,
        this->Internal->triStrips.elementsArray[eCount],
        GL_UNSIGNED_INT,
        (GLvoid *)(this->Internal->triStrips.offsetArray[eCount]));
      }
    this->Internal->triStrips.ibo.Release();
    }

  this->Internal->points.vao.Release();
  this->Internal->vbo.Release();

  vtkOpenGLCheckErrorMacro("failed after RenderOverlay");
}

//----------------------------------------------------------------------------
void vtkOpenGL2PolyDataMapper2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
