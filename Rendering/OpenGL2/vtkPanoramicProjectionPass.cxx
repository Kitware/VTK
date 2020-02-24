/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPanoramicProjectionPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPanoramicProjectionPass.h"

#include "vtkCamera.h"
#include "vtkCullerCollection.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLQuadHelper.h"
#include "vtkOpenGLRenderUtilities.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkPerspectiveTransform.h"
#include "vtkRenderState.h"
#include "vtkRenderer.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"
#include "vtkTransform.h"

#include <sstream>

vtkStandardNewMacro(vtkPanoramicProjectionPass);

// ----------------------------------------------------------------------------
void vtkPanoramicProjectionPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "CubeResolution: " << this->CubeResolution << "\n";
  os << indent << "ProjectionType: ";
  switch (this->ProjectionType)
  {
    case Equirectangular:
      os << "Equirectangular\n";
      break;
    case Azimuthal:
      os << "Azimuthal\n";
      break;
    default:
      os << "Unknown\n";
  }
  os << indent << "Angle: " << this->Angle << "\n";
}

// ----------------------------------------------------------------------------
void vtkPanoramicProjectionPass::Render(const vtkRenderState* s)
{
  vtkOpenGLClearErrorMacro();

  this->NumberOfRenderedProps = 0;

  vtkRenderer* r = s->GetRenderer();
  vtkOpenGLRenderWindow* renWin = static_cast<vtkOpenGLRenderWindow*>(r->GetRenderWindow());
  vtkOpenGLState* ostate = renWin->GetState();

  vtkOpenGLState::ScopedglEnableDisable bsaver(ostate, GL_BLEND);
  vtkOpenGLState::ScopedglEnableDisable dsaver(ostate, GL_DEPTH_TEST);

  if (this->DelegatePass == nullptr)
  {
    vtkWarningMacro("no delegate in vtkPanoramicProjectionPass.");
    return;
  }

  int x, y, w, h;
  r->GetTiledSizeAndOrigin(&w, &h, &x, &y);

  // create FBO and cubemap
  this->InitOpenGLResources(renWin);

  ostate->vtkglViewport(0, 0, this->CubeResolution, this->CubeResolution);
  ostate->vtkglScissor(0, 0, this->CubeResolution, this->CubeResolution);

  // set property in order to preserve viewport in for volume rendering
  this->PreRender(s);

  // render all direction into a cubemap face
  for (int faceIndex = GL_TEXTURE_CUBE_MAP_POSITIVE_X; faceIndex <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
       faceIndex++)
  {
    this->RenderOnFace(s, faceIndex);
  }

  this->PostRender(s);

  ostate->vtkglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  ostate->vtkglDisable(GL_BLEND);
  ostate->vtkglDisable(GL_DEPTH_TEST);
  ostate->vtkglDisable(GL_SCISSOR_TEST);
  ostate->vtkglViewport(x, y, w, h);
  ostate->vtkglScissor(x, y, w, h);

  this->Project(renWin);

  vtkOpenGLCheckErrorMacro("failed after Render");
}

// ----------------------------------------------------------------------------
void vtkPanoramicProjectionPass::InitOpenGLResources(vtkOpenGLRenderWindow* renWin)
{
  if (this->CubeMapTexture && this->CubeMapTexture->GetMTime() < this->MTime)
  {
    this->CubeMapTexture->Delete();
    this->CubeMapTexture = nullptr;
  }

  if (this->CubeMapTexture == nullptr)
  {
    // the cubemap is used to render the complete scene
    // linear interpolation give better results at lower resolutions
    // wrap mode must be clamped to avoid artifacts on seams
    // alpha channel is also mandatory for remote rendering
    this->CubeMapTexture = vtkTextureObject::New();
    this->CubeMapTexture->SetContext(renWin);
    if (this->Interpolate)
    {
      this->CubeMapTexture->SetMinificationFilter(vtkTextureObject::Linear);
      this->CubeMapTexture->SetMagnificationFilter(vtkTextureObject::Linear);
    }
    this->CubeMapTexture->SetWrapS(vtkTextureObject::ClampToEdge);
    this->CubeMapTexture->SetWrapT(vtkTextureObject::ClampToEdge);
    this->CubeMapTexture->SetWrapR(vtkTextureObject::ClampToEdge);
    this->CubeMapTexture->CreateCubeFromRaw(
      this->CubeResolution, this->CubeResolution, 4, VTK_UNSIGNED_CHAR, nullptr);
  }

  if (this->FrameBufferObject && this->FrameBufferObject->GetMTime() < this->MTime)
  {
    this->FrameBufferObject->Delete();
    this->FrameBufferObject = nullptr;
  }

  if (this->FrameBufferObject == nullptr)
  {
    this->FrameBufferObject = vtkOpenGLFramebufferObject::New();
  }

  if (!this->FrameBufferObject->GetFBOIndex())
  {
    this->FrameBufferObject->SetContext(renWin);
    renWin->GetState()->PushFramebufferBindings();
    this->FrameBufferObject->Bind();
    this->FrameBufferObject->Resize(this->CubeResolution, this->CubeResolution);
    this->FrameBufferObject->AddDepthAttachment();
    renWin->GetState()->PopFramebufferBindings();
  }
}

