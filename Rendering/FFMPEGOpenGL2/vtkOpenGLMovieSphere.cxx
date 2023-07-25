// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOpenGLMovieSphere.h"

#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkFFMPEGVideoSource.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLShaderProperty.h"
#include "vtkOpenGLState.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkShaderProgram.h"
#include "vtkTexture.h"
#include "vtkTextureObject.h"

#include <cmath>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOpenGLMovieSphere);

vtkOpenGLMovieSphere::vtkOpenGLMovieSphere()
{
  for (int i = 0; i < 6; ++i)
  {
    this->TextureData[i] = nullptr;
  }
  this->ReadIndex = 0;
  this->WriteIndex = 3;
  this->BuildIndex = 0;
  this->DrawIndex = 3;
  this->Height = 0;
  this->Width = 0;
  this->NewData = 0;
  this->HaveData = 0;

  this->CubeMapper->RemoveAllObservers();
  this->CubeMapper->AddObserver(
    vtkCommand::UpdateShaderEvent, this, &vtkOpenGLMovieSphere::UpdateUniforms);
}

vtkOpenGLMovieSphere::~vtkOpenGLMovieSphere()
{
  if (this->VideoSource)
  {
    this->VideoSource->SetVideoCallback(nullptr, nullptr);
  }
  for (int i = 0; i < 6; ++i)
  {
    delete[] this->TextureData[i];
  }
}

void vtkOpenGLMovieSphere::UpdateUniforms(vtkObject* a, unsigned long eid, void* calldata)
{
  this->Superclass::UpdateUniforms(a, eid, calldata);
  vtkShaderProgram* program = reinterpret_cast<vtkShaderProgram*>(calldata);

  program->SetUniformi("YTexture", this->YTexture);
  program->SetUniformi("UTexture", this->UTexture);
  program->SetUniformi("VTexture", this->VTexture);
}

vtkFFMPEGVideoSource* vtkOpenGLMovieSphere::GetVideoSource()
{
  return this->VideoSource.Get();
}

void vtkOpenGLMovieSphere::SetVideoSource(vtkFFMPEGVideoSource* video)
{
  if (this->VideoSource == video)
  {
    return;
  }

  if (this->VideoSource)
  {
    this->VideoSource->SetVideoCallback(nullptr, nullptr);
  }

  this->VideoSource = video;

  this->NewData = 0;
  this->HaveData = 0;

  video->SetVideoCallback(
    std::bind(&vtkOpenGLMovieSphere::VideoCallback, this, std::placeholders::_1), nullptr);
  this->Modified();
}

void vtkOpenGLMovieSphere::VideoCallback(vtkFFMPEGVideoSourceVideoCallbackData const& cbd)
{
  // make sure the data is allocated
  int* fsize = cbd.Caller->GetFrameSize();
  if (fsize[1] != this->Height)
  {
    const std::lock_guard<std::mutex> lock(this->TextureUpdateMutex);
    (void)lock;
    for (int i = 0; i < 6; ++i)
    {
      delete[] this->TextureData[i];
    }
    this->Height = fsize[1];
    this->Width = fsize[0];
    this->TextureData[0] = new unsigned char[this->Height * this->Width];
    this->TextureData[3] = new unsigned char[this->Height * this->Width];
    this->UVHeight = this->Height / 2;
    this->UVWidth = this->Width / 2;
    this->TextureData[1] = new unsigned char[this->UVHeight * this->UVWidth];
    this->TextureData[2] = new unsigned char[this->UVHeight * this->UVWidth];
    this->TextureData[4] = new unsigned char[this->UVHeight * this->UVWidth];
    this->TextureData[5] = new unsigned char[this->UVHeight * this->UVWidth];
  }

  // copy each row due to linesize possibly being larger than width
  for (int i = 0; i < this->Height; ++i)
  {
    memcpy(this->TextureData[this->WriteIndex] + this->Width * i, cbd.Data[0] + i * cbd.LineSize[0],
      this->Width);
  }
  for (int i = 0; i < this->UVHeight; ++i)
  {
    memcpy(this->TextureData[this->WriteIndex + 1] + this->UVWidth * i,
      cbd.Data[1] + i * cbd.LineSize[1], this->UVWidth);
    memcpy(this->TextureData[this->WriteIndex + 2] + this->UVWidth * i,
      cbd.Data[2] + i * cbd.LineSize[2], this->UVWidth);
  }

  {
    const std::lock_guard<std::mutex> lock(this->TextureUpdateMutex);
    (void)lock;
    this->ReadIndex = this->WriteIndex;
  }
  this->WriteIndex = this->ReadIndex ? 0 : 3;
  this->NewData = 1;
  this->HaveData = 1;
}

