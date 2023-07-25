// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkRenderState
 * @brief   Context in which a vtkRenderPass will render.
 *
 * vtkRenderState is a lightweight effective class which gather information
 * used by a vtkRenderPass to perform its execution.
 * @attention
 * Get methods are const to enforce that a renderpass cannot modify the
 * RenderPass object. It works in conjunction with vtkRenderPass::Render where
 * the argument vtkRenderState is const.
 * @sa
 * vtkRenderPass vtkRenderer vtkFrameBufferObject vtkProp
 */

#ifndef vtkRenderState_h
#define vtkRenderState_h

#include "vtkObject.h"
#include "vtkRenderingCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkRenderer;
class vtkProp;
class vtkFrameBufferObjectBase;
class vtkInformation;

class VTKRENDERINGCORE_EXPORT vtkRenderState
{
public:
  /**
   * Constructor. All values are initialized to 0 or NULL.
   * \pre renderer_exists: renderer!=0
   * \post renderer_is_set: GetRenderer()==renderer.
   * \post valid_state: IsValid()
   */
  vtkRenderState(vtkRenderer* renderer);

  /**
   * Destructor. As a vtkRenderState does not own any of its variables,
   * the destructor does nothing.
   */
  ~vtkRenderState() = default;

  /**
   * Tells if the RenderState is a valid one (Renderer is not null).
   */
  bool IsValid() const;

  /**
   * Return the Renderer. This is the renderer in which the render pass is
   * performed. It gives access to the RenderWindow, to the props.
   * \post result_exists: result!=0
   */
  vtkRenderer* GetRenderer() const;

  /**
   * Return the FrameBuffer. This is the framebuffer in use. NULL means it is
   * the FrameBuffer provided by the RenderWindow (it can actually be an FBO
   * in case the RenderWindow is in off screen mode).
   */
  vtkFrameBufferObjectBase* GetFrameBuffer() const;

  /**
   * Set the FrameBuffer. See GetFrameBuffer().
   * \post is_set: GetFrameBuffer()==fbo
   */
  void SetFrameBuffer(vtkFrameBufferObjectBase* fbo);

  /**
   * Get the window size of the state.
   */
  void GetWindowSize(int size[2]) const;

  /**
   * Return the array of filtered props. See SetPropArrayAndCount().
   */
  vtkProp** GetPropArray() const;

  /**
   * Return the size of the array of filtered props.
   * See SetPropArrayAndCount().
   * \post positive_result: result>=0
   */
  int GetPropArrayCount() const;

  /**
   * Set the array of filtered props and its size.
   * It is a subset of props to render. A renderpass might ignore this
   * filtered list and access to all the props of the vtkRenderer object
   * directly. For example, a render pass may filter props that are visible and
   * not culled by the frustum, but a sub render pass building a shadow map may
   * need all the visible props.
   * \pre positive_size: propArrayCount>=0
   * \pre valid_null_array: propArray!=0 || propArrayCount==0
   * \post is_set: GetPropArray()==propArray && GetPropArrayCount()==propArrayCount
   */
  void SetPropArrayAndCount(vtkProp** propArray, int propArrayCount);

  /**
   * Return the required property keys for the props. It tells that the
   * current render pass it supposed to render only props that have all the
   * RequiredKeys in their property keys.
   */
  vtkInformation* GetRequiredKeys() const;

  /**
   * Set the required property keys for the props. See GetRequiredKeys().
   * \post is_set: GetRequiredKeys()==keys
   */
  void SetRequiredKeys(vtkInformation* keys);

protected:
  /**
   * The renderer in which the render pass is performed.
   * It gives access to the RenderWindow, to the props.
   */
  vtkRenderer* Renderer;

  /**
   * The framebuffer in use. NULL means the FrameBuffer provided by
   * the RenderWindow (it can actually be an FBO in case the RenderWindow
   * is in off screen mode).
   */
  vtkFrameBufferObjectBase* FrameBuffer;

  ///@{
  /**
   * Subset of props to render. A renderpass might ignore this filtered list
   * and access to all the props of the vtkRenderer object directly.
   * For example, a render pass may filter props that are visible and
   * not culled by the frustum, but a sub render pass building a shadow map may
   * need all the visible props.
   */
  vtkProp** PropArray;
  int PropArrayCount;
  ///@}

  /**
   * It tells that the current render pass it supposed to render only props
   * that have all the RequiredKeys in their property keys.
   */
  vtkInformation* RequiredKeys;

private:
  vtkRenderState() = delete; // no default constructor.
  vtkRenderState(const vtkRenderState&) = delete;
  void operator=(const vtkRenderState&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkRenderState.h
