/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSSAOPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSSAOPass
 * @brief   Implement a screen-space ambient occlusion pass.
 *
 * SSAO darkens some pixels to improve depth perception simulating ambient occlusion
 * in screen space.
 * For each fragment, random samples inside a hemisphere at the fragment position oriented with
 * the normal are tested against other fragments to compute an average occlusion.
 * The number of samples and the radius of the hemisphere are configurables.
 *
 * @sa
 * vtkRenderPass
 */

#ifndef vtkSSAOPass_h
#define vtkSSAOPass_h

#include "vtkImageProcessingPass.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro

#include <vector> // For vector

class vtkMatrix4x4;
class vtkOpenGLFramebufferObject;
class vtkOpenGLQuadHelper;
class vtkTextureObject;

class VTKRENDERINGOPENGL2_EXPORT vtkSSAOPass : public vtkImageProcessingPass
{
public:
  static vtkSSAOPass* New();
  vtkTypeMacro(vtkSSAOPass, vtkImageProcessingPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform rendering according to a render state.
   */
  void Render(const vtkRenderState* s) override;

  /**
   * Release graphics resources and ask components to release their own resources.
   */
  void ReleaseGraphicsResources(vtkWindow* w) override;

  /**
   * Pre replace shader values
   */
  bool PreReplaceShaderValues(std::string& vertexShader, std::string& geometryShader,
    std::string& fragmentShader, vtkAbstractMapper* mapper, vtkProp* prop) override;

  /**
   * Post replace shader values
   */
  bool PostReplaceShaderValues(std::string& vertexShader, std::string& geometryShader,
    std::string& fragmentShader, vtkAbstractMapper* mapper, vtkProp* prop) override;

  /**
   * Set shader parameters. Set the draw buffers depending on the mapper.
   */
  bool SetShaderParameters(vtkShaderProgram* program, vtkAbstractMapper* mapper, vtkProp* prop,
    vtkOpenGLVertexArrayObject* VAO = nullptr) override;

  //@{
  /**
   * Get/Set the SSAO hemisphere radius.
   * Default is 0.5
   */
  vtkGetMacro(Radius, double);
  vtkSetMacro(Radius, double);
  //@}

  //@{
  /**
   * Get/Set the number of samples.
   * Default is 32
   */
  vtkGetMacro(KernelSize, unsigned int);
  vtkSetClampMacro(KernelSize, unsigned int, 1, 1000);
  //@}

  //@{
  /**
   * Get/Set the bias when comparing samples.
   * Default is 0.01
   */
  vtkGetMacro(Bias, double);
  vtkSetMacro(Bias, double);
  //@}

  //@{
  /**
   * Get/Set blurring of the ambient occlusion.
   * Blurring can help to improve the result if samples number is low.
   * Default is false
   */
  vtkGetMacro(Blur, bool);
  vtkSetMacro(Blur, bool);
  vtkBooleanMacro(Blur, bool);
  //@}

protected:
  vtkSSAOPass() = default;
  ~vtkSSAOPass() override = default;

  void ComputeKernel();
  void InitializeGraphicsResources(vtkOpenGLRenderWindow* renWin, int w, int h);

  void RenderDelegate(const vtkRenderState* s, int w, int h);
  void RenderSSAO(vtkOpenGLRenderWindow* renWin, vtkMatrix4x4* projection, int w, int h);
  void RenderCombine(vtkOpenGLRenderWindow* renWin);

  vtkTextureObject* ColorTexture = nullptr;
  vtkTextureObject* PositionTexture = nullptr;
  vtkTextureObject* NormalTexture = nullptr;
  vtkTextureObject* SSAOTexture = nullptr;
  vtkTextureObject* DepthTexture = nullptr;

  vtkOpenGLFramebufferObject* FrameBufferObject = nullptr;

  vtkOpenGLQuadHelper* SSAOQuadHelper = nullptr;
  vtkOpenGLQuadHelper* CombineQuadHelper = nullptr;

  std::vector<float> Kernel;
  unsigned int KernelSize = 32;
  double Radius = 0.5;
  double Bias = 0.01;
  bool Blur = false;

private:
  vtkSSAOPass(const vtkSSAOPass&) = delete;
  void operator=(const vtkSSAOPass&) = delete;
};

#endif
