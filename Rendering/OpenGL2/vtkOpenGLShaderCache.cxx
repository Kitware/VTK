/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLShaderCache.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLShaderCache.h"
#include "vtk_glew.h"

#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLError.h"

#include <math.h>

#include "vtkObjectFactory.h"

#include "vtkglVBOHelper.h"

#include "vtksys/MD5.h"

using vtkgl::replace;

class vtkOpenGLShaderCache::Private
{
public:
  vtksysMD5* md5;

  // map of hash to shader program structs
  std::map<std::string, CachedShaderProgram *> ShaderPrograms;

  Private()
  {
  md5 = vtksysMD5_New();
  }

  ~Private()
  {
  vtksysMD5_Delete(this->md5);
  }

  //-----------------------------------------------------------------------------
  void ComputeMD5(const char* content,
                  const char* content2,
                  const char* content3,
                  std::string &hash)
  {
    unsigned char digest[16];
    char md5Hash[33];
    md5Hash[32] = '\0';

    vtksysMD5_Initialize(this->md5);
    vtksysMD5_Append(this->md5, reinterpret_cast<const unsigned char *>(content), (int)strlen(content));
    vtksysMD5_Append(this->md5, reinterpret_cast<const unsigned char *>(content2), (int)strlen(content2));
    vtksysMD5_Append(this->md5, reinterpret_cast<const unsigned char *>(content3), (int)strlen(content3));
    vtksysMD5_Finalize(this->md5, digest);
    vtksysMD5_DigestToHex(digest, md5Hash);

    hash = md5Hash;
  }


};

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLShaderCache);

// ----------------------------------------------------------------------------
vtkOpenGLShaderCache::vtkOpenGLShaderCache() : Internal(new Private)
{
  this->LastShaderBound  = NULL;
}

// ----------------------------------------------------------------------------
vtkOpenGLShaderCache::~vtkOpenGLShaderCache()
{
  delete this->Internal;
}

// return NULL if there is an issue
vtkOpenGLShaderCache::CachedShaderProgram *vtkOpenGLShaderCache::ReadyShader(
  const char *vertexCode,
  const char *fragmentCode,
  const char *geometryCode)
{
  // perform system wide shader replacements
  // desktops to not use percision statements
#if GL_ES_VERSION_2_0 != 1
  std::string VSSource = vertexCode;
  VSSource = replace(VSSource,"//VTK::System::Dec",
                              "#define highp\n"
                              "#define mediump\n"
                              "#define lowp");
  std::string FSSource = fragmentCode;
  FSSource = replace(FSSource,"//VTK::System::Dec",
                              "#define highp\n"
                              "#define mediump\n"
                              "#define lowp");
  std::string GSSource = geometryCode;
  GSSource = replace(GSSource,"//VTK::System::Dec",
                              "#define highp\n"
                              "#define mediump\n"
                              "#define lowp");
  CachedShaderProgram *shader = this->GetShader(VSSource.c_str(), FSSource.c_str(), GSSource.c_str());
#else
  std::string FSSource = fragmentCode;
  FSSource = replace(FSSource,"//VTK::System::Dec",
                               "#ifdef GL_ES\n"
                               "#ifdef GL_FRAGMENT_PRECISION_HIGH\n"
                               "precision highp float;\n"
                               "#else\n"
                               "precision mediump float;\n"
                               "#endif\n"
                               "#endif\n");
  CachedShaderProgram *shader = this->GetShader(vertexCode, FSSource.c_str(), geometryCode);
#endif

  if (!shader)
    {
    return NULL;
    }

  // compile if needed
  if (!shader->Compiled && !this->CompileShader(shader))
    {
    return NULL;
    }

  // bind if needed
  if (!this->BindShader(shader))
    {
    return NULL;
    }

  return shader;
}

// return NULL if there is an issue
vtkOpenGLShaderCache::CachedShaderProgram *vtkOpenGLShaderCache::ReadyShader(
    vtkOpenGLShaderCache::CachedShaderProgram *shader)
{
  // compile if needed
  if (!shader->Compiled && !this->CompileShader(shader))
    {
    return NULL;
    }

  // bind if needed
  if (!this->BindShader(shader))
    {
    return NULL;
    }

  return shader;
}

