// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkOpenGLToneMappingPass.h"

#include "vtkImageProcessingPass.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLQuadHelper.h"
#include "vtkOpenGLRenderUtilities.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkOverrideAttribute.h"
#include "vtkRenderState.h"
#include "vtkRenderer.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"

VTK_ABI_NAMESPACE_BEGIN
namespace
{
// Renders the delegate into the framebuffer object the way the image processing passes do.
class vtkToneMappingDelegateRenderer : public vtkImageProcessingPass
{
public:
  static vtkToneMappingDelegateRenderer* New();
  vtkTypeMacro(vtkToneMappingDelegateRenderer, vtkImageProcessingPass);
  void Render(const vtkRenderState*) override {}
  using vtkImageProcessingPass::RenderDelegate;
};
vtkStandardNewMacro(vtkToneMappingDelegateRenderer);
}

vtkStandardNewMacro(vtkOpenGLToneMappingPass);

//------------------------------------------------------------------------------
vtkOverrideAttribute* vtkOpenGLToneMappingPass::CreateOverrideAttributes()
{
  auto* renderingBackendAttribute =
    vtkOverrideAttribute::CreateAttributeChain("RenderingBackend", "OpenGL", nullptr);
  return renderingBackendAttribute;
}

//------------------------------------------------------------------------------
vtkOpenGLToneMappingPass::~vtkOpenGLToneMappingPass()
{
  if (this->FrameBufferObject)
  {
    vtkErrorMacro("FrameBufferObject should have been deleted in ReleaseGraphicsResources().");
  }
  if (this->ColorTexture)
  {
    vtkErrorMacro("ColorTexture should have been deleted in ReleaseGraphicsResources().");
  }
  if (this->QuadHelper)
  {
    vtkErrorMacro("QuadHelper should have been deleted in ReleaseGraphicsResources().");
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLToneMappingPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FrameBufferObject:";
  if (this->FrameBufferObject != nullptr)
  {
    this->FrameBufferObject->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }
  os << indent << "ColorTexture:";
  if (this->ColorTexture != nullptr)
  {
    this->ColorTexture->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLToneMappingPass::Render(const vtkRenderState* s)
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
    vtkWarningMacro("no delegate in vtkToneMappingPass.");
    return;
  }

  // create FBO and texture
  int x, y, w, h;
  r->GetTiledSizeAndOrigin(&w, &h, &x, &y);

  if (this->ColorTexture == nullptr)
  {
    this->ColorTexture = vtkTextureObject::New();
    this->ColorTexture->SetContext(renWin);
    this->ColorTexture->SetMinificationFilter(vtkTextureObject::Linear);
    this->ColorTexture->SetMagnificationFilter(vtkTextureObject::Linear);
    this->ColorTexture->Allocate2D(w, h, 4, VTK_FLOAT);
  }
  this->ColorTexture->Resize(w, h);

  if (this->FrameBufferObject == nullptr)
  {
    this->FrameBufferObject = vtkOpenGLFramebufferObject::New();
    this->FrameBufferObject->SetContext(renWin);
  }

  vtkNew<vtkToneMappingDelegateRenderer> delegateRenderer;
  delegateRenderer->SetDelegatePass(this->DelegatePass);
  renWin->GetState()->PushFramebufferBindings();
  delegateRenderer->RenderDelegate(s, w, h, w, h, this->FrameBufferObject, this->ColorTexture);
  renWin->GetState()->PopFramebufferBindings();
  this->NumberOfRenderedProps = this->DelegatePass->GetNumberOfRenderedProps();

  if (this->QuadHelper &&
    (static_cast<unsigned int>(this->ToneMappingType) != this->QuadHelper->ShaderChangeValue ||
      this->UseACES != this->UseACESChangeValue))
  {
    delete this->QuadHelper;
    this->QuadHelper = nullptr;
  }

  if (!this->QuadHelper)
  {
    std::string FSSource = vtkOpenGLRenderUtilities::GetFullScreenQuadFragmentShaderTemplate();

    vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Decl",
      "uniform sampler2D source;\n"
      "//VTK::FSQ::Decl");

    // Inverse gamma correction
    vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Impl",
      "  vec4 pixel = texture2D(source, texCoord);\n"
      "  vec3 color = pow(pixel.rgb, vec3(2.2));\n" // to linear color space
      "//VTK::FSQ::Impl");

    switch (this->ToneMappingType)
    {
      case Clamp:
        vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Impl",
          "  vec3 toned = min(color, vec3(1.0));\n"
          "//VTK::FSQ::Impl");
        break;
      case Reinhard:
        vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Impl",
          "  vec3 toned = color / (color + 1.0);\n"
          "//VTK::FSQ::Impl");
        break;
      case Exponential:
        vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Decl", "uniform float exposure;\n");
        vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Impl",
          "  vec3 toned = (1.0 - exp(-color*exposure));\n"
          "  //VTK::FSQ::Impl");
        break;
      case GenericFilmic:
        vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Decl",
          "uniform float exposure;\n"
          "uniform float a;\n"
          "uniform float b;\n"
          "uniform float c;\n"
          "uniform float d;\n"
          "//VTK::FSQ::Decl");

        if (this->UseACES)
        {
          vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Decl",
            "const mat3 acesInputMat = mat3(0.5972782409, 0.0760130499, 0.0284085382,\n"
            "0.3545713181, 0.9083220973, 0.1338243154,\n"
            "0.0482176639, 0.0156579968, 0.8375684636);\n"
            "const mat3 acesOutputMat = mat3( 1.6047539945, -0.1020831870, -0.0032670420,\n"
            "-0.5310794927, 1.1081322801, -0.0727552477,\n"
            "-0.0736720338, -0.0060518756, 1.0760219533);\n"
            "//VTK::FSQ::Decl");
        }
        vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Impl",
          "  vec3 toned = color * exposure;\n"
          "//VTK::FSQ::Impl");
        if (this->UseACES)
        {
          vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Impl",
            "  toned = acesInputMat * toned;\n"
            "//VTK::FSQ::Impl");
        }
        vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Impl",
          "  toned = pow(toned, vec3(a)) / (pow(toned, vec3(a * d)) * b + c);\n"
          "//VTK::FSQ::Impl");
        if (this->UseACES)
        {
          vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Impl",
            "  toned = acesOutputMat * toned;\n"
            "//VTK::FSQ::Impl");
        }
        vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Impl",
          "  toned = clamp(toned, vec3(0.f), vec3(1.f));\n"
          "//VTK::FSQ::Impl");
        break;
      case NeutralPBR:
        // adapted from Khronos reference implementation:
        // https://github.com/KhronosGroup/ToneMapping/blob/main/PBR_Neutral/pbrNeutral.glsl
        vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Impl",
          "  const float startCompression = 0.8 - 0.04;\n"
          "  const float desaturation = 0.15;\n"
          "  float x = min(color.r, min(color.g, color.b));\n"
          "  float offset = x < 0.08 ? x - 6.25 * x * x : 0.04;\n"
          "  vec3 toned = color - vec3(offset);\n"
          "  float peak = max(toned.r, max(toned.g, toned.b));\n"
          "  if (peak >= startCompression)\n"
          "  {\n"
          "    const float d = 1. - startCompression;\n"
          "    float newPeak = 1. - d * d / (peak + d - startCompression);\n"
          "    toned *= newPeak / peak;\n"
          "    float g = 1. - 1. / (desaturation * (peak - newPeak) + 1.);\n"
          "    toned = mix(toned, newPeak * vec3(1, 1, 1), g);\n"
          "  }\n"
          "//VTK::FSQ::Impl");
    }

    // Recorrect gamma and output
    vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Impl",
      "  toned = pow(toned, vec3(1.0/2.2));\n" // to sRGB color space
      "  gl_FragData[0] = mix(pixel, vec4(toned , pixel.a), pixel.a);\n"
      "//VTK::FSQ::Impl");

    this->QuadHelper = new vtkOpenGLQuadHelper(renWin,
      vtkOpenGLRenderUtilities::GetFullScreenQuadVertexShader().c_str(), FSSource.c_str(), "");

    this->QuadHelper->ShaderChangeValue = this->ToneMappingType;
    this->UseACESChangeValue = this->UseACES;
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

  this->ColorTexture->Activate();
  this->QuadHelper->Program->SetUniformi("source", this->ColorTexture->GetTextureUnit());

  // Precompute generic filmic parameters after each modification
  this->UpdateAnchorCurveGenericFilmic();

  if (this->ToneMappingType == Exponential)
  {
    this->QuadHelper->Program->SetUniformf("exposure", this->Exposure);
  }
  else if (this->ToneMappingType == GenericFilmic)
  {
    this->QuadHelper->Program->SetUniformf("exposure", this->Exposure);
    this->QuadHelper->Program->SetUniformf("a", this->Contrast);
    this->QuadHelper->Program->SetUniformf("b", this->ClippingPoint);
    this->QuadHelper->Program->SetUniformf("c", this->ToeSpeed);
    this->QuadHelper->Program->SetUniformf("d", this->Shoulder);
  }

  ostate->vtkglDisable(GL_BLEND);
  ostate->vtkglDisable(GL_DEPTH_TEST);
  ostate->vtkglClear(GL_DEPTH_BUFFER_BIT);
  ostate->vtkglViewport(x, y, w, h);
  ostate->vtkglScissor(x, y, w, h);

  this->QuadHelper->Render();

  this->ColorTexture->Deactivate();

  vtkOpenGLCheckErrorMacro("failed after Render");
}

//------------------------------------------------------------------------------
void vtkOpenGLToneMappingPass::ReleaseGraphicsResources(vtkWindow* w)
{
  this->Superclass::ReleaseGraphicsResources(w);

  if (this->QuadHelper)
  {
    delete this->QuadHelper;
    this->QuadHelper = nullptr;
  }
  if (this->FrameBufferObject)
  {
    this->FrameBufferObject->Delete();
    this->FrameBufferObject = nullptr;
  }
  if (this->ColorTexture)
  {
    this->ColorTexture->Delete();
    this->ColorTexture = nullptr;
  }
}
VTK_ABI_NAMESPACE_END
