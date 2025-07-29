// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOpenGLSurfaceProbeVolumeMapper.h"

#include "vtkCommand.h"
#include "vtkExecutive.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLResourceFreeCallback.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLTexture.h"
#include "vtkOpenGLVertexBufferObject.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRenderer.h"
#include "vtkShaderProgram.h"
#include "vtkShaderProperty.h"
#include "vtkTextureObject.h"
#include "vtkVolumeTexture.h"

#include <vtk_glad.h>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOpenGLSurfaceProbeVolumeMapper);

//------------------------------------------------------------------------------
vtkOpenGLSurfaceProbeVolumeMapper::vtkOpenGLSurfaceProbeVolumeMapper()
{
  this->SetNumberOfInputPorts(3);
}

//------------------------------------------------------------------------------
int vtkOpenGLSurfaceProbeVolumeMapper::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
    return 1;
  }
  else if (port == 1)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
    return 1;
  }
  else if (port == 2)
  {
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
    return 1;
  }
  return 0;
}

//------------------------------------------------------------------------------
void vtkOpenGLSurfaceProbeVolumeMapper::SetProbeInputData(vtkPolyData* in)
{
  this->SetInputDataObject(2, in);
}

//------------------------------------------------------------------------------
vtkPolyData* vtkOpenGLSurfaceProbeVolumeMapper::GetProbeInput()
{
  if (this->GetNumberOfInputConnections(2) < 1)
  {
    return nullptr;
  }

  return vtkPolyData::SafeDownCast(this->GetExecutive()->GetInputData(2, 0));
}

//------------------------------------------------------------------------------
void vtkOpenGLSurfaceProbeVolumeMapper::SetProbeInputConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(2, algOutput);
}

//------------------------------------------------------------------------------
void vtkOpenGLSurfaceProbeVolumeMapper::SetSourceData(vtkImageData* in)
{
  this->SetInputDataObject(1, in);
}

//------------------------------------------------------------------------------
vtkImageData* vtkOpenGLSurfaceProbeVolumeMapper::GetSource()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return nullptr;
  }

  return vtkImageData::SafeDownCast(this->GetExecutive()->GetInputData(1, 0));
}

//------------------------------------------------------------------------------
void vtkOpenGLSurfaceProbeVolumeMapper::SetSourceConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//------------------------------------------------------------------------------
void vtkOpenGLSurfaceProbeVolumeMapper::RenderPiece(vtkRenderer* ren, vtkActor* actor)
{
  // Make sure that we have been properly initialized.
  if (ren->GetRenderWindow()->CheckAbortStatus())
  {
    return;
  }

  this->ResourceCallback->RegisterGraphicsResources(
    static_cast<vtkOpenGLRenderWindow*>(ren->GetRenderWindow()));

  // The second input is used for probing if it exists.
  // The first input is always the one being rendered to avoid having to recompute bounds.
  vtkPolyData* secondInput = vtkPolyData::SafeDownCast(this->GetExecutive()->GetInputData(2, 0));
  if (secondInput)
  {
    this->CurrentInput = secondInput;
  }
  else
  {
    this->CurrentInput = this->GetInput();
  }

  // Source volume being probed
  vtkImageData* sourceInput = vtkImageData::SafeDownCast(this->GetExecutive()->GetInputData(1, 0));

  if (this->CurrentInput == nullptr || sourceInput == nullptr)
  {
    vtkErrorMacro(<< "No input or source!");
    return;
  }

  this->InvokeEvent(vtkCommand::StartEvent, nullptr);
  if (!this->Static)
  {
    this->GetInputAlgorithm()->Update();

    // Update probed volume
    this->GetInputAlgorithm(1, 0)->Update();

    // Update probe surface
    if (secondInput)
    {
      this->GetInputAlgorithm(2, 0)->Update();
    }
  }
  this->InvokeEvent(vtkCommand::EndEvent, nullptr);

  // if there are no points then we are done
  if (!this->CurrentInput->GetPoints())
  {
    return;
  }

  this->UpdateCameraShiftScale(ren, actor);
  this->RenderPieceStart(ren, actor);

  // 1. Position texture pass
  this->ReplaceShaderPositionPass(actor);

  // Render positions and normals into FBO textures
  this->ReplaceActiveFBO(ren);

  this->RenderPieceDraw(ren, actor);

  this->RestoreActiveFBO(ren);

  // Clear position pass shader replacements
  // WARNING: This has the side-effect of clearing the user's shader replacement.
  // To prevent this we should use ClearVertexShaderReplacements/ClearFragmentShaderReplacements
  // with the original strings used in ReplaceShaderPositionPass.
  actor->GetShaderProperty()->ClearAllVertexShaderReplacements();
  actor->GetShaderProperty()->ClearAllFragmentShaderReplacements();

  // 2. Probe pass

  // Replace input
  this->CurrentInput = this->GetInput();

  if (this->CurrentInput == nullptr)
  {
    vtkErrorMacro(<< "No input!");
    return;
  }

  // if there are no points then we are done
  if (!this->CurrentInput->GetPoints())
  {
    return;
  }

  this->RenderPieceStart(ren, actor);

  this->ReplaceShaderProbePass(actor);

  this->RenderPieceDraw(ren, actor);

  // Deactivate textures used in probe pass
  this->PositionsTextureObject->Deactivate();
  if (this->GetBlendMode() != BlendModes::NONE)
  {
    this->NormalsTextureObject->Deactivate();
  }
  this->VolumeTexture->GetCurrentBlock()->TextureObject->Deactivate();

  this->RenderPieceFinish(ren, actor);

  // Clear probe pass shader replacements
  // WARNING: This has the side-effect of clearing the user's shader replacement.
  // To prevent this we should use ClearVertexShaderReplacements/ClearFragmentShaderReplacements
  // with the original strings used in ReplaceShaderProbePass.
  actor->GetShaderProperty()->ClearAllVertexShaderReplacements();
  actor->GetShaderProperty()->ClearAllFragmentShaderReplacements();
}

