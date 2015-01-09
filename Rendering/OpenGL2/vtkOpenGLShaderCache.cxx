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
#include "vtkShader.h"
#include "vtkShaderProgram.h"

#include "vtksys/MD5.h"

using vtkgl::replace;

class vtkOpenGLShaderCache::Private
{
public:
  vtksysMD5* md5;

  // map of hash to shader program structs
  std::map<std::string, vtkShaderProgram *> ShaderPrograms;

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
  typedef std::map<std::string,vtkShaderProgram*>::const_iterator SMapIter;
  SMapIter iter = this->Internal->ShaderPrograms.begin();
  for ( ; iter != this->Internal->ShaderPrograms.end(); iter++)
    {
    iter->second->Delete();
    }

  delete this->Internal;
}

// return NULL if there is an issue
vtkShaderProgram *vtkOpenGLShaderCache::ReadyShader(
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
  vtkShaderProgram *shader = this->GetShader(VSSource.c_str(), FSSource.c_str(), GSSource.c_str());
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
  vtkShaderProgram *shader = this->GetShader(vertexCode, FSSource.c_str(), geometryCode);
#endif

  if (!shader)
    {
    return NULL;
    }

  // compile if needed
  if (!shader->GetCompiled() && !shader->CompileShader())
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
vtkShaderProgram *vtkOpenGLShaderCache::ReadyShader(
    vtkShaderProgram *shader)
{
  // compile if needed
  if (!shader->GetCompiled() && !shader->CompileShader())
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

vtkShaderProgram *vtkOpenGLShaderCache::GetShader(
  const char *vertexCode,
  const char *fragmentCode,
  const char *geometryCode)
{
  // compute the MD5 and the check the map
  std::string result;
  this->Internal->ComputeMD5(vertexCode, fragmentCode, geometryCode, result);

  // does it already exist?
  typedef std::map<std::string,vtkShaderProgram*>::const_iterator SMapIter;
  SMapIter found = this->Internal->ShaderPrograms.find(result);
  if (found == this->Internal->ShaderPrograms.end())
    {
    // create one
    vtkShaderProgram *sps = vtkShaderProgram::New();
    sps->GetVertexShader()->SetSource(vertexCode);
    sps->GetFragmentShader()->SetSource(fragmentCode);
    if (geometryCode != NULL)
      {
      sps->GetGeometryShader()->SetSource(geometryCode);
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

void vtkOpenGLShaderCache::ReleaseGraphicsResources(vtkWindow *win)
{
  // NOTE:
  // In the current implementation as of October 26th, if a shader
  // program is created by ShaderCache then it should make sure
  // that it releases the graphics resouces used by these programs.
  // It is not wisely for callers to do that since then they would
  // have to loop over all the programs were in use and invoke
  // release graphics resources individually.

  this->ReleaseCurrentShader();

  typedef std::map<std::string,vtkShaderProgram*>::const_iterator SMapIter;
  SMapIter iter = this->Internal->ShaderPrograms.begin();
  for ( ; iter != this->Internal->ShaderPrograms.end(); iter++)
    {
    iter->second->ReleaseGraphicsResources(win);
    }
}

void vtkOpenGLShaderCache::ReleaseCurrentShader()
{
  // release prior shader
  if (this->LastShaderBound)
    {
    this->LastShaderBound->Release();
    this->LastShaderBound = NULL;
    }
}

int vtkOpenGLShaderCache::BindShader(vtkShaderProgram* shader)
{
  if (this->LastShaderBound == shader)
    {
    return 1;
    }

  // release prior shader
  if (this->LastShaderBound)
    {
    this->LastShaderBound->Release();
    }
  shader->Bind();
  this->LastShaderBound = shader;
  return 1;
}


// ----------------------------------------------------------------------------
void vtkOpenGLShaderCache::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
