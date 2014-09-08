/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLProjectedTetrahedraMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkOpenGLGPUVolumeRayCastMapper_h
#define __vtkOpenGLGPUVolumeRayCastMapper_h

#include "vtkVolume2Module.h" // For export macro

#include "vtkGPUVolumeRayCastMapper.h"

#include <vtkSetGet.h>

//----------------------------------------------------------------------------
///
/// \brief The vtkOpenGLGPUVolumeRayCastMapper class
///
class VTKVOLUME2_EXPORT vtkOpenGLGPUVolumeRayCastMapper :
  public vtkGPUVolumeRayCastMapper
{
  public:
    static vtkOpenGLGPUVolumeRayCastMapper* New();

    vtkTypeMacro(vtkOpenGLGPUVolumeRayCastMapper, vtkGPUVolumeRayCastMapper);
    void PrintSelf( ostream& os, vtkIndent indent );

  protected:
    vtkOpenGLGPUVolumeRayCastMapper();
    ~vtkOpenGLGPUVolumeRayCastMapper();

    /// Description:
    /// Build vertex and fragment shader for the volume rendering
    void BuildShader(vtkRenderer* ren, vtkVolume* vol);

    /// Description:
    /// Rendering volume on GPU
    void GPURender(vtkRenderer *ren, vtkVolume *vol);

    class vtkInternal;
    vtkInternal* Implementation;

private:
    vtkOpenGLGPUVolumeRayCastMapper(const vtkOpenGLGPUVolumeRayCastMapper&);  // Not implemented.
    void operator=(const vtkOpenGLGPUVolumeRayCastMapper&);  // Not implemented.
};

#endif // __vtkOpenGLGPUVolumeRayCastMapper_h
