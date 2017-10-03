/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOptiXPtxLoader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOptiXPtxLoader.h"

#include "vtkOptiXConfig.h"
#include "vtkObjectFactory.h"

#include <optixu/optixpp_namespace.h>

#if _WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

//============================================================================
vtkStandardNewMacro(vtkOptiXPtxLoader);

//------------------------------------------------------------------------------
struct vtkOptiXPtxLoaderInternals
{
  // To keep programs persistent across the ptx loader's lifetime
  std::vector<optix::Program> ProgramHandles;
};

//------------------------------------------------------------------------------
vtkOptiXPtxLoader::vtkOptiXPtxLoader()
  : Internals(new vtkOptiXPtxLoaderInternals)
{
}

//------------------------------------------------------------------------------
vtkOptiXPtxLoader::~vtkOptiXPtxLoader()
{
  delete this->Internals;
}

//------------------------------------------------------------------------------
void vtkOptiXPtxLoader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
std::string vtkOptiXPtxLoader::GetFullPathToFile(const char* file)
{
  std::string fileName = VTK_OPTIX_PTX_DIR + file;
  const int MAX_PATH_LEN = 1024;
  char pathName[MAX_PATH_LEN];
#ifdef _WIN32
  int nameLen = GetModuleFileName(nullptr, pathName, MAX_PATH_LEN);
#else
  int nameLen = readlink("/proc/self/exe", pathName, MAX_PATH_LEN);
#endif
  if (nameLen == -1 || nameLen == 0 || nameLen == MAX_PATH_LEN)
  {
    vtkGenericWarningMacro(<< "Path to executable not found or too long (>=1024). \
      Trying with hardcoded path, which may fail.");
  }
  else
  {
    char* p = &(pathName[nameLen-1]);
    while (*p != '\\' && *p != '/' && p > pathName)
    {
      p--;
    }
    *(p+1) = '\0';

#ifdef _WIN32
    struct _stat statBuf;
#else
    struct stat statBuf;
#endif
    fileName = (std::string(pathName) + "../lib/ptx") + file;
#ifdef _WIN32
    if (_stat(fileName.c_str(), &statBuf) == -1)
#else
    if (stat(fileName.c_str(), &statBuf) == -1)
#endif
    {
      fileName = (std::string(pathName) + "../../lib/ptx") + file;
#ifdef _WIN32
      if (_stat(fileName.c_str(), &statBuf) == -1)
#else
      if (stat(fileName.c_str(), &statBuf) == -1)
#endif
      {
        vtkGenericWarningMacro(<< "OptiX Ptx files not found in ../lib/ptx \
          or ../../lib/ptx. Trying with hardcoded path, which may fail.");
      }
    }
  }

  return fileName;
}

//------------------------------------------------------------------------------
void vtkOptiXPtxLoader::LoadPrograms(optix::ContextObj* ctx)
{
  this->SphereIsectProgram = this->LoadProgram(
    "/cuda_compile_ptx_1_generated_Sphere.cu.ptx",
    "SphereIntersect", ctx);

  this->SphereBoundsProgram = this->LoadProgram(
    "/cuda_compile_ptx_1_generated_Sphere.cu.ptx",
    "SphereBounds", ctx);

  this->CylinderIsectProgram = this->LoadProgram(
    "/cuda_compile_ptx_1_generated_Cylinder.cu.ptx",
    "CylinderIntersect", ctx);

  this->CylinderBoundsProgram = this->LoadProgram(
    "/cuda_compile_ptx_1_generated_Cylinder.cu.ptx",
    "CylinderBounds", ctx);

  this->TriangleIsectProgram = this->LoadProgram(
    "/cuda_compile_ptx_1_generated_TriangleMesh.cu.ptx",
    "TriangleMeshIntersection", ctx);

  this->TriangleBoundsProgram = this->LoadProgram(
    "/cuda_compile_ptx_1_generated_TriangleMesh.cu.ptx",
    "TriangleMeshBoundingBox", ctx);

  this->ClosestHitProgram = this->LoadProgram(
    "/cuda_compile_ptx_1_generated_Phong.cu.ptx",
    "LambertianClosestHit", ctx);

  this->AnyHitProgram = this->LoadProgram(
    "/cuda_compile_ptx_1_generated_Phong.cu.ptx",
    "LambertianAnyHit", ctx);

  this->MissProgram = this->LoadProgram(
    "/cuda_compile_ptx_1_generated_Phong.cu.ptx",
    "Miss", ctx);

  this->RayGenProgram = this->LoadProgram(
    "/cuda_compile_ptx_1_generated_PerspectiveCamera.cu.ptx",
    "PerspectiveCameraRayGen", ctx);
}

//------------------------------------------------------------------------------
optix::ProgramObj* vtkOptiXPtxLoader::LoadProgram(const char* filename, const char* entrypoint,
  optix::ContextObj* ctx)
{
  std::string fileStr = vtkOptiXPtxLoader::GetFullPathToFile(filename);
  optix::Program program = ctx->createProgramFromPTXFile(fileStr, entrypoint);
  this->Internals->ProgramHandles.push_back(program);

  return program.get();
}