vtkOpenGLShaderCache::CachedShaderProgram *vtkOpenGLShaderCache::GetShader(
  const char *vertexCode,
  const char *fragmentCode,
  const char *geometryCode)
{
  // compute the MD5 and the check the map
  std::string result;
  this->Internal->ComputeMD5(vertexCode, fragmentCode, geometryCode, result);

  // does it already exist?
  typedef std::map<std::string,vtkOpenGLShaderCache::CachedShaderProgram*>::const_iterator SMapIter;
  SMapIter found = this->Internal->ShaderPrograms.find(result);
  if (found == this->Internal->ShaderPrograms.end())
    {
    // create one
    vtkOpenGLShaderCache::CachedShaderProgram *sps = new vtkOpenGLShaderCache::CachedShaderProgram();
    sps->VS.SetSource(vertexCode);
    sps->VS.SetType(vtkgl::Shader::Vertex);
    sps->FS.SetSource(fragmentCode);
    sps->FS.SetType(vtkgl::Shader::Fragment);
    if (geometryCode != NULL)
      {
      sps->GS.SetSource(geometryCode);
      sps->GS.SetType(vtkgl::Shader::Geometry);
      }
    sps->Compiled = false;
    sps->md5Hash = result;
    sps->ShaderCache = this;
    this->Internal->ShaderPrograms.insert(std::make_pair(result, sps));
    return sps;
    }
  else
    {
    return found->second;
    }
}

// return 0 if there is an issue
int vtkOpenGLShaderCache::CompileShader(vtkOpenGLShaderCache::CachedShaderProgram* shader)
{
  if (!shader->VS.Compile())
    {
    vtkErrorMacro(<< shader->VS.GetError());

    int lineNum = 1;
    std::istringstream stream(shader->VS.GetSource());
    std::stringstream sstm;
    std::string aline;
    while (std::getline(stream, aline))
      {
      sstm << lineNum << ": " << aline << "\n";
      lineNum++;
      }
    vtkErrorMacro(<< sstm.str());
    return 0;
    }
  if (!shader->FS.Compile())
    {
    vtkErrorMacro(<< shader->FS.GetError());
    int lineNum = 1;
    std::istringstream stream(shader->FS.GetSource());
    std::stringstream sstm;
    std::string aline;
    while (std::getline(stream, aline))
      {
      sstm << lineNum << ": " << aline << "\n";
      lineNum++;
      }
    vtkErrorMacro(<< sstm.str());
    return 0;
    }
  if (!shader->Program.AttachShader(shader->VS))
    {
    vtkErrorMacro(<< shader->Program.GetError());
    return 0;
    }
  if (!shader->Program.AttachShader(shader->FS))
    {
    vtkErrorMacro(<< shader->Program.GetError());
    return 0;
    }
  if (!shader->Program.Link())
    {
    vtkErrorMacro(<< "Links failed: " << shader->Program.GetError());
    return 0;
    }

  shader->Compiled = true;
  return 1;
}

void vtkOpenGLShaderCache::ReleaseGraphicsResources(
    vtkOpenGLShaderCache::CachedShaderProgram *shader)
{
  // release if we need to
  if (this->LastShaderBound == shader)
    {
    this->LastShaderBound->Program.Release();
    }
  this->LastShaderBound = NULL;

  if (shader->Compiled)
    {
    shader->Program.DetachShader(shader->VS);
    shader->Program.DetachShader(shader->FS);
    shader->VS.Cleanup();
    shader->FS.Cleanup();
    shader->Program.ReleaseGraphicsResources();
    shader->Compiled = false;
    }
}


int vtkOpenGLShaderCache::BindShader(vtkOpenGLShaderCache::CachedShaderProgram* shader)
{
  if (this->LastShaderBound == shader)
    {
    return 1;
    }

  // release prior shader
  if (this->LastShaderBound)
    {
    this->LastShaderBound->Program.Release();
    }
  shader->Program.Bind();
  this->LastShaderBound = shader;
  return 1;
}


// ----------------------------------------------------------------------------
void vtkOpenGLShaderCache::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
