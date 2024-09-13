// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOpenGLShaderCache.h"
#include "vtk_glad.h"

#include "vtkObjectFactory.h"
#include "vtkOpenGLHelper.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkShader.h"
#include "vtkShaderProgram.h"

#include <sstream>

#include "vtksys/MD5.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLShaderCache::Private
{
public:
  vtksysMD5* md5;

  // map of hash to shader program structs
  std::map<std::string, vtkShaderProgram*> ShaderPrograms;

  Private() { md5 = vtksysMD5_New(); }

  ~Private() { vtksysMD5_Delete(this->md5); }

  //-----------------------------------------------------------------------------
  void ComputeMD5(std::initializer_list<const char*> contents, std::string& hash)
  {
    unsigned char digest[16];
    char md5Hash[33];
    md5Hash[32] = '\0';

    vtksysMD5_Initialize(this->md5);
    for (const auto& content : contents)
    {
      if (content)
      {
        vtksysMD5_Append(this->md5, reinterpret_cast<const unsigned char*>(content),
          static_cast<int>(strlen(content)));
      }
    }
    vtksysMD5_Finalize(this->md5, digest);
    vtksysMD5_DigestToHex(digest, md5Hash);

    hash = md5Hash;
  }
};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLShaderCache);

//------------------------------------------------------------------------------
vtkOpenGLShaderCache::vtkOpenGLShaderCache()
  : Internal(new Private)
{
  this->LastShaderBound = nullptr;
  this->OpenGLMajorVersion = 0;
  this->OpenGLMinorVersion = 0;
  this->SyncGLSLShaderVersion = false;
}

//------------------------------------------------------------------------------
vtkOpenGLShaderCache::~vtkOpenGLShaderCache()
{
  typedef std::map<std::string, vtkShaderProgram*>::const_iterator SMapIter;
  SMapIter iter = this->Internal->ShaderPrograms.begin();
  for (; iter != this->Internal->ShaderPrograms.end(); ++iter)
  {
    iter->second->Delete();
  }

  delete this->Internal;
}

