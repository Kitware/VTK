/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOpenVRModel.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenVRModel.h"

#include "vtkObjectFactory.h"
#include "vtkOpenVRRenderWindow.h"
#include "vtkOpenVRCamera.h"
#include "vtkOpenGLVertexBufferObject.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkOpenGLIndexBufferObject.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkShaderProgram.h"
#include "vtkOpenGLHelper.h"
#include "vtkTextureObject.h"
#include "vtkRendererCollection.h"
#include "vtkMatrix4x4.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkInteractorObserver.h"

/*=========================================================================
vtkOpenVRRay
=========================================================================*/
class vtkOpenVRRay : public vtkObject
{
public:
  static vtkOpenVRRay *New();
  vtkTypeMacro(vtkOpenVRRay, vtkObject);

  bool Build(vtkOpenVRRenderWindow *win);
  void Render(vtkOpenVRRenderWindow *win,
    vtkMatrix4x4 *poseMatrix);

  // show the model
  void SetShow(bool v) {
    this->Show = v;
  };
  bool GetShow() {
    return this->Show;
  };

  void SetLength(float s) {
    this->Length = s;
  };

  void ReleaseGraphicsResources(vtkRenderWindow *win);

protected:
  vtkOpenVRRay();
  ~vtkOpenVRRay();

  bool Show;
  bool Loaded;

  vtkOpenGLHelper ModelHelper;
  vtkOpenGLVertexBufferObject *ModelVBO;
  vtkNew<vtkMatrix4x4> PoseMatrix;

  float Length;
};

vtkStandardNewMacro(vtkOpenVRRay);

vtkOpenVRRay::vtkOpenVRRay()
{
  this->Show = false;
  this->Length = 1.0f;
  this->Loaded = false;
  this->ModelVBO = vtkOpenGLVertexBufferObject::New();
};

vtkOpenVRRay::~vtkOpenVRRay()
{
  this->ModelVBO->Delete();
  this->ModelVBO = 0;
}

void vtkOpenVRRay::ReleaseGraphicsResources(vtkRenderWindow *win)
{
  this->ModelVBO->ReleaseGraphicsResources();
  this->ModelHelper.ReleaseGraphicsResources(win);
}

bool vtkOpenVRRay::Build(vtkOpenVRRenderWindow *win)
{
  //Ray geometry
  float vert[] = {
    0, 0, 0,
    0, 0, -1};
  unsigned short ind[] = {0, 1};

  this->ModelVBO->Upload(
    vert,
    2 * 3,
    vtkOpenGLBufferObject::ArrayBuffer);
  this->ModelHelper.IBO->Upload(
    ind,
    1 * 2,
    vtkOpenGLBufferObject::ElementArrayBuffer);
  this->ModelHelper.IBO->IndexCount = 1 * 2;

  this->ModelHelper.Program = win->GetShaderCache()->ReadyShaderProgram(

    // vertex shader
    "//VTK::System::Dec\n"
    "uniform mat4 matrix;\n"
    "uniform float scale;\n"
    "attribute vec3 position;\n"
    "void main()\n"
    "{\n"
    " gl_Position =  matrix * vec4(scale * position, 1);\n"
    "}\n",

    //fragment shader
    "//VTK::System::Dec\n"
    "//VTK::Output::Dec\n"
    "void main()\n"
    "{\n"
    "   gl_FragData[0] = vec4(1,0,0,1);\n"
    "}\n",

    // geom shader
    ""
    );
  vtkShaderProgram *program = this->ModelHelper.Program;
  this->ModelHelper.VAO->Bind();
  if (!this->ModelHelper.VAO->AddAttributeArray(program,
    this->ModelVBO,
    "position",
    0,
    3 * sizeof(float),
    VTK_FLOAT, 3, false))
  {
    vtkErrorMacro(<< "Error setting position in shader VAO.");
  }

  return true;
}

