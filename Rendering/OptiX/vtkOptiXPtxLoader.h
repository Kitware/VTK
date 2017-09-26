/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOptiXPtxLoader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOptiXPtxLoader
 * @brief   for loading of OptiX .ptx files
 *
 * Loads in OptiX .ptx files by checking a number of standard directories.
 *
*/

#ifndef vtkOptiXPtxLoader_h
#define vtkOptiXPtxLoader_h

#include "vtkRenderingOptiXModule.h" // For export macro

#include "vtkObject.h"

namespace optix
{
  class ProgramObj;
  class ContextObj;
}

struct vtkOptiXPtxLoaderInternals;

class VTKRENDERINGOPTIX_EXPORT vtkOptiXPtxLoader : public vtkObject
{
public:

  static vtkOptiXPtxLoader* New();
  vtkTypeMacro(vtkOptiXPtxLoader, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Retrieve the full path to the given .ptx file.
   */
  static std::string GetFullPathToFile(const char* filename);

  /**
  * Load all programs.
  */
  void LoadPrograms(optix::ContextObj* ctx);

  //@{
  /**
  * Convenience program handles
  */
  optix::ProgramObj* SphereIsectProgram;
  optix::ProgramObj* SphereBoundsProgram;
  optix::ProgramObj* CylinderIsectProgram;
  optix::ProgramObj* CylinderBoundsProgram;
  optix::ProgramObj* TriangleIsectProgram;
  optix::ProgramObj* TriangleBoundsProgram;
  optix::ProgramObj* ClosestHitProgram;
  optix::ProgramObj* AnyHitProgram;
  optix::ProgramObj* MissProgram;
  optix::ProgramObj* RayGenProgram;
  //@}

protected:

  vtkOptiXPtxLoader();
  ~vtkOptiXPtxLoader();

  vtkOptiXPtxLoaderInternals* Internals;

  optix::ProgramObj* LoadProgram(const char* filename, const char* entrypoint,
    optix::ContextObj* ctx);

private:

  vtkOptiXPtxLoader(const vtkOptiXPtxLoader&) = delete;
  void operator=(const vtkOptiXPtxLoader&) = delete;
};
#endif
