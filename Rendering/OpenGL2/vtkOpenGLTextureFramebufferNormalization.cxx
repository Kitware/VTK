// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkOpenGLTextureFramebufferNormalization.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderWindow.h"

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkOpenGLTextureFramebufferNormalization);

vtkOpenGLTextureFramebufferNormalization::vtkOpenGLTextureFramebufferNormalization()
{
  this->Mode = ConversionMode::CopyTexImage;
}

vtkOpenGLTextureFramebufferNormalization::~vtkOpenGLTextureFramebufferNormalization()
{
  if (this->ConversionProgram)
  {
    glDeleteProgram(this->ConversionProgram);
  }
  if (this->ConversionFramebuffer)
  {
    glDeleteFramebuffers(1, &this->ConversionFramebuffer);
  }
  if (this->IntermediateTexture)
  {
    glDeleteTextures(1, &this->IntermediateTexture);
  }
  if (this->ConversionVAO)
  {
    glDeleteVertexArrays(1, &this->ConversionVAO);
  }
}

void vtkOpenGLTextureFramebufferNormalization::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkOpenGLTextureFramebufferNormalization::Initialize(vtkOpenGLRenderWindow* context)
{
  if (!context)
  {
    this->Mode = ConversionMode::Unsupported;
    return;
  }

  this->Context = context;
  context->MakeCurrent();

  if (!this->InitializeConversionShader())
  {
    this->Mode = ConversionMode::Unsupported;
  }
}

bool vtkOpenGLTextureFramebufferNormalization::InitializeConversionShader()
{
  // Vertex shader: renders a fullscreen quad using gl_VertexID
  const char* vertexShaderSource = R"(
#version 300 es
precision highp float;

const vec2 vertices[4] = vec2[](
  vec2(-1.0, -1.0),
  vec2( 1.0, -1.0),
  vec2( 1.0,  1.0),
  vec2(-1.0,  1.0)
);

out vec2 texCoord;

void main() {
  gl_Position = vec4(vertices[gl_VertexID], 0.0, 1.0);
  texCoord = (vertices[gl_VertexID] + 1.0) * 0.5;
}
)";

  // Fragment shader: reads unsigned integer texture, normalizes to float
  const char* fragmentShaderSource = R"(
#version 300 es
precision highp float;

in vec2 texCoord;
out vec4 fragColor;

uniform usampler2D inputTexture;
uniform float scaleFactor;

void main() {
  // Read unsigned integer value
  uint value = texture(inputTexture, texCoord).x;

  // Normalize to float
  float normalized = float(value) * scaleFactor;

  fragColor = vec4(normalized, 0.0, 0.0, 1.0);
}
)";

  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
  glCompileShader(vertexShader);

  GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
  glCompileShader(fragmentShader);

  // Check compilation
  GLint compiled = 0;
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &compiled);
  if (!compiled)
  {
    vtkOpenGLStaticCheckErrorMacro("Failed to compile vertex shader");
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return false;
  }
  glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &compiled);
  if (!compiled)
  {
    vtkOpenGLStaticCheckErrorMacro("Failed to compile fragment shader");
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return false;
  }

  // Link program
  this->ConversionProgram = glCreateProgram();
  glAttachShader(this->ConversionProgram, vertexShader);
  glAttachShader(this->ConversionProgram, fragmentShader);
  glLinkProgram(this->ConversionProgram);

  GLint linked = 0;
  glGetProgramiv(this->ConversionProgram, GL_LINK_STATUS, &linked);
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  if (!linked)
  {
    vtkOpenGLStaticCheckErrorMacro("Failed to link conversion program");
    glDeleteProgram(this->ConversionProgram);
    this->ConversionProgram = 0;
    return false;
  }

  // Create VAO for fullscreen quad
  glGenVertexArrays(1, &this->ConversionVAO);

  return true;
}

bool vtkOpenGLTextureFramebufferNormalization::UploadToIntermediateTexture(const void* sourceData,
  size_t vtkNotUsed(numValues), int vtkNotUsed(numComps), unsigned int width, unsigned int height,
  GLenum internalFormat, GLenum dataFormat)
{
  if (!this->IntermediateTexture)
  {
    glGenTextures(1, &this->IntermediateTexture);
  }

  glBindTexture(GL_TEXTURE_2D, this->IntermediateTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, static_cast<GLsizei>(width),
    static_cast<GLsizei>(height), 0, dataFormat, GL_UNSIGNED_SHORT, sourceData);

  vtkOpenGLStaticCheckErrorMacro("Failed to upload intermediate texture");
  return true;
}

