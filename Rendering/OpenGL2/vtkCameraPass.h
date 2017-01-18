/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCameraPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCameraPass
 * @brief   Implement the camera render pass.
 *
 * Render the camera.
 *
 * It setups the projection and modelview matrices and can clear the background
 * It calls its delegate once.
 * After its delegate returns, it restore the modelview matrix stack.
 *
 * Its delegate is usually set to a vtkSequencePass with a vtkLigthsPass and
 * a list of passes for the geometry.
 *
 * @sa
 * vtkRenderPass
*/

#ifndef vtkCameraPass_h
#define vtkCameraPass_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkRenderPass.h"

class VTKRENDERINGOPENGL2_EXPORT vtkCameraPass : public vtkRenderPass
{
public:
  static vtkCameraPass *New();
  vtkTypeMacro(vtkCameraPass,vtkRenderPass);
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
   * Delegate for rendering the geometry.
   * If it is NULL, nothing will be rendered and a warning will be emitted.
   * It is usually set to a vtkSequencePass with a vtkLigthsPass and
   * a list of passes for the geometry.
   * Initial value is a NULL pointer.
   */
  vtkGetObjectMacro(DelegatePass,vtkRenderPass);
  virtual void SetDelegatePass(vtkRenderPass *delegatePass);
  //@}

  //@{
  /**
   * Used to override the aspect ratio used when computing the projection
   * matrix. This is useful when rendering for tile-displays for example.
   */
  vtkSetMacro(AspectRatioOverride, double);
  vtkGetMacro(AspectRatioOverride, double);
 protected:
  //@}
  /**
   * Default constructor. DelegatePass is set to NULL.
   */
  vtkCameraPass();

  //@{
  /**
   * Destructor.
   */
  ~vtkCameraPass() VTK_OVERRIDE;
  virtual void GetTiledSizeAndOrigin(
    const vtkRenderState* render_state,
    int* width, int* height, int *originX,
    int* originY);
  //@}

  vtkRenderPass *DelegatePass;

  double AspectRatioOverride;
 private:
  vtkCameraPass(const vtkCameraPass&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCameraPass&) VTK_DELETE_FUNCTION;
};

#endif
