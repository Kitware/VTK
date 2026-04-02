/**
 * @file vtkOpenGLTextureNormalizationHelper.cxx
 * @brief GPU-based texture normalization implementation for GLES 3.0
 */

#include "vtkOpenGLTextureNormalizationHelper.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLState.h"
#include <algorithm>
#include <cstring>

// =============================================================================
// vtkOpenGLTextureNormalizationHelper - Factory Method
// =============================================================================

std::shared_ptr<vtkOpenGLTextureNormalizationHelper> vtkOpenGLTextureNormalizationHelper::Create(
  vtkOpenGLRenderWindow* context)
{
#ifdef GL_ES_VERSION_3_0
  if (!context)
  {
    return std::make_shared<vtkOpenGLTextureCPUNormalization>();
  }

  context->MakeCurrent();

  // Check for compute shader support (GL 4.3+ or ARB_compute_shader)
#ifdef GL_COMPUTE_SHADER
  if (GLAD_GL_ARB_compute_shader || GLAD_GL_VERSION_4_3)
  {
    auto helper = std::make_shared<vtkOpenGLTextureComputeShaderNormalization>(context);
    if (helper->GetConversionMode() !=
      vtkOpenGLTextureNormalizationHelper::ConversionMode::Unsupported)
    {
      return helper;
    }
  }
#endif

  // Fallback to framebuffer-based conversion
  auto helper = std::make_shared<vtkOpenGLTextureFramebufferNormalization>(context);
  if (helper->GetConversionMode() !=
    vtkOpenGLTextureNormalizationHelper::ConversionMode::Unsupported)
  {
    return helper;
  }

  // Final fallback: CPU conversion
  return std::make_shared<vtkOpenGLTextureCPUNormalization>();
#else
  // Desktop OpenGL: CPU conversion is fine, or use other strategies
  return std::make_shared<vtkOpenGLTextureCPUNormalization>();
#endif
}

// =============================================================================
// vtkOpenGLTextureCPUNormalization - CPU-based Fallback
// =============================================================================

bool vtkOpenGLTextureCPUNormalization::ConvertUShortToFloat(const void* sourceData,
  size_t numValues, int numComps, GLuint targetTexture, unsigned int width, unsigned int height)
{
  if (!sourceData || numValues == 0)
  {
    return false;
  }

  // Allocate or reuse buffer
  if (ConversionBuffer.size() < numValues)
  {
    ConversionBuffer.resize(numValues);
  }

  // Convert unsigned short to float
  const unsigned short* src = static_cast<const unsigned short*>(sourceData);
  float* dst = ConversionBuffer.data();
  for (size_t i = 0; i < numValues; ++i)
  {
    dst[i] = src[i] / 65535.0f;
  }

  // Upload converted data
  glBindTexture(GL_TEXTURE_2D, targetTexture);
  GLenum format = (numComps == 1) ? GL_RED
    : (numComps == 2)             ? GL_RG
    : (numComps == 3)             ? GL_RGB
                                  : GL_RGBA;
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F + (numComps - 1), static_cast<GLsizei>(width),
    static_cast<GLsizei>(height), 0, format, GL_FLOAT, dst);

  vtkOpenGLCheckErrorMacro("Failed to upload converted texture data");
  return true;
}

bool vtkOpenGLTextureCPUNormalization::ConvertShortToFloat(const void* sourceData, size_t numValues,
  int numComps, GLuint targetTexture, unsigned int width, unsigned int height)
{
  if (!sourceData || numValues == 0)
  {
    return false;
  }

  // Allocate or reuse buffer
  if (ConversionBuffer.size() < numValues)
  {
    ConversionBuffer.resize(numValues);
  }

  // Convert signed short to float
  const short* src = static_cast<const short*>(sourceData);
  float* dst = ConversionBuffer.data();
  for (size_t i = 0; i < numValues; ++i)
  {
    dst[i] = src[i] / 32767.0f;
  }

  // Upload converted data
  glBindTexture(GL_TEXTURE_2D, targetTexture);
  GLenum format = (numComps == 1) ? GL_RED
    : (numComps == 2)             ? GL_RG
    : (numComps == 3)             ? GL_RGB
                                  : GL_RGBA;
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F + (numComps - 1), static_cast<GLsizei>(width),
    static_cast<GLsizei>(height), 0, format, GL_FLOAT, dst);

  vtkOpenGLCheckErrorMacro("Failed to upload converted texture data");
  return true;
}

// =============================================================================
// vtkOpenGLTextureComputeShaderNormalization - Compute Shader Implementation
// =============================================================================