// perform System and Output replacements
unsigned int vtkOpenGLShaderCache::ReplaceShaderValues(std::string& VSSource, std::string& FSSource,
  std::string& GSSource, std::string& TCSSource, std::string& TESSource)
{
  // first handle renaming any Fragment shader inputs
  // if we have a geometry shader. By default fragment shaders
  // assume their inputs come from a Vertex Shader. When we
  // have a Geometry shader we rename the fragment shader inputs
  // to come from the geometry shader
  if (!GSSource.empty())
  {
    vtkShaderProgram::Substitute(FSSource, "VSOut", "GSOut");
  }
  // otherwise, if there is a tessellation evaluation shader, rename the inputs
  // to come from TES.
  else if (!TESSource.empty())
  {
    vtkShaderProgram::Substitute(FSSource, "VSOut", "TESOut");
  }

#ifdef GL_ES_VERSION_3_0
  std::string version = "#version 300 es\n";
#else
  if (!this->OpenGLMajorVersion)
  {
    this->OpenGLMajorVersion = 3;
    this->OpenGLMinorVersion = 2;
    glGetIntegerv(GL_MAJOR_VERSION, &this->OpenGLMajorVersion);
    glGetIntegerv(GL_MINOR_VERSION, &this->OpenGLMinorVersion);
  }

  std::string version = "#version 150\n";
  if (this->OpenGLMajorVersion == 3)
  {
    if (this->OpenGLMinorVersion == 1)
    {
      version = "#version 140\n";
    }
    else if (this->SyncGLSLShaderVersion && this->OpenGLMinorVersion > 2)
    {
      std::stringstream ss;
      ss << "#version " << this->OpenGLMajorVersion << this->OpenGLMinorVersion << "0\n";
      version = ss.str();
    }
  }
  else if (this->SyncGLSLShaderVersion)
  {
    std::stringstream ss;
    ss << "#version " << this->OpenGLMajorVersion << this->OpenGLMinorVersion << "0\n";
    version = ss.str();
  }
#endif

  vtkShaderProgram::Substitute(VSSource, "//VTK::System::Dec",
    version +
      "#ifndef GL_ES\n"
      "#define highp\n"
      "#define mediump\n"
      "#define lowp\n"
#ifdef GL_ES_VERSION_3_0
      "#else\n"
      "#define texelFetchBuffer(a,b) texelFetch(a, Get2DIndexFrom1DIndex(b, textureSize(a, 0)), "
      "0)\n"
#else
      "#define texelFetchBuffer texelFetch\n"
#endif
      "#endif // GL_ES\n"
      "#define attribute in\n" // to be safe
      "#define varying out\n"  // to be safe
#ifdef GL_ES_VERSION_3_0
      "ivec2 Get2DIndexFrom1DIndex(int idx, ivec2 texSize)\n"
      "{\n"
      "  int w = texSize.x;\n"
      "  int i = idx % w;\n"
      "  int j = (idx - i) / texSize.x;\n"
      "  return ivec2(i, j);\n"
      "}"
#endif
  );

  vtkShaderProgram::Substitute(FSSource, "//VTK::System::Dec",
    version +
      "#ifdef GL_ES\n"
      "#ifdef GL_FRAGMENT_PRECISION_HIGH\n"
      "precision highp float;\n"
      "precision highp sampler2D;\n"
      "precision highp sampler3D;\n"
      "#else\n"
      "precision mediump float;\n"
      "precision mediump sampler2D;\n"
      "precision mediump sampler3D;\n"
      "#endif\n"
#ifdef GL_ES_VERSION_3_0
      "#define texelFetchBuffer(a,b) texelFetch(a, Get2DIndexFrom1DIndex(b, textureSize(a, 0)), "
      "0)\n"
#else
      "#define texelFetchBuffer texelFetch\n"
#endif
      "#define texture1D texture\n"
      "#define texture2D texture\n"
      "#define texture3D texture\n"
      "#else // GL_ES\n"
      "#define highp\n"
      "#define mediump\n"
      "#define lowp\n"
      "#if __VERSION__ >= 150\n"
      "#define texelFetchBuffer texelFetch\n"
      "#define texture1D texture\n"
      "#define texture2D texture\n"
      "#define texture3D texture\n"
      "#endif\n"
      "#endif // GL_ES\n"
      "#define varying in\n" // to be safe
#ifdef GL_ES_VERSION_3_0
      "ivec2 Get2DIndexFrom1DIndex(int idx, ivec2 texSize)\n"
      "{\n"
      "  int w = texSize.x;\n"
      "  int i = idx % w;\n"
      "  int j = (idx - i) / texSize.x;\n"
      "  return ivec2(i, j);\n"
      "}"
#endif
  );

  vtkShaderProgram::Substitute(GSSource, "//VTK::System::Dec",
    version +
      "#ifdef GL_ES\n"
      "#ifdef GL_FRAGMENT_PRECISION_HIGH\n"
      "precision highp float;\n"
      "#else\n"
      "precision mediump float;\n"
      "#endif\n"
      "#else // GL_ES\n"
      "#define highp\n"
      "#define mediump\n"
      "#define lowp\n"
      "#endif // GL_ES\n");

  for (std::string* tessSource : { &TCSSource, &TESSource })
  {
    std::string extensionEnable;
    if (this->OpenGLMajorVersion < 4)
    {
      extensionEnable = "#extension GL_ARB_tessellation_shader : enable\n";
    }
    vtkShaderProgram::Substitute(*tessSource, "//VTK::System::Dec",
      version + extensionEnable +
        "#if __VERSION__ >= 150\n"
        "#define texelFetchBuffer texelFetch\n"
        "#endif\n");
  }

  unsigned int count = 0;
  std::string fragDecls;
  bool done = false;
  while (!done)
  {
    std::ostringstream src;
    std::ostringstream dst;
    src << "gl_FragData[" << count << "]";
    // this naming has to match the bindings
    // in vtkOpenGLShaderProgram.cxx
    dst << "fragOutput" << count;
    done = !vtkShaderProgram::Substitute(FSSource, src.str(), dst.str());
    if (!done)
    {
#ifdef GL_ES_VERSION_3_0
      src.str("");
      src.clear();
      src << count;
      fragDecls += "layout(location = " + src.str() + ") ";
#endif
      fragDecls += "out vec4 " + dst.str() + ";\n";
      count++;
    }
  }
#ifdef GL_ES_VERSION_3_0
  // Emulate texture buffers with 2D textures.
  vtkShaderProgram::Substitute(VSSource, "samplerBuffer", "sampler2D");
  vtkShaderProgram::Substitute(FSSource, "samplerBuffer", "sampler2D");
#endif
  vtkShaderProgram::Substitute(FSSource, "//VTK::Output::Dec", fragDecls);
  return count;
}