// ----------------------------------------------------------------------------
void vtkPanoramicProjectionPass::Project(vtkOpenGLRenderWindow* renWin)
{
  if (this->QuadHelper && this->MTime > this->QuadHelper->ShaderChangeValue)
  {
    delete this->QuadHelper;
    this->QuadHelper = nullptr;
  }

  if (!this->QuadHelper)
  {
    std::string FSSource = vtkOpenGLRenderUtilities::GetFullScreenQuadFragmentShaderTemplate();

    vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Decl",
      "uniform samplerCube source;\n"
      "uniform float angle;\n"
      "uniform vec2 scale;\n"
      "uniform vec2 shift;\n\n");

    std::stringstream ss;

    // in case of tile rendering, we need to scale and shift coords
    ss << "  float x = texCoord.x * scale.x + shift.x;\n"
          "  float y = texCoord.y * scale.y + shift.y;\n";

    switch (this->ProjectionType)
    {
      case Equirectangular:
        ss << "  const float pi = 3.14159265359;\n"
              "  float phi = y * pi;\n"
              "  float theta = angle * x + (pi - 0.5 * angle);\n"
              "  vec3 dir = vec3(-sin(phi)*sin(theta), cos(phi), -sin(phi)*cos(theta));\n"
              "  gl_FragData[0] = texture(source, dir);\n";
        break;
      case Azimuthal:
        ss << "  vec2 v = 2.0 * vec2(x - 0.5, 0.5 - y);\n"
              "  float phi = length(v);\n"
              "  if (phi <= 1.0)\n"
              "  {\n"
              "    phi *= 0.5 * angle;\n"
              "    float theta = atan(v.y, v.x);\n"
              "    vec3 dir = vec3(sin(phi)*cos(theta), sin(theta)*sin(phi), cos(phi));\n"
              "    gl_FragData[0] = texture(source, dir);\n"
              "  }\n"
              "  else\n"
              "  {\n"
              "    gl_FragData[0] = vec4(0.0, 0.0, 0.0, 1.0);\n"
              "  }\n";
        break;
      default:
        vtkErrorMacro("Projection type unknown");
        break;
    }

    vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Impl", ss.str());

    this->QuadHelper = new vtkOpenGLQuadHelper(renWin,
      vtkOpenGLRenderUtilities::GetFullScreenQuadVertexShader().c_str(), FSSource.c_str(), "");

    this->QuadHelper->ShaderChangeValue = this->MTime;
  }
  else
  {
    renWin->GetShaderCache()->ReadyShaderProgram(this->QuadHelper->Program);
  }

  if (!this->QuadHelper->Program || !this->QuadHelper->Program->GetCompiled())
  {
    vtkErrorMacro("Couldn't build the shader program.");
    return;
  }

  this->CubeMapTexture->Activate();
  this->QuadHelper->Program->SetUniformi("source", this->CubeMapTexture->GetTextureUnit());
  this->QuadHelper->Program->SetUniformf("angle", vtkMath::RadiansFromDegrees(this->Angle));

  double viewport[4];
  renWin->GetTileViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
  float scale[2] = { static_cast<float>(viewport[2] - viewport[0]),
    static_cast<float>(viewport[3] - viewport[1]) };
  float shift[2] = { static_cast<float>(viewport[0]), static_cast<float>(viewport[1]) };

  this->QuadHelper->Program->SetUniform2f("scale", scale);
  this->QuadHelper->Program->SetUniform2f("shift", shift);

#ifndef GL_ES_VERSION_3_0
  vtkOpenGLState* ostate = renWin->GetState();
  ostate->vtkglEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
#endif

  this->QuadHelper->Render();

  this->CubeMapTexture->Deactivate();
}

