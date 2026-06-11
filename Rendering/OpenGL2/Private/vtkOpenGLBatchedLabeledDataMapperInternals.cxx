// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "Private/vtkOpenGLBatchedLabeledDataMapperInternals.h"

#include "vtkBatchedLabeledDataMapper.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLBatchedLabeledDataMapper.h"
#include "vtkOpenGLHelper.h"
#include "vtkOpenGLShaderProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkShader.h"
#include "vtkShaderProgram.h"
#include "vtkStringFormatter.h"
#include "vtkTextureObject.h"

#include "vtksys/SystemTools.hxx"

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkOpenGLBatchedLabeledDataMapperInternals);

//----------------------------------------------------------------------------
// Sets up all shader replacements for the label rendering pipeline. Called from
// BuildShaders below. OpenGL uses a geometry shader to expand each input point
// into up to 3 triangle-strip quads (glyph / background / frame). WebGPU cannot
// use geometry shaders; it achieves the same expansion with instanced rendering
// (18 vertices per instance: 3 layers × 6 vertices) driven by the vertex shader.
static void MakeupShaders(vtkOpenGLShaderProperty* sp)
{
  const std::string smaxprops = vtk::to_string(vtkBatchedLabeledDataMapper::MaxTextProperties);
  const std::string spadsz = vtk::to_string(vtkBatchedLabeledDataMapper::GlyphAtlasPadding);

  sp->AddShaderReplacement(vtkShader::Vertex, "//VTK::Normal::Dec", true,
    "//VTK::Normal::Dec\n"
    "\n"
    "in vec4 glyphExtentsVS;\n"
    "in float coff;\n"
    "in float propid;\n"
    "in vec3 framecolors;\n"
    "out ivec4 glyphExtentsGS;\n"
    "out float COFF;\n"
    "out int PROPID;\n"
    "out vec3 framecolorsGS;\n",
    false);

  sp->AddShaderReplacement(vtkShader::Vertex, "//VTK::Normal::Impl", true,
    "//VTK::Normal::Impl\n"
    "\n"
    "  glyphExtentsGS = ivec4(glyphExtentsVS + vec4(0.5f));\n"
    "  COFF = coff;\n"
    "  PROPID = int(propid);\n"
    "  framecolorsGS = framecolors;\n",
    false);

  std::string geomp_str("//VTK::System::Dec\n"
                        "layout(points) in;\n"
                        "//VTK::Output::Dec\n"
                        "//VTK::Picking::Dec\n"
                        "layout(triangle_strip, max_vertices = 18) out;\n"
                        "in ivec4[] glyphExtentsGS;\n"
                        "in float[] COFF;\n"
                        "in int[] PROPID;\n"
                        "in vec3[] framecolorsGS;\n"
                        "flat out int FPROPID;\n"
                        "flat out vec3 framecolorsFS;\n"
                        "out vec2 UV2;\n"
                        "flat out int layer;\n"
                        "uniform int FrameWidths[@MAXPROPS@];\n"
                        "uniform int MaxGlyphHeights[@MAXPROPS@];\n"
                        "uniform int Descenders[@MAXPROPS@];\n"
                        "uniform vec4 vp;\n"
                        "uniform vec4 nvp;\n"
                        "uniform ivec2 vpDims;\n"
                        "uniform ivec2 winDims;\n"
                        "uniform ivec2 atlasDims;\n"
                        "uniform ivec2 anchorCenter;\n"
                        "uniform ivec2 DisplayOffset;\n"

                        "vec2 vpOff = vec2(vp[0] * winDims[0] + 0.5,\n"
                        "                  vp[1] * winDims[1] + 0.5);\n"
                        "\n"

                        "vec4 ClipCoordToDisplayCoord(vec4 clipCoord)\n"
                        "{\n"
                        "  vec4 dispCoord = clipCoord;\n"
                        "  dispCoord.xyz /= dispCoord.w;\n"
                        "  dispCoord.x = nvp[0] + ((dispCoord.x + 1.) / 2.) * (nvp[2] - nvp[0]);\n"
                        "  dispCoord.y = nvp[1] + ((dispCoord.y + 1.) / 2.) * (nvp[3] - nvp[1]);\n"
                        "  dispCoord.x = (dispCoord.x - vp[0]) / (vp[2] - vp[0]);\n"
                        "  dispCoord.y = (dispCoord.y - vp[1]) / (vp[3] - vp[1]);\n"
                        "  dispCoord.x *= vpDims.x - 1.;\n"
                        "  dispCoord.y *= vpDims.y - 1.;\n"
                        "  dispCoord.xy += vpOff.xy;\n"
                        "  return dispCoord;\n"
                        "}\n"
                        "\n"
                        "vec4 DisplayCoordToClipCoord(vec4 dispCoord)\n"
                        "{\n"
                        "  vec4 clipCoord = dispCoord;\n"
                        "  clipCoord.xy -= vpOff.xy;\n"
                        "  clipCoord.x /= vpDims.x - 1;\n"
                        "  clipCoord.y /= vpDims.y - 1;\n"
                        "  clipCoord.x = clipCoord.x * (vp[2] - vp[0]) + vp[0];\n"
                        "  clipCoord.y = clipCoord.y * (vp[3] - vp[1]) + vp[1];\n"
                        "  clipCoord.x = 2. * (clipCoord.x - nvp[0]) / (nvp[2] - nvp[0]) - 1.;\n"
                        "  clipCoord.y = 2. * (clipCoord.y - nvp[1]) / (nvp[3] - nvp[1]) - 1.;\n"
                        "  clipCoord.xyz *= clipCoord.w;\n"
                        "  return clipCoord;\n"
                        "}\n"
                        "\n"

                        "void main()\n"
                        "{\n"
                        "  int i = 0;\n"
                        "  //VTK::Picking::Impl\n"

                        "  FPROPID = PROPID[0];\n"
                        "  framecolorsFS = framecolorsGS[0];\n"
                        "  int frameWidth = FrameWidths[FPROPID];\n"
                        "  int descender = Descenders[FPROPID];\n"

                        "  ivec4 glyphExt = glyphExtentsGS[0];\n"
                        "  ivec2 glyphGeom = ivec2(glyphExt[1] - glyphExt[0] + 1 -2*@PADSZ@,\n"
                        "                          glyphExt[3] - glyphExt[2] + 1 -2*@PADSZ@);\n"

                        "  vec2 tcMin = vec2((glyphExt[0] +@PADSZ@) / float(atlasDims.x),\n"
                        "                    (glyphExt[2] +@PADSZ@) / float(atlasDims.y));\n"
                        "  vec2 tcMax = vec2((glyphExt[1] + 1 -@PADSZ@) / float(atlasDims.x),\n"
                        "                    (glyphExt[3] + 1 -@PADSZ@) / float(atlasDims.y));\n"

                        "  vec4 anchor = ClipCoordToDisplayCoord(gl_in[0].gl_Position);\n"
                        "  anchor.x += round(COFF[0]);\n"
                        "  anchor.xy += vec2(DisplayOffset.x, DisplayOffset.y);\n"

                        "  int acenterX = 0;\n"
                        "  int acenterY = 0;\n"
                        "  if (anchorCenter[0]<0)\n"
                        "    {acenterX = frameWidth + 1 + descender; }\n"
                        "  if (anchorCenter[0]==0)\n"
                        "    {acenterX = 0; }\n"
                        "  if (anchorCenter[0]>0)\n"
                        "    {acenterX = -(frameWidth + 1 + descender); }\n"
                        "  if (anchorCenter[1]<0)\n"
                        "    {acenterY = frameWidth; }\n"
                        "  if (anchorCenter[1]==0)\n"
                        "    {acenterY = -(descender+glyphGeom.y)/2; }\n"
                        "  if (anchorCenter[1]>0)\n"
                        "    {acenterY = -(frameWidth+descender+glyphGeom.y); }\n"
                        "  anchor.xy += vec2(acenterX, acenterY);\n"

                        "  anchor.xy = floor(anchor.xy);\n"

                        "  vec4 blDisp = anchor;\n"
                        "  vec4 trDisp = vec4(anchor.xy + glyphGeom.xy, anchor.zw);\n"
                        "  vec4 brDisp = vec4(trDisp.x, anchor.yzw);\n"
                        "  vec4 tlDisp = vec4(anchor.x, trDisp.y, anchor.zw);\n"

                        "  vec4 bl = DisplayCoordToClipCoord(blDisp);\n"
                        "  vec4 tr = DisplayCoordToClipCoord(trDisp);\n"
                        "  vec4 br = DisplayCoordToClipCoord(brDisp);\n"
                        "  vec4 tl = DisplayCoordToClipCoord(tlDisp);\n"

                        "  layer = 0;\n"

                        "  UV2 = tcMin;\n"
                        "  gl_Position = bl;\n"
                        "  EmitVertex();\n"
                        "  UV2 = vec2(tcMax.x, tcMin.y);\n"
                        "  gl_Position = br;\n"
                        "  EmitVertex();\n"
                        "  UV2 = tcMax;\n"
                        "  gl_Position = tr;\n"
                        "  EmitVertex();\n"
                        "  EndPrimitive();\n"

                        "  UV2 = tcMin;\n"
                        "  gl_Position = bl;\n"
                        "  EmitVertex();\n"
                        "  UV2 = tcMax;\n"
                        "  gl_Position = tr;\n"
                        "  EmitVertex();\n"
                        "  UV2 = vec2(tcMin.x, tcMax.y);\n"
                        "  gl_Position = tl;\n"
                        "  EmitVertex();\n"
                        "  EndPrimitive();\n"

                        "  layer = 1;\n"

                        "  int bgHeight = MaxGlyphHeights[FPROPID];\n"
                        "  blDisp.x -= 1;\n"
                        "  tlDisp.x -= 1;\n"
                        "  tlDisp.y = blDisp.y + bgHeight;\n"
                        "  trDisp.y = brDisp.y + bgHeight;\n"

                        "  blDisp.x -= descender;\n"
                        "  brDisp.x += descender;\n"
                        "  tlDisp.x -= descender;\n"
                        "  tlDisp.y += descender;\n"
                        "  trDisp.xy += ivec2(descender);\n"

                        "  bl = DisplayCoordToClipCoord(blDisp);\n"
                        "  br = DisplayCoordToClipCoord(brDisp);\n"
                        "  tl = DisplayCoordToClipCoord(tlDisp);\n"
                        "  tr = DisplayCoordToClipCoord(trDisp);\n"

                        "  gl_Position = tl;\n"
                        "  EmitVertex();\n"
                        "  gl_Position = tr;\n"
                        "  EmitVertex();\n"
                        "  gl_Position = bl;\n"
                        "  EmitVertex();\n"
                        "  EndPrimitive();\n"

                        "  gl_Position = tr;\n"
                        "  EmitVertex();\n"
                        "  gl_Position = bl;\n"
                        "  EmitVertex();\n"
                        "  gl_Position = br;\n"
                        "  EmitVertex();\n"
                        "  EndPrimitive();\n"

                        "  if (frameWidth > 0)\n"
                        "  {\n"
                        "    layer = 2;\n"
                        "    blDisp.xy -= vec2(frameWidth);\n"
                        "    trDisp.xy += vec2(frameWidth);\n"
                        "    brDisp.x = trDisp.x;\n"
                        "    brDisp.y = blDisp.y;\n"
                        "    tlDisp.x = blDisp.x;\n"
                        "    tlDisp.y = trDisp.y;\n"

                        "    bl = DisplayCoordToClipCoord(blDisp);\n"
                        "    tr = DisplayCoordToClipCoord(trDisp);\n"
                        "    br = DisplayCoordToClipCoord(brDisp);\n"
                        "    tl = DisplayCoordToClipCoord(tlDisp);\n"

                        "    gl_Position = tl;\n"
                        "    EmitVertex();\n"
                        "    gl_Position = tr;\n"
                        "    EmitVertex();\n"
                        "    gl_Position = bl;\n"
                        "    EmitVertex();\n"
                        "    EndPrimitive();\n"

                        "    gl_Position = tr;\n"
                        "    EmitVertex();\n"
                        "    gl_Position = bl;\n"
                        "    EmitVertex();\n"
                        "    gl_Position = br;\n"
                        "    EmitVertex();\n"
                        "    EndPrimitive();\n"
                        "  }\n"
                        "}\n");
  vtksys::SystemTools::ReplaceString(geomp_str, "@MAXPROPS@", smaxprops);
  vtksys::SystemTools::ReplaceString(geomp_str, "@PADSZ@", spadsz);
  sp->SetGeometryShaderCode(geomp_str.c_str());

  sp->AddShaderReplacement(vtkShader::Fragment, "//VTK::TCoord::Dec", true,
    "in vec2 UV2;\n"
    "flat in int FPROPID;\n"
    "flat in int layer;\n",
    false);
  sp->AddShaderReplacement(
    vtkShader::Fragment, "//VTK::Color::Impl", true, "//NO COLOR IMPL", false);
  sp->AddShaderReplacement(
    vtkShader::Fragment, "//VTK::Normal::Impl", true, "//NO NORMAL IMPL", false);
  sp->AddShaderReplacement(vtkShader::Fragment, "//VTK::Coincident::Dec", true,
    "float cscale = length(vec2(dFdx(gl_FragCoord.z),dFdy(gl_FragCoord.z)));\n", false);
  sp->AddShaderReplacement(vtkShader::Fragment, "//VTK::Depth::Impl", true,
    "if (layer == 0) {\n"
    "  gl_FragDepth = gl_FragCoord.z;\n"
    "}\n"
    "if (layer == 1) {\n"
    "  gl_FragDepth = gl_FragCoord.z + 2*cscale + 0.000016*2.0;\n"
    "}\n"
    "if (layer == 2) {\n"
    "  gl_FragDepth = gl_FragCoord.z + 2*cscale + 0.000016*4.0;\n"
    "}\n",
    false);
  sp->AddShaderReplacement(vtkShader::Fragment, "//VTK::Light::Dec", true,
    std::string("uniform vec4 BackgroundColors[") + smaxprops +
      "];\n"
      "uniform sampler2D atlasTex;\n"
      "flat in vec3 framecolorsFS;\n"
      "//VTK::Light::Dec",
    false);
  sp->AddShaderReplacement(vtkShader::Fragment, "//VTK::Light::Impl", true,
    "if (layer == 0) {\n"
    " gl_FragData[0] = texture(atlasTex, UV2);\n"
    "}\n"
    "if (layer == 1) {\n"
    " gl_FragData[0] = BackgroundColors[FPROPID];\n"
    "}\n"
    "if (layer == 2) {\n"
    " gl_FragData[0] = vec4(framecolorsFS,1);\n"
    "}\n",
    false);
  sp->AddShaderReplacement(
    vtkShader::Fragment, "//VTK::TCoord::Impl", true, "//NO TCOORD IMPL", false);
}

