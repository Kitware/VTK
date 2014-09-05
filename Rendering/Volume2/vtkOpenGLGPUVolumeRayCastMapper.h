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

#include <vtkGPUVolumeRayCastMapper.h>

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

    // Methods called by the AMR Volume Mapper.
    virtual void PreRender(vtkRenderer *ren,
                           vtkVolume *vol,
                           double datasetBounds[6],
                           double scalarRange[2],
                           int numberOfScalarComponents,
                           unsigned int numberOfLevels){};

    // \pre input is up-to-date
    virtual void RenderBlock(vtkRenderer *ren,
                             vtkVolume *vol,
                             unsigned int level){};

    virtual void PostRender(vtkRenderer *ren,
                            int numberOfScalarComponents){};

    void GetReductionRatio(double ratio[3]){};

    class vtkInternal;
    vtkInternal* Implementation;

private:
    vtkOpenGLGPUVolumeRayCastMapper(const vtkOpenGLGPUVolumeRayCastMapper&);  // Not implemented.
    void operator=(const vtkOpenGLGPUVolumeRayCastMapper&);  // Not implemented.
};

#endif // __vtkOpenGLGPUVolumeRayCastMapper_h
