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

// VTK includes
#include "vtkNew.h"                          // For vtkNew
#include "vtkRenderingVolumeOpenGL2Module.h" // For export macro
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkShader.h"                       // For methods

// STL includes
#include <map>                               // For shader replacements

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
  void PrintSelf( ostream& os, vtkIndent indent ) override;

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
  void GetDepthImage(vtkImageData* im) override;

  // Description:
  // Low level API to export the color texture as vtkImageData in
  // RenderToImage mode.
  void GetColorImage(vtkImageData* im) override;

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

  //@{
  /**
   * This function enables you to apply your own substitutions
   * to the shader creation process. The shader code in this class
   * is created by applying a bunch of string replacements to a
   * shader template. Using this function you can apply your
   * own string replacements to add features you desire.
   */
  void AddShaderReplacement(
    vtkShader::Type shaderType, // vertex, fragment, etc
    const std::string& originalValue,
    bool replaceFirst,  // do this replacement before the default
    const std::string& replacementValue,
    bool replaceAll);
  void ClearShaderReplacement(
    vtkShader::Type shaderType, // vertex, fragment, etc
    const std::string& originalValue,
    bool replaceFirst);
  //@}

  //@{
  /**
   * Allow the program to set the shader codes used directly
   * instead of using the built in templates. Be aware, if
   * set, this template will be used for all cases,
   * primitive types, picking etc.
   */
  vtkSetStringMacro(VertexShaderCode);
  vtkGetStringMacro(VertexShaderCode);
  vtkSetStringMacro(FragmentShaderCode);
  vtkGetStringMacro(FragmentShaderCode);
  //@}

protected:
  vtkOpenGLGPUVolumeRayCastMapper();
  ~vtkOpenGLGPUVolumeRayCastMapper() override;

  // Description:
  // Delete OpenGL objects.
  // \post done: this->OpenGLObjectsCreated==0
  void ReleaseGraphicsResources(vtkWindow *window) override;
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
                         unsigned int vtkNotUsed(numberOfLevels)) override {};

  // \pre input is up-to-date
  void RenderBlock(vtkRenderer *vtkNotUsed(ren),
                           vtkVolume *vtkNotUsed(vol),
                           unsigned int vtkNotUsed(level)) override {}

  void PostRender(vtkRenderer *vtkNotUsed(ren),
                          int vtkNotUsed(noOfComponents)) override {}

  // Description:
  // Rendering volume on GPU
  void GPURender(vtkRenderer *ren, vtkVolume *vol) override;

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
  void GetReductionRatio(double* ratio) override
  {
    ratio[0] = ratio[1] = ratio[2] = 1.0;
  }


  // Description:
  // Empty implementation.
  int IsRenderSupported(vtkRenderWindow *vtkNotUsed(window),
                                vtkVolumeProperty *vtkNotUsed(property)) override
  {
    return 1;
  }

  //@{
  /**
   *  \brief vtkOpenGLRenderPass API
   */
  vtkMTimeType GetRenderPassStageMTime(vtkVolume* vol);

  /**
   * Create the basic shader template strings before substitutions
   */
  void GetShaderTemplate(std::map<vtkShader::Type, vtkShader*>& shaders);

  /**
   * Perform string replacements on the shader templates
   */
  void ReplaceShaderValues(std::map<vtkShader::Type, vtkShader*>& shaders,
    vtkRenderer* ren, vtkVolume* vol, int numComps);

  /**
   *  RenderPass string replacements on shader templates called from
   *  ReplaceShaderValues.
   */
  void ReplaceShaderBase(std::map<vtkShader::Type, vtkShader*>& shaders,
    vtkRenderer* ren, vtkVolume* vol, int numComps);
  void ReplaceShaderTermination(std::map<vtkShader::Type, vtkShader*>& shaders,
    vtkRenderer* ren, vtkVolume* vol, int numComps);
  void ReplaceShaderShading(std::map<vtkShader::Type, vtkShader*>& shaders,
    vtkRenderer* ren, vtkVolume* vol, int numComps);
  void ReplaceShaderCompute(std::map<vtkShader::Type, vtkShader*>& shaders,
    vtkRenderer* ren, vtkVolume* vol, int numComps);
  void ReplaceShaderCropping(std::map<vtkShader::Type, vtkShader*>& shaders,
    vtkRenderer* ren, vtkVolume* vol, int numComps);
  void ReplaceShaderClipping(std::map<vtkShader::Type, vtkShader*>& shaders,
    vtkRenderer* ren, vtkVolume* vol, int numComps);
  void ReplaceShaderMasking(std::map<vtkShader::Type, vtkShader*>& shaders,
    vtkRenderer* ren, vtkVolume* vol, int numComps);
  void ReplaceShaderPicking(std::map<vtkShader::Type, vtkShader*>& shaders,
    vtkRenderer* ren, vtkVolume* vol, int numComps);
  void ReplaceShaderRTT(std::map<vtkShader::Type, vtkShader*>& shaders,
    vtkRenderer* ren, vtkVolume* vol, int numComps);
  void ReplaceShaderRenderPass(std::map<vtkShader::Type, vtkShader*>& shaders,
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
  char *VertexShaderCode;
  char *FragmentShaderCode;
  std::map<const vtkShader::ReplacementSpec, vtkShader::ReplacementValue>
    UserShaderReplacements;

private:
  class vtkInternal;
  vtkInternal* Impl;

  friend class vtkVolumeTexture;
  vtkVolumeTexture* VolumeTexture;

  vtkImplicitFunction* NoiseGenerator;
  int NoiseTextureSize[2];

  vtkOpenGLGPUVolumeRayCastMapper(
    const vtkOpenGLGPUVolumeRayCastMapper&) = delete;
  void operator=(const vtkOpenGLGPUVolumeRayCastMapper&) = delete;
};

#endif // vtkOpenGLGPUVolumeRayCastMapper_h
