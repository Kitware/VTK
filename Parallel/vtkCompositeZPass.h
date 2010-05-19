/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeZPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCompositeZPass - Merge depth buffers of processes.
// .SECTION Description
// Merge the depth buffers of satellite processes into the root process depth
// buffer. It assumes that all the depth buffers have the same number of bits.
// The depth buffer of the satellite processes are not changed.
// 
// This pass requires a OpenGL context that supports texture objects (TO),
// and pixel buffer objects (PBO). If not, it will emit an error message
// and will render its delegate and return.
//
// .SECTION See Also
// vtkRenderPass

#ifndef __vtkCompositeZPass_h
#define __vtkCompositeZPass_h

#include "vtkRenderPass.h"
  
class vtkMultiProcessController;

class vtkPixelBufferObject;
class vtkTextureObject;
class vtkShaderProgram2;
class vtkOpenGLRenderWindow;

class VTK_PARALLEL_EXPORT vtkCompositeZPass : public vtkRenderPass
{
public:
  static vtkCompositeZPass *New();
  vtkTypeMacro(vtkCompositeZPass,vtkRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Perform rendering according to a render state \p s.
  // \pre s_exists: s!=0
  virtual void Render(const vtkRenderState *s);
  
  // Description:
  // Release graphics resources and ask components to release their own
  // resources.
  // \pre w_exists: w!=0
  void ReleaseGraphicsResources(vtkWindow *w);
  
  // Description:
  // Controller
  // If it is NULL, nothing will be rendered and a warning will be emitted.
  // Initial value is a NULL pointer.
  vtkGetObjectMacro(Controller,vtkMultiProcessController);
  virtual void SetController(vtkMultiProcessController *controller);
  
  // Description:
  // Is the pass supported by the OpenGL context?
  bool IsSupported(vtkOpenGLRenderWindow *context);
  
 protected:
  // Description:
  // Default constructor. Controller is set to NULL.
  vtkCompositeZPass();

  // Description:
  // Destructor.
  virtual ~vtkCompositeZPass();
  
  // Description:
  // Create program for texture mapping.
  // \pre context_exists: context!=0
  // \pre Program_void: this->Program==0
  // \post Program_exists: this->Program!=0
  void CreateProgram(vtkOpenGLRenderWindow *context);
  
  vtkMultiProcessController *Controller;
  
  vtkPixelBufferObject *PBO;
  vtkTextureObject *ZTexture;
  vtkShaderProgram2 *Program;
  float *RawZBuffer;
  size_t RawZBufferSize;
  
 private:
  vtkCompositeZPass(const vtkCompositeZPass&);  // Not implemented.
  void operator=(const vtkCompositeZPass&);  // Not implemented.
};

#endif
