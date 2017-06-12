/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFramebufferDepthPeelingPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkFramebufferDepthPeelingPass
 * @brief   Implement Depth Peeling for use within a frambuffer pass
 *
 *
 * This implementation makes use of textures and is suitable for ES3
 * It must be embedded within a pass that makes use of framebuffers
 * so that the required OpaqueZTexture and OpaqueRGBATexture can be
 * passed from the outer frambuffer pass. For OpenGL ES3 be aware the
 * occlusion ratio test is not supported. The maximum number of peels
 * is used instead so set it to a reasonable value. For many scenes
 * a value of 4 or 5 will work well.
 *
 * @sa
 * vtkRenderPass, vtkTranslucentPass, vtkFramebufferPass
*/

#ifndef vtkFramebufferDepthPeelingPass_h
#define vtkFramebufferDepthPeelingPass_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkOpenGLRenderPass.h"
#include <vector>  // STL Header

class vtkOpenGLFramebufferObject;
class vtkTextureObject;
class vtkOpenGLRenderWindow;
class vtkOpenGLHelper;

class VTKRENDERINGOPENGL2_EXPORT vtkFramebufferDepthPeelingPass
    : public vtkOpenGLRenderPass
{
public:
  static vtkFramebufferDepthPeelingPass *New();
  vtkTypeMacro(vtkFramebufferDepthPeelingPass,vtkOpenGLRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Perform rendering according to a render state \p s.
   * \pre s_exists: s!=0
   */
  void Render(const vtkRenderState *s) VTK_OVERRIDE;

  /**
   * Release graphics resources and ask components to release their own
   * resources.
   * \pre w_exists: w!=0
   */
  void ReleaseGraphicsResources(vtkWindow *w) VTK_OVERRIDE;

  //@{
  /**
   * Delegate for rendering the translucent polygonal geometry.
   * If it is NULL, nothing will be rendered and a warning will be emitted.
   * It is usually set to a vtkTranslucentPass.
   * Initial value is a NULL pointer.
   */
  vtkGetObjectMacro(TranslucentPass,vtkRenderPass);
  virtual void SetTranslucentPass(vtkRenderPass *translucentPass);
  //@}

  //@{
  /**
   * In case of use of depth peeling technique for rendering translucent
   * material, define the threshold under which the algorithm stops to
   * iterate over peel layers. This is the ratio of the number of pixels
   * that have been touched by the last layer over the total number of pixels
   * of the viewport area.
   * Initial value is 0.0, meaning rendering have to be exact. Greater values
   * may speed-up the rendering with small impact on the quality.
   */
  vtkSetClampMacro(OcclusionRatio,double,0.0,0.5);
  vtkGetMacro(OcclusionRatio,double);
  //@}

  //@{
  /**
   * In case of depth peeling, define the maximum number of peeling layers.
   * Initial value is 4. A special value of 0 means no maximum limit.
   * It has to be a positive value.
   */
  vtkSetMacro(MaximumNumberOfPeels,int);
  vtkGetMacro(MaximumNumberOfPeels,int);
  //@}

  /**
   * Is rendering at translucent geometry stage using depth peeling and
   * rendering a layer other than the first one? (Boolean value)
   * If so, the uniform variables UseTexture and Texture can be set.
   * (Used by vtkOpenGLProperty or vtkOpenGLTexture)
   * int GetDepthPeelingHigherLayer();
   */

  // vtkOpenGLRenderPass virtuals:
  bool PostReplaceShaderValues(std::string &vertexShader,
                                   std::string &geometryShader,
                                   std::string &fragmentShader,
                                   vtkAbstractMapper *mapper,
                                   vtkProp *prop) VTK_OVERRIDE;
  bool SetShaderParameters(vtkShaderProgram *program,
                           vtkAbstractMapper *mapper, vtkProp *prop,
                           vtkOpenGLVertexArrayObject* VAO = NULL) VTK_OVERRIDE;

  // Set Opaque Z texture, this must be set from the outer FO
  void SetOpaqueZTexture(vtkTextureObject *);

  // Set Opaque RGBA texture, this must be set from the outer FO
  void SetOpaqueRGBATexture(vtkTextureObject *);

 protected:
  /**
   * Default constructor. TranslucentPass is set to NULL.
   */
  vtkFramebufferDepthPeelingPass();

  /**
   * Destructor.
   */
  ~vtkFramebufferDepthPeelingPass() VTK_OVERRIDE;

  vtkRenderPass *TranslucentPass;
  vtkTimeStamp CheckTime;

  //@{
  /**
   * Cache viewport values for depth peeling.
   */
  int ViewportX;
  int ViewportY;
  int ViewportWidth;
  int ViewportHeight;
  //@}

  /**
   * In case of use of depth peeling technique for rendering translucent
   * material, define the threshold under which the algorithm stops to
   * iterate over peel layers. This is the ratio of the number of pixels
   * that have been touched by the last layer over the total number of pixels
   * of the viewport area.
   * Initial value is 0.0, meaning rendering have to be exact. Greater values
   * may speed-up the rendering with small impact on the quality.
   */
  double OcclusionRatio;

  /**
   * In case of depth peeling, define the maximum number of peeling layers.
   * Initial value is 4. A special value of 0 means no maximum limit.
   * It has to be a positive value.
   */
  int MaximumNumberOfPeels;

  // Is rendering at translucent geometry stage using depth peeling and
  // rendering a layer other than the first one? (Boolean value)
  // If so, the uniform variables UseTexture and Texture can be set.
  // (Used by vtkOpenGLProperty or vtkOpenGLTexture)
  int DepthPeelingHigherLayer;

  vtkOpenGLFramebufferObject *Framebuffer;

  vtkOpenGLHelper *FinalBlendProgram;
  vtkOpenGLHelper *IntermediateBlendProgram;

  // obtained from the outer FO, we read from them
  vtkTextureObject *OpaqueZTexture;
  vtkTextureObject *OpaqueRGBATexture;

  // each peel merges two color buffers into one result
  vtkTextureObject *TranslucentRGBATexture[3];
  int ColorDrawCount;
  int PeelCount;

  // each peel compares a prior Z and writes to next
  vtkTextureObject *TranslucentZTexture[2];

  void BlendIntermediatePeels(vtkOpenGLRenderWindow *renWin, bool);
  void BlendFinalPeel(vtkOpenGLRenderWindow *renWin);

 private:
  vtkFramebufferDepthPeelingPass(const vtkFramebufferDepthPeelingPass&) VTK_DELETE_FUNCTION;
  void operator=(const vtkFramebufferDepthPeelingPass&) VTK_DELETE_FUNCTION;
};

#endif