//------------------------------------------------------------------------------
void vtkOpenGLSurfaceProbeVolumeMapper::UpdateShaders(
  vtkOpenGLHelper& cellBO, vtkRenderer* ren, vtkActor* act)
{
  vtkOpenGLPolyDataMapper::UpdateShaders(cellBO, ren, act);

  // Update uniforms according to the current pass
  switch (this->CurrentPass)
  {
    case PassTypes::POSITION_TEXTURE:
      // Handle VBO shift and scale only when the actor matrix is identity.
      // Otherwise VBOShiftScale is already multiplied with the actor matrix in the base class.
      if (act->GetIsIdentity() && !this->VBOShiftScale->IsIdentity())
      {
        cellBO.Program->SetUniformMatrix("MCWCMatrix", this->VBOShiftScale);
      }
      break;
    case PassTypes::PROBE:
      this->UpdateShadersProbePass(cellBO, ren);
      break;
    default:
      break;
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLSurfaceProbeVolumeMapper::ReplaceShaderPositionPass(vtkActor* actor)
{
  // Position implementation.
  // Expect TCoords in the [0, 1] range and normalize them to define the fragment position.
  // The vertex position is passed to the fragment shader to be written in the texture.
  std::string positionImpl =
    "gl_Position = vec4(tcoord.x * 2.0 - 1.0, tcoord.y * 2.0 - 1.0, 0.0, 1.0);\n";

  if (!this->VBOShiftScale->IsIdentity() || !actor->GetIsIdentity())
  {
    actor->GetShaderProperty()->AddVertexShaderReplacement("//VTK::PositionVC::Dec", true,
      "//VTK::PositionVC::Dec\n"
      "uniform mat4 MCWCMatrix;\n",
      true);

    positionImpl += "vertexVCVSOutput = MCWCMatrix * vertexMC;\n";
  }
  else
  {
    positionImpl += "vertexVCVSOutput = vertexMC;\n";
  }

  actor->GetShaderProperty()->AddVertexShaderReplacement(
    "//VTK::PositionVC::Impl", true, positionImpl, true);

  // TCoords attribute are always uploaded to the GPU when they exist in the superclass, but
  // tcoord is only defined in the shader when the actor is textured. Force the declaration here.
  actor->GetShaderProperty()->AddVertexShaderReplacement(
    "//VTK::TCoord::Dec", false, "in vec2 tcoord;", true);

  // Write vertex position in texture
  // Override gl_FragData set in Light::Impl and TCoord::Impl
  std::string tcoordsImpl = "gl_FragData[0] = vertexVCVSOutput;\n";

  // Blending requires normals
  if (this->GetBlendMode() != BlendModes::NONE)
  {
    // Pass normals from the vertex to the fragment shader
    actor->GetShaderProperty()->AddVertexShaderReplacement(
      "//VTK::Normal::Impl", true, "normalVCVSOutput = normalMC;\n", true);

    // Write normals in additional render target
    tcoordsImpl += "gl_FragData[1] = vec4(normalVCVSOutput, 0.0);\n";
  }

  actor->GetShaderProperty()->AddFragmentShaderReplacement(
    "//VTK::TCoord::Impl", true, tcoordsImpl, true);

  // Prevent OIT pass from overriding gl_FragData values
  actor->GetShaderProperty()->AddFragmentShaderReplacement(
    "//VTK::DepthPeeling::Impl", true, "", true);

  // Switch to positions/normals pass to update shaders uniforms accordingly
  this->CurrentPass = PassTypes::POSITION_TEXTURE;
}

//------------------------------------------------------------------------------
void vtkOpenGLSurfaceProbeVolumeMapper::ReplaceShaderProbePass(vtkActor* actor)
{
  // Pass texture coordinates from vertex shader to fragment shader
  actor->GetShaderProperty()->AddVertexShaderReplacement("//VTK::TCoord::Dec", true,
    "in vec2 tcoord;\n"
    "out vec2 tcoordVCVSOutput;\n",
    true);

  actor->GetShaderProperty()->AddVertexShaderReplacement(
    "//VTK::TCoord::Impl", true, "tcoordVCVSOutput = tcoord;\n", true);

  // Textures and coloring declaration
  std::string tmapDec = "//VTK::TMap::Dec\n" // keep default replacement
                        "uniform sampler2D positionTexture;\n"
                        "uniform sampler3D in_volume;\n"
                        "uniform mat4 in_inverseTextureDatasetMatrix;\n"
                        "uniform mat4 in_cellToPoint;\n"
                        // Coloring
                        "uniform vec4 in_volume_scale;\n"
                        "uniform vec4 in_volume_bias;\n"
                        "uniform float in_window;\n"
                        "uniform float in_level;\n";

  // Blending requires normals and blending uniforms
  if (this->GetBlendMode() != BlendModes::NONE)
  {
    tmapDec += "uniform sampler2D normalTexture;\n"
               "uniform vec3 in_volume_spacing;\n"
               "uniform float blend_width;\n";
  }

  actor->GetShaderProperty()->AddFragmentShaderReplacement("//VTK::TMap::Dec", true, tmapDec, true);

  actor->GetShaderProperty()->AddFragmentShaderReplacement("//VTK::TCoord::Dec", true,
    "in vec2 tcoordVCVSOutput;\n"
    // Window/Level declaration
    "vec4 applyWindowLevel(vec4 color)\n"
    "{\n"
    "  float l = in_level; \n"
    "  float w = in_window;\n"
    "  float s = w > 0.0 ? 0.5 : -0.5;\n" // handle negative window
    "  color = clamp(color, l - s * w, l + s * w);\n"
    "  return (color - (l - 0.5 * w)) / w;\n"
    "}\n",
    true);

  std::string tcoordsImpl =
    // Get the current fragment position on the probe surface
    "vec3 fragmentPos = texture2D(positionTexture, tcoordVCVSOutput).xyz;\n"
    // Background value when sampling outside volume.
    // WARNING: Initialization to 0 currently required for average blending
    "vec4 volumeValue = vec4(0, 0, 0, 0);\n"
    "int sampleCount = 0;\n"; // Keep track of the number of samples for blending

  if (this->GetBlendMode() == BlendModes::NONE)
  {
    tcoordsImpl +=
      "vec3 texPos = (in_cellToPoint * in_inverseTextureDatasetMatrix * vec4(fragmentPos.xyz, "
      "1.0)).xyz;\n"
      "if ((all(lessThanEqual(texPos, vec3(1.0))) &&\n"
      "  all(greaterThanEqual(texPos, vec3(0.0)))))\n"
      "{\n"
      "  volumeValue = texture3D(in_volume, texPos)  * in_volume_scale[0] + in_volume_bias[0];\n"
      "  sampleCount++;\n"
      "}\n";
  }
  else // Blend modes
  {
    tcoordsImpl +=
      "float epsilon = 1e-7;\n"
      // Get the current fragment normal on the probe surface
      "vec3 fragmentNormal = texture2D(normalTexture, tcoordVCVSOutput).xyz;\n"
      "fragmentNormal = normalize(fragmentNormal);\n"
      // Use the half of the minimum spacing values as sampling step.
      "float spacing = 0.5 * min(min(in_volume_spacing[0], in_volume_spacing[1]), "
      "in_volume_spacing[2]);\n"
      "spacing = max(spacing, epsilon);\n" // Force positive spacing to avoid infinite loop below
      "float offset = -0.5 * (blend_width + epsilon);\n"
      "while(offset < 0.5 * (blend_width + epsilon))\n"
      "{"
      "  vec3 pos = fragmentPos + offset * fragmentNormal;\n"
      "  vec3 texPos = (in_cellToPoint * in_inverseTextureDatasetMatrix * vec4(pos.xyz, "
      "1.0)).xyz;\n"
      "  if ((all(lessThanEqual(texPos, vec3(1.0))) && \n"
      "    all(greaterThanEqual(texPos, vec3(0.0)))))\n"
      "  {\n"
      "    vec4 currentColor = texture3D(in_volume, texPos) * in_volume_scale[0] + "
      "in_volume_bias[0];\n";
    switch (this->GetBlendMode())
    {
      case BlendModes::MAX:
        tcoordsImpl +=
          "    volumeValue.r = max(currentColor.r, sampleCount > 0 ? volumeValue.r : 0.0);\n";
        break;
      case BlendModes::MIN:
        tcoordsImpl +=
          "    volumeValue.r = min(currentColor.r, sampleCount > 0 ? volumeValue.r : 1.0);\n";
        break;
      case BlendModes::AVERAGE:
        tcoordsImpl += "    volumeValue += currentColor;\n";
        break;
      default:
        break;
    }
    tcoordsImpl += "    sampleCount++;\n"
                   "  }"
                   "  offset += spacing;"
                   "}";
  }

  if (this->GetBlendMode() == BlendModes::AVERAGE)
  {
    tcoordsImpl += "if (sampleCount > 0)\n"
                   "{\n"
                   "  volumeValue = volumeValue / sampleCount;\n"
                   "}\n";
  }

  // Compute final color
  tcoordsImpl +=
    "if (sampleCount > 0)\n"
    "{\n"
    "  volumeValue = applyWindowLevel(volumeValue);"
    "  volumeValue.a = opacityUniform;"
    "}\n"
    // Only support grayscale volumes.
    "gl_FragData[0] = vec4(volumeValue.r, volumeValue.r, volumeValue.r, volumeValue.a);\n";

  actor->GetShaderProperty()->AddFragmentShaderReplacement(
    "//VTK::TCoord::Impl", true, tcoordsImpl, true);

  // Switch to probing pass to update shaders uniforms accordingly
  this->CurrentPass = PassTypes::PROBE;
}

//------------------------------------------------------------------------------
void vtkOpenGLSurfaceProbeVolumeMapper::UpdateShadersProbePass(
  vtkOpenGLHelper& cellBO, vtkRenderer* ren)
{
  if (!this->VolumeTexture->GetLoadedScalars())
  {
    vtkImageData* sourceInput = this->GetSource();

    // The extent of the volume must start at (0,0,0)
    // see "void vtkGPUVolumeRayCastMapper::TransformInput"
    this->TransformedSource->ShallowCopy(sourceInput);
    // Get the current extents.
    int extents[6];
    this->TransformedSource->GetExtent(extents);

    // Get the current origin and spacing.
    double origin[3], spacing[3];
    this->TransformedSource->GetOrigin(origin);
    this->TransformedSource->GetSpacing(spacing);

    for (int cc = 0; cc < 3; cc++)
    {
      // Transform the origin and the extents.
      origin[cc] = origin[cc] + extents[2 * cc] * spacing[cc];
      extents[2 * cc + 1] -= extents[2 * cc];
      extents[2 * cc] = 0;
    }

    this->TransformedSource->SetOrigin(origin);
    this->TransformedSource->SetExtent(extents);

    // Always use scalar point data.
    // Mimic vtkAbstractVolumeMapper::GetScalars to handle array access and cell scalars if needed.
    vtkDataArray* scalars = this->TransformedSource->GetPointData()->GetScalars();
    int isCellData = 0;

    // Load volume
    this->VolumeTexture->LoadVolume(
      ren, this->TransformedSource, scalars, isCellData, VTK_LINEAR_INTERPOLATION);
  }

  std::vector<float> VolMatVec = {};
  std::vector<float> InvTexMatVec = {};
  std::vector<float> CellToPointVec = {};
  VolMatVec.resize(16, 0);
  InvTexMatVec.resize(16, 0);
  CellToPointVec.resize(16, 0);

  vtkNew<vtkMatrix4x4> texToDataMat;
  texToDataMat->DeepCopy(this->VolumeTexture->GetCurrentBlock()->TextureToDataset.GetPointer());
  texToDataMat->Transpose();

  texToDataMat->Invert();

  vtkNew<vtkMatrix4x4> cellToPointMat;
  cellToPointMat->DeepCopy(this->VolumeTexture->CellToPointMatrix.GetPointer());
  cellToPointMat->Transpose();

  for (int i = 0; i < 16; i++)
  {
    InvTexMatVec[i] = texToDataMat->Element[i / 4][i % 4];
    CellToPointVec[i] = cellToPointMat->Element[i / 4][i % 4];
  }

  cellBO.Program->SetUniformMatrix4x4("in_inverseTextureDatasetMatrix", InvTexMatVec.data());
  cellBO.Program->SetUniformMatrix4x4("in_cellToPoint", CellToPointVec.data());

  cellBO.Program->SetUniform4f("in_volume_scale", this->VolumeTexture->Scale);
  cellBO.Program->SetUniform4f("in_volume_bias", this->VolumeTexture->Bias);

  if (this->GetBlendMode() != BlendModes::NONE)
  {
    cellBO.Program->SetUniform3f("in_volume_spacing", this->TransformedSource->GetSpacing());
    cellBO.Program->SetUniformf("blend_width", this->GetBlendWidth());
  }

  // Rescale window/level.
  float scalarRange[2] = { this->VolumeTexture->ScalarRange[0][0],
    this->VolumeTexture->ScalarRange[0][1] };
  double finalWindow = this->Window / (scalarRange[1] - scalarRange[0]);
  double finalLevel = (this->Level - scalarRange[0]) / (scalarRange[1] - scalarRange[0]);
  cellBO.Program->SetUniformf("in_window", finalWindow);
  cellBO.Program->SetUniformf("in_level", finalLevel);

  // Handle single block.
  vtkTextureObject* volumeTextureObject = this->VolumeTexture->GetCurrentBlock()->TextureObject;
  volumeTextureObject->Activate();
  cellBO.Program->SetUniformi("in_volume", volumeTextureObject->GetTextureUnit());

  this->PositionsTextureObject->Activate();
  cellBO.Program->SetUniformi("positionTexture", this->PositionsTextureObject->GetTextureUnit());

  if (this->GetBlendMode() != BlendModes::NONE)
  {
    this->NormalsTextureObject->Activate();
    cellBO.Program->SetUniformi("normalTexture", this->NormalsTextureObject->GetTextureUnit());
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLSurfaceProbeVolumeMapper::CreateTexture(
  vtkTextureObject* texture, vtkOpenGLRenderWindow* renWin)
{
  if (texture->GetHandle() == 0)
  {
    texture->SetContext(renWin);
    texture->SetFormat(GL_RGBA);
    texture->SetInternalFormat(GL_RGBA16F);
    texture->SetDataType(GL_FLOAT);
    texture->SetWrapS(vtkTextureObject::ClampToEdge);
    texture->SetWrapT(vtkTextureObject::ClampToEdge);
    texture->SetMinificationFilter(vtkTextureObject::Linear);
    texture->SetMagnificationFilter(vtkTextureObject::Linear);
    texture->Allocate2D(renWin->GetSize()[0], renWin->GetSize()[1], 4, VTK_FLOAT);
  }
  else
  {
    texture->Resize(renWin->GetSize()[0], renWin->GetSize()[1]);
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLSurfaceProbeVolumeMapper::ReplaceActiveFBO(vtkRenderer* ren)
{
  vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
  if (!renWin)
  {
    return;
  }

  // Save viewport. It must be queried from the current state as it might not
  // match vtkRenderer::GetTiledSizeAndOrigin when using the OIT render pass.
  renWin->GetState()->vtkglGetIntegerv(GL_VIEWPORT, this->SavedViewport);
  // Save scissor test and blend state
  this->SavedScissorTestState = renWin->GetState()->GetEnumState(GL_SCISSOR_TEST);
  this->SavedBlendState = renWin->GetState()->GetEnumState(GL_BLEND);

  // Use the entire render window to render textures, even when having multiple renderers
  renWin->GetState()->vtkglViewport(0, 0, renWin->GetSize()[0], renWin->GetSize()[1]);
  renWin->GetState()->vtkglDisable(GL_SCISSOR_TEST);
  renWin->GetState()->vtkglDisable(GL_BLEND);

  this->CreateTexture(this->PositionsTextureObject, renWin);
  this->PositionsTextureObject->Activate();

  // Blending requires normals
  if (this->GetBlendMode() != BlendModes::NONE)
  {
    this->CreateTexture(this->NormalsTextureObject, renWin);
    this->NormalsTextureObject->Activate();
  }

  renWin->GetState()->PushFramebufferBindings();

  this->FBO->SetContext(renWin);
  this->FBO->Bind(GL_FRAMEBUFFER);
  this->FBO->AddColorAttachment(0U, this->PositionsTextureObject);
  if (this->GetBlendMode() != BlendModes::NONE)
  {
    this->FBO->AddColorAttachment(1U, this->NormalsTextureObject);
  }
  this->FBO->ActivateDrawBuffers(this->GetBlendMode() != BlendModes::NONE ? 2 : 1);
  this->FBO->CheckFrameBufferStatus(GL_FRAMEBUFFER);
  this->FBO->GetContext()->GetState()->vtkglClearColor(0.0, 0.0, 0.0, 0.0);
  this->FBO->GetContext()->GetState()->vtkglClear(GL_COLOR_BUFFER_BIT);
}

//------------------------------------------------------------------------------
void vtkOpenGLSurfaceProbeVolumeMapper::RestoreActiveFBO(vtkRenderer* ren)
{
  vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
  if (!renWin)
  {
    return;
  }

  this->FBO->RemoveColorAttachment(0U);
  if (this->GetBlendMode() != BlendModes::NONE)
  {
    this->FBO->RemoveColorAttachment(1U);
  }

  this->FBO->DeactivateDrawBuffers();
  this->FBO->GetContext()->GetState()->PopFramebufferBindings();

  this->PositionsTextureObject->Deactivate();
  if (this->GetBlendMode() != BlendModes::NONE)
  {
    this->NormalsTextureObject->Deactivate();
  }

  // Restore scissor test, blend and viewport state
  this->SavedScissorTestState ? renWin->GetState()->vtkglEnable(GL_SCISSOR_TEST)
                              : renWin->GetState()->vtkglDisable(GL_SCISSOR_TEST);
  this->SavedBlendState ? renWin->GetState()->vtkglEnable(GL_BLEND)
                        : renWin->GetState()->vtkglDisable(GL_BLEND);
  renWin->GetState()->vtkglViewport(
    this->SavedViewport[0], this->SavedViewport[1], this->SavedViewport[2], this->SavedViewport[3]);
}
VTK_ABI_NAMESPACE_END