// Actual Skybox render method.
void vtkOpenGLMovieSphere::Render(vtkRenderer* ren, vtkMapper* mapper)
{
  vtkOpenGLClearErrorMacro();

  if (this->LastProjection != this->Projection)
  {
    vtkOpenGLShaderProperty* sp =
      vtkOpenGLShaderProperty::SafeDownCast(this->OpenGLActor->GetShaderProperty());
    if (this->Projection == vtkSkybox::Sphere)
    {
      // Replace VTK fragment shader
      sp->SetFragmentShaderCode("//VTK::System::Dec\n" // always start with this line
                                "//VTK::Output::Dec\n" // always have this line in your FS
                                "in vec3 TexCoords;\n"
                                "uniform vec3 cameraPos;\n" // wc camera position
                                "uniform sampler2D YTexture;\n"
                                "uniform sampler2D UTexture;\n"
                                "uniform sampler2D VTexture;\n"
                                "uniform vec4 floorPlane;\n" // floor plane eqn
                                "uniform vec3 floorRight;\n" // floor plane right
                                "uniform vec3 floorFront;\n" // floor plane front
                                "void main () {\n"
                                "  vec3 diri = normalize(TexCoords - cameraPos);\n"
                                "  vec3 dirv = vec3(dot(diri,floorRight),\n"
                                "    dot(diri,floorPlane.xyz),\n"
                                "    dot(diri,floorFront));\n"
                                "  float phix = length(vec2(dirv.x, dirv.z));\n"
                                "  vec2 tval = vec2(0.5*atan(dirv.x, dirv.z)/3.1415927 + 0.5, 1.0 "
                                "- atan(dirv.y,phix)/3.1415927 - 0.5);\n"
                                // See https://www.fourcc.org/fccyvrgb.php note we use the
                                // Video Demystified version which I believe is correct for
                                // ffmpeg going from yuv NOT in the full range of 0 to 1.0
                                // (due to the 16,235 limits) to RGB IN the full range of
                                // 0 to 1.0.
                                "  float y = 1.164*(texture2D(YTexture, tval).r - 0.0627);\n"
                                "  float u = texture2D(UTexture, tval).r - 0.5;\n"
                                "  float v = texture2D(VTexture, tval).r - 0.5;\n"
                                "  float r = y + 1.596 * v;\n"
                                "  float g = y - 0.391 * u - 0.813 * v;\n"
                                "  float b = y + 2.018 * u;\n"
                                "  gl_FragData[0] = vec4(r,g,b,1.0);\n"
                                "}\n");
    }
    if (this->Projection == vtkSkybox::StereoSphere)
    {
      // Replace VTK fragment shader
      sp->SetFragmentShaderCode("//VTK::System::Dec\n" // always start with this line
                                "//VTK::Output::Dec\n" // always have this line in your FS
                                "in vec3 TexCoords;\n"
                                "uniform vec3 cameraPos;\n" // wc camera position
                                "uniform sampler2D YTexture;\n"
                                "uniform sampler2D UTexture;\n"
                                "uniform sampler2D VTexture;\n"
                                "uniform vec4 floorPlane;\n" // floor plane eqn
                                "uniform vec3 floorRight;\n" // floor plane right
                                "uniform vec3 floorFront;\n" // floor plane front
                                "uniform float leftEye;\n"   // 1.0 for left, 0.0 for right
                                "void main () {\n"
                                "  vec3 diri = normalize(TexCoords - cameraPos);\n"
                                "  vec3 dirv = vec3(dot(diri,floorRight),\n"
                                "    dot(diri,floorPlane.xyz),\n"
                                "    dot(diri,floorFront));\n"
                                "  float phix = length(vec2(dirv.x, dirv.z));\n"
                                "  vec2 tval = vec2(0.5*atan(dirv.x, dirv.z)/3.1415927 + 0.5, 1.0 "
                                "- 0.5*atan(dirv.y,phix)/3.1415927 - 0.25 + 0.5*leftEye);\n"
                                // See https://www.fourcc.org/fccyvrgb.php note we use the
                                // Video Demystified version which I believe is correct for
                                // ffmpeg going from yuv NOT in the full range of 0 to 1.0
                                // (due to the 16,235 limits) to RGB IN the full range of
                                // 0 to 1.0.
                                "  float y = 1.164*(texture2D(YTexture, tval).r - 0.0627);\n"
                                "  float u = texture2D(UTexture, tval).r - 0.5;\n"
                                "  float v = texture2D(VTexture, tval).r - 0.5;\n"
                                "  float r = y + 1.596 * v;\n"
                                "  float g = y - 0.391 * u - 0.813 * v;\n"
                                "  float b = y + 2.018 * u;\n"
                                "  gl_FragData[0] = vec4(r,g,b,1.0);\n"
                                "}\n");
    }
    this->CubeMapper->Modified();
    this->LastProjection = this->Projection;
  }

  double* pos = ren->GetActiveCamera()->GetPosition();
  this->LastCameraPosition[0] = pos[0];
  this->LastCameraPosition[1] = pos[1];
  this->LastCameraPosition[2] = pos[2];

  this->CurrentRenderer = ren;

  // get opacity
  static_cast<vtkOpenGLRenderer*>(ren)->GetState()->vtkglDepthMask(GL_TRUE);
  static_cast<vtkOpenGLRenderer*>(ren)->GetState()->vtkglDepthFunc(GL_LEQUAL);

  vtkOpenGLRenderWindow* renWin = static_cast<vtkOpenGLRenderWindow*>(ren->GetVTKWindow());

  if (this->Textures[0]->GetHandle() == 0)
  {
    for (int i = 0; i < 6; ++i)
    {
      this->Textures[i]->SetContext(renWin);
      this->Textures[i]->SetMinificationFilter(vtkTextureObject::Linear);
      this->Textures[i]->SetMagnificationFilter(vtkTextureObject::Linear);
    }
  }

  // send a render to the mapper; update pipeline
  if (this->HaveData.load() == 0)
  {
    return;
  }

  if (this->NewData.load() == 1)
  {
    const std::lock_guard<std::mutex> lock(this->TextureUpdateMutex);
    (void)lock;
    this->Textures[this->BuildIndex]->Create2DFromRaw(
      this->Width, this->Height, 1, VTK_UNSIGNED_CHAR, this->TextureData[this->ReadIndex]);
    this->Textures[this->BuildIndex + 1]->Create2DFromRaw(
      this->UVWidth, this->UVHeight, 1, VTK_UNSIGNED_CHAR, this->TextureData[this->ReadIndex + 1]);
    this->Textures[this->BuildIndex + 2]->Create2DFromRaw(
      this->UVWidth, this->UVHeight, 1, VTK_UNSIGNED_CHAR, this->TextureData[this->ReadIndex + 2]);
    this->NewData = 0;
    this->DrawIndex = this->BuildIndex;
    this->BuildIndex = this->DrawIndex ? 0 : 3;
  }

  this->Textures[this->DrawIndex]->Activate();
  this->Textures[this->DrawIndex + 1]->Activate();
  this->Textures[this->DrawIndex + 2]->Activate();

  this->YTexture = this->Textures[this->DrawIndex]->GetTextureUnit();
  this->UTexture = this->Textures[this->DrawIndex + 1]->GetTextureUnit();
  this->VTexture = this->Textures[this->DrawIndex + 2]->GetTextureUnit();

  mapper->Render(ren, this->OpenGLActor);

  this->Textures[this->DrawIndex]->Deactivate();
  this->Textures[this->DrawIndex + 1]->Deactivate();
  this->Textures[this->DrawIndex + 2]->Deactivate();

  vtkOpenGLCheckErrorMacro("failed after Render");
}

//----------------------------------------------------------------------------
void vtkOpenGLMovieSphere::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
