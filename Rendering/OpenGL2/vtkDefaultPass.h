/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDefaultPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkDefaultPass
 * @brief   Implement the basic render passes.
 *
 * vtkDefaultPass implements the basic standard render passes of VTK.
 * Subclasses can easily be implemented by reusing some parts of the basic
 * implementation.
 *
 * It implements classic Render operations as well as versions with property
 * key checking.
 *
 * This pass expects an initialized depth buffer and color buffer.
 * Initialized buffers means they have been cleared with farest z-value and
 * background color/gradient/transparent color.
 *
 * @sa
 * vtkRenderPass
*/

#ifndef vtkDefaultPass_h
#define vtkDefaultPass_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkRenderPass.h"

class vtkOpenGLRenderWindow;
class vtkDefaultPassLayerList; // Pimpl

class VTKRENDERINGOPENGL2_EXPORT vtkDefaultPass : public vtkRenderPass
{
public:
  static vtkDefaultPass *New();
  vtkTypeMacro(vtkDefaultPass,vtkRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Perform rendering according to a render state \p s.
   * Call RenderOpaqueGeometry(), RenderTranslucentPolygonalGeometry(),
   * RenderVolumetricGeometry(), RenderOverlay()
   * \pre s_exists: s!=0
   */
  void Render(const vtkRenderState *s) VTK_OVERRIDE;

 protected:
  /**
   * Default constructor.
   */
  vtkDefaultPass();

  /**
   * Destructor.
   */
  ~vtkDefaultPass() VTK_OVERRIDE;

  /**
   * Opaque pass without key checking.
   * \pre s_exists: s!=0
   */
  virtual void RenderOpaqueGeometry(const vtkRenderState *s);

  /**
   * Opaque pass with key checking.
   * \pre s_exists: s!=0
   */
  virtual void RenderFilteredOpaqueGeometry(const vtkRenderState *s);

  /**
   * Translucent pass without key checking.
   * \pre s_exists: s!=0
   */
  virtual void RenderTranslucentPolygonalGeometry(const vtkRenderState *s);

  /**
   * Translucent pass with key checking.
   * \pre s_exists: s!=0
   */
  virtual void RenderFilteredTranslucentPolygonalGeometry(
    const vtkRenderState *s);

  /**
   * Volume pass without key checking.
   * \pre s_exists: s!=0
   */
  virtual void RenderVolumetricGeometry(const vtkRenderState *s);

  /**
   * Translucent pass with key checking.
   * \pre s_exists: s!=0
   */
  virtual void RenderFilteredVolumetricGeometry(const vtkRenderState *s);

  /**
   * Overlay pass without key checking.
   * \pre s_exists: s!=0
   */
  virtual void RenderOverlay(const vtkRenderState *s);

  /**
   * Overlay pass with key checking.
   * \pre s_exists: s!=0
   */
  virtual void RenderFilteredOverlay(const vtkRenderState *s);

 private:
  vtkDefaultPass(const vtkDefaultPass&) VTK_DELETE_FUNCTION;
  void operator=(const vtkDefaultPass&) VTK_DELETE_FUNCTION;
};

#endif
