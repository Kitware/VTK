/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLContextDevice2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLContextDevice2D.h"

#include "vtkAbstractContextBufferId.h"
#include "vtkBrush.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkMathTextUtilities.h"
#include "vtkMatrix3x3.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLIndexBufferObject.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLTexture.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkOpenGLVertexBufferObject.h"
#include "vtkPen.h"
#include "vtkPoints2D.h"
#include "vtkRect.h"
#include "vtkShaderProgram.h"
#include "vtkSmartPointer.h"
#include "vtkTextProperty.h"
#include "vtkTextRendererStringToImage.h"
#include "vtkTexture.h"
#include "vtkTextureUnitManager.h"
#include "vtkTransform.h"
#include "vtkVector.h"
#include "vtkViewport.h"
#include "vtkWindow.h"
#include "vtkOpenGLHelper.h"

#include "vtkObjectFactory.h"

#include "vtkOpenGLContextDevice2DPrivate.h"

#include <algorithm>

#define BUFFER_OFFSET(i) ((char *)NULL + (i))


//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLContextDevice2D)

//-----------------------------------------------------------------------------
vtkOpenGLContextDevice2D::vtkOpenGLContextDevice2D()
{
  this->Renderer = 0;
  this->InRender = false;
  this->TextRenderer = vtkTextRendererStringToImage::New();
  this->Storage = new vtkOpenGLContextDevice2D::Private;
  this->RenderWindow = NULL;
  this->MaximumMarkerCacheSize = 20;
  this->ProjectionMatrix = vtkTransform::New();
  this->ModelMatrix = vtkTransform::New();
  this->VBO =  new vtkOpenGLHelper;
  this->VCBO =  new vtkOpenGLHelper;
  this->VTBO =  new vtkOpenGLHelper;
  this->SBO =  new vtkOpenGLHelper;
  this->SCBO =  new vtkOpenGLHelper;
}

//-----------------------------------------------------------------------------
vtkOpenGLContextDevice2D::~vtkOpenGLContextDevice2D()
{
  delete this->VBO;
  this->VBO = 0;
  delete this->VCBO;
  this->VCBO = 0;
  delete this->SBO;
  this->SBO = 0;
  delete this->SCBO;
  this->SCBO = 0;
  delete this->VTBO;
  this->VTBO = 0;

  while (!this->MarkerCache.empty())
    {
    this->MarkerCache.back().Value->Delete();
    this->MarkerCache.pop_back();
    }

  this->TextRenderer->Delete();
  this->ProjectionMatrix->Delete();
  this->ModelMatrix->Delete();
  delete this->Storage;
}

vtkMatrix4x4 *vtkOpenGLContextDevice2D::GetProjectionMatrix()
{
  return this->ProjectionMatrix->GetMatrix();
}

vtkMatrix4x4 *vtkOpenGLContextDevice2D::GetModelMatrix()
{
  return this->ModelMatrix->GetMatrix();
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::Begin(vtkViewport* viewport)
{
  vtkOpenGLClearErrorMacro();
  // Need the actual pixel size of the viewport - ask OpenGL.
  GLint vp[4];
  glGetIntegerv(GL_VIEWPORT, vp);
  this->Storage->Offset.Set(static_cast<int>(vp[0]),
                            static_cast<int>(vp[1]));

  this->Storage->Dim.Set(static_cast<int>(vp[2]),
                         static_cast<int>(vp[3]));

  // push a 2D matrix on the stack
  this->ProjectionMatrix->Push();
  this->ProjectionMatrix->Identity();
  this->PushMatrix();
  this->ModelMatrix->Identity();

  double offset = 0.5;
  double xmin = offset;
  double xmax = vp[2]+offset-1.0;
  double ymin = offset;
  double ymax = vp[3]+offset-1.0;
  double znear = -2000;
  double zfar = 2000;

  double matrix[4][4];
  vtkMatrix4x4::Identity(*matrix);

  matrix[0][0] = 2/(xmax - xmin);
  matrix[1][1] = 2/(ymax - ymin);
  matrix[2][2] = -2/(zfar - znear);

  matrix[0][3] = -(xmin + xmax)/(xmax - xmin);
  matrix[1][3] = -(ymin + ymax)/(ymax - ymin);
  matrix[2][3] = -(znear + zfar)/(zfar - znear);

  this->ProjectionMatrix->SetMatrix(*matrix);

  // Store the previous state before changing it
  this->Storage->SaveGLState();
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);

  this->Renderer = vtkRenderer::SafeDownCast(viewport);

  this->RenderWindow = vtkOpenGLRenderWindow::SafeDownCast(this->Renderer->GetRenderWindow());
  this->RenderWindow->GetShaderCache()->ReleaseCurrentShader();

  // Enable simple line, point and polygon antialiasing if multisampling is on.
  if (this->Renderer->GetRenderWindow()->GetMultiSamples())
    {
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POLYGON_SMOOTH);
    }

  this->InRender = true;
  vtkOpenGLCheckErrorMacro("failed after Begin");
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::End()
{
  if (!this->InRender)
    {
    return;
    }

  this->ProjectionMatrix->Pop();
  this->PopMatrix();

  vtkOpenGLClearErrorMacro();

  // Restore the GL state that we changed
  this->Storage->RestoreGLState();

  // Disable simple line, point and polygon antialiasing if multisampling is on.
  if (this->Renderer->GetRenderWindow()->GetMultiSamples())
    {
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_POLYGON_SMOOTH);
    }

  this->RenderWindow = NULL;
  this->InRender = false;

  vtkOpenGLCheckErrorMacro("failed after End");
}

// ----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::BufferIdModeBegin(
  vtkAbstractContextBufferId *bufferId)
{
  assert("pre: not_yet" && !this->GetBufferIdMode());
  assert("pre: bufferId_exists" && bufferId!=0);

  vtkOpenGLClearErrorMacro();

  this->BufferId=bufferId;

  // Save OpenGL state.
  this->Storage->SaveGLState(true);

  int lowerLeft[2];
  int usize, vsize;
  this->Renderer->GetTiledSizeAndOrigin(&usize,&vsize,lowerLeft,lowerLeft+1);

  // push a 2D matrix on the stack
  this->ProjectionMatrix->Push();
  this->ProjectionMatrix->Identity();
  this->PushMatrix();
  this->ModelMatrix->Identity();

  double xmin = 0.5;
  double xmax = usize+0.5;
  double ymin = 0.5;
  double ymax = vsize+0.5;
  double znear = -1;
  double zfar = 1;

  double matrix[4][4];
  vtkMatrix4x4::Identity(*matrix);

  matrix[0][0] = 2/(xmax - xmin);
  matrix[1][1] = 2/(ymax - ymin);
  matrix[2][2] = -2/(zfar - znear);

  matrix[0][3] = -(xmin + xmax)/(xmax - xmin);
  matrix[1][3] = -(ymin + ymax)/(ymax - ymin);
  matrix[2][3] = -(znear + zfar)/(zfar - znear);

  this->ProjectionMatrix->SetMatrix(*matrix);

  glDrawBuffer(GL_BACK_LEFT);
  glClearColor(0.0,0.0,0.0,0.0); // id=0 means no hit, just background
  glClear(GL_COLOR_BUFFER_BIT);
  glDisable(GL_ALPHA_TEST);
  glDisable(GL_STENCIL_TEST);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);

  vtkOpenGLCheckErrorMacro("failed after BufferIdModeBegin");

  assert("post: started" && this->GetBufferIdMode());
}