vtkShaderProgram* vtkOpenGLShaderCache::ReadyShaderProgram(
  std::map<vtkShader::Type, vtkShader*> shaders, vtkTransformFeedback* cap)
{
  vtkShader* vertShader = nullptr;
  vtkShader* fragShader = nullptr;
  // vertex and fragment shader must always be provided.
  auto vsIter = shaders.find(vtkShader::Vertex);
  if (vsIter != shaders.end())
  {
    vertShader = vsIter->second;
  }
  else
  {
    vtkErrorMacro(<< "A vertex shader is required!");
    return nullptr;
  }
  auto fsIter = shaders.find(vtkShader::Fragment);
  if (fsIter != shaders.end())
  {
    fragShader = fsIter->second;
  }
  else
  {
    vtkErrorMacro(<< "A fragment shader is required!");
    return nullptr;
  }
  // now prepare placeholders for optional shaders.
  vtkSmartPointer<vtkShader> geomShader, tcShader, teShader;
  auto gsIter = shaders.find(vtkShader::Geometry);
  if (gsIter != shaders.end())
  {
    geomShader = gsIter->second;
  }
  else
  {
    geomShader = vtk::TakeSmartPointer(vtkShader::New());
    shaders[vtkShader::Geometry] = geomShader;
  }
  auto tcsIter = shaders.find(vtkShader::TessControl);
  if (tcsIter != shaders.end())
  {
    tcShader = tcsIter->second;
  }
  else
  {
    tcShader = vtk::TakeSmartPointer(vtkShader::New());
    shaders[vtkShader::TessControl] = tcShader;
  }
  auto tesIter = shaders.find(vtkShader::TessEvaluation);
  if (tesIter != shaders.end())
  {
    teShader = tesIter->second;
  }
  else
  {
    teShader = vtk::TakeSmartPointer(vtkShader::New());
    shaders[vtkShader::TessEvaluation] = teShader;
  }
  std::string VSSource = vertShader->GetSource();
  std::string FSSource = fragShader->GetSource();
  std::string GSSource = geomShader->GetSource();
  std::string TCSSource = tcShader->GetSource();
  std::string TESSource = teShader->GetSource();

  unsigned int count =
    this->ReplaceShaderValues(VSSource, FSSource, GSSource, TCSSource, TESSource);
  vertShader->SetSource(VSSource);
  fragShader->SetSource(FSSource);
  geomShader->SetSource(GSSource);
  tcShader->SetSource(TCSSource);
  teShader->SetSource(TESSource);

  vtkShaderProgram* shader = this->GetShaderProgram(shaders);
  shader->SetNumberOfOutputs(count);

  return this->ReadyShaderProgram(shader, cap);
}

// return nullptr if there is an issue
vtkShaderProgram* vtkOpenGLShaderCache::ReadyShaderProgram(const char* vertexCode,
  const char* fragmentCode, const char* geometryCode, vtkTransformFeedback* cap)
{
  return this->ReadyShaderProgram(vertexCode, fragmentCode, geometryCode, "", "", cap);
}

// return nullptr if there is an issue
vtkShaderProgram* vtkOpenGLShaderCache::ReadyShaderProgram(const char* vertexCode,
  const char* fragmentCode, const char* geometryCode, const char* tessControlCode,
  const char* tessEvalCode, vtkTransformFeedback* cap)
{
  // perform system wide shader replacements
  // desktops to not use precision statements
  std::string VSSource = vertexCode ? vertexCode : "";
  std::string FSSource = fragmentCode ? fragmentCode : "";
  std::string GSSource = geometryCode ? geometryCode : "";
  std::string TCSSource = tessControlCode ? tessControlCode : "";
  std::string TESSource = tessEvalCode ? tessEvalCode : "";

  unsigned int count =
    this->ReplaceShaderValues(VSSource, FSSource, GSSource, TCSSource, TESSource);
  vtkShaderProgram* shader = this->GetShaderProgram(
    VSSource.c_str(), FSSource.c_str(), GSSource.c_str(), TCSSource.c_str(), TESSource.c_str());
  shader->SetNumberOfOutputs(count);

  return this->ReadyShaderProgram(shader, cap);
}

// return nullptr if there is an issue
vtkShaderProgram* vtkOpenGLShaderCache::ReadyShaderProgram(
  vtkShaderProgram* shader, vtkTransformFeedback* cap)
{
  if (!shader)
  {
    return nullptr;
  }

  if (shader->GetTransformFeedback() != cap)
  {
    this->ReleaseCurrentShader();
    shader->ReleaseGraphicsResources(nullptr);
    shader->SetTransformFeedback(cap);
  }

  // compile if needed
  if (!shader->GetCompiled() && !shader->CompileShader())
  {
    return nullptr;
  }

  // bind if needed
  if (!this->BindShader(shader))
  {
    return nullptr;
  }

  return shader;
}

