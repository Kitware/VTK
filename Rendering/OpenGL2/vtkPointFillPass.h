/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointFillPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPointFillPass
 * @brief   Implement a post-processing fillpass
 *
 *
 * This pass is designed to fill in rendering of sparse point sets/coulds
 * The delegate is used once and is usually set to a vtkCameraPass or
 * to a post-processing pass.
 *
 * @sa
 * vtkRenderPass
*/

#ifndef vtkPointFillPass_h
#define vtkPointFillPass_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkDepthImageProcessingPass.h"

class vtkDepthPeelingPassLayerList; // Pimpl
class vtkFrameBufferObject;
class vtkOpenGLHelper;
class vtkOpenGLRenderWindow;
class vtkTextureObject;

class VTKRENDERINGOPENGL2_EXPORT vtkPointFillPass : public vtkDepthImageProcessingPass
{
public:
  static vtkPointFillPass *New();
  vtkTypeMacro(vtkPointFillPass,vtkDepthImageProcessingPass);
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
   * How far in front of a point must a neighboring point
   * be to be used as a filler candidate.  Expressed as
   * a multiple of the points distance from the camera.
   * Defaults to 0.95
   */
  vtkSetMacro(CandidatePointRatio,float);
  vtkGetMacro(CandidatePointRatio,float);
  //@}

  //@{
  /**
   * How large of an angle must the filler candidates
   * span before a point will be filled. Expressed in
   * radians. A value of pi will keep edges from growing out.
   * Large values require more support, lower values less.
   */
  vtkSetMacro(MinimumCandidateAngle,float);
  vtkGetMacro(MinimumCandidateAngle,float);
  //@}

 protected:
  /**
   * Default constructor. DelegatePass is set to NULL.
   */
  vtkPointFillPass();

  /**
   * Destructor.
   */
  virtual ~vtkPointFillPass();

  /**
   * Graphics resources.
   */
  vtkFrameBufferObject *FrameBufferObject;
  vtkTextureObject *Pass1; // render target for the scene
  vtkTextureObject *Pass1Depth; // render target for the depth

  // Structures for the various cell types we render.
  vtkOpenGLHelper *BlurProgram;

  bool Supported;
  bool SupportProbed;

  float CandidatePointRatio;
  float MinimumCandidateAngle;

 private:
  vtkPointFillPass(const vtkPointFillPass&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPointFillPass&) VTK_DELETE_FUNCTION;
};

#endif
