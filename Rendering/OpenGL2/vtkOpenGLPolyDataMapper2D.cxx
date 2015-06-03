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

#include "vtkOpenGLHelper.h"

#include "vtkActor2D.h"
#include "vtkCellArray.h"
#include "vtkInformation.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLBufferObject.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLIndexBufferObject.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLTexture.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkOpenGLVertexBufferObject.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty2D.h"
#include "vtkProperty.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"
#include "vtkUnsignedCharArray.h"
#include "vtkViewport.h"

// Bring in our shader symbols.
#include "vtkPolyData2DVS.h"
#include "vtkPolyData2DFS.h"

vtkStandardNewMacro(vtkOpenGLPolyDataMapper2D);

//-----------------------------------------------------------------------------
vtkOpenGLPolyDataMapper2D::vtkOpenGLPolyDataMapper2D()
{
  this->TransformedPoints = NULL;
  this->CellScalarTexture = NULL;
  this->CellScalarBuffer = NULL;
  this->VBO = vtkOpenGLVertexBufferObject::New();
  this->AppleBugPrimIDBuffer = 0;
}

//-----------------------------------------------------------------------------
vtkOpenGLPolyDataMapper2D::~vtkOpenGLPolyDataMapper2D()
{
  if (this->TransformedPoints)
    {
    this->TransformedPoints->UnRegister(this);
    }
  if (this->CellScalarTexture)
    { // Resources released previously.
    this->CellScalarTexture->Delete();
    this->CellScalarTexture = 0;
    }
  if (this->CellScalarBuffer)
    { // Resources released previously.
    this->CellScalarBuffer->Delete();
    this->CellScalarBuffer = 0;
    }
  this->HaveCellScalars = false;
  this->VBO->Delete();
  this->VBO = 0;
  if (this->AppleBugPrimIDBuffer)
    {
    this->AppleBugPrimIDBuffer->Delete();
    }

}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper2D::ReleaseGraphicsResources(vtkWindow* win)
{
  this->VBO->ReleaseGraphicsResources();
  this->Points.ReleaseGraphicsResources(win);
  this->Lines.ReleaseGraphicsResources(win);
  this->Tris.ReleaseGraphicsResources(win);
  this->TriStrips.ReleaseGraphicsResources(win);
 if (this->CellScalarTexture)
    {
    this->CellScalarTexture->ReleaseGraphicsResources(win);
    }
  if (this->CellScalarBuffer)
    {
    this->CellScalarBuffer->ReleaseGraphicsResources();
    }
  if (this->AppleBugPrimIDBuffer)
    {
    this->AppleBugPrimIDBuffer->ReleaseGraphicsResources();
    }

  this->Modified();
}