vtkShaderProgram* vtkOpenGLShaderCache::GetShaderProgram(
  std::map<vtkShader::Type, vtkShader*> shaders)
{
  // compute the MD5 and the check the map
  std::string result;
  this->Internal->ComputeMD5({ shaders[vtkShader::Vertex]->GetSource().c_str(),
                               shaders[vtkShader::Fragment]->GetSource().c_str(),
                               shaders[vtkShader::Geometry]->GetSource().c_str(),
                               shaders[vtkShader::TessControl]->GetSource().c_str(),
                               shaders[vtkShader::TessEvaluation]->GetSource().c_str() },
    result);

  // does it already exist?
  typedef std::map<std::string, vtkShaderProgram*>::const_iterator SMapIter;
  SMapIter found = this->Internal->ShaderPrograms.find(result);
  if (found == this->Internal->ShaderPrograms.end())
  {
    // create one
    vtkShaderProgram* sps = vtkShaderProgram::New();
    sps->SetVertexShader(shaders[vtkShader::Vertex]);
    sps->SetFragmentShader(shaders[vtkShader::Fragment]);
    sps->SetGeometryShader(shaders[vtkShader::Geometry]);
    sps->SetTessControlShader(shaders[vtkShader::TessControl]);
    sps->SetTessEvaluationShader(shaders[vtkShader::TessEvaluation]);
    sps->SetMD5Hash(result); // needed?
    this->Internal->ShaderPrograms.insert(std::make_pair(result, sps));
    return sps;
  }
  else
  {
    return found->second;
  }
}

vtkShaderProgram* vtkOpenGLShaderCache::GetShaderProgram(const char* vertexCode,
  const char* fragmentCode, const char* geometryCode, const char* tessControlCode,
  const char* tessEvalCode)
{
  // compute the MD5 and the check the map
  std::string result;
  this->Internal->ComputeMD5(
    { vertexCode, fragmentCode, geometryCode, tessControlCode, tessEvalCode }, result);

  // does it already exist?
  typedef std::map<std::string, vtkShaderProgram*>::const_iterator SMapIter;
  SMapIter found = this->Internal->ShaderPrograms.find(result);
  if (found == this->Internal->ShaderPrograms.end())
  {
    // create one
    vtkShaderProgram* sps = vtkShaderProgram::New();
    sps->GetVertexShader()->SetSource(vertexCode);
    sps->GetFragmentShader()->SetSource(fragmentCode);
    if (geometryCode != nullptr)
    {
      sps->GetGeometryShader()->SetSource(geometryCode);
    }
    if (tessControlCode != nullptr)
    {
      sps->GetTessControlShader()->SetSource(tessControlCode);
    }
    if (tessEvalCode != nullptr)
    {
      sps->GetTessEvaluationShader()->SetSource(tessEvalCode);
    }
    sps->SetMD5Hash(result); // needed?
    this->Internal->ShaderPrograms.insert(std::make_pair(result, sps));
    return sps;
  }
  else
  {
    return found->second;
  }
}

void vtkOpenGLShaderCache::ReleaseGraphicsResources(vtkWindow* win)
{
  // NOTE:
  // In the current implementation as of October 26th, if a shader
  // program is created by ShaderCache then it should make sure
  // that it releases the graphics resources used by these programs.
  // It is not wisely for callers to do that since then they would
  // have to loop over all the programs were in use and invoke
  // release graphics resources individually.

  this->ReleaseCurrentShader();

  typedef std::map<std::string, vtkShaderProgram*>::const_iterator SMapIter;
  SMapIter iter = this->Internal->ShaderPrograms.begin();
  for (; iter != this->Internal->ShaderPrograms.end(); ++iter)
  {
    iter->second->ReleaseGraphicsResources(win);
  }
  this->OpenGLMajorVersion = 0;
}

void vtkOpenGLShaderCache::ReleaseCurrentShader()
{
  // release prior shader
  if (this->LastShaderBound)
  {
    this->LastShaderBound->Release();
    this->LastShaderBound = nullptr;
  }
}

int vtkOpenGLShaderCache::BindShader(vtkShaderProgram* shader)
{
  if (this->LastShaderBound != shader)
  {
    // release prior shader
    if (this->LastShaderBound)
    {
      this->LastShaderBound->Release();
    }
    shader->Bind();
    this->LastShaderBound = shader;
  }

  if (shader->IsUniformUsed("vtkElapsedTime"))
  {
    shader->SetUniformf("vtkElapsedTime", this->ElapsedTime);
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkOpenGLShaderCache::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