// ----------------------------------------------------------------------------
void vtkPanoramicProjectionPass::RenderOnFace(const vtkRenderState* s, int faceIndex)
{
  // We can cull the back face is angle is inferior to 2 * (pi - atan(sqrt(2))) radians
  const double cullBackFaceAngle = 250.528779;

  if (faceIndex == GL_TEXTURE_CUBE_MAP_NEGATIVE_Z && this->Angle <= cullBackFaceAngle)
  {
    return;
  }

  vtkOpenGLRenderer* r = vtkOpenGLRenderer::SafeDownCast(s->GetRenderer());
  vtkRenderState s2(r);
  s2.SetPropArrayAndCount(s->GetPropArray(), s->GetPropArrayCount());

  // Adapt camera to square rendering
  vtkSmartPointer<vtkCamera> oldCamera = r->GetActiveCamera();
  vtkNew<vtkCamera> newCamera;
  r->SetActiveCamera(newCamera);

  newCamera->SetPosition(oldCamera->GetPosition());
  newCamera->SetFocalPoint(oldCamera->GetFocalPoint());
  newCamera->SetViewUp(oldCamera->GetViewUp());
  newCamera->SetViewAngle(90.0);
  newCamera->OrthogonalizeViewUp();

  if (r->GetRenderWindow()->GetStereoRender())
  {
    double right[3];
    double pos[3];
    double sign = oldCamera->GetLeftEye() ? -1.0 : 1.0;
    vtkMath::Cross(newCamera->GetDirectionOfProjection(), newCamera->GetViewUp(), right);
    newCamera->GetPosition(pos);
    double sep = oldCamera->GetEyeSeparation();
    pos[0] += sign * sep * right[0];
    pos[1] += sign * sep * right[1];
    pos[2] += sign * sep * right[2];
    newCamera->SetPosition(pos[0], pos[1], pos[2]);
  }

  // lights should not be rotated with camera, so we use an inverse transform for lights
  vtkNew<vtkTransform> lightsTransform;

  switch (faceIndex)
  {
    case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
      newCamera->Yaw(-90);
      lightsTransform->RotateY(90);
      break;
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
      newCamera->Yaw(90);
      lightsTransform->RotateY(-90);
      break;
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
      newCamera->Pitch(-90);
      lightsTransform->RotateX(90);
      break;
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
      newCamera->Pitch(90);
      lightsTransform->RotateX(-90);
      break;
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
      break;
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
      newCamera->Yaw(180);
      lightsTransform->RotateY(180);
      break;
    default:
      break;
  }

  double range[2];
  oldCamera->GetClippingRange(range);
  newCamera->SetClippingRange(range);
  vtkNew<vtkPerspectiveTransform> perspectiveTransform;

  // the fov is 90 degree in each direction, the frustum can be simplified
  // xmin and ymin are -near and xmax and ymax are +near
  perspectiveTransform->Frustum(-range[0], range[0], -range[0], range[0], range[0], range[1]);

  newCamera->UseExplicitProjectionTransformMatrixOn();
  newCamera->SetExplicitProjectionTransformMatrix(perspectiveTransform->GetMatrix());

  s2.SetFrameBuffer(this->FrameBufferObject);

  this->FrameBufferObject->GetContext()->GetState()->PushFramebufferBindings();
  this->FrameBufferObject->Bind();
  this->FrameBufferObject->AddColorAttachment(0, this->CubeMapTexture, 0, faceIndex);
  this->FrameBufferObject->ActivateBuffer(0);

  this->FrameBufferObject->Start(this->CubeResolution, this->CubeResolution);

  r->SetUserLightTransform(lightsTransform);

  this->DelegatePass->Render(&s2);
  this->NumberOfRenderedProps += this->DelegatePass->GetNumberOfRenderedProps();

  r->SetUserLightTransform(nullptr);

  this->FrameBufferObject->RemoveColorAttachment(0);
  this->FrameBufferObject->GetContext()->GetState()->PopFramebufferBindings();

  r->SetActiveCamera(oldCamera);
}

// ----------------------------------------------------------------------------
void vtkPanoramicProjectionPass::ReleaseGraphicsResources(vtkWindow* w)
{
  this->Superclass::ReleaseGraphicsResources(w);

  delete this->QuadHelper;
  this->QuadHelper = nullptr;

  if (this->FrameBufferObject)
  {
    this->FrameBufferObject->Delete();
    this->FrameBufferObject = nullptr;
  }
  if (this->CubeMapTexture)
  {
    this->CubeMapTexture->Delete();
    this->CubeMapTexture = nullptr;
  }
}