//-----------------------------------------------------------------------------
bool vtkOpenGLPolyDataMapper2D::GetNeedToRebuildShader(vtkOpenGLHelper &cellBO,
      vtkViewport* vtkNotUsed(viewport), vtkActor2D *actor)
{
  // has something changed that would require us to recreate the shader?
  // candidates are
  // property modified (representation interpolation and lighting)
  // input modified
  // light complexity changed
  if (cellBO.Program == 0 ||
      cellBO.ShaderSourceTime < this->GetMTime() ||
      cellBO.ShaderSourceTime < actor->GetMTime() ||
      cellBO.ShaderSourceTime < this->GetInput()->GetMTime())
    {
    return true;
    }

  return false;
}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper2D::BuildShader(
  std::string &VSSource, std::string &FSSource, std::string &GSSource,
  vtkViewport* vtkNotUsed(viewport), vtkActor2D *vtkNotUsed(actor))
{
  VSSource = vtkPolyData2DVS;
  FSSource = vtkPolyData2DFS;
  GSSource.clear();

  // Build our shader if necessary.
  if (this->HaveCellScalars)
    {
    vtkShaderProgram::Substitute(FSSource,
        "//VTK::Color::Dec",
        "uniform samplerBuffer textureC;");
    vtkShaderProgram::Substitute(FSSource,
        "//VTK::Color::Impl",
        "gl_FragData[0] = texelFetchBuffer(textureC, gl_PrimitiveID + PrimitiveIDOffset);");
    }
  else
    {
    if (this->Colors &&
        this->Colors->GetNumberOfComponents())
      {
      vtkShaderProgram::Substitute(VSSource,
         "//VTK::Color::Dec",
         "attribute vec4 diffuseColor;\n"
         "varying vec4 fcolor;");
      vtkShaderProgram::Substitute(VSSource,
          "//VTK::Color::Impl",
          "fcolor = diffuseColor;");
      vtkShaderProgram::Substitute(FSSource,
          "//VTK::Color::Dec",
          "varying vec4 fcolor;");
      vtkShaderProgram::Substitute(FSSource,
          "//VTK::Color::Impl",
          "gl_FragData[0] = fcolor;");
      }
    else
      {
      vtkShaderProgram::Substitute(FSSource,
          "//VTK::Color::Dec",
          "uniform vec4 diffuseColor;");
      vtkShaderProgram::Substitute(FSSource,
          "//VTK::Color::Impl",
          "gl_FragData[0] = diffuseColor;");
      }
    }

  if (this->VBO->TCoordComponents)
    {
    if (this->VBO->TCoordComponents == 1)
      {
      vtkShaderProgram::Substitute(VSSource,
        "//VTK::TCoord::Dec",
        "attribute float tcoordMC; varying float tcoordVC;");
      vtkShaderProgram::Substitute(VSSource,
        "//VTK::TCoord::Impl",
        "tcoordVC = tcoordMC;");
      vtkShaderProgram::Substitute(FSSource,
        "//VTK::TCoord::Dec",
        "varying float tcoordVC; uniform sampler2D texture1;");
      vtkShaderProgram::Substitute(FSSource,
        "//VTK::TCoord::Impl",
        "gl_FragData[0] = gl_FragData[0]*texture2D(texture1, vec2(tcoordVC,0));");
      }
    else
      {
      vtkShaderProgram::Substitute(VSSource,
        "//VTK::TCoord::Dec",
        "attribute vec2 tcoordMC; varying vec2 tcoordVC;");
      vtkShaderProgram::Substitute(VSSource,
        "//VTK::TCoord::Impl",
        "tcoordVC = tcoordMC;");
      vtkShaderProgram::Substitute(FSSource,
        "//VTK::TCoord::Dec",
        "varying vec2 tcoordVC; uniform sampler2D texture1;");
      vtkShaderProgram::Substitute(FSSource,
        "//VTK::TCoord::Impl",
        "gl_FragData[0] = gl_FragData[0]*texture2D(texture1, tcoordVC.st);");
      }
    }

  // are we handling the apple bug?
  if (this->AppleBugPrimIDs.size())
    {
    vtkShaderProgram::Substitute(VSSource,"//VTK::PrimID::Dec",
      "attribute vec4 appleBugPrimID;\n"
      "varying vec4 applePrimID;");
    vtkShaderProgram::Substitute(VSSource,"//VTK::PrimID::Impl",
      "applePrimID = appleBugPrimID;");
    vtkShaderProgram::Substitute(FSSource,"//VTK::PrimID::Dec",
      "varying vec4 applePrimID;");
    vtkShaderProgram::Substitute(FSSource,"//VTK::PrimID::Impl",
      "int vtkPrimID = int(applePrimID[0]*255.1) + int(applePrimID[1]*255.1)*256 + int(applePrimID[2]*255.1)*65536;");
    vtkShaderProgram::Substitute(FSSource,"gl_PrimitiveID","vtkPrimID");
    }

}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper2D::UpdateShader(vtkOpenGLHelper &cellBO,
    vtkViewport* viewport, vtkActor2D *actor)
{
  vtkOpenGLRenderWindow *renWin = vtkOpenGLRenderWindow::SafeDownCast(viewport->GetVTKWindow());

  if (this->GetNeedToRebuildShader(cellBO, viewport, actor))
    {
    std::string VSSource;
    std::string FSSource;
    std::string GSSource;
    this->BuildShader(VSSource,FSSource,GSSource,viewport,actor);
    vtkShaderProgram *newShader =
      renWin->GetShaderCache()->ReadyShader(VSSource.c_str(),
                                            FSSource.c_str(),
                                            GSSource.c_str());
    cellBO.ShaderSourceTime.Modified();
    // if the shader changed reinitialize the VAO
    if (newShader != cellBO.Program)
      {
      cellBO.Program = newShader;
      cellBO.VAO->ShaderProgramChanged(); // reset the VAO as the shader has changed
      }
    }
  else
    {
    renWin->GetShaderCache()->ReadyShader(cellBO.Program);
    }


  this->SetMapperShaderParameters(cellBO, viewport, actor);
  this->SetPropertyShaderParameters(cellBO, viewport, actor);
  this->SetCameraShaderParameters(cellBO, viewport, actor);
  cellBO.VAO->Bind();
}