// ----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::BufferIdModeEnd()
{
  assert("pre: started" && this->GetBufferIdMode());

  vtkOpenGLClearErrorMacro();

  // Assume the renderer has been set previously during rendering (sse Begin())
  int lowerLeft[2];
  int usize, vsize;
  this->Renderer->GetTiledSizeAndOrigin(&usize,&vsize,lowerLeft,lowerLeft+1);
  this->BufferId->SetValues(lowerLeft[0],lowerLeft[1]);

  this->ProjectionMatrix->Pop();
  this->PopMatrix();

  this->Storage->RestoreGLState(true);

  this->BufferId=0;

  vtkOpenGLCheckErrorMacro("failed after BufferIdModeEnd");

  assert("post: done" && !this->GetBufferIdMode());
}


void vtkOpenGLContextDevice2D::SetMatrices(vtkShaderProgram *prog)
{
  prog->SetUniformMatrix("WCDCMatrix",
    this->ProjectionMatrix->GetMatrix());
  prog->SetUniformMatrix("MCWCMatrix",
    this->ModelMatrix->GetMatrix());
}

void vtkOpenGLContextDevice2D::BuildVBO(
  vtkOpenGLHelper *cellBO,
  float *f, int nv,
  unsigned char *colors, int nc,
  float *tcoords)
{
  int stride = 2;
  int cOffset = 0;
  int tOffset = 0;
  if (colors)
    {
    cOffset = stride;
    stride++;
    }
  if (tcoords)
    {
    tOffset = stride;
    stride += 2;
    }

  std::vector<float> va;
  va.resize(nv*stride);
  vtkucfloat c;
  for (int i = 0; i < nv; i++)
    {
    va[i*stride] = f[i*2];
    va[i*stride+1] = f[i*2+1];
    if (colors)
      {
      c.c[0] = colors[nc*i];
      c.c[1] = colors[nc*i+1];
      c.c[2] = colors[nc*i+2];
      if (nc == 4)
        {
        c.c[3] = colors[nc*i+3];
        }
      else
        {
        c.c[3] =  255;
        }
      va[i*stride+cOffset] = c.f;
      }
    if (tcoords)
      {
      va[i*stride+tOffset] = tcoords[i*2];
      va[i*stride+tOffset+1] = tcoords[i*2+1];
      }
    }

  // upload the data
  cellBO->IBO->Upload(va, vtkOpenGLBufferObject::ArrayBuffer);
  cellBO->VAO->Bind();
  if (!cellBO->VAO->AddAttributeArray(
        cellBO->Program, cellBO->IBO,
        "vertexMC", 0,
        sizeof(float)*stride,
        VTK_FLOAT, 2, false))
    {
    vtkErrorMacro(<< "Error setting vertexMC in shader VAO.");
    }
  if (colors)
    {
    if (!cellBO->VAO->AddAttributeArray(
          cellBO->Program, cellBO->IBO,
          "vertexScalar", sizeof(float)*cOffset,
          sizeof(float)*stride,
          VTK_UNSIGNED_CHAR, 4, true))
      {
      vtkErrorMacro(<< "Error setting vertexScalar in shader VAO.");
      }
    }
  if (tcoords)
    {
    if (!cellBO->VAO->AddAttributeArray(
          cellBO->Program, cellBO->IBO,
          "tcoordMC", sizeof(float)*tOffset,
          sizeof(float)*stride,
          VTK_FLOAT, 2, false))
      {
      vtkErrorMacro(<< "Error setting tcoordMC in shader VAO.");
      }
    }

  cellBO->VAO->Bind();
}

void vtkOpenGLContextDevice2D::ReadyVBOProgram()
{
  if (!this->VBO->Program)
    {
    this->VBO->Program  =
        this->RenderWindow->GetShaderCache()->ReadyShaderProgram(
        // vertex shader
        "//VTK::System::Dec\n"
        "attribute vec2 vertexMC;\n"
        "uniform mat4 WCDCMatrix;\n"
        "uniform mat4 MCWCMatrix;\n"
        "void main() {\n"
        "vec4 vertex = vec4(vertexMC.xy, 0.0, 1.0);\n"
        "gl_Position = vertex*MCWCMatrix*WCDCMatrix; }\n",
        // fragment shader
        "//VTK::System::Dec\n"
        "//VTK::Output::Dec\n"
        "uniform vec4 vertexColor;\n"
        "void main() { gl_FragData[0] = vertexColor; }",
        // geometry shader
        "");
    }
  else
    {
    this->RenderWindow->GetShaderCache()->ReadyShaderProgram(this->VBO->Program);
    }
}

void vtkOpenGLContextDevice2D::ReadyVCBOProgram()
{
  if (!this->VCBO->Program)
    {
    this->VCBO->Program  =
        this->RenderWindow->GetShaderCache()->ReadyShaderProgram(
        // vertex shader
        "//VTK::System::Dec\n"
        "attribute vec2 vertexMC;\n"
        "attribute vec4 vertexScalar;\n"
        "uniform mat4 WCDCMatrix;\n"
        "uniform mat4 MCWCMatrix;\n"
        "varying vec4 vertexColor;\n"
        "void main() {\n"
        "vec4 vertex = vec4(vertexMC.xy, 0.0, 1.0);\n"
        "vertexColor = vertexScalar;\n"
        "gl_Position = vertex*MCWCMatrix*WCDCMatrix; }\n",
        // fragment shader
        "//VTK::System::Dec\n"
        "//VTK::Output::Dec\n"
        "varying vec4 vertexColor;\n"
        "void main() { gl_FragData[0] = vertexColor; }",
        // geometry shader
        "");
    }
  else
    {
    this->RenderWindow->GetShaderCache()->ReadyShaderProgram(this->VCBO->Program);
    }
}

void vtkOpenGLContextDevice2D::ReadyVTBOProgram()
{
  if (!this->VTBO->Program)
    {
    this->VTBO->Program  =
        this->RenderWindow->GetShaderCache()->ReadyShaderProgram(
        // vertex shader
        "//VTK::System::Dec\n"
        "attribute vec2 vertexMC;\n"
        "attribute vec2 tcoordMC;\n"
        "uniform mat4 WCDCMatrix;\n"
        "uniform mat4 MCWCMatrix;\n"
        "varying vec2 tcoord;\n"
        "void main() {\n"
        "vec4 vertex = vec4(vertexMC.xy, 0.0, 1.0);\n"
        "tcoord = tcoordMC;\n"
        "gl_Position = vertex*MCWCMatrix*WCDCMatrix; }\n",
        // fragment shader
        "//VTK::System::Dec\n"
        "//VTK::Output::Dec\n"
        "varying vec2 tcoord;\n"
        "uniform sampler2D texture1;\n"
        "void main() { gl_FragData[0] = texture2D(texture1, tcoord); }",
        // geometry shader
        "");
    }
  else
    {
    this->RenderWindow->GetShaderCache()->ReadyShaderProgram(this->VTBO->Program);
    }
}

void vtkOpenGLContextDevice2D::ReadySBOProgram()
{
  if (!this->SBO->Program)
    {
    this->SBO->Program  =
        this->RenderWindow->GetShaderCache()->ReadyShaderProgram(
        // vertex shader
        "//VTK::System::Dec\n"
        "attribute vec2 vertexMC;\n"
        "uniform mat4 WCDCMatrix;\n"
        "uniform mat4 MCWCMatrix;\n"
        "void main() {\n"
        "vec4 vertex = vec4(vertexMC.xy, 0.0, 1.0);\n"
        "gl_Position = vertex*MCWCMatrix*WCDCMatrix; }\n",
        // fragment shader
        "//VTK::System::Dec\n"
        "//VTK::Output::Dec\n"
        "uniform vec4 vertexColor;\n"
        "uniform sampler2D texture1;\n"
        "void main() { gl_FragData[0] = vertexColor*texture2D(texture1, gl_PointCoord); }",
        // geometry shader
        "");
    }
  else
    {
    this->RenderWindow->GetShaderCache()->ReadyShaderProgram(this->SBO->Program);
    }
}