void vtkOpenVRRay::Render(
  vtkOpenVRRenderWindow *win,
  vtkMatrix4x4 *poseMatrix)
{
  //Load ray
  if (!this->Loaded)
  {
    if (!this->Build(win))
    {
      vtkErrorMacro("Unable to build controller ray ");
    }
    this->Loaded = true;
  }

  // Render ray
  win->GetShaderCache()->ReadyShaderProgram(this->ModelHelper.Program);
  this->ModelHelper.VAO->Bind();
  this->ModelHelper.IBO->Bind();

  vtkRenderer *ren = static_cast< vtkRenderer * >(
    win->GetRenderers()->GetItemAsObject(0));
  if (!ren)
  {
    vtkErrorMacro("Unable get renderer");
    return;
  }

  double unitV[4] = {0, 0, 0, 1};
  double scaleFactor =
    vtkMath::Norm( poseMatrix->MultiplyDoublePoint(unitV));

  this->ModelHelper.Program->SetUniformf("scale",
    this->Length / scaleFactor);

  this->ModelHelper.Program->SetUniformMatrix("matrix",
    poseMatrix);

  glDrawElements(GL_LINES,
    static_cast<GLsizei>(this->ModelHelper.IBO->IndexCount),
    GL_UNSIGNED_SHORT, 0);
}

/*=========================================================================
vtkOpenVRModel
=========================================================================*/
vtkStandardNewMacro(vtkOpenVRModel);

vtkOpenVRModel::vtkOpenVRModel()
{
  this->RawModel = NULL;
  this->RawTexture = NULL;
  this->Loaded = false;
  this->ModelVBO = vtkOpenGLVertexBufferObject::New();
  this->FailedToLoad = false;
};

vtkOpenVRModel::~vtkOpenVRModel()
{
  this->ModelVBO->Delete();
  this->ModelVBO = 0;
}

void vtkOpenVRModel::ReleaseGraphicsResources(vtkRenderWindow *win)
{
  this->ModelVBO->ReleaseGraphicsResources();
  this->ModelHelper.ReleaseGraphicsResources(win);
  this->TextureObject->ReleaseGraphicsResources(win);
}

bool vtkOpenVRModel::Build(vtkOpenVRRenderWindow *win)
{
  this->ModelVBO->Upload(
    this->RawModel->rVertexData,
    this->RawModel->unVertexCount,
    vtkOpenGLBufferObject::ArrayBuffer);

  this->ModelHelper.IBO->Upload(
    this->RawModel->rIndexData,
    this->RawModel->unTriangleCount * 3,
    vtkOpenGLBufferObject::ElementArrayBuffer);
  this->ModelHelper.IBO->IndexCount = this->RawModel->unTriangleCount * 3;

  this->ModelHelper.Program = win->GetShaderCache()->ReadyShaderProgram(

    // vertex shader -- use normals?? yes?
    "//VTK::System::Dec\n"
    "uniform mat4 matrix;\n"
    "attribute vec4 position;\n"
    //    "attribute vec3 v3NormalIn;\n"
    "attribute vec2 v2TexCoordsIn;\n"
    "out vec2 v2TexCoord;\n"
    "void main()\n"
    "{\n"
    " v2TexCoord = v2TexCoordsIn;\n"
    " gl_Position = matrix * vec4(position.xyz, 1);\n"
    "}\n",

    //fragment shader
    "//VTK::System::Dec\n"
    "//VTK::Output::Dec\n"
    "uniform sampler2D diffuse;\n"
    "in vec2 v2TexCoord;\n"
    "out vec4 outputColor;\n"
    "void main()\n"
    "{\n"
    "  gl_FragData[0] = texture(diffuse, v2TexCoord);\n"
    "}\n",

    // geom shader
    ""
    );

  vtkShaderProgram *program = this->ModelHelper.Program;
  this->ModelHelper.VAO->Bind();
  if (!this->ModelHelper.VAO->AddAttributeArray(program,
    this->ModelVBO,
    "position",
    offsetof(vr::RenderModel_Vertex_t, vPosition),
    sizeof(vr::RenderModel_Vertex_t),
    VTK_FLOAT, 3, false))
  {
    vtkErrorMacro(<< "Error setting position in shader VAO.");
  }
  if (!this->ModelHelper.VAO->AddAttributeArray(program,
    this->ModelVBO,
    "v2TexCoordsIn",
    offsetof(vr::RenderModel_Vertex_t, rfTextureCoord),
    sizeof(vr::RenderModel_Vertex_t),
    VTK_FLOAT, 2, false))
  {
    vtkErrorMacro(<< "Error setting tcoords in shader VAO.");
  }

  // create and populate the texture
  this->TextureObject->SetContext(win);
  this->TextureObject->Create2DFromRaw(
    this->RawTexture->unWidth, this->RawTexture->unHeight,
    4, VTK_UNSIGNED_CHAR,
    const_cast<void *>(static_cast<const void *const>(
      this->RawTexture->rubTextureMapData)));
  this->TextureObject->SetWrapS(vtkTextureObject::ClampToEdge);
  this->TextureObject->SetWrapT(vtkTextureObject::ClampToEdge);

  this->TextureObject->SetMinificationFilter(
    vtkTextureObject::LinearMipmapLinear);
  this->TextureObject->SetGenerateMipmap(true);

  return true;
}