//----------------------------------------------------------------------------
void vtkOpenGLBatchedLabeledDataMapperInternals::BuildShaders(
  std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* actor)
{
  // OpenGL uses a geometry shader to expand each point into up to 3 quads (glyph, background,
  // frame). WebGPU has no geometry shaders; it instead uses instanced rendering with
  // 18 vertices per instance (3 layers × 6 vertices) driven by the vertex shader.
  auto* sp = vtkOpenGLShaderProperty::SafeDownCast(actor->GetShaderProperty());
  if (sp)
  {
    MakeupShaders(sp);
  }
  this->Superclass::BuildShaders(shaders, ren, actor);
}

//----------------------------------------------------------------------------
// Uploads per-frame uniform data to the shader program. This is the OpenGL
// equivalent of vtkWebGPUBatchedLabeledDataMapperInternals::UpdateUniformBuffer,
// which writes the same data into a LabelUniforms2D struct in a WebGPU uniform buffer.
// Both functions must stay in sync: atlasDims, vpDims, winDims, anchorCenter, vp, nvp,
// displayOffset, backgroundColors, frameWidths, maxGlyphHeights, descenders.
void vtkOpenGLBatchedLabeledDataMapperInternals::SetMapperShaderParameters(
  vtkOpenGLHelper& cellBO, vtkRenderer* ren, vtkActor* actor)
{
  if (!this->Parent)
  {
    this->Superclass::SetMapperShaderParameters(cellBO, ren, actor);
    return;
  }

  vtkShaderProgram* program = cellBO.Program;

  vtkImageData* atlas = this->Parent->GetGlyphAtlas();
  if (atlas)
  {
    program->SetUniform2i("atlasDims", atlas->GetDimensions());
  }
  program->SetUniform2i("vpDims", ren->GetSize());
  program->SetUniform2i("winDims", ren->GetRenderWindow()->GetSize());
  program->SetUniformi("atlasTex", this->Parent->GlyphsTO->GetTextureUnit());

  int anchorCenter[2] = { -1, -1 };
  switch (this->Parent->GetTextAnchor())
  {
    case vtkBatchedLabeledDataMapper::LowerLeft:
    default:
      break;
    case vtkBatchedLabeledDataMapper::LowerEdge:
      anchorCenter[0] = 0;
      break;
    case vtkBatchedLabeledDataMapper::LowerRight:
      anchorCenter[0] = 1;
      break;
    case vtkBatchedLabeledDataMapper::LeftEdge:
      anchorCenter[1] = 0;
      break;
    case vtkBatchedLabeledDataMapper::Center:
      anchorCenter[0] = 0;
      anchorCenter[1] = 0;
      break;
    case vtkBatchedLabeledDataMapper::RightEdge:
      anchorCenter[0] = 1;
      anchorCenter[1] = 0;
      break;
    case vtkBatchedLabeledDataMapper::UpperLeft:
      anchorCenter[1] = 1;
      break;
    case vtkBatchedLabeledDataMapper::UpperEdge:
      anchorCenter[0] = 0;
      anchorCenter[1] = 1;
      break;
    case vtkBatchedLabeledDataMapper::UpperRight:
      anchorCenter[0] = 1;
      anchorCenter[1] = 1;
      break;
  }
  program->SetUniform2i("anchorCenter", anchorCenter);

  double vp[4];
  ren->GetViewport(vp);
  float vpf[4] = { static_cast<float>(vp[0]), static_cast<float>(vp[1]), static_cast<float>(vp[2]),
    static_cast<float>(vp[3]) };
  program->SetUniform4f("vp", vpf);

  double tileVP[4];
  ren->GetRenderWindow()->GetTileViewport(tileVP);
  float nvpf[4] = { static_cast<float>(std::max(vp[0], tileVP[0])),
    static_cast<float>(std::max(vp[1], tileVP[1])), static_cast<float>(std::min(vp[2], tileVP[2])),
    static_cast<float>(std::min(vp[3], tileVP[3])) };
  program->SetUniform4f("nvp", nvpf);

  program->SetUniform2i("DisplayOffset", this->Parent->GetDisplayOffset());

  program->SetUniform4fv("BackgroundColors", vtkBatchedLabeledDataMapper::MaxTextProperties,
    this->Parent->GetBackgroundColors());
  program->SetUniform1iv(
    "FrameWidths", vtkBatchedLabeledDataMapper::MaxTextProperties, this->Parent->GetFrameWidths());
  program->SetUniform1iv("MaxGlyphHeights", vtkBatchedLabeledDataMapper::MaxTextProperties,
    this->Parent->GetMaxGlyphHeights());
  program->SetUniform1iv(
    "Descenders", vtkBatchedLabeledDataMapper::MaxTextProperties, this->Parent->GetDescenders());

  this->Superclass::SetMapperShaderParameters(cellBO, ren, actor);
}

VTK_ABI_NAMESPACE_END
