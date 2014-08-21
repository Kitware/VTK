/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLGlyph3DHelper.h"

#include "vtkglVBOHelper.h"

#include "vtkCamera.h"
#include "vtkTransform.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"
#include "vtkProperty.h"
#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"

#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"

#include "vtkHardwareSelector.h"

using vtkgl::replace;

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLGlyph3DHelper)

//-----------------------------------------------------------------------------
vtkOpenGLGlyph3DHelper::vtkOpenGLGlyph3DHelper()
  : ModelTransformMatrix(NULL)
{
}


//-----------------------------------------------------------------------------
vtkOpenGLGlyph3DHelper::~vtkOpenGLGlyph3DHelper()
{
}


void vtkOpenGLGlyph3DHelper::GlyphRender(vtkRenderer* ren, vtkActor* actor, unsigned char rgba[4], vtkMatrix4x4 *gmat, int stage)
{
  // handle starting up
  if (stage == 1)
    {
    this->RenderPieceStart(ren,actor);
    this->UpdateShader(this->Tris, ren, actor);
    this->Tris.ibo.Bind();
    return;
    }

  // handle ending
  if (stage == 3)
    {
    this->Tris.ibo.Release();
    this->RenderPieceFinish(ren,actor);
    return;
    }

  // handle the middle
  vtkgl::ShaderProgram &program = this->Tris.CachedProgram->Program;
  vtkgl::VBOLayout &layout = this->Layout;


  // these next four lines could be cached and passed in to save time
  vtkCamera *cam = ren->GetActiveCamera();
  vtkNew<vtkMatrix4x4> tmpMat;
  tmpMat->DeepCopy(cam->GetModelViewTransformMatrix());
  vtkMatrix4x4::Multiply4x4(tmpMat.Get(), actor->GetMatrix(), tmpMat.Get());

  // Apply this extra transform from things like the glyph mapper.
  vtkMatrix4x4::Multiply4x4(tmpMat.Get(), gmat, tmpMat.Get());

  tmpMat->Transpose();
  program.SetUniformMatrix("MCVCMatrix", tmpMat.Get());

  // for lit shaders set normal matrix
  if (this->LastLightComplexity > 0)
    {
    tmpMat->Transpose();

    // set the normal matrix and send it down
    // (make this a function in camera at some point returning a 3x3)
    // Reuse the matrix we already got (and possibly multiplied with model mat.
    vtkNew<vtkTransform> aTF;
    aTF->SetMatrix(tmpMat.Get());
    double *scale = aTF->GetScale();
    aTF->Scale(1.0 / scale[0], 1.0 / scale[1], 1.0 / scale[2]);
    tmpMat->DeepCopy(aTF->GetMatrix());
    vtkNew<vtkMatrix3x3> tmpMat3d;
    for(int i = 0; i < 3; ++i)
      {
      for (int j = 0; j < 3; ++j)
        {
        tmpMat3d->SetElement(i, j, tmpMat->GetElement(i, j));
        }
      }
    tmpMat3d->Invert();
    program.SetUniformMatrix("normalMatrix", tmpMat3d.Get());
    }

  // Query the actor for some of the properties that can be applied.
  float diffuseColor[3] = {rgba[0]/255.0f,rgba[1]/255.0f,rgba[2]/255.0f};
  float opacity = rgba[3]/255.0f;

  program.SetUniformf("opacityUniform", opacity);
  program.SetUniform3f("diffuseColorUniform", diffuseColor);

  vtkHardwareSelector* selector = ren->GetSelector();
  if (selector)
    {
    program.SetUniform3f("mapperIndex", selector->GetPropColorValue());
    float *fv = selector->GetPropColorValue();
    int iv = (int)(fv[0]*255) + (int)(fv[1]*255)*256;
    if (iv == 0)
      {
      abort();
      }
    }

  // First we do the triangles, update the shader, set uniforms, etc.
  if (actor->GetProperty()->GetRepresentation() == VTK_POINTS)
    {
    glDrawRangeElements(GL_POINTS, 0,
                        static_cast<GLuint>(layout.VertexCount - 1),
                        static_cast<GLsizei>(this->Tris.indexCount),
                        GL_UNSIGNED_INT,
                        reinterpret_cast<const GLvoid *>(NULL));
    }
  if (actor->GetProperty()->GetRepresentation() == VTK_WIREFRAME)
    {
    // TODO wireframe of triangles is not lit properly right now
    // you either have to generate normals and send them down
    // or use a geometry shader.
    glMultiDrawElements(GL_LINE_LOOP,
                      (GLsizei *)(&this->Tris.elementsArray[0]),
                      GL_UNSIGNED_INT,
                      reinterpret_cast<const GLvoid **>(&(this->Tris.offsetArray[0])),
                      (GLsizei)this->Tris.offsetArray.size());
    }
  if (actor->GetProperty()->GetRepresentation() == VTK_SURFACE)
    {
    glDrawRangeElements(GL_TRIANGLES, 0,
                        static_cast<GLuint>(layout.VertexCount - 1),
                        static_cast<GLsizei>(this->Tris.indexCount),
                        GL_UNSIGNED_INT,
                        reinterpret_cast<const GLvoid *>(NULL));
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLGlyph3DHelper::SetCameraShaderParameters(vtkgl::CellBO &cellBO,
                                                    vtkRenderer* ren, vtkActor *actor)
{
  // do the superclass and then reset a couple values
  this->Superclass::SetCameraShaderParameters(cellBO,ren,actor);

  vtkgl::ShaderProgram &program = cellBO.CachedProgram->Program;

  // set the MCWC matrix for positional lighting
  if (this->LastLightComplexity > 2)
    {
    program.SetUniformMatrix("MCWCMatrix", actor->GetMatrix());
    }

  vtkCamera *cam = ren->GetActiveCamera();

  vtkNew<vtkMatrix4x4> tmpMat;
  tmpMat->DeepCopy(cam->GetModelViewTransformMatrix());

  // compute the combined ModelView matrix and send it down to save time in the shader
  vtkMatrix4x4::Multiply4x4(tmpMat.Get(), actor->GetMatrix(), tmpMat.Get());

  // Apply this extra transform from the glyph mapper.
  if (this->ModelTransformMatrix)
    {
    vtkMatrix4x4::Multiply4x4(tmpMat.Get(), this->ModelTransformMatrix,
                              tmpMat.Get());
   }

  tmpMat->Transpose();
  program.SetUniformMatrix("MCVCMatrix", tmpMat.Get());

  // for lit shaders set normal matrix
  if (this->LastLightComplexity > 0)
    {
    tmpMat->Transpose();

    // set the normal matrix and send it down
    // (make this a function in camera at some point returning a 3x3)
    // Reuse the matrix we already got (and possibly multiplied with model mat.
    if (!actor->GetIsIdentity() || this->ModelTransformMatrix)
      {
      vtkNew<vtkTransform> aTF;
      aTF->SetMatrix(tmpMat.Get());
      double *scale = aTF->GetScale();
      aTF->Scale(1.0 / scale[0], 1.0 / scale[1], 1.0 / scale[2]);
      tmpMat->DeepCopy(aTF->GetMatrix());
      }
    vtkNew<vtkMatrix3x3> tmpMat3d;
    for(int i = 0; i < 3; ++i)
      {
      for (int j = 0; j < 3; ++j)
        {
        tmpMat3d->SetElement(i, j, tmpMat->GetElement(i, j));
        }
      }
    tmpMat3d->Invert();
    program.SetUniformMatrix("normalMatrix", tmpMat3d.Get());
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLGlyph3DHelper::SetPropertyShaderParameters(vtkgl::CellBO &cellBO,
                                                       vtkRenderer *ren, vtkActor *actor)
{
  // do the superclass and then reset a couple values
  this->Superclass::SetPropertyShaderParameters(cellBO,ren,actor);

  vtkgl::ShaderProgram &program = cellBO.CachedProgram->Program;

  // Override the model color when the value was set directly on the mapper.
  float diffuseColor[3];
  for (int i = 0; i < 3; ++i)
    {
    diffuseColor[i] = this->ModelColor[i]/255.0;
    }
  float opacity = this->ModelColor[3]/255.0;

  program.SetUniformf("opacityUniform", opacity);
  program.SetUniform3f("diffuseColorUniform", diffuseColor);
}


//-----------------------------------------------------------------------------
void vtkOpenGLGlyph3DHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
