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
#include "vtkOpenGLPolyDataMapper2D.h"

#include "vtkglVBOHelper.h"

#include "vtkActor2D.h"
#include "vtkTexturedActor2D.h"
#include "vtkCellArray.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLTexture.h"
#include "vtkPlane.h"
#include "vtkPlaneCollection.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty2D.h"
#include "vtkScalarsToColors.h"
#include "vtkUnsignedCharArray.h"
#include "vtkViewport.h"
#include "vtkWindow.h"
#include "vtkOpenGLError.h"

#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLRenderer.h"

#include <cmath>

// Bring in our shader symbols.
#include "vtkglPolyData2DVS.h"
#include "vtkglPolyData2DFS.h"

class vtkOpenGLPolyDataMapper2D::Private
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

  int LastDepthPeeling;
  vtkTimeStamp DepthPeelingChanged;

  Private()
  {
  this->LastDepthPeeling = 0;
  }
};

vtkStandardNewMacro(vtkOpenGLPolyDataMapper2D);

//-----------------------------------------------------------------------------
vtkOpenGLPolyDataMapper2D::vtkOpenGLPolyDataMapper2D()
  : Internal(new Private)
{
  this->TransformedPoints = NULL;
}

//-----------------------------------------------------------------------------
vtkOpenGLPolyDataMapper2D::~vtkOpenGLPolyDataMapper2D()
{
  if (this->TransformedPoints)
    {
    this->TransformedPoints->UnRegister(this);
    }
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper2D::BuildShader(
  std::string &VSSource, std::string &FSSource,
  vtkViewport* viewport, vtkActor2D *vtkNotUsed(actor))
{
  VSSource = vtkglPolyData2DVS;
  FSSource = vtkglPolyData2DFS;

  vtkOpenGLRenderer *ren = vtkOpenGLRenderer::SafeDownCast(viewport);

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

  if (ren && ren->GetLastRenderingUsedDepthPeeling() == 2)
    {
    FSSource = vtkgl::replace(FSSource,
      "//VTK::DepthPeeling::Dec",
      "uniform sampler2DRect translucentRGBATexture;"
      "uniform sampler2DRect currentRGBATexture;");
    FSSource = vtkgl::replace(FSSource,
      "//VTK::DepthPeeling::Impl", ""
      "vec4 t1Color = texture2DRect(translucentRGBATexture, gl_FragCoord.xy); "
      "vec4 t2Color = texture2DRect(currentRGBATexture, gl_FragCoord.xy); "
      "gl_FragColor.a = t1Color.a + t2Color.a * (1.0-t1Color.a); "
      "if (gl_FragColor.a > 0.0) { gl_FragColor.rgb = (t1Color.rgb*t1Color.a + t2Color.rgb*t2Color.a*(1.0-t1Color.a))/gl_FragColor.a; } "
      "else { gl_FragColor.rgb = vec3(0.0,0.0,0.0); }"
      );
    }
  if (ren && ren->GetLastRenderingUsedDepthPeeling() == 3)
    {
    FSSource = vtkgl::replace(FSSource,
      "//VTK::DepthPeeling::Dec",
      "uniform sampler2DRect translucentRGBATexture;"
      "uniform sampler2DRect currentRGBATexture;");
    FSSource = vtkgl::replace(FSSource,
      "//VTK::DepthPeeling::Impl", ""
      "vec4 t1Color = texture2DRect(translucentRGBATexture, gl_FragCoord.xy); "
      "vec4 t2Color = texture2DRect(currentRGBATexture, gl_FragCoord.xy); "
      "gl_FragColor.a = 1.0;"
      "gl_FragColor.rgb = (t1Color.rgb*t1Color.a + t2Color.rgb*(1.0-t1Color.a));"
      );
    }

  //cout << "VS: " << VSSource << endl;
}


//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper2D::UpdateShader(vtkgl::CellBO &cellBO,
    vtkViewport* viewport, vtkActor2D *actor)
{
  vtkOpenGLRenderWindow *renWin = vtkOpenGLRenderWindow::SafeDownCast(viewport->GetVTKWindow());

  vtkOpenGLRenderer *ren = vtkOpenGLRenderer::SafeDownCast(viewport);

  if (ren != NULL && this->Internal->LastDepthPeeling !=
      ren->GetLastRenderingUsedDepthPeeling())
    {
    this->Internal->DepthPeelingChanged.Modified();
    this->Internal->LastDepthPeeling = ren->GetLastRenderingUsedDepthPeeling();
    }

  // has something changed that would require us to recreate the shader?
  // candidates are
  // property modified (representation interpolation and lighting)
  // input modified
  // light complexity changed
  if (cellBO.ShaderSourceTime < this->GetMTime() ||
      cellBO.ShaderSourceTime < actor->GetMTime() ||
      cellBO.ShaderSourceTime < this->Internal->DepthPeelingChanged ||
      cellBO.ShaderSourceTime < this->GetInput()->GetMTime())
    {
    std::string VSSource;
    std::string FSSource;
    this->BuildShader(VSSource,FSSource,viewport,actor);
    vtkOpenGLShaderCache::CachedShaderProgram *newShader =
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
    vtkTexturedActor2D *ta = vtkTexturedActor2D::SafeDownCast(actor);
    int tunit = 0;
    if (ta)
      {
      vtkTexture *texture = ta->GetTexture();
      tunit = renWin->GetTextureUnitForTexture(texture);
      }
    cellBO.CachedProgram->Program.SetUniformi("texture1", tunit);
    }

  // if depth peeling for trabslucetn compositing
  if (ren->GetLastRenderingUsedDepthPeeling() == 2)
    {
    int ttunit = renWin->GetTextureUnitForTexture(ren->GetTranslucentRGBATexture());
    cellBO.CachedProgram->Program.SetUniformi("translucentRGBATexture", ttunit);
    int ctunit = renWin->GetTextureUnitForTexture(ren->GetCurrentRGBATexture());
    cellBO.CachedProgram->Program.SetUniformi("currentRGBATexture", ctunit);
    }
  // if depth peeling final compositing
  if (ren->GetLastRenderingUsedDepthPeeling() == 3)
    {
    int ttunit = renWin->GetTextureUnitForTexture(ren->GetTranslucentRGBATexture());
    cellBO.CachedProgram->Program.SetUniformi("translucentRGBATexture", ttunit);
    int ctunit = renWin->GetTextureUnitForTexture(ren->GetOpaqueRGBATexture());
    cellBO.CachedProgram->Program.SetUniformi("currentRGBATexture", ctunit);
    }



  this->SetPropertyShaderParameters(cellBO, viewport, actor);
  this->SetCameraShaderParameters(cellBO, viewport, actor);
  cellBO.vao.Bind();
}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper2D::SetPropertyShaderParameters(
  vtkgl::CellBO &cellBO, vtkViewport*, vtkActor2D *actor)
{
  vtkgl::ShaderProgram &program = cellBO.CachedProgram->Program;

  // Query the actor for some of the properties that can be applied.
  float opacity = static_cast<float>(actor->GetProperty()->GetOpacity());
  double *dColor = actor->GetProperty()->GetColor();
  float diffuseColor[4] = {dColor[0], dColor[1], dColor[2], opacity};

  program.SetUniform4f("diffuseColor", diffuseColor);
}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper2D::SetCameraShaderParameters(
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
  program.SetUniformMatrix("WCVCMatrix", tmpMat);

  tmpMat->Delete();
}