void vtkOpenGLContextDevice2D::ReadySCBOProgram()
{
  if (!this->SCBO->Program)
    {
    this->SCBO->Program  =
        this->RenderWindow->GetShaderCache()->ReadyShaderProgram(
        // vertex shader
        "//VTK::System::Dec\n"
        "attribute vec2 vertexMC;\n"
        "attribute vec4 vertexScalar;\n"
        "uniform mat4 WCDCMatrix;\n"
        "uniform mat4 MCWCMatrix;\n"
        "varying vec4 vertexColor;\n"
        "void main() {\n"
        "vec4 vertex = vec4(vertexMC.xy, 0.0, 1.0);\n"
        "vertexColor = vertexScalar;\n"
        "gl_Position = vertex*MCWCMatrix*WCDCMatrix; }\n",
        // fragment shader
        "//VTK::System::Dec\n"
        "//VTK::Output::Dec\n"
        "varying vec4 vertexColor;\n"
        "uniform sampler2D texture1;\n"
        "void main() { gl_FragData[0] = vertexColor*texture2D(texture1, gl_PointCoord); }",
        // geometry shader
        "");
    }
  else
    {
    this->RenderWindow->GetShaderCache()->ReadyShaderProgram(this->SCBO->Program);
    }
}

namespace
{
  void copyColors(std::vector<unsigned char> &newColors,
    unsigned char *colors, int nc)
    {
    for (int j = 0; j < nc; j++)
      {
      newColors.push_back(colors[j]);
      }
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::DrawPoly(float *f, int n, unsigned char *colors,
                                        int nc)
{
  assert("f must be non-null" && f != NULL);
  assert("n must be greater than 0" && n > 0);

  if (this->Pen->GetLineType() == vtkPen::NO_PEN)
    {
    return;
    }

  vtkOpenGLClearErrorMacro();
  this->SetLineType(this->Pen->GetLineType());

  vtkOpenGLHelper *cbo = 0;
  if (colors)
    {
    this->ReadyVCBOProgram();
    cbo = this->VCBO;
    }
  else
    {
    this->ReadyVBOProgram();
    cbo = this->VBO;
    cbo->Program->SetUniform4uc("vertexColor",
      this->Pen->GetColor());
    }

  this->SetMatrices(cbo->Program);

  if (this->Pen->GetWidth() > 1.0)
    {
    double *scale = this->ModelMatrix->GetScale();
    // convert to triangles and draw, this is because
    // OpenGL no longer supports wide lines directly
    float hwidth = this->Pen->GetWidth()/2.0;
    std::vector<float> newVerts;
    std::vector<unsigned char> newColors;
    for (int i = 0; i < n-1; i++)
      {
      // for each line segment draw two triangles
      // start by computing the direction
      vtkVector2f dir((f[i*2+2] - f[i*2])*scale[0],
        (f[i*2+3] - f[i*2+1])*scale[1]);
      vtkVector2f norm(-dir.GetY(),dir.GetX());
      norm.Normalize();
      norm.SetX(hwidth*norm.GetX()/scale[0]);
      norm.SetY(hwidth*norm.GetY()/scale[1]);

      newVerts.push_back(f[i*2]+norm.GetX());
      newVerts.push_back(f[i*2+1]+norm.GetY());
      newVerts.push_back(f[i*2]-norm.GetX());
      newVerts.push_back(f[i*2+1]-norm.GetY());
      newVerts.push_back(f[i*2+2]-norm.GetX());
      newVerts.push_back(f[i*2+3]-norm.GetY());

      newVerts.push_back(f[i*2]+norm.GetX());
      newVerts.push_back(f[i*2+1]+norm.GetY());
      newVerts.push_back(f[i*2+2]-norm.GetX());
      newVerts.push_back(f[i*2+3]-norm.GetY());
      newVerts.push_back(f[i*2+2]+norm.GetX());
      newVerts.push_back(f[i*2+3]+norm.GetY());

      if (colors)
        {
        copyColors(newColors, colors+i*nc, nc);
        copyColors(newColors, colors+i*nc, nc);
        copyColors(newColors, colors+(i+1)*nc, nc);
        copyColors(newColors, colors+i*nc, nc);
        copyColors(newColors, colors+(i+1)*nc, nc);
        copyColors(newColors, colors+(i+1)*nc, nc);
        }
      }

    this->BuildVBO(cbo, &(newVerts[0]), newVerts.size()/2,
      colors ? &(newColors[0]) : NULL, nc, NULL);
    glDrawArrays(GL_TRIANGLES, 0, newVerts.size()/2);
    }
  else
    {
    this->SetLineWidth(this->Pen->GetWidth());
    this->BuildVBO(cbo, f, n, colors, nc, NULL);
    glDrawArrays(GL_LINE_STRIP, 0, n);
    this->SetLineWidth(1.0);
    }

  // free everything
  cbo->ReleaseGraphicsResources(this->RenderWindow);

  vtkOpenGLCheckErrorMacro("failed after DrawPoly");
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::DrawLines(float *f, int n, unsigned char *colors,
                                         int nc)
{
  assert("f must be non-null" && f != NULL);
  assert("n must be greater than 0" && n > 0);

  if (this->Pen->GetLineType() == vtkPen::NO_PEN)
    {
    return;
    }

  vtkOpenGLClearErrorMacro();

  this->SetLineType(this->Pen->GetLineType());

  vtkOpenGLHelper *cbo = 0;
  if (colors)
    {
    this->ReadyVCBOProgram();
    cbo = this->VCBO;
    }
  else
    {
    this->ReadyVBOProgram();
    cbo = this->VBO;
    cbo->Program->SetUniform4uc("vertexColor",
      this->Pen->GetColor());
    }

  this->SetMatrices(cbo->Program);

  if (this->Pen->GetWidth() > 1.0)
    {
    double *scale = this->ModelMatrix->GetScale();
    // convert to triangles and draw, this is because
    // OpenGL no longer supports wide lines directly
    float hwidth = this->Pen->GetWidth()/2.0;
    std::vector<float> newVerts;
    std::vector<unsigned char> newColors;
    for (int i = 0; i < n-1; i += 2)
      {
      // for each line segment draw two triangles
      // start by computing the direction
      vtkVector2f dir((f[i*2+2] - f[i*2])*scale[0],
        (f[i*2+3] - f[i*2+1])*scale[1]);
      vtkVector2f norm(-dir.GetY(),dir.GetX());
      norm.Normalize();
      norm.SetX(hwidth*norm.GetX()/scale[0]);
      norm.SetY(hwidth*norm.GetY()/scale[1]);

      newVerts.push_back(f[i*2]+norm.GetX());
      newVerts.push_back(f[i*2+1]+norm.GetY());
      newVerts.push_back(f[i*2]-norm.GetX());
      newVerts.push_back(f[i*2+1]-norm.GetY());
      newVerts.push_back(f[i*2+2]-norm.GetX());
      newVerts.push_back(f[i*2+3]-norm.GetY());

      newVerts.push_back(f[i*2]+norm.GetX());
      newVerts.push_back(f[i*2+1]+norm.GetY());
      newVerts.push_back(f[i*2+2]-norm.GetX());
      newVerts.push_back(f[i*2+3]-norm.GetY());
      newVerts.push_back(f[i*2+2]+norm.GetX());
      newVerts.push_back(f[i*2+3]+norm.GetY());

      if (colors)
        {
        copyColors(newColors, colors+i*nc, nc);
        copyColors(newColors, colors+i*nc, nc);
        copyColors(newColors, colors+(i+1)*nc, nc);
        copyColors(newColors, colors+i*nc, nc);
        copyColors(newColors, colors+(i+1)*nc, nc);
        copyColors(newColors, colors+(i+1)*nc, nc);
        }
      }

    this->BuildVBO(cbo, &(newVerts[0]), newVerts.size()/2,
      colors ? &(newColors[0]) : NULL, nc, NULL);
    glDrawArrays(GL_TRIANGLES, 0, newVerts.size()/2);
    }
  else
    {
    this->SetLineWidth(this->Pen->GetWidth());
    this->BuildVBO(cbo, f, n, colors, nc, NULL);
    glDrawArrays(GL_LINES, 0, n);
    this->SetLineWidth(1.0);
    }

  // free everything
  cbo->ReleaseGraphicsResources(this->RenderWindow);

  vtkOpenGLCheckErrorMacro("failed after DrawLines");
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::DrawPoints(float *f, int n, unsigned char *c,
                                          int nc)
{
  vtkOpenGLClearErrorMacro();

  vtkOpenGLHelper *cbo = 0;
  if (c)
    {
    this->ReadyVCBOProgram();
    cbo = this->VCBO;
    }
  else
    {
    this->ReadyVBOProgram();
    cbo = this->VBO;
    cbo->Program->SetUniform4uc("vertexColor",
      this->Pen->GetColor());
    }

  this->SetPointSize(this->Pen->GetWidth());

  this->BuildVBO(cbo, f, n, c, nc, NULL);
  this->SetMatrices(cbo->Program);

  glDrawArrays(GL_POINTS, 0, n);

  // free everything
  cbo->ReleaseGraphicsResources(this->RenderWindow);

  vtkOpenGLCheckErrorMacro("failed after DrawPoints");
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::DrawPointSprites(vtkImageData *sprite,
                                                float *points, int n,
                                                unsigned char *colors,
                                                int nc_comps)
{
  vtkOpenGLClearErrorMacro();
  if (points && n > 0)
    {
    this->SetPointSize(this->Pen->GetWidth());

    vtkOpenGLHelper *cbo = 0;
    if (colors)
      {
      this->ReadySCBOProgram();
      cbo = this->SCBO;
      }
    else
      {
      this->ReadySBOProgram();
      cbo = this->SBO;
      cbo->Program->SetUniform4uc("vertexColor",
        this->Pen->GetColor());
      }

    this->BuildVBO(cbo, points, n, colors, nc_comps, NULL);
    this->SetMatrices(cbo->Program);

    if (sprite)
      {
      if (!this->Storage->SpriteTexture)
        {
        this->Storage->SpriteTexture = vtkTexture::New();
        }
      int properties = this->Brush->GetTextureProperties();
      this->Storage->SpriteTexture->SetInputData(sprite);
      this->Storage->SpriteTexture->SetRepeat(properties & vtkContextDevice2D::Repeat);
      this->Storage->SpriteTexture->SetInterpolate(properties & vtkContextDevice2D::Linear);
      this->Storage->SpriteTexture->Render(this->Renderer);
      int tunit = vtkOpenGLTexture::SafeDownCast(
        this->Storage->SpriteTexture)->GetTextureUnit();
      cbo->Program->SetUniformi("texture1", tunit);
      }

    // We can actually use point sprites here
    if (!vtkOpenGLRenderWindow::GetContextSupportsOpenGL32())
      {
      glEnable(GL_POINT_SPRITE);
      glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);
      }
    glPointParameteri(GL_POINT_SPRITE_COORD_ORIGIN, GL_LOWER_LEFT);

    glDrawArrays(GL_POINTS, 0, n);

    // free everything
    cbo->ReleaseGraphicsResources(this->RenderWindow);

    if (!vtkOpenGLRenderWindow::GetContextSupportsOpenGL32())
      {
      glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_FALSE);
      glDisable(GL_POINT_SPRITE);
      }

    if (sprite)
      {
      this->Storage->SpriteTexture->PostRender(this->Renderer);
      }
    }
  else
    {
    vtkWarningMacro(<< "Points supplied without a valid image or pointer.");
    }
  vtkOpenGLCheckErrorMacro("failed after DrawPointSprites");
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::DrawMarkers(int shape, bool highlight,
                                           float *points, int n,
                                           unsigned char *colors, int nc_comps)
{
  // Get a point sprite for the shape
  vtkImageData *sprite = this->GetMarker(shape, this->Pen->GetWidth(),
                                         highlight);
  this->DrawPointSprites(sprite, points, n, colors, nc_comps);
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::DrawQuad(float *f, int n)
{
  if (!f || n <= 0)
    {
    vtkWarningMacro(<< "Points supplied that were not of type float.");
    return;
    }

  // convert quads to triangles
  std::vector<float> tverts;
  int numTVerts = 6*n/4;
  tverts.resize(numTVerts*2);
  int offset[6] = {0,1,2,0,2,3};
  for (int i = 0; i < numTVerts; i++)
    {
    int index = 2*(4*(i/6)+offset[i%6]);
    tverts[i*2] = f[index];
    tverts[i*2+1] = f[index+1];
    }

  this->CoreDrawTriangles(tverts);
}

void vtkOpenGLContextDevice2D::CoreDrawTriangles(
  std::vector<float> &tverts)
{
  vtkOpenGLClearErrorMacro();

  float* texCoord = 0;
  vtkOpenGLHelper *cbo = 0;
  if (this->Brush->GetTexture())
    {
    this->ReadyVTBOProgram();
    cbo = this->VTBO;
    this->SetTexture(this->Brush->GetTexture(),
                     this->Brush->GetTextureProperties());
    this->Storage->Texture->Render(this->Renderer);
    texCoord = this->Storage->TexCoords(&(tverts[0]), tverts.size()/2);

    int tunit = vtkOpenGLTexture::SafeDownCast(this->Storage->Texture)->GetTextureUnit();
    cbo->Program->SetUniformi("texture1", tunit);
    }
  else
    {
    this->ReadyVBOProgram();
    cbo = this->VBO;
    }
  cbo->Program->SetUniform4uc("vertexColor",
      this->Brush->GetColor());

  this->BuildVBO(cbo, &(tverts[0]), tverts.size()/2, NULL, 0, texCoord);
  this->SetMatrices(cbo->Program);

  glDrawArrays(GL_TRIANGLES, 0, tverts.size()/2);

  // free everything
  cbo->ReleaseGraphicsResources(this->RenderWindow);

  if (this->Storage->Texture)
    {
    this->Storage->Texture->PostRender(this->Renderer);
    delete [] texCoord;
    }
  vtkOpenGLCheckErrorMacro("failed after DrawQuad");
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::DrawQuadStrip(float *f, int n)
{
  if (!f || n <= 0)
    {
    vtkWarningMacro(<< "Points supplied that were not of type float.");
    return;
    }

  // convert quad strips to triangles
  std::vector<float> tverts;
  int numTVerts = 3*(n-2);
  tverts.resize(numTVerts*2);
  int offset[6] = {0,1,3,0,3,2};
  for (int i = 0; i < numTVerts; i++)
    {
    int index = 2*(2*(i/6)+offset[i%6]);
    tverts[i*2] = f[index];
    tverts[i*2+1] = f[index+1];
    }

  this->CoreDrawTriangles(tverts);
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::DrawPolygon(float *f, int n)
{
  if (!f || n <= 0)
    {
    vtkWarningMacro(<< "Points supplied that were not of type float.");
    return;
    }

  // convert polygon to triangles
  std::vector<float> tverts;
  int numTVerts = 3*(n-2);
  tverts.resize(numTVerts*2);
  for (int i = 0; i < n-2; i++)
    {
    tverts.push_back(f[0]);
    tverts.push_back(f[1]);
    tverts.push_back(f[i*2+2]);
    tverts.push_back(f[i*2+3]);
    tverts.push_back(f[i*2+4]);
    tverts.push_back(f[i*2+5]);
    }

  this->CoreDrawTriangles(tverts);
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::DrawEllipseWedge(float x, float y, float outRx,
                                                float outRy, float inRx,
                                                float inRy, float startAngle,
                                                float stopAngle)

{
  assert("pre: positive_outRx" && outRx>=0.0f);
  assert("pre: positive_outRy" && outRy>=0.0f);
  assert("pre: positive_inRx" && inRx>=0.0f);
  assert("pre: positive_inRy" && inRy>=0.0f);
  assert("pre: ordered_rx" && inRx<=outRx);
  assert("pre: ordered_ry" && inRy<=outRy);

  if(outRy==0.0f && outRx==0.0f)
    {
    // we make sure maxRadius will never be null.
    return;
    }

  int iterations=this->GetNumberOfArcIterations(outRx,outRy,startAngle,
                                                stopAngle);

  // step in radians.
  double step =
    vtkMath::RadiansFromDegrees(stopAngle-startAngle)/(iterations);

  // step have to be lesser or equal to maxStep computed inside
  // GetNumberOfIterations()

  double rstart=vtkMath::RadiansFromDegrees(startAngle);

  // the A vertices (0,2,4,..) are on the inner side
  // the B vertices (1,3,5,..) are on the outer side
  // (A and B vertices terms come from triangle strip definition in
  // OpenGL spec)
  // we are iterating counterclockwise

  // convert polygon to triangles
  std::vector<float> tverts;
  int numTVerts = 6*iterations;
  tverts.resize(numTVerts*2);
  int offset[6] = {0,1,3,0,3,2};
  for (int i = 0; i < numTVerts; i++)
    {
    int index = i/6 + offset[i%6]/2;
    double radiusX = offset[i%6]%2 ? outRx : inRx;
    double radiusY = offset[i%6]%2 ? outRy : inRy;
    double a=rstart+index*step;
    tverts.push_back(radiusX * cos(a) + x);
    tverts.push_back(radiusY * sin(a) + y);
    }

  this->CoreDrawTriangles(tverts);
}

// ----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::DrawEllipticArc(float x, float y, float rX,
                                               float rY, float startAngle,
                                               float stopAngle)
{
  assert("pre: positive_rX" && rX>=0);
  assert("pre: positive_rY" && rY>=0);

  if(rX==0.0f && rY==0.0f)
    {
    // we make sure maxRadius will never be null.
    return;
    }

  vtkOpenGLClearErrorMacro();

  int iterations = this->GetNumberOfArcIterations(rX, rY, startAngle, stopAngle);

  float *p = new float[2*(iterations+1)];

  // step in radians.
  double step =
    vtkMath::RadiansFromDegrees(stopAngle-startAngle)/(iterations);

  // step have to be lesser or equal to maxStep computed inside
  // GetNumberOfIterations()
  double rstart=vtkMath::RadiansFromDegrees(startAngle);

  // we are iterating counterclockwise
  for(int i = 0; i <= iterations; ++i)
    {
    double a=rstart+i*step;
    p[2*i  ] = rX * cos(a) + x;
    p[2*i+1] = rY * sin(a) + y;
    }

  this->DrawPolygon(p,iterations+1);
  this->DrawPoly(p,iterations+1);
  delete[] p;

  vtkOpenGLCheckErrorMacro("failed after DrawEllipseArc");
}

// ----------------------------------------------------------------------------
int vtkOpenGLContextDevice2D::GetNumberOfArcIterations(float rX,
                                                       float rY,
                                                       float startAngle,
                                                       float stopAngle)
{
  assert("pre: positive_rX" && rX>=0.0f);
  assert("pre: positive_rY" && rY>=0.0f);
  assert("pre: not_both_null" && (rX>0.0 || rY>0.0));

// 1.0: pixel precision. 0.5 (subpixel precision, useful with multisampling)
  double error = 4.0; // experience shows 4.0 is visually enough.

  // The tessellation is the most visible on the biggest radius.
  double maxRadius;
  if(rX >= rY)
    {
    maxRadius = rX;
    }
  else
    {
    maxRadius = rY;
    }

  if(error > maxRadius)
    {
    // to make sure the argument of asin() is in a valid range.
    error = maxRadius;
    }

  // Angle of a sector so that its chord is `error' pixels.
  // This is will be our maximum angle step.
  double maxStep = 2.0 * asin(error / (2.0 * maxRadius));

  // ceil because we want to make sure we don't underestimate the number of
  // iterations by 1.
  return static_cast<int>(
    ceil(vtkMath::RadiansFromDegrees(stopAngle - startAngle) / maxStep));
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::AlignText(double orientation, float width,
                                         float height, float *p)
{
  // Special case multiples of 90 as no transformation is required...
  if (orientation > -0.0001 && orientation < 0.0001)
    {
    switch (this->TextProp->GetJustification())
      {
      case VTK_TEXT_LEFT:
        break;
      case VTK_TEXT_CENTERED:
        p[0] -= floor(width / 2.0);
        break;
      case VTK_TEXT_RIGHT:
        p[0] -= width;
        break;
      }
    switch (this->TextProp->GetVerticalJustification())
      {
      case VTK_TEXT_BOTTOM:
        break;
      case VTK_TEXT_CENTERED:
        p[1] -= floor(height / 2.0);
        break;
      case VTK_TEXT_TOP:
        p[1] -= height;
        break;
      }
    }
  else if (orientation > 89.9999 && orientation < 90.0001)
    {
    switch (this->TextProp->GetJustification())
      {
      case VTK_TEXT_LEFT:
        break;
      case VTK_TEXT_CENTERED:
        p[1] -= floor(height / 2.0);
        break;
      case VTK_TEXT_RIGHT:
        p[1] -= height;
        break;
      }
    switch (this->TextProp->GetVerticalJustification())
      {
      case VTK_TEXT_TOP:
        break;
      case VTK_TEXT_CENTERED:
        p[0] -= floor(width / 2.0);
        break;
      case VTK_TEXT_BOTTOM:
        p[0] -= width;
        break;
      }
    }
  else if (orientation > 179.9999 && orientation < 180.0001)
    {
    switch (this->TextProp->GetJustification())
      {
      case VTK_TEXT_RIGHT:
        break;
      case VTK_TEXT_CENTERED:
        p[0] -= floor(width / 2.0);
        break;
      case VTK_TEXT_LEFT:
        p[0] -= width;
        break;
      }
    switch (this->TextProp->GetVerticalJustification())
      {
      case VTK_TEXT_TOP:
        break;
      case VTK_TEXT_CENTERED:
        p[1] -= floor(height / 2.0);
        break;
      case VTK_TEXT_BOTTOM:
        p[1] -= height;
        break;
      }
    }
  else if (orientation > 269.9999 && orientation < 270.0001)
    {
    switch (this->TextProp->GetJustification())
      {
      case VTK_TEXT_LEFT:
        break;
      case VTK_TEXT_CENTERED:
        p[1] -= floor(height / 2.0);
        break;
      case VTK_TEXT_RIGHT:
        p[1] -= height;
        break;
      }
    switch (this->TextProp->GetVerticalJustification())
      {
      case VTK_TEXT_BOTTOM:
        break;
      case VTK_TEXT_CENTERED:
        p[0] -= floor(width / 2.0);
        break;
      case VTK_TEXT_TOP:
        p[0] -= width;
        break;
      }
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::DrawString(float *point,
                                          const vtkStdString &string)
{
  this->DrawString(point, vtkUnicodeString::from_utf8(string));
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::ComputeStringBounds(const vtkStdString &string,
                                                   float bounds[4])
{
  this->ComputeStringBounds(vtkUnicodeString::from_utf8(string), bounds);
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::DrawString(float *point,
                                          const vtkUnicodeString &string)
{
  vtkOpenGLClearErrorMacro();

  double *mv = this->ModelMatrix->GetMatrix()->Element[0];
  float xScale = mv[0];
  float yScale = mv[5];

  float p[] = { std::floor(point[0] * xScale) / xScale,
                std::floor(point[1] * yScale) / yScale };

  // TODO this currently ignores vtkContextScene::ScaleTiles. Not sure how to
  // get at that from here, but this is better than ignoring scaling altogether.
  // TODO Also, FreeType supports anisotropic DPI. Might be needed if the
  // tileScale isn't homogeneous, but we'll need to update the textrenderer API
  // and see if MPL/mathtext can support it.
  int tileScale[2];
  this->RenderWindow->GetTileScale(tileScale);
  int dpi = this->RenderWindow->GetDPI() * std::max(tileScale[0], tileScale[1]);

  // Cache rendered text strings
  vtkTextureImageCache<UTF16TextPropertyKey>::CacheData &cache =
      this->Storage->TextTextureCache.GetCacheData(
        UTF16TextPropertyKey(this->TextProp, string, dpi));
  vtkImageData* image = cache.ImageData;
  if (image->GetNumberOfPoints() == 0 && image->GetNumberOfCells() == 0)
    {
    int textDims[2];
    if (!this->TextRenderer->RenderString(this->TextProp, string, dpi, image,
                                          textDims))
      {
      return;
      }
    cache.TextWidth = textDims[0];
    cache.TextHeight = textDims[1];
    }
  vtkTexture* texture = cache.Texture;
  texture->Render(this->Renderer);

  int imgDims[3];
  image->GetDimensions(imgDims);

  float width = cache.TextWidth / xScale;
  float height = cache.TextHeight / yScale;

  float xw = cache.TextWidth / static_cast<float>(imgDims[0]);
  float xh = cache.TextHeight / static_cast<float>(imgDims[1]);

  this->AlignText(this->TextProp->GetOrientation(), width, height, p);

  float points[] = { p[0]        , p[1],
                     p[0] + width, p[1],
                     p[0] + width, p[1] + height,
                     p[0]        , p[1],
                     p[0] + width, p[1] + height,
                     p[0]        , p[1] + height };

  float texCoord[] = { 0.0f, 0.0f,
                       xw,   0.0f,
                       xw,   xh,
                       0.0f,   0.0f,
                       xw,   xh,
                       0.0f, xh };

  vtkOpenGLClearErrorMacro();

  vtkOpenGLHelper *cbo = 0;
  this->ReadyVTBOProgram();
  cbo = this->VTBO;
  int tunit = vtkOpenGLTexture::SafeDownCast(texture)->GetTextureUnit();
  cbo->Program->SetUniformi("texture1", tunit);

  this->BuildVBO(cbo, points, 6, NULL, 0, texCoord);
  this->SetMatrices(cbo->Program);

  glDrawArrays(GL_TRIANGLES, 0, 6);

  // free everything
  cbo->ReleaseGraphicsResources(this->RenderWindow);

  texture->PostRender(this->Renderer);

  vtkOpenGLCheckErrorMacro("failed after DrawString");
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::ComputeStringBounds(const vtkUnicodeString &string,
                                                   float bounds[4])
{
  // TODO this currently ignores vtkContextScene::ScaleTiles. Not sure how to
  // get at that from here, but this is better than ignoring scaling altogether.
  // TODO Also, FreeType supports anisotropic DPI. Might be needed if the
  // tileScale isn't homogeneous, but we'll need to update the textrenderer API
  // and see if MPL/mathtext can support it.
  int tileScale[2];
  this->RenderWindow->GetTileScale(tileScale);
  int dpi = this->RenderWindow->GetDPI() * std::max(tileScale[0], tileScale[1]);

  vtkVector2i box = this->TextRenderer->GetBounds(this->TextProp, string, dpi);
  // Check for invalid bounding box
  if (box[0] == VTK_INT_MIN || box[0] == VTK_INT_MAX ||
      box[1] == VTK_INT_MIN || box[1] == VTK_INT_MAX)
    {
    bounds[0] = static_cast<float>(0);
    bounds[1] = static_cast<float>(0);
    bounds[2] = static_cast<float>(0);
    bounds[3] = static_cast<float>(0);
    return;
    }

  double *mv = this->ModelMatrix->GetMatrix()->Element[0];
  float xScale = mv[0];
  float yScale = mv[5];
  bounds[0] = static_cast<float>(0);
  bounds[1] = static_cast<float>(0);
  bounds[2] = static_cast<float>(box.GetX() / xScale);
  bounds[3] = static_cast<float>(box.GetY() / yScale);
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::DrawMathTextString(float point[2],
                                                  const vtkStdString &string)
{
  vtkMathTextUtilities *mathText = vtkMathTextUtilities::GetInstance();
  if (!mathText || !mathText->IsAvailable())
    {
    vtkWarningMacro(<<"MathText is not available to parse string "
                    << string.c_str() << ". Install matplotlib and enable "
                    "python to use MathText.");
    return;
    }

  vtkOpenGLClearErrorMacro();

  float p[] = { std::floor(point[0]), std::floor(point[1]) };

  // TODO this currently ignores vtkContextScene::ScaleTiles. Not sure how to
  // get at that from here, but this is better than ignoring scaling altogether.
  // TODO Also, FreeType supports anisotropic DPI. Might be needed if the
  // tileScale isn't homogeneous, but we'll need to update the textrenderer API
  // and see if MPL/mathtext can support it.
  int tileScale[2];
  this->RenderWindow->GetTileScale(tileScale);
  int dpi = this->RenderWindow->GetDPI() * std::max(tileScale[0], tileScale[1]);

  // Cache rendered text strings
  vtkTextureImageCache<UTF8TextPropertyKey>::CacheData &cache =
    this->Storage->MathTextTextureCache.GetCacheData(
      UTF8TextPropertyKey(this->TextProp, string, dpi));
  vtkImageData* image = cache.ImageData;
  if (image->GetNumberOfPoints() == 0 && image->GetNumberOfCells() == 0)
    {
    int textDims[2];
    if (!mathText->RenderString(string.c_str(), image, this->TextProp, dpi,
                                textDims))
      {
      return;
      }
    cache.TextWidth = textDims[0];
    cache.TextHeight = textDims[1];
    }

  vtkTexture* texture = cache.Texture;
  texture->Render(this->Renderer);

  double *mv = this->ModelMatrix->GetMatrix()->Element[0];
  float xScale = mv[0];
  float yScale = mv[5];

  int imgDims[3];
  image->GetDimensions(imgDims);

  float width = cache.TextWidth / xScale;
  float height = cache.TextHeight / yScale;

  float xw = cache.TextWidth / static_cast<float>(imgDims[0]);
  float xh = cache.TextHeight / static_cast<float>(imgDims[1]);

  this->AlignText(this->TextProp->GetOrientation(), width, height, p);

  float points[] = { p[0]        , p[1],
                     p[0] + width, p[1],
                     p[0] + width, p[1] + height,
                     p[0]        , p[1],
                     p[0] + width, p[1] + height,
                     p[0]        , p[1] + height };

  float texCoord[] = { 0.0f, 0.0f,
                       xw,   0.0f,
                       xw,   xh,
                       0.0f,   0.0f,
                       xw,   xh,
                       0.0f, xh };

  vtkOpenGLClearErrorMacro();

  vtkOpenGLHelper *cbo = 0;
  this->ReadyVTBOProgram();
  cbo = this->VTBO;
  int tunit = vtkOpenGLTexture::SafeDownCast(texture)->GetTextureUnit();
  cbo->Program->SetUniformi("texture1", tunit);

  this->BuildVBO(cbo, points, 6, NULL, 0, texCoord);
  this->SetMatrices(cbo->Program);

  glDrawArrays(GL_TRIANGLES, 0, 6);

  // free everything
  cbo->ReleaseGraphicsResources(this->RenderWindow);

  texture->PostRender(this->Renderer);

  vtkOpenGLCheckErrorMacro("failed after DrawMathTexString");
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::DrawImage(float p[2], float scale,
                                         vtkImageData *image)
{
  vtkOpenGLClearErrorMacro();

  this->SetTexture(image);
  this->Storage->Texture->Render(this->Renderer);
  int *extent = image->GetExtent();
  float points[] = { p[0]                     , p[1],
                     p[0]+scale*extent[1]+1.0f, p[1],
                     p[0]+scale*extent[1]+1.0f, p[1]+scale*extent[3]+1.0f,
                     p[0]                     , p[1],
                     p[0]+scale*extent[1]+1.0f, p[1]+scale*extent[3]+1.0f,
                     p[0]                     , p[1]+scale*extent[3]+1.0f };

  float texCoord[] = { 0.0f,   0.0f,
                       1.0f,   0.0f,
                       1.0f,   1.0f,
                       0.0f,   0.0f,
                       1.0f,   1.0f,
                       0.0f,   1.0f };

  vtkOpenGLClearErrorMacro();

  vtkOpenGLHelper *cbo = 0;
  this->ReadyVTBOProgram();
  cbo = this->VTBO;
  int tunit = vtkOpenGLTexture::SafeDownCast(
    this->Storage->Texture)->GetTextureUnit();
  cbo->Program->SetUniformi("texture1", tunit);

    cerr << "doing image\n";
    if (this->Storage->Texture->GetTransform())
      {
        cerr << "have a transform\n";
      }

  this->BuildVBO(cbo, points, 6, NULL, 0, texCoord);
  this->SetMatrices(cbo->Program);

  glDrawArrays(GL_TRIANGLES, 0, 6);

  // free everything
  cbo->ReleaseGraphicsResources(this->RenderWindow);

  this->Storage->Texture->PostRender(this->Renderer);

  vtkOpenGLCheckErrorMacro("failed after DrawImage");
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::DrawImage(const vtkRectf& pos,
                                         vtkImageData *image)
{
  vtkVector2f tex(1.0, 1.0);
  GLuint index = 0;
  if (this->Storage->PowerOfTwoTextures)
    {
    index = this->Storage->TextureFromImage(image, tex);
    }
  else
    {
    index = this->Storage->TextureFromImage(image, tex);
    }

  float points[] = {
    pos.GetX()                 , pos.GetY(),
    pos.GetX() + pos.GetWidth(), pos.GetY(),
    pos.GetX() + pos.GetWidth(), pos.GetY() + pos.GetHeight(),
    pos.GetX()                 , pos.GetY(),
    pos.GetX() + pos.GetWidth(), pos.GetY() + pos.GetHeight(),
    pos.GetX()                 , pos.GetY() + pos.GetHeight() };

  float texCoord[] = { 0.0f,   0.0f,
                       tex[0], 0.0f,
                       tex[0], tex[1],
                       0.0f,   0.0f,
                       tex[0], tex[1],
                       0.0f,   tex[1]};


  vtkOpenGLHelper *cbo = 0;
  this->ReadyVTBOProgram();
  cbo = this->VTBO;
  int tunit =  this->RenderWindow->GetTextureUnitManager()->Allocate();
  if (tunit < 0)
    {
    vtkErrorMacro("Hardware does not support the number of textures defined.");
    return;
    }
  glActiveTexture(GL_TEXTURE0 + tunit);

  cbo->Program->SetUniformi("texture1", tunit);

  this->BuildVBO(cbo, points, 6, NULL, 0, texCoord);
  this->SetMatrices(cbo->Program);

  glDrawArrays(GL_TRIANGLES, 0, 6);

  this->RenderWindow->GetTextureUnitManager()->Free(tunit);

  // free everything
  cbo->ReleaseGraphicsResources(this->RenderWindow);

  glDeleteTextures(1, &index);

  vtkOpenGLCheckErrorMacro("failed after DrawImage");

}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::SetColor4(unsigned char *)
{
  vtkErrorMacro("color cannot be set this way\n");
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::SetColor(unsigned char *)
{
  vtkErrorMacro("color cannot be set this way\n");
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::SetTexture(vtkImageData* image, int properties)
{
  if (image == NULL)
    {
    if (this->Storage->Texture)
      {
      this->Storage->Texture->Delete();
      this->Storage->Texture = 0;
      }
    return;
    }
  if (this->Storage->Texture == NULL)
    {
    this->Storage->Texture = vtkTexture::New();
    }
  this->Storage->Texture->SetInputData(image);
  this->Storage->TextureProperties = properties;
  this->Storage->Texture->SetRepeat(properties & vtkContextDevice2D::Repeat);
  this->Storage->Texture->SetInterpolate(properties & vtkContextDevice2D::Linear);
  this->Storage->Texture->EdgeClampOn();
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::SetPointSize(float size)
{
  glPointSize(size);
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::SetLineWidth(float width)
{
  glLineWidth(width);
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::SetLineType(int type)
{
  if (type == vtkPen::SOLID_LINE || type == vtkPen::NO_PEN)
    {
    return;
    }
  vtkWarningMacro(<< "Line Stipples are no longer supported");
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::MultiplyMatrix(vtkMatrix3x3 *m)
{
  // We must construct a 4x4 matrix from the 3x3 matrix for OpenGL
  double *M = m->GetData();
  double matrix[16];

  matrix[0] = M[0];
  matrix[1] = M[1];
  matrix[2] = 0.0;
  matrix[3] = M[2];
  matrix[4] = M[3];
  matrix[5] = M[4];
  matrix[6] = 0.0;
  matrix[7] = M[5];
  matrix[8] = 0.0;
  matrix[9] = 0.0;
  matrix[10] = 1.0;
  matrix[11] = 0.0;
  matrix[12] = M[6];
  matrix[13] = M[7];
  matrix[14] = 0.0;
  matrix[15] = M[8];

  this->ModelMatrix->Concatenate(matrix);
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::SetMatrix(vtkMatrix3x3 *m)
{
  // We must construct a 4x4 matrix from the 3x3 matrix for OpenGL
  double *M = m->GetData();
  double matrix[16];

  matrix[0] = M[0];
  matrix[1] = M[1];
  matrix[2] = 0.0;
  matrix[3] = M[2];
  matrix[4] = M[3];
  matrix[5] = M[4];
  matrix[6] = 0.0;
  matrix[7] = M[5];
  matrix[8] = 0.0;
  matrix[9] = 0.0;
  matrix[10] = 1.0;
  matrix[11] = 0.0;
  matrix[12] = M[6];
  matrix[13] = M[7];
  matrix[14] = 0.0;
  matrix[15] = M[8];

  this->ModelMatrix->SetMatrix(matrix);
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::GetMatrix(vtkMatrix3x3 *m)
{
  assert("pre: non_null" && m != NULL);
  // We must construct a 4x4 matrix from the 3x3 matrix for OpenGL
  double *M = m->GetData();
  double *matrix = this->ModelMatrix->GetMatrix()->Element[0];

  M[0] = matrix[0];
  M[1] = matrix[1];
  M[2] = matrix[3];
  M[3] = matrix[4];
  M[4] = matrix[5];
  M[5] = matrix[7];
  M[6] = matrix[12];
  M[7] = matrix[13];
  M[8] = matrix[15];

  m->Modified();
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::PushMatrix()
{
  this->ModelMatrix->Push();
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::PopMatrix()
{
  this->ModelMatrix->Pop();
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::SetClipping(int *dim)
{
  // Check the bounds, and clamp if necessary
  GLint vp[4] = { this->Storage->Offset.GetX(), this->Storage->Offset.GetY(),
    this->Storage->Dim.GetX(),this->Storage->Dim.GetY()};

  if (dim[0] > 0 && dim[0] < vp[2] )
    {
    vp[0] += dim[0];
    }
  if (dim[1] > 0 && dim[1] < vp[3])
    {
    vp[1] += dim[1];
    }
  if (dim[2] > 0 && dim[2] < vp[2])
    {
    vp[2] = dim[2];
    }
  if (dim[3] > 0 && dim[3] < vp[3])
    {
    vp[3] = dim[3];
    }

  glScissor(vp[0], vp[1], vp[2], vp[3]);
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::EnableClipping(bool enable)
{
  if (enable)
    {
    glEnable(GL_SCISSOR_TEST);
    }
  else
    {
    glDisable(GL_SCISSOR_TEST);
    }
}

//-----------------------------------------------------------------------------
bool vtkOpenGLContextDevice2D::SetStringRendererToFreeType()
{
  // FreeType is the only choice - nothing to do here
  return true;
}

//-----------------------------------------------------------------------------
bool vtkOpenGLContextDevice2D::SetStringRendererToQt()
{
  // The Qt based strategy is not available
  return false;
}

//----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::ReleaseGraphicsResources(vtkWindow *window)
{
  this->VBO->ReleaseGraphicsResources(window);
  this->VCBO->ReleaseGraphicsResources(window);
  this->SBO->ReleaseGraphicsResources(window);
  this->SCBO->ReleaseGraphicsResources(window);
  this->VTBO->ReleaseGraphicsResources(window);
  if (this->Storage->Texture)
    {
    this->Storage->Texture->ReleaseGraphicsResources(window);
    }
  if (this->Storage->SpriteTexture)
    {
    this->Storage->SpriteTexture->ReleaseGraphicsResources(window);
    }
  this->Storage->TextTextureCache.ReleaseGraphicsResources(window);
  this->Storage->MathTextTextureCache.ReleaseGraphicsResources(window);
}

//----------------------------------------------------------------------------
bool vtkOpenGLContextDevice2D::HasGLSL()
{
  return true;
}

//-----------------------------------------------------------------------------
vtkImageData *vtkOpenGLContextDevice2D::GetMarker(int shape, int size,
                                                  bool highlight)
{
  // Generate the cache key for this marker
  vtkTypeUInt64 key = highlight ? (1U << 31) : 0U;
  key |= static_cast<vtkTypeUInt16>(shape);
  key <<= 32;
  key |= static_cast<vtkTypeUInt32>(size);

  // Try to find the marker in the cache
  std::list<vtkMarkerCacheObject>::iterator match =
      std::find(this->MarkerCache.begin(), this->MarkerCache.end(), key);

  // Was it in the cache?
  if (match != this->MarkerCache.end())
    {
    // Yep -- move it to the front and return the data.
    if (match == this->MarkerCache.begin())
      {
      return match->Value;
      }
    else
      {
      vtkMarkerCacheObject result = *match;
      this->MarkerCache.erase(match);
      this->MarkerCache.push_front(result);
      return result.Value;
      }
    }

  // Nope -- we'll need to generate it. Create the image data:
  vtkMarkerCacheObject result;
  result.Key = key;
  result.Value = this->GenerateMarker(shape, size, highlight);

  // If there was an issue generating the marker, just return NULL.
  if (!result.Value)
    {
    vtkErrorMacro(<<"Error generating marker: shape,size: "
                  << shape << "," << size)
    return NULL;
    }

  // Check the current cache size.
  while (this->MarkerCache.size() >
         static_cast<size_t>(this->MaximumMarkerCacheSize - 1) &&
         !this->MarkerCache.empty())
    {
    this->MarkerCache.back().Value->Delete();
    this->MarkerCache.pop_back();
    }

   // Add to the cache
   this->MarkerCache.push_front(result);
   return result.Value;
}

//-----------------------------------------------------------------------------
vtkImageData *vtkOpenGLContextDevice2D::GenerateMarker(int shape, int width,
                                                       bool highlight)
{
  // Set up the image data, if highlight then the mark shape is different
  vtkImageData *result = vtkImageData::New();

  result->SetExtent(0, width - 1, 0, width - 1, 0, 0);
  result->AllocateScalars(VTK_UNSIGNED_CHAR, 4);

  unsigned char* image =
      static_cast<unsigned char*>(result->GetScalarPointer());
  memset(image, 0, width * width * 4);

  // Generate the marker image at the required size
  switch (shape)
    {
    case VTK_MARKER_CROSS:
      {
      int center = (width + 1) / 2;
      for (int i = 0; i < center; ++i)
        {
        int j = width - i - 1;
        memset(image + (4 * (width * i + i)), 255, 4);
        memset(image + (4 * (width * i + j)), 255, 4);
        memset(image + (4 * (width * j + i)), 255, 4);
        memset(image + (4 * (width * j + j)), 255, 4);
        if (highlight)
          {
          memset(image + (4 * (width * (j-1) + (i))), 255, 4);
          memset(image + (4 * (width * (i+1) + (i))), 255, 4);
          memset(image + (4 * (width * (i) + (i+1))), 255, 4);
          memset(image + (4 * (width * (i) + (j-1))), 255, 4);
          memset(image + (4 * (width * (i+1) + (j))), 255, 4);
          memset(image + (4 * (width * (j-1) + (j))), 255, 4);
          memset(image + (4 * (width * (j) + (j-1))), 255, 4);
          memset(image + (4 * (width * (j) + (i+1))), 255, 4);
          }
        }
      break;
      }
    default: // Maintaining old behavior, which produces plus for unknown shape
      vtkWarningMacro(<<"Invalid marker shape: " << shape);
      VTK_FALLTHROUGH;
    case VTK_MARKER_PLUS:
      {
      int center = (width + 1) / 2;
      for (int i = 0; i < center; ++i)
        {
        int j = width - i - 1;
        int c = center - 1;
        memset(image + (4 * (width * c + i)), 255, 4);
        memset(image + (4 * (width * c + j)), 255, 4);
        memset(image + (4 * (width * i + c)), 255, 4);
        memset(image + (4 * (width * j + c)), 255, 4);
        if (highlight)
          {
          memset(image + (4 * (width * (c - 1) + i)), 255, 4);
          memset(image + (4 * (width * (c + 1) + i)), 255, 4);
          memset(image + (4 * (width * (c - 1) + j)), 255, 4);
          memset(image + (4 * (width * (c + 1) + j)), 255, 4);
          memset(image + (4 * (width * i + (c - 1))), 255, 4);
          memset(image + (4 * (width * i + (c + 1))), 255, 4);
          memset(image + (4 * (width * j + (c - 1))), 255, 4);
          memset(image + (4 * (width * j + (c + 1))), 255, 4);
          }
        }
      break;
      }
    case VTK_MARKER_SQUARE:
      {
      memset(image, 255, width * width * 4);
      break;
      }
    case VTK_MARKER_CIRCLE:
      {
      double r = width/2.0;
      double r2 = r * r;
      for (int i = 0; i < width; ++i)
        {
        double dx2 = (i - r) * (i - r);
        for (int j = 0; j < width; ++j)
          {
          double dy2 = (j - r) * (j - r);
          if ((dx2 + dy2) < r2)
            {
            memset(image + (4 * width * i) + (4 * j), 255, 4);
            }
          }
        }
      break;
      }
    case VTK_MARKER_DIAMOND:
      {
      int r = width/2;
      for (int i = 0; i < width; ++i)
        {
        int dx = abs(i - r);
        for (int j = 0; j < width; ++j)
          {
          int dy = abs(j - r);
          if (r - dx >= dy)
            {
            memset(image + (4 * width * i) + (4 * j), 255, 4);
            }
          }
        }
      break;
      }
    }
  return result;
}

//-----------------------------------------------------------------------------
void vtkOpenGLContextDevice2D::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Renderer: ";
  if (this->Renderer)
    {
    os << endl;
    this->Renderer->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
  os << indent << "Text Renderer: ";
  if (this->TextRenderer)
    {
    os << endl;
    this->TextRenderer->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
  os << indent << "MaximumMarkerCacheSize: "
     << this->MaximumMarkerCacheSize << endl;
  os << indent << "MarkerCache: " << this->MarkerCache.size()
     << " entries." << endl;
}
