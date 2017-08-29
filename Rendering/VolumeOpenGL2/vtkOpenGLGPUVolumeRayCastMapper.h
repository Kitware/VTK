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

#include "vtkNew.h"                          // For vtkNew
#include "vtkRenderingVolumeOpenGL2Module.h" // For export macro
#include "vtkGPUVolumeRayCastMapper.h"

// Forward declarations
class vtkGenericOpenGLResourceFreeCallback;
class vtkImplicitFunction;
class vtkOpenGLCamera;
class vtkShaderProgram;
class vtkTextureObject;
class vtkVolumeTexture;

//----------------------------------------------------------------------------
class VTKRENDERINGVOLUMEOPENGL2_EXPORT vtkOpenGLGPUVolumeRayCastMapper :
  public vtkGPUVolumeRayCastMapper
{
public:
  static vtkOpenGLGPUVolumeRayCastMapper* New();

  enum Passes
  {
    RenderPass,
    DepthPass = 1
  };

  vtkTypeMacro(vtkOpenGLGPUVolumeRayCastMapper, vtkGPUVolumeRayCastMapper);
  void PrintSelf( ostream& os, vtkIndent indent ) VTK_OVERRIDE;

  // Description:
  // Low level API to enable access to depth texture in
  // RenderToTexture mode. It will return either NULL if
  // RenderToImage was never turned on or texture captured
  // the last time RenderToImage was on.
  vtkTextureObject* GetDepthTexture();

  // Description:
  // Low level API to enable access to color texture in
  // RenderToTexture mode. It will return either NULL if
  // RenderToImage was never turned on or texture captured
  // the last time RenderToImage was on.
  vtkTextureObject* GetColorTexture();

  // Description:
  // Low level API to export the depth texture as vtkImageData in
  // RenderToImage mode.
  void GetDepthImage(vtkImageData* im) VTK_OVERRIDE;

  // Description:
  // Low level API to export the color texture as vtkImageData in
  // RenderToImage mode.
  void GetColorImage(vtkImageData* im) VTK_OVERRIDE;

  // Description:
  // Mapper can have multiple passes and internally it will set
  // the state. The state can not be set externally explicitly
  // but can be set indirectly depending on the options set by
  // the user.
  vtkGetMacro(CurrentPass, int);

  //@{
  /**
   * Sets a user defined function to generate the ray jittering noise.
   * vtkPerlinNoise is used by default with a texture size equivlent to
   * the window size. These settings will have no effect when UseJittering
   * is Off.
   */
  void SetNoiseGenerator(vtkImplicitFunction* generator);
  vtkSetVector2Macro(NoiseTextureSize, int);
  //@}

  /**
   * Set a fixed number of partitions in which to split the volume
   * during rendring. This will force by-block rendering without
   * trying to compute an optimum number of partitions.
   */
  void SetPartitions(unsigned short x, unsigned short y, unsigned short z);

  /**
   *  Load the volume texture into GPU memory.  Actual loading occurs
   *  in vtkVolumeTexture::LoadVolume.  The mapper by default loads data
   *  lazily (at render time), so it is most commonly not necessary to call
   *  this function.  This method is only exposed in order to support on-site
   *  loading which is useful in cases where the user needs to know a-priori
   *  whether loading will succeed  or not.
   */
  bool PreLoadData(vtkRenderer* ren, vtkVolume* vol);

protected:
  vtkOpenGLGPUVolumeRayCastMapper();
  ~vtkOpenGLGPUVolumeRayCastMapper() VTK_OVERRIDE;

  // Description:
  // Delete OpenGL objects.
  // \post done: this->OpenGLObjectsCreated==0
  void ReleaseGraphicsResources(vtkWindow *window) VTK_OVERRIDE;
  vtkGenericOpenGLResourceFreeCallback *ResourceCallback;

  // Description:
  // Build vertex and fragment shader for the volume rendering
  void BuildDepthPassShader(vtkRenderer* ren, vtkVolume* vol,
                            int noOfComponents,
                            int independentComponents);

  // Description:
  // Build vertex and fragment shader for the volume rendering
  void BuildShader(vtkRenderer* ren, vtkVolume* vol, int noOfCmponents);

  // TODO Take these out as these are no longer needed
  // Methods called by the AMR Volume Mapper.
  void PreRender(vtkRenderer * vtkNotUsed(ren),
                         vtkVolume *vtkNotUsed(vol),
                         double vtkNotUsed(datasetBounds)[6],
                         double vtkNotUsed(scalarRange)[2],
                         int vtkNotUsed(noOfComponents),
                         unsigned int vtkNotUsed(numberOfLevels)) VTK_OVERRIDE {};

  // \pre input is up-to-date
  void RenderBlock(vtkRenderer *vtkNotUsed(ren),
                           vtkVolume *vtkNotUsed(vol),
                           unsigned int vtkNotUsed(level)) VTK_OVERRIDE {}

  void PostRender(vtkRenderer *vtkNotUsed(ren),
                          int vtkNotUsed(noOfComponents)) VTK_OVERRIDE {}

  // Description:
  // Rendering volume on GPU
  void GPURender(vtkRenderer *ren, vtkVolume *vol) VTK_OVERRIDE;

  // Description:
  // Method that performs the actual rendering given a volume and a shader
  void DoGPURender(vtkRenderer* ren,
                   vtkVolume* vol,
                   vtkOpenGLCamera* cam,
                   vtkShaderProgram* shaderProgram,
                   int noOfComponents,
                   int independentComponents);

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
  void GetReductionRatio(double* ratio) VTK_OVERRIDE
  {
    ratio[0] = ratio[1] = ratio[2] = 1.0;
  }


  // Description:
  // Empty implementation.
  int IsRenderSupported(vtkRenderWindow *vtkNotUsed(window),
                                vtkVolumeProperty *vtkNotUsed(property)) VTK_OVERRIDE
  {
    return 1;
  }

  //@{
  /**
   *  \brief vtkOpenGLRenderPass API
   */
  vtkMTimeType GetRenderPassStageMTime(vtkVolume* vol);

  /**
   *  RenderPass string replacements on shader templates.
   */
  void ReplaceShaderRenderPass(std::string& vertShader, std::string& fragShader,
    vtkVolume* vol, bool prePass);

  /**
   *  Update parameters from RenderPass
   */
  void SetShaderParametersRenderPass(vtkVolume* vol);

  /**
   *  Caches the vtkOpenGLRenderPass::RenderPasses() information.
   *  Note: Do not dereference the pointers held by this object. There is no
   *  guarantee that they are still valid!
   */
  vtkNew<vtkInformation> LastRenderPassInfo;
  //@}

  double ReductionFactor;
  int    CurrentPass;

private:
  class vtkInternal;
  vtkInternal* Impl;

  friend class vtkVolumeTexture;
  vtkVolumeTexture* VolumeTexture;

  vtkImplicitFunction* NoiseGenerator;
  int NoiseTextureSize[2];

  vtkOpenGLGPUVolumeRayCastMapper(
    const vtkOpenGLGPUVolumeRayCastMapper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenGLGPUVolumeRayCastMapper&) VTK_DELETE_FUNCTION;
};

#endif // vtkOpenGLGPUVolumeRayCastMapper_h
