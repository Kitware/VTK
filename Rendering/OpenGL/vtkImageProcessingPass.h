/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageProcessingPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageProcessingPass - Convenient class for post-processing passes.
// render pass.
// .SECTION Description
// Abstract class with some convenient methods frequently used in subclasses.
//
// .SECTION Implementation

// .SECTION See Also
// vtkRenderPass vtkGaussianBlurPass vtkSobelGradientMagnitudePass

#ifndef vtkImageProcessingPass_h
#define vtkImageProcessingPass_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkRenderPass.h"

class vtkOpenGLRenderWindow;
class vtkDepthPeelingPassLayerList; // Pimpl
class vtkShaderProgram2;
class vtkShader2;
class vtkFrameBufferObject;
class vtkTextureObject;

class VTKRENDERINGOPENGL_EXPORT vtkImageProcessingPass : public vtkRenderPass
{
public:
  vtkTypeMacro(vtkImageProcessingPass,vtkRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Release graphics resources and ask components to release their own
  // resources.
  // \pre w_exists: w!=0
  void ReleaseGraphicsResources(vtkWindow *w);

  // Description:
  // Delegate for rendering the image to be processed.
  // If it is NULL, nothing will be rendered and a warning will be emitted.
  // It is usually set to a vtkCameraPass or to a post-processing pass.
  // Initial value is a NULL pointer.
  vtkGetObjectMacro(DelegatePass,vtkRenderPass);
  virtual void SetDelegatePass(vtkRenderPass *delegatePass);

 protected:
  // Description:
  // Default constructor. DelegatePass is set to NULL.
  vtkImageProcessingPass();

  // Description:
  // Destructor.
  virtual ~vtkImageProcessingPass();

  // Description:
  // Render delegate with a image of different dimensions than the
  // original one.
  // \pre s_exists: s!=0
  // \pre fbo_exists: fbo!=0
  // \pre fbo_has_context: fbo->GetContext()!=0
  // \pre target_exists: target!=0
  // \pre target_has_context: target->GetContext()!=0
  void RenderDelegate(const vtkRenderState *s,
                      int width,
                      int height,
                      int newWidth,
                      int newHeight,
                      vtkFrameBufferObject *fbo,
                      vtkTextureObject *target);


  vtkRenderPass *DelegatePass;

 private:
  vtkImageProcessingPass(const vtkImageProcessingPass&);  // Not implemented.
  void operator=(const vtkImageProcessingPass&);  // Not implemented.
};

#endif