//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper2D::SetMapperShaderParameters(
  vtkOpenGLHelper &cellBO, vtkViewport *vtkNotUsed(viewport), vtkActor2D *actor)
{
  // Now to update the VAO too, if necessary.
  if (this->VBOUpdateTime > cellBO.AttributeUpdateTime ||
      cellBO.ShaderSourceTime > cellBO.AttributeUpdateTime)
    {
    cellBO.VAO->Bind();
    if (!cellBO.VAO->AddAttributeArray(cellBO.Program, this->VBO,
                                    "vertexWC", this->VBO->VertexOffset,
                                    this->VBO->Stride, VTK_FLOAT, 3, false))
      {
      vtkErrorMacro(<< "Error setting 'vertexWC' in shader program.");
      }
    if (this->VBO->TCoordComponents)
      {
      if (!cellBO.VAO->AddAttributeArray(cellBO.Program, this->VBO,
                                      "tcoordMC", this->VBO->TCoordOffset,
                                      this->VBO->Stride, VTK_FLOAT, this->VBO->TCoordComponents, false))
        {
        vtkErrorMacro(<< "Error setting 'tcoordMC' in shader VAO.");
        }
      }
    if (this->VBO->ColorComponents != 0)
      {
      if (!cellBO.VAO->AddAttributeArray(cellBO.Program, this->VBO,
                                      "diffuseColor", this->VBO->ColorOffset,
                                      this->VBO->Stride, VTK_UNSIGNED_CHAR,
                                      this->VBO->ColorComponents, true))
        {
        vtkErrorMacro(<< "Error setting 'diffuseColor' in shader program.");
        }
      }
    if (this->AppleBugPrimIDs.size())
      {
      this->AppleBugPrimIDBuffer->Bind();
      if (!cellBO.VAO->AddAttributeArray(cellBO.Program,
          this->AppleBugPrimIDBuffer,
          "appleBugPrimID",
           0, sizeof(float), VTK_UNSIGNED_CHAR, 4, true))
        {
        vtkErrorMacro(<< "Error setting 'appleBugPrimID' in shader VAO.");
        }
      this->AppleBugPrimIDBuffer->Release();
      }

    cellBO.AttributeUpdateTime.Modified();
    }

  if (this->HaveCellScalars)
    {
    int tunit = this->CellScalarTexture->GetTextureUnit();
    cellBO.Program->SetUniformi("textureC", tunit);
    }

  if (this->VBO->TCoordComponents)
    {
    vtkInformation *info = actor->GetPropertyKeys();
    if (info && info->Has(vtkProp::GeneralTextureUnit()))
      {
      int tunit = info->Get(vtkProp::GeneralTextureUnit());
      cellBO.Program->SetUniformi("texture1", tunit);
      }
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper2D::SetPropertyShaderParameters(
  vtkOpenGLHelper &cellBO, vtkViewport*, vtkActor2D *actor)
{
  if (!this->Colors || !this->Colors->GetNumberOfComponents())
    {
    vtkShaderProgram *program = cellBO.Program;

    // Query the actor for some of the properties that can be applied.
    float opacity = static_cast<float>(actor->GetProperty()->GetOpacity());
    double *dColor = actor->GetProperty()->GetColor();
    float diffuseColor[4] = {static_cast<float>(dColor[0]),
      static_cast<float>(dColor[1]),
      static_cast<float>(dColor[2]),
      static_cast<float>(opacity)};

    program->SetUniform4f("diffuseColor", diffuseColor);
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper2D::SetCameraShaderParameters(
  vtkOpenGLHelper &cellBO, vtkViewport* viewport, vtkActor2D *actor)
{
  vtkShaderProgram *program = cellBO.Program;

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
  // XXX(cppcheck): possible division by zero
  tmpMat->SetElement(2,2,-2.0/(farV - nearV));
  tmpMat->SetElement(3,3,1.0);
  tmpMat->SetElement(0,3,-1.0*(right+left)/(right-left));
  tmpMat->SetElement(1,3,-1.0*(top+bottom)/(top-bottom));
  // XXX(cppcheck): possible division by zero
  tmpMat->SetElement(2,3,-1.0*(farV+nearV)/(farV-nearV));
  tmpMat->Transpose();
  program->SetUniformMatrix("WCVCMatrix", tmpMat);

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

  // check if this system is subject to the apple primID bug
  this->HaveAppleBug = true;

#ifdef __APPLE__
  std::string vendor = (const char *)glGetString(GL_VENDOR);
  if (vendor.find("ATI") != std::string::npos ||
      vendor.find("AMD") != std::string::npos ||
      vendor.find("amd") != std::string::npos)
    {
    this->HaveAppleBug = true;
    }
#endif

  this->HaveCellScalars = false;
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
      this->HaveCellScalars = true;
      }
    }

  // on apple with the AMD PrimID bug we use a slow
  // painful approach to work around it
  this->AppleBugPrimIDs.resize(0);
  if (this->HaveAppleBug && this->HaveCellScalars)
    {
    if (!this->AppleBugPrimIDBuffer)
      {
      this->AppleBugPrimIDBuffer = vtkOpenGLBufferObject::New();
      }
    poly = vtkOpenGLPolyDataMapper::HandleAppleBug(poly,
      this->AppleBugPrimIDs);
    this->AppleBugPrimIDBuffer->Bind();
    this->AppleBugPrimIDBuffer->Upload(
     this->AppleBugPrimIDs, vtkOpenGLBufferObject::ArrayBuffer);
    this->AppleBugPrimIDBuffer->Release();

    vtkWarningMacro("VTK is working around a bug in Apple-AMD hardware related to gl_PrimitiveID.  This may cause significant memory and performance impacts. Your hardware has been identified as vendor "
      << (const char *)glGetString(GL_VENDOR) << " with renderer of "
      << (const char *)glGetString(GL_RENDERER));
    }

  // if we have cell scalars then we have to
  // build the texture
  vtkCellArray *prims[4];
  prims[0] =  poly->GetVerts();
  prims[1] =  poly->GetLines();
  prims[2] =  poly->GetPolys();
  prims[3] =  poly->GetStrips();
  std::vector<unsigned int> cellCellMap;
  vtkDataArray *c = this->Colors;
  if (this->HaveCellScalars)
    {
    if (this->HaveAppleBug)
      {
      unsigned int numCells = poly->GetNumberOfCells();
      for (unsigned int i = 0; i < numCells; i++)
        {
        cellCellMap.push_back(i);
        }
      }
    else
      {
      vtkOpenGLIndexBufferObject::CreateCellSupportArrays(
        prims, cellCellMap, VTK_SURFACE);
      }

    if (!this->CellScalarTexture)
      {
      this->CellScalarTexture = vtkTextureObject::New();
      this->CellScalarBuffer = vtkOpenGLBufferObject::New();
      this->CellScalarBuffer->SetType(vtkOpenGLBufferObject::TextureBuffer);
      }
    this->CellScalarTexture->SetContext(
      static_cast<vtkOpenGLRenderWindow*>(viewport->GetVTKWindow()));
    // create the cell scalar array adjusted for ogl Cells
    std::vector<unsigned char> newColors;
    unsigned char *colorPtr = this->Colors->GetPointer(0);
    int numComp = this->Colors->GetNumberOfComponents();
    assert(numComp == 4);
    for (unsigned int i = 0; i < cellCellMap.size(); i++)
      {
      for (j = 0; j < numComp; j++)
        {
        newColors.push_back(colorPtr[cellCellMap[i]*numComp + j]);
        }
      }
    this->CellScalarBuffer->Upload(newColors,
      vtkOpenGLBufferObject::ArrayBuffer);
    this->CellScalarTexture->CreateTextureBuffer(
      static_cast<unsigned int>(cellCellMap.size()),
      numComp,
      VTK_UNSIGNED_CHAR,
      this->CellScalarBuffer);
    c = NULL;
    }

  // do we have texture maps?
  bool haveTextures = false;
  vtkInformation *info = act->GetPropertyKeys();
  if (info && info->Has(vtkProp::GeneralTextureUnit()))
    {
    haveTextures = true;
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
  this->VBO->CreateVBO(p,
    poly->GetPoints()->GetNumberOfPoints(),
    NULL,
    haveTextures ? poly->GetPointData()->GetTCoords() : NULL,
    c ? (unsigned char *) c->GetVoidPointer(0) : NULL,
    c ? c->GetNumberOfComponents() : 0);

  this->Points.IBO->IndexCount =
    this->Points.IBO->CreatePointIndexBuffer(prims[0]);
  this->Lines.IBO->IndexCount =
    this->Lines.IBO->CreateLineIndexBuffer(prims[1]);
  this->Tris.IBO->IndexCount =
    this->Tris.IBO->CreateTriangleIndexBuffer(prims[2], poly->GetPoints());
  this->TriStrips.IBO->IndexCount =
    this->TriStrips.IBO->CreateStripIndexBuffer(prims[3], false);

  // free up polydata if allocated due to apple bug
  if (poly != this->CurrentInput)
    {
    poly->Delete();
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

  this->VBO->Bind();

  if (this->HaveCellScalars)
    {
    this->CellScalarTexture->Activate();
    }

  // Figure out and build the appropriate shader for the mapped geometry.
  this->PrimitiveIDOffset = 0;
  this->UpdateShader(this->Points, viewport, actor);
  this->Points.Program->SetUniformi("PrimitiveIDOffset",
    this->PrimitiveIDOffset);

  if (this->Points.IBO->IndexCount)
    {
    // Set the PointSize
#if GL_ES_VERSION_2_0 != 1
    glPointSize(actor->GetProperty()->GetPointSize()); // not on ES2
#endif
    this->Points.IBO->Bind();
    glDrawRangeElements(GL_POINTS, 0,
                        static_cast<GLuint>(this->VBO->VertexCount - 1),
                        static_cast<GLsizei>(this->Points.IBO->IndexCount),
                        GL_UNSIGNED_INT,
                        reinterpret_cast<const GLvoid *>(NULL));
    this->Points.IBO->Release();
    this->PrimitiveIDOffset += (int)this->Points.IBO->IndexCount;
    this->Points.Program->SetUniformi("PrimitiveIDOffset",
      this->PrimitiveIDOffset);
    }

  if (this->Lines.IBO->IndexCount)
    {
    // Set the LineWidth
    if (vtkOpenGLRenderWindow::GetContextSupportsOpenGL32() &&
        actor->GetProperty()->GetLineWidth() > 1.0)
      {
      vtkWarningMacro("line widths above 1.0 are not supported by OpenGL 3.2");
      }
    glLineWidth(actor->GetProperty()->GetLineWidth());
    this->Lines.IBO->Bind();
    glDrawRangeElements(GL_LINES, 0,
                        static_cast<GLuint>(this->VBO->VertexCount - 1),
                        static_cast<GLsizei>(this->Lines.IBO->IndexCount),
                        GL_UNSIGNED_INT,
                        reinterpret_cast<const GLvoid *>(NULL));
    this->Lines.IBO->Release();
    this->PrimitiveIDOffset += (int)this->Lines.IBO->IndexCount/2;
    this->Points.Program->SetUniformi("PrimitiveIDOffset",
      this->PrimitiveIDOffset);
    }

  // now handle lit primatives
  if (this->Tris.IBO->IndexCount)
    {
    this->Tris.IBO->Bind();
    glDrawRangeElements(GL_TRIANGLES, 0,
                        static_cast<GLuint>(this->VBO->VertexCount - 1),
                        static_cast<GLsizei>(this->Tris.IBO->IndexCount),
                        GL_UNSIGNED_INT,
                        reinterpret_cast<const GLvoid *>(NULL));
    this->Tris.IBO->Release();
    this->PrimitiveIDOffset += (int)this->Tris.IBO->IndexCount/3;
    this->Points.Program->SetUniformi("PrimitiveIDOffset",
      this->PrimitiveIDOffset);
    }

  if (this->TriStrips.IBO->IndexCount)
    {
    this->TriStrips.IBO->Bind();
    glDrawRangeElements(GL_TRIANGLES, 0,
                        static_cast<GLuint>(this->VBO->VertexCount - 1),
                        static_cast<GLsizei>(this->TriStrips.IBO->IndexCount),
                        GL_UNSIGNED_INT,
                        reinterpret_cast<const GLvoid *>(NULL));
    this->TriStrips.IBO->Release();
    }

  if (this->HaveCellScalars)
    {
    this->CellScalarTexture->Deactivate();
    }

  this->Points.VAO->Release();
  this->VBO->Release();

  vtkOpenGLCheckErrorMacro("failed after RenderOverlay");
}

//----------------------------------------------------------------------------
void vtkOpenGLPolyDataMapper2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
