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

#include <GL/glew.h>

#include "vtkActor2D.h"
#include "vtkCellArray.h"
#include "vtkMath.h"
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
#include <math.h>

#include "vtkglVBOHelper.h"

#include "vtkglBufferObject.h"
#include "vtkglShader.h"
#include "vtkglShaderProgram.h"

// Bring in our shader symbols.
#include "vtkglPolyData2DVS.h"
#include "vtkglPolyDataFS.h"

class vtkOpenGL2PolyDataMapper2D::Private
{
public:
  vtkgl::BufferObject vbo;
  vtkgl::VBOLayout layout;
  vtkgl::BufferObject lineIBO;
  vtkgl::BufferObject polyIBO;
  vtkgl::BufferObject pointIBO;
  vtkgl::BufferObject triStripIBO;
  size_t numberOfPoints;
  size_t numberOfIndices;
  std::vector<GLintptr> offsetArray;
  std::vector<unsigned int> elementsArray;

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
    this->vertexShader.setType(vtkgl::Shader::Vertex);
    this->fragmentShader.setType(vtkgl::Shader::Fragment);
  }
};

vtkStandardNewMacro(vtkOpenGL2PolyDataMapper2D);

//-----------------------------------------------------------------------------
vtkOpenGL2PolyDataMapper2D::vtkOpenGL2PolyDataMapper2D()
  : Internal(new Private)
{
}


