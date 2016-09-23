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
/**
 * @class   vtkCompositeZPass
 * @brief   Merge depth buffers of processes.
 *
 * Merge the depth buffers of satellite processes into the root process depth
 * buffer. It assumes that all the depth buffers have the same number of bits.
 * The depth buffer of the satellite processes are not changed.
 *
 * This pass requires a OpenGL context that supports texture objects (TO),
 * and pixel buffer objects (PBO). If not, it will emit an error message
 * and will render its delegate and return.
 *
 * @sa
 * vtkRenderPass
*/

#ifndef vtkCompositeZPass_h
#define vtkCompositeZPass_h

#include "vtkRenderingParallelModule.h" // For export macro
#include "vtkRenderPass.h"

class vtkMultiProcessController;

class vtkPixelBufferObject;
class vtkTextureObject;
class vtkOpenGLRenderWindow;
#ifdef VTK_OPENGL2
class vtkOpenGLHelper;
#else
class vtkShaderProgram2;
#endif

class VTKRENDERINGPARALLEL_EXPORT vtkCompositeZPass : public vtkRenderPass
{
public:
  static vtkCompositeZPass *New();
  vtkTypeMacro(vtkCompositeZPass,vtkRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Perform rendering according to a render state \p s.
   * \pre s_exists: s!=0
   */
  virtual void Render(const vtkRenderState *s);

  /**
   * Release graphics resources and ask components to release their own
   * resources.
   * \pre w_exists: w!=0
   */
  void ReleaseGraphicsResources(vtkWindow *w);

  //@{
  /**
   * Controller
   * If it is NULL, nothing will be rendered and a warning will be emitted.
   * Initial value is a NULL pointer.
   */
  vtkGetObjectMacro(Controller,vtkMultiProcessController);
  virtual void SetController(vtkMultiProcessController *controller);
  //@}

  /**
   * Is the pass supported by the OpenGL context?
   */
  bool IsSupported(vtkOpenGLRenderWindow *context);

 protected:
  /**
   * Default constructor. Controller is set to NULL.
   */
  vtkCompositeZPass();

  /**
   * Destructor.
   */
  virtual ~vtkCompositeZPass();

  /**
   * Create program for texture mapping.
   * \pre context_exists: context!=0
   * \pre Program_void: this->Program==0
   * \post Program_exists: this->Program!=0
   */
  void CreateProgram(vtkOpenGLRenderWindow *context);

  vtkMultiProcessController *Controller;

  vtkPixelBufferObject *PBO;
  vtkTextureObject *ZTexture;
#ifdef VTK_OPENGL2
  vtkOpenGLHelper *Program;
#else
  vtkShaderProgram2 *Program;
#endif
  float *RawZBuffer;
  size_t RawZBufferSize;

 private:
  vtkCompositeZPass(const vtkCompositeZPass&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCompositeZPass&) VTK_DELETE_FUNCTION;
};

#endif