vtkOpenGLTextureComputeShaderNormalization::vtkOpenGLTextureComputeShaderNormalization(
  vtkOpenGLRenderWindow* context)
  : vtkOpenGLTextureNormalizationHelper(ConversionMode::ComputeShader)
  , Context(context)
{
  if (!context)
  {
    return;
  }

  context->MakeCurrent();

  if (!InitializeComputeProgram())
  {
    // Fall back to CPU
    this->Mode = ConversionMode::Unsupported;
  }
}

vtkOpenGLTextureComputeShaderNormalization::~vtkOpenGLTextureComputeShaderNormalization()
{
  if (this->ComputeProgram)
  {
    glDeleteProgram(this->ComputeProgram);
  }
  if (this->IntermediateStorageTexture)
  {
    glDeleteTextures(1, &this->IntermediateStorageTexture);
  }
}

bool vtkOpenGLTextureComputeShaderNormalization::InitializeComputeProgram()
{
  // Compile compute shader that normalizes 16-bit integer to 32-bit float
  const char* computeShaderSource = R"(
#version 430
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(binding = 0, r16ui) uniform uimage2D inputImage;
layout(binding = 1, rgba32f) uniform image2D outputImage;

uniform float scaleFactor;

void main() {
  ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
  
  // Read 16-bit unsigned integer
  uint value = imageLoad(inputImage, coord).x;
  
  // Normalize
  float normalized = float(value) * scaleFactor;
  
  // Write to output
  imageStore(outputImage, coord, vec4(normalized, 0.0, 0.0, 1.0));
}
)";

  GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
  glShaderSource(computeShader, 1, &computeShaderSource, nullptr);
  glCompileShader(computeShader);

  // Check compilation
  GLint compiled = 0;
  glGetShaderiv(computeShader, GL_COMPILE_STATUS, &compiled);
  if (!compiled)
  {
    vtkOpenGLCheckErrorMacro("Failed to compile compute shader");
    glDeleteShader(computeShader);
    return false;
  }

  // Link program
  this->ComputeProgram = glCreateProgram();
  glAttachShader(this->ComputeProgram, computeShader);
  glLinkProgram(this->ComputeProgram);

  GLint linked = 0;
  glGetProgramiv(this->ComputeProgram, GL_LINK_STATUS, &linked);
  glDeleteShader(computeShader);

  if (!linked)
  {
    vtkOpenGLCheckErrorMacro("Failed to link compute program");
    glDeleteProgram(this->ComputeProgram);
    this->ComputeProgram = 0;
    return false;
  }

  return true;
}

bool vtkOpenGLTextureComputeShaderNormalization::UploadToIntermediateTexture(const void* sourceData,
  size_t numValues, int numComps, unsigned int width, unsigned int height, GLenum sourceFormat)
{
  // Create or reuse intermediate texture
  if (!this->IntermediateStorageTexture)
  {
    glGenTextures(1, &this->IntermediateStorageTexture);
  }

  glBindTexture(GL_TEXTURE_2D, this->IntermediateStorageTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  // Upload source data as integer texture
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R16UI, static_cast<GLsizei>(width),
    static_cast<GLsizei>(height), 0, GL_RED_INTEGER, GL_UNSIGNED_SHORT, sourceData);

  vtkOpenGLCheckErrorMacro("Failed to upload intermediate texture");
  return true;
}

bool vtkOpenGLTextureComputeShaderNormalization::RunNormalizationCompute(
  int numComps, GLuint targetTexture, unsigned int width, unsigned int height)
{
  glUseProgram(this->ComputeProgram);

  // Bind input texture
  glBindImageTexture(0, this->IntermediateStorageTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R16UI);

  // Bind output texture
  GLenum outputFormat = (numComps == 1) ? GL_R32F
    : (numComps == 2)                   ? GL_RG32F
    : (numComps == 3)                   ? GL_RGB32F
                                        : GL_RGBA32F;
  glBindImageTexture(1, targetTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, outputFormat);

  // Set scale factor
  GLint scaleLoc = glGetUniformLocation(this->ComputeProgram, "scaleFactor");
  glUniform1f(scaleLoc, 1.0f / 65535.0f);

  // Dispatch compute shader
  GLsizei groupSize = 8;
  glDispatchCompute((width + groupSize - 1) / groupSize, (height + groupSize - 1) / groupSize, 1);

  // Wait for computation
  glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT | GL_FRAMEBUFFER_BARRIER_BIT);

  vtkOpenGLCheckErrorMacro("Compute shader normalization failed");
  return true;
}

