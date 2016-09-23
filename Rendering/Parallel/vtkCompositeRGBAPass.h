/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeRGBAPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCompositeRGBAPass
 * @brief   Blend RGBA buffers of processes.
 *
 * Blend the RGBA buffers of satellite processes over the root process RGBA
 * buffer.
 * The RGBA buffer of the satellite processes are not changed.
 *
 * This pass requires a OpenGL context that supports texture objects (TO),
 * and pixel buffer objects (PBO). If not, it will emit an error message
 * and will render its delegate and return.
 *
 * @sa
 * vtkRenderPass
*/

#ifndef vtkCompositeRGBAPass_h
#define vtkCompositeRGBAPass_h

#include "vtkRenderingParallelModule.h" // For export macro
#include "vtkRenderPass.h"

class vtkMultiProcessController;

class vtkPixelBufferObject;
class vtkTextureObject;
class vtkOpenGLRenderWindow;
class vtkPKdTree;

class VTKRENDERINGPARALLEL_EXPORT vtkCompositeRGBAPass : public vtkRenderPass
{
public:
  static vtkCompositeRGBAPass *New();
  vtkTypeMacro(vtkCompositeRGBAPass,vtkRenderPass);
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

  //@{
  /**
   * kd tree that gives processes ordering. Initial value is a NULL pointer.
   */
  vtkGetObjectMacro(Kdtree,vtkPKdTree);
  virtual void SetKdtree(vtkPKdTree *kdtree);
  //@}

  /**
   * Is the pass supported by the OpenGL context?
   */
  bool IsSupported(vtkOpenGLRenderWindow *context);

 protected:
  /**
   * Default constructor. Controller is set to NULL.
   */
  vtkCompositeRGBAPass();

  /**
   * Destructor.
   */
  virtual ~vtkCompositeRGBAPass();

  vtkMultiProcessController *Controller;
  vtkPKdTree *Kdtree;

  vtkPixelBufferObject *PBO;
  vtkTextureObject *RGBATexture;
  vtkTextureObject *RootTexture;
  float *RawRGBABuffer;
  size_t RawRGBABufferSize;

 private:
  vtkCompositeRGBAPass(const vtkCompositeRGBAPass&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCompositeRGBAPass&) VTK_DELETE_FUNCTION;
};

#endif