void vtkOpenVRModel::Render(
  vtkOpenVRRenderWindow *win,
  const vr::TrackedDevicePose_t &pose)
{
  if (this->FailedToLoad)
  {
    return;
  }

  // do we not have the model loaded? Keep trying it is async
  if (!this->RawModel)
  {
    // start loading the model if we didn't find one
    vr::EVRRenderModelError result = vr::VRRenderModels()->LoadRenderModel_Async(
      this->GetName().c_str(), &this->RawModel);
    if (result
        > vr::EVRRenderModelError::VRRenderModelError_Loading)
    {
      this->FailedToLoad = true;
      if (result != vr::VRRenderModelError_NotEnoughTexCoords)
      {
        vtkErrorMacro("Unable to load render model " <<
          this->GetName() << " with error code " << result);
      }
      return; // move on to the next tracked device
    }
  }

  // if we have the model try loading the texture
  if (this->RawModel && !this->RawTexture)
  {
    if (vr::VRRenderModels()->LoadTexture_Async(
      this->RawModel->diffuseTextureId, &this->RawTexture)
        > vr::EVRRenderModelError::VRRenderModelError_Loading)
    {
      vtkErrorMacro(<< "Unable to load render texture for render model "
        << this->GetName());
    }

    // if this call succeeded and we have the texture
    // then build the VTK structures
    if (this->RawTexture)
    {
      if (!this->Build(win))
      {
        vtkErrorMacro("Unable to create GL model from render model "
          << this->GetName());
      }
      vr::VRRenderModels()->FreeRenderModel(this->RawModel);
      vr::VRRenderModels()->FreeTexture(this->RawTexture);
      this->Loaded = true;
    }
  }

  if (this->Loaded)
  {
    // render the model
    win->GetShaderCache()->ReadyShaderProgram(this->ModelHelper.Program);
    this->ModelHelper.VAO->Bind();
    this->ModelHelper.IBO->Bind();

    this->TextureObject->Activate();
    this->ModelHelper.Program->SetUniformi("diffuse",
      this->TextureObject->GetTextureUnit());

    vtkRenderer *ren = static_cast<vtkRenderer *>(
      win->GetRenderers()->GetItemAsObject(0));
    if (ren)
    {
      vtkOpenVRCamera *cam =
        static_cast<vtkOpenVRCamera *>(ren->GetActiveCamera());

      double elems[16];
      for (int j = 0; j < 3; j++)
      {
        for (int i = 0; i < 4; i++)
        {
          elems[j + i * 4] = pose.mDeviceToAbsoluteTracking.m[j][i];
        }
      }
      elems[3] = 0.0;
      elems[7] = 0.0;
      elems[11] = 0.0;
      elems[15] = 1.0;

      vtkMatrix4x4* tcdc;
      cam->GetTrackingToDCMatrix(tcdc);

      vtkMatrix4x4::Multiply4x4(
        elems,
        (double *)(tcdc->Element),
        (double *)(this->PoseMatrix->Element));

      this->ModelHelper.Program->SetUniformMatrix("matrix",
        this->PoseMatrix.Get());
    }

    glDrawElements(GL_TRIANGLES,
      static_cast<GLsizei>(this->ModelHelper.IBO->IndexCount),
      GL_UNSIGNED_SHORT, 0);
    this->TextureObject->Deactivate();

    //Handle drawing of the ray associated to this model
    if (!win->GetInteractor())
    {
      vtkErrorMacro("Unable to get interactor");
      return;
    }
    if (!win->GetInteractor()->GetInteractorStyle())
    {
      vtkErrorMacro("Unable to get interactor style");
      return;
    }
    //Update ray points and draw state
    win->GetInteractor()->GetInteractorStyle()->InvokeEvent(
      vtkCommand::RenderEvent);
    //Draw ray
    if (this->Ray->GetShow())
    {
      this->Ray->Render(win, this->PoseMatrix.Get());
    }
  }
}

void vtkOpenVRModel::SetShowRay(bool v)
{
  this->Ray->SetShow(v);
}

void vtkOpenVRModel::SetRayLength(double length)
{
  this->Ray->SetLength(length);
}