bool vtkOpenGLTextureComputeShaderNormalization::ConvertUShortToFloat(const void* sourceData,
  size_t numValues, int numComps, GLuint targetTexture, unsigned int width, unsigned int height)
{
  if (!this->ComputeProgram || !sourceData || numValues == 0)
  {
    return false;
  }

  // Pre-allocate target texture
  GLenum outputFormat = (numComps == 1) ? GL_R32F
    : (numComps == 2)                   ? GL_RG32F
    : (numComps == 3)                   ? GL_RGB32F
                                        : GL_RGBA32F;
  glBindTexture(GL_TEXTURE_2D, targetTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, outputFormat, static_cast<GLsizei>(width),
    static_cast<GLsizei>(height), 0, GL_RED, GL_FLOAT, nullptr);

  // Upload to intermediate R16UI texture
  if (!UploadToIntermediateTexture(sourceData, numValues, numComps, width, height, GL_RED_INTEGER))
  {
    return false;
  }

  // Run compute shader
  return RunNormalizationCompute(numComps, targetTexture, width, height);
}

bool vtkOpenGLTextureComputeShaderNormalization::ConvertShortToFloat(const void* sourceData,
  size_t numValues, int numComps, GLuint targetTexture, unsigned int width, unsigned int height)
{
  if (!this->ComputeProgram || !sourceData || numValues == 0)
  {
    return false;
  }

  // Similar to ConvertUShortToFloat but with signed values
  // Note: Would need separate compute shader variant for signed normalization
  // For now, delegate to CPU
  auto cpuConverter = std::make_shared<vtkOpenGLTextureCPUNormalization>();
  return cpuConverter->ConvertShortToFloat(
    sourceData, numValues, numComps, targetTexture, width, height);
}

// =============================================================================
// vtkOpenGLTextureFramebufferNormalization - Framebuffer-based Conversion
// =============================================================================

vtkOpenGLTextureFramebufferNormalization::vtkOpenGLTextureFramebufferNormalization(
  vtkOpenGLRenderWindow* context)
  : vtkOpenGLTextureNormalizationHelper(ConversionMode::CopyTexImage)
  , Context(context)
{
  if (!context)
  {
    return;
  }

  context->MakeCurrent();

  if (!InitializeConversionShader())
  {
    this->Mode = ConversionMode::Unsupported;
  }
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

bool vtkOpenGLTextureFramebufferNormalization::InitializeConversionShader()
{
  // Fragment shader that converts normalized integer to float
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
  if (!compiled || !glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &compiled))
  {
    vtkOpenGLCheckErrorMacro("Failed to compile conversion shaders");
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
    vtkOpenGLCheckErrorMacro("Failed to link conversion program");
    glDeleteProgram(this->ConversionProgram);
    this->ConversionProgram = 0;
    return false;
  }

  // Create VAO for fullscreen quad
  glGenVertexArrays(1, &this->ConversionVAO);

  return true;
}

bool vtkOpenGLTextureFramebufferNormalization::UploadToIntermediateTexture(const void* sourceData,
  size_t numValues, int numComps, unsigned int width, unsigned int height, GLenum internalFormat,
  GLenum dataFormat)
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

  vtkOpenGLCheckErrorMacro("Failed to upload intermediate texture");
  return true;
}

bool vtkOpenGLTextureFramebufferNormalization::PerformFramebufferConversion(
  GLuint targetTexture, int numComps, unsigned int width, unsigned int height, float scaleFactor)
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
    vtkOpenGLCheckErrorMacro("Framebuffer not complete");
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

  vtkOpenGLCheckErrorMacro("Framebuffer conversion render failed");
  return true;
}

bool vtkOpenGLTextureFramebufferNormalization::ConvertUShortToFloat(const void* sourceData,
  size_t numValues, int numComps, GLuint targetTexture, unsigned int width, unsigned int height)
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
  if (!UploadToIntermediateTexture(
        sourceData, numValues, numComps, width, height, intermediateFormat, GL_RED_INTEGER))
  {
    return false;
  }

  // Perform conversion via framebuffer
  return PerformFramebufferConversion(targetTexture, numComps, width, height, 1.0f / 65535.0f);
}

bool vtkOpenGLTextureFramebufferNormalization::ConvertShortToFloat(const void* sourceData,
  size_t numValues, int numComps, GLuint targetTexture, unsigned int width, unsigned int height)
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
  if (!UploadToIntermediateTexture(
        sourceData, numValues, numComps, width, height, intermediateFormat, GL_RED_INTEGER))
  {
    return false;
  }

  // Perform conversion via framebuffer
  return PerformFramebufferConversion(targetTexture, numComps, width, height, 1.0f / 32767.0f);
}
