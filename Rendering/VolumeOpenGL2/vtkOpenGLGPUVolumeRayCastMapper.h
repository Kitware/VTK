/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLGPUVolumeRayCastMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkOpenGLGPUVolumeRayCastMapper_h
#define vtkOpenGLGPUVolumeRayCastMapper_h

#include "vtkRenderingVolumeOpenGL2Module.h" // For export macro

#include <vtkGPUVolumeRayCastMapper.h>

//----------------------------------------------------------------------------
class VTKRENDERINGVOLUMEOPENGL2_EXPORT vtkOpenGLGPUVolumeRayCastMapper :
  public vtkGPUVolumeRayCastMapper
{
public:
  static vtkOpenGLGPUVolumeRayCastMapper* New();

  vtkTypeMacro(vtkOpenGLGPUVolumeRayCastMapper, vtkGPUVolumeRayCastMapper);
  void PrintSelf( ostream& os, vtkIndent indent );

protected:
  vtkOpenGLGPUVolumeRayCastMapper();
  ~vtkOpenGLGPUVolumeRayCastMapper();

  // Description:
  // Delete OpenGL objects.
  // \post done: this->OpenGLObjectsCreated==0
  virtual void ReleaseGraphicsResources(vtkWindow *window);

  // Description:
  // Build vertex and fragment shader for the volume rendering
  void BuildShader(vtkRenderer* ren, vtkVolume* vol, int noOfCmponents);

  // TODO Take these out as these are no longer needed
  // Methods called by the AMR Volume Mapper.
  virtual void PreRender(vtkRenderer * vtkNotUsed(ren),
                         vtkVolume *vtkNotUsed(vol),
                         double vtkNotUsed(datasetBounds)[6],
                         double vtkNotUsed(scalarRange)[2],
                         int vtkNotUsed(noOfComponents),
                         unsigned int vtkNotUsed(numberOfLevels)) {}

  // \pre input is up-to-date
  virtual void RenderBlock(vtkRenderer *vtkNotUsed(ren),
                           vtkVolume *vtkNotUsed(vol),
                           unsigned int vtkNotUsed(level)) {}

  virtual void PostRender(vtkRenderer *vtkNotUsed(ren),
                          int vtkNotUsed(noOfComponents)) {}

  // Description:
  // Rendering volume on GPU
  void GPURender(vtkRenderer *ren, vtkVolume *vol);

  // Description:
  // Update the reduction factor of the render viewport (this->ReductionFactor)
  // according to the time spent in seconds to render the previous frame
  // (this->TimeToDraw) and a time in seconds allocated to render the next
  // frame (allocatedTime).
  // \pre valid_current_reduction_range: this->ReductionFactor>0.0 && this->ReductionFactor<=1.0
  // \pre positive_TimeToDraw: this->TimeToDraw>=0.0
  // \pre positive_time: allocatedTime>0
  // \post valid_new_reduction_range: this->ReductionFactor>0.0 && this->ReductionFactor<=1.0
  void ComputeReductionFactor(double allocatedTime);

  // Description:
  // Empty implementation.
  void GetReductionRatio(double* ratio)
    {
    ratio[0] = ratio[1] = ratio[2] = 1.0;
    }

  // Description:
  // Empty implementation.
  virtual int IsRenderSupported(vtkRenderWindow *vtkNotUsed(window),
                                vtkVolumeProperty *vtkNotUsed(property))
    {
    return 1;
    }

  double ReductionFactor;

private:
  class vtkInternal;
  vtkInternal* Impl;

  vtkOpenGLGPUVolumeRayCastMapper(
    const vtkOpenGLGPUVolumeRayCastMapper&); // Not implemented.
  void operator=(const vtkOpenGLGPUVolumeRayCastMapper&); // Not implemented.
};

#endif // vtkOpenGLGPUVolumeRayCastMapper_h
