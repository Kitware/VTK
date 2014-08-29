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

#ifndef __vtkGPUVolumeRayCastMapper_h
#define __vtkGPUVolumeRayCastMapper_h

#include "vtkVolumeModule.h" // For export macro

#include <vtkVolumeMapper.h>
#include <vtkSetGet.h>

//----------------------------------------------------------------------------
///
/// \brief The vtkGPUVolumeRayCastMapper class
///
class VTKVOLUME_EXPORT vtkGPUVolumeRayCastMapper : public vtkVolumeMapper
{
  public:
    static vtkGPUVolumeRayCastMapper* New();

    vtkTypeMacro(vtkGPUVolumeRayCastMapper, vtkVolumeMapper);
    void PrintSelf( ostream& os, vtkIndent indent );

    /// Description:
    /// Render volume
    virtual void Render(vtkRenderer* ren, vtkVolume* vol);

    /// Description:
    /// If AutoAdjustSampleDistances is on, the the ImageSampleDistance
    /// will be varied to achieve the allocated render time of this
    /// prop (controlled by the desired update rate and any culling in
    /// use).
    vtkSetClampMacro( AutoAdjustSampleDistances, int, 0, 1 );
    vtkGetMacro( AutoAdjustSampleDistances, int );
    vtkBooleanMacro( AutoAdjustSampleDistances, int );

    /// Description:
    /// Set/Get the distance between samples used for rendering
    /// Initial value is 1.0.
    vtkSetMacro(SampleDistance, double);
    vtkGetMacro(SampleDistance, double);

  protected:
    vtkGPUVolumeRayCastMapper();
    ~vtkGPUVolumeRayCastMapper();

    /// Description:
    /// Build vertex and fragment shader for the volume rendering
    void BuildShader(vtkRenderer* ren, vtkVolume* vol) = 0;

    /// Description:
    /// Validate before performing volume rendering
    int ValidateRender(vtkRenderer* ren, vtkVolume* vol);

    /// Description:
    /// Rendering volume on GPU
    void GPURender(vtkRenderer *ren, vtkVolume *vol) = 0;

protected:
    int AutoAdjustSampleDistances;
    double SampleDistance;

private:
    vtkGPUVolumeRayCastMapper(const vtkGPUVolumeRayCastMapper&);  // Not implemented.
    void operator=(const vtkGPUVolumeRayCastMapper&);  // Not implemented.
};

#endif // __vtkGPUVolumeRayCastMapper_h