//-----------------------------------------------------------------------------
void vtkOpenGL2PolyDataMapper2D::UpdateShader(vtkViewport* vtkNotUsed(viewport), vtkActor2D *vtkNotUsed(actor))
{
  this->Internal->fragmentShaderFile = vtkglPolyDataFS;
  this->Internal->vertexShaderFile = vtkglPolyData2DVS;

  // compile and link the shader program if it has changed
  // eventually use some sort of caching here
  if (this->Internal->vertexShader.type() == vtkgl::Shader::Unknown ||
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

    this->Internal->vertexShader.setSource(vertexShaderSource);
    this->Internal->fragmentShader.setSource(this->Internal->fragmentShaderFile);
    if (!this->Internal->vertexShader.compile())
      {
      vtkErrorMacro(<< this->Internal->vertexShader.error());
      }
    if (!this->Internal->fragmentShader.compile())
      {
      vtkErrorMacro(<< this->Internal->fragmentShader.error());
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

//-------------------------------------------------------------------------
void vtkOpenGL2PolyDataMapper2D::UpdateVBO(vtkActor2D *vtkNotUsed(act))
{
  vtkPolyData *poly = this->GetInput();
  if (poly == NULL)
    {
    return;
    }

  // Mark our properties as updated.
  this->Internal->propertiesTime.Modified();

  // Due to the requirement to use derived classes rather than typedefs for
  // the vtkVector types, it is simpler to add a few convenience typedefs here
  // than use the classes which are then harder for the compiler to interpret.
  typedef vtkVector<float,  3> Vector3f;
  typedef vtkVector<double, 3> Vector3d;

/*
  aPrim = input->GetVerts();
  glBegin(GL_POINTS);
  for (aPrim->InitTraversal(); aPrim->GetNextCell(npts,pts); cellNum++)
    {
    for (j = 0; j < npts; j++)
      {
      if (c)
        {
        if (cellScalars)
          {
          rgba = c->GetPointer(4*cellNum);
          }
        else
          {
          rgba = c->GetPointer(4*pts[j]);
          }
        glColor4ubv(rgba);
        }
      if (t)
        {
        glTexCoord2dv(t->GetTuple(pts[j]));
        }
      // this is done to work around an OpenGL bug, otherwise we could just
      // call glVertex2dv
      dptr = p->GetPoint(pts[j]);
      glVertex3d(dptr[0],dptr[1],0);
      }
    }
  glEnd();
*/

/*
  aPrim = input->GetStrips();
  for (aPrim->InitTraversal(); aPrim->GetNextCell(npts,pts); cellNum++)
    {
    glBegin(GL_TRIANGLE_STRIP);
    for (j = 0; j < npts; j++)
      {
      if (c)
        {
        if (cellScalars)
          {
          rgba = c->GetPointer(4*cellNum);
          }
        else
          {
          rgba = c->GetPointer(4*pts[j]);
          }
        glColor4ubv(rgba);
        }
      if (t)
        {
        glTexCoord2dv(t->GetTuple(pts[j]));
        }
      // this is done to work around an OpenGL bug, otherwise we could just
      // call glVertex2dv
      dptr = p->GetPoint(pts[j]);
      glVertex3d(dptr[0],dptr[1],0);
      }
    glEnd();
    }

  aPrim = input->GetPolys();
  for (aPrim->InitTraversal(); aPrim->GetNextCell(npts,pts); cellNum++)
    {
    glBegin(GL_POLYGON);
    for (j = 0; j < npts; j++)
      {
      if (c)
        {
        if (cellScalars)
          {
          rgba = c->GetPointer(4*cellNum);
          }
        else
          {
          rgba = c->GetPointer(4*pts[j]);
          }
        glColor4ubv(rgba);
        }
      if (t)
        {
        glTexCoord2dv(t->GetTuple(pts[j]));
        }
      // this is done to work around an OpenGL bug, otherwise we could just
      // call glVertex2dv
      dptr = p->GetPoint(pts[j]);
      glVertex3d(dptr[0],dptr[1],0);
      }
    glEnd();
    }
*/

  // Iterate through all of the different types in the polydata, building VBOs
  // and IBOs as appropriate for each type.
  vtkPoints* p = poly->GetPoints();
  switch(p->GetDataType())
    {
    vtkTemplateMacro(
      this->Internal->layout =
        CreateTriangleVBO(static_cast<VTK_TT*>(p->GetVoidPointer(0)),
                          static_cast<VTK_TT*>(NULL),
                          p->GetNumberOfPoints(),
                          this->Internal->colorComponents ? &this->Internal->colors[0] : NULL,
                          this->Internal->colorComponents,
                          this->Internal->vbo));
    }

  vtkIdType      *pts = 0;
  vtkIdType      npts = 0;
  int            cellNum = 0;
  vtkCellArray* lines = poly->GetLines();
  std::vector<unsigned int> indexArray;
  unsigned int count = 0;
  indexArray.reserve(lines->GetNumberOfCells() * 3);
  for (lines->InitTraversal(); lines->GetNextCell(npts,pts); cellNum++)
    {
    this->Internal->offsetArray.push_back(count);
    this->Internal->elementsArray.push_back(npts);
    for (int j = 0; j < npts; ++j)
      {
      indexArray.push_back(static_cast<unsigned int>(pts[j]));
      count++;
      }
    }
  this->Internal->lineIBO.upload(indexArray, vtkgl::BufferObject::ElementArrayBuffer);
  this->Internal->numberOfIndices = indexArray.size();

  this->Internal->numberOfPoints = CreateIndexBuffer(poly->GetVerts(),
                                                    this->Internal->pointIBO, 1);
}


void vtkOpenGL2PolyDataMapper2D::RenderOverlay(vtkViewport* viewport,
                                              vtkActor2D* actor)
{
  vtkOpenGLClearErrorMacro();
  int            numPts;
  vtkPolyData    *input=static_cast<vtkPolyData *>(this->GetInput());
  int            j;
  vtkPoints      *p, *displayPts;
  unsigned char  color[4];
  vtkPlaneCollection *clipPlanes;
  vtkPlane           *plane;
  int                 i,numClipPlanes;
  double              planeEquation[4];
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

  // Set up the font color from the text actor
  double* actorColor = actor->GetProperty()->GetColor();
  color[0] = static_cast<unsigned char>(actorColor[0] * 255.0);
  color[1] = static_cast<unsigned char>(actorColor[1] * 255.0);
  color[2] = static_cast<unsigned char>(actorColor[2] * 255.0);
  color[3] = static_cast<unsigned char>(
    255.0*actor->GetProperty()->GetOpacity());

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

  if (actor->GetProperty()->GetDisplayLocation() ==
       VTK_FOREGROUND_LOCATION)
    {
    glOrtho(left, right, bottom, top, 0, 1);
    }
  else
    {
    glOrtho(left, right, bottom, top, -1, 0);
    }

  // Clipping plane stuff
  clipPlanes = this->ClippingPlanes;

  if (clipPlanes == NULL)
    {
    numClipPlanes = 0;
    }
  else
    {
    numClipPlanes = clipPlanes->GetNumberOfItems();
    if (numClipPlanes > 4)
      {
      vtkErrorMacro(<< "Only 4 clipping planes are used with 2D mappers");
      }
    }

  for (i = 0; i < numClipPlanes; i++)
    {
    glEnable(static_cast<GLenum>(GL_CLIP_PLANE0+i));
    }

  for (i = 0; i < numClipPlanes; i++)
    {
    plane = static_cast<vtkPlane *>(clipPlanes->GetItemAsObject(i));

    planeEquation[0] = plane->GetNormal()[0];
    planeEquation[1] = plane->GetNormal()[1];
    planeEquation[2] = plane->GetNormal()[2];
    planeEquation[3] = -(planeEquation[0]*plane->GetOrigin()[0]+
                         planeEquation[1]*plane->GetOrigin()[1]+
                         planeEquation[2]*plane->GetOrigin()[2]);
    glClipPlane(static_cast<GLenum>(GL_CLIP_PLANE0+i),planeEquation);
    }

  // Set the PointSize
  glPointSize(actor->GetProperty()->GetPointSize());
  vtkOpenGLGL2PSHelper::SetPointSize(actor->GetProperty()->GetPointSize());

  if ( this->TransformCoordinate )
    {
    p->Delete();
    }

  for (i = 0; i < numClipPlanes; i++)
    {
    glDisable(static_cast<GLenum>(GL_CLIP_PLANE0+i));
    }

  // Turn it back on in case we've turned it off
  glDepthMask(GL_TRUE);
  glDisable(GL_TEXTURE_2D);

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

  //this->SetPropertyShaderParameters(ren, actor);

  this->Internal->vbo.bind();

  size_t stride = sizeof(float) * 3;
  if (this->Internal->colorAttributes)
    {
    stride += sizeof(float);
    }

  this->Internal->program.enableAttributeArray("vertexMC");
  this->Internal->program.useAttributeArray("vertexMC", 0,
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
                                              this->Internal->colorComponents,
                                              vtkgl::ShaderProgram::Normalize);
    }

  if (this->Internal->offsetArray.size() > 0)
    {
    this->Internal->lineIBO.bind();
    glMultiDrawElements(GL_LINE_STRIP,
                        (GLsizei *)(&this->Internal->elementsArray[0]),
                        GL_UNSIGNED_INT,
                        reinterpret_cast<const GLvoid **>(&(this->Internal->offsetArray[0])),
                        this->Internal->offsetArray.size());
    this->Internal->lineIBO.release();
    }

  if (this->Internal->numberOfPoints)
    {
    this->Internal->pointIBO.bind();
    glDrawRangeElements(GL_POINTS, 0,
                        static_cast<GLuint>(this->Internal->layout.VertexCount - 1),
                        static_cast<GLsizei>(this->Internal->numberOfPoints),
                        GL_UNSIGNED_INT,
                        reinterpret_cast<const GLvoid *>(NULL));
    this->Internal->pointIBO.release();
    }


  this->Internal->vbo.release();
  this->Internal->program.disableAttributeArray("vertexMC");
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