bool vtkOpenGLTextureFramebufferNormalization::PerformFramebufferConversion(
  unsigned int targetTexture, int vtkNotUsed(numComps), unsigned int width, unsigned int height,
  float scaleFactor)
{
  if (!this->ConversionFramebuffer)
  {
    glGenFramebuffers(1, &this->ConversionFramebuffer);
  }

  // Bind framebuffer and attach target texture
  glBindFramebuffer(GL_FRAMEBUFFER, this->ConversionFramebuffer);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, targetTexture, 0);

  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE)
  {
    vtkOpenGLStaticCheckErrorMacro("Framebuffer not complete");
    return false;
  }

  // Setup rendering state
  glViewport(0, 0, width, height);
  glUseProgram(this->ConversionProgram);

  // Bind input texture
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, this->IntermediateTexture);
  GLint texLoc = glGetUniformLocation(this->ConversionProgram, "inputTexture");
  glUniform1i(texLoc, 0);

  // Set scale factor
  GLint scaleLoc = glGetUniformLocation(this->ConversionProgram, "scaleFactor");
  glUniform1f(scaleLoc, scaleFactor);

  // Render fullscreen quad
  glBindVertexArray(this->ConversionVAO);
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

  vtkOpenGLStaticCheckErrorMacro("Framebuffer conversion render failed");
  return true;
}

bool vtkOpenGLTextureFramebufferNormalization::ConvertUShortToFloat(const void* sourceData,
  size_t numValues, int numComps, unsigned int targetTexture, unsigned int width,
  unsigned int height)
{
  if (!this->ConversionProgram || !sourceData || numValues == 0)
  {
    return false;
  }

  // Pre-allocate target texture
  GLenum outputFormat = (numComps == 1) ? GL_R32F
    : (numComps == 2)                   ? GL_RG32F
    : (numComps == 3)                   ? GL_RGB32F
                                        : GL_RGBA32F;
  glBindTexture(GL_TEXTURE_2D, targetTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, outputFormat, static_cast<GLsizei>(width),
    static_cast<GLsizei>(height), 0, GL_RED, GL_FLOAT, nullptr);

  // Upload to intermediate R16UI texture
  GLenum intermediateFormat = (numComps == 1) ? GL_R16UI
    : (numComps == 2)                         ? GL_RG16UI
    : (numComps == 3)                         ? GL_RGB16UI
                                              : GL_RGBA16UI;
  if (!this->UploadToIntermediateTexture(
        sourceData, numValues, numComps, width, height, intermediateFormat, GL_RED_INTEGER))
  {
    return false;
  }

  // Perform conversion via framebuffer
  return this->PerformFramebufferConversion(
    targetTexture, numComps, width, height, 1.0f / 65535.0f);
}

bool vtkOpenGLTextureFramebufferNormalization::ConvertShortToFloat(const void* sourceData,
  size_t numValues, int numComps, unsigned int targetTexture, unsigned int width,
  unsigned int height)
{
  if (!this->ConversionProgram || !sourceData || numValues == 0)
  {
    return false;
  }

  // Pre-allocate target texture
  GLenum outputFormat = (numComps == 1) ? GL_R32F
    : (numComps == 2)                   ? GL_RG32F
    : (numComps == 3)                   ? GL_RGB32F
                                        : GL_RGBA32F;
  glBindTexture(GL_TEXTURE_2D, targetTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, outputFormat, static_cast<GLsizei>(width),
    static_cast<GLsizei>(height), 0, GL_RED, GL_FLOAT, nullptr);

  // Upload to intermediate R16I texture
  GLenum intermediateFormat = (numComps == 1) ? GL_R16I
    : (numComps == 2)                         ? GL_RG16I
    : (numComps == 3)                         ? GL_RGB16I
                                              : GL_RGBA16I;
  if (!this->UploadToIntermediateTexture(
        sourceData, numValues, numComps, width, height, intermediateFormat, GL_RED_INTEGER))
  {
    return false;
  }

  // Perform conversion via framebuffer
  return this->PerformFramebufferConversion(
    targetTexture, numComps, width, height, 1.0f / 32767.0f);
}

VTK_ABI_NAMESPACE_END