//-------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper2D::UpdateVBO(vtkActor2D *act, vtkViewport *viewport)
{
  vtkPoints      *p;
  int            numPts;
  int            j;

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

  // Transform the points, if necessary
  p = poly->GetPoints();
  if ( this->TransformCoordinate )
    {
    numPts = p->GetNumberOfPoints();
    if (!this->TransformedPoints)
      {
      this->TransformedPoints = vtkPoints::New();
      }
    this->TransformedPoints->SetNumberOfPoints(numPts);
    for ( j=0; j < numPts; j++ )
      {
      this->TransformCoordinate->SetValue(p->GetPoint(j));
      if (this->TransformCoordinateUseDouble)
        {
        double* dtmp = this->TransformCoordinate->GetComputedDoubleViewportValue(viewport);
        this->TransformedPoints->SetPoint(j,dtmp[0], dtmp[1], 0.0);
        }
      else
        {
        int* itmp = this->TransformCoordinate->GetComputedViewportValue(viewport);
        this->TransformedPoints->SetPoint(j,itmp[0], itmp[1], 0.0);
        }
      }
    p = this->TransformedPoints;
    }

  // Iterate through all of the different types in the polydata, building VBOs
  // and IBOs as appropriate for each type.
  this->Internal->layout =
    CreateVBO(p,
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


void vtkOpenGLPolyDataMapper2D::RenderOverlay(vtkViewport* viewport,
                                              vtkActor2D* actor)
{
  vtkOpenGLClearErrorMacro();
  int numPts;
  vtkPolyData    *input=static_cast<vtkPolyData *>(this->GetInput());

  vtkDebugMacro (<< "vtkOpenGLPolyDataMapper2D::Render");

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

  // Assume we want to do Zbuffering for now.
  // we may turn this off later
  glDepthMask(GL_TRUE);

  // Update the VBO if needed.
  if (this->VBOUpdateTime < this->GetMTime() ||
      this->VBOUpdateTime < actor->GetMTime() ||
      this->VBOUpdateTime < input->GetMTime() )
    {
    this->UpdateVBO(actor, viewport);
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
void vtkOpenGLPolyDataMapper2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
