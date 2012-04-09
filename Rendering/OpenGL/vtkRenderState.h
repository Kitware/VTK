/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderState.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkRenderState - Context in which a vtkRenderPass will render.
// .SECTION Description
// vtkRenderState is a ligthweight effective class which gather information
// used by a vtkRenderPass to perform its execution.
// .SECTION Implementation Notes
// Get methods are const to enforce that a renderpass cannot modify the
// RenderPass object. It works in conjunction with vtkRenderPass::Render where
// the argument vtkRenderState is const.
// .SECTION See Also
// vtkRenderPass vtkRenderer vtkFrameBufferObject vtkProp

#ifndef __vtkRenderState_h
#define __vtkRenderState_h

#include "vtkObject.h"

class vtkRenderer;
class vtkProp;
class vtkFrameBufferObject;
class vtkInformation;

class VTK_RENDERING_EXPORT vtkRenderState
{
 public:
  // Description:
  // Constructor. All values are initialized to 0 or NULL.
  // \pre renderer_exists: renderer!=0
  // \post renderer_is_set: GetRenderer()==renderer.
  // \post valid_state: IsValid()
  vtkRenderState(vtkRenderer *renderer);

  // Description:
  // Destructor. As a vtkRenderState does not own any of its variables,
  // the destructor does nothing.
  ~vtkRenderState();

  // Description:
  // Tells if the RenderState is a valid one (Renderer is not null).
  bool IsValid() const;

  // Description:
  // Return the Renderer. This is the renderer in which the render pass is
  // performed. It gives access to the RenderWindow, to the props.
  // \post result_exists: result!=0
  vtkRenderer *GetRenderer() const;

  // Description:
  // Return the FrameBuffer. This is the framebuffer in use. NULL means it is
  // the FrameBuffer provided by the RenderWindow (it can actually be an FBO
  // in case the RenderWindow is in off screen mode).
  vtkFrameBufferObject *GetFrameBuffer() const;

  // Description:
  // Set the FrameBuffer. See GetFrameBuffer().
  // \post is_set: GetFrameBuffer()==fbo
  void SetFrameBuffer(vtkFrameBufferObject *fbo);

  // Description:
  // Get the window size of the state.
  void GetWindowSize(int size[2]) const;
  
  // Description:
  // Return the array of filtered props. See SetPropArrayAndCount().
  vtkProp **GetPropArray() const;

  // Description:
  // Return the size of the array of filtered props.
  // See SetPropArrayAndCount().
  // \post positive_result: result>=0
  int GetPropArrayCount() const;

  // Description:
  // Set the array of of filtered props and its size.
  // It is a subset of props to render. A renderpass might ignore this
  // filtered list and access to all the props of the vtkRenderer object
  // directly. For example, a render pass may filter props that are visible and
  // not culled by the frustum, but a sub render pass building a shadow map may
  // need all the visible props.
  // \pre positive_size: propArrayCount>=0
  // \pre valid_null_array: propArray!=0 || propArrayCount==0
  // \post is_set: GetPropArray()==propArray && GetPropArrayCount()==propArrayCount
  void SetPropArrayAndCount(vtkProp **propArray,
                            int propArrayCount);

  // Description:
  // Return the required property keys for the props. It tells that the
  // current render pass it supposed to render only props that have all the
  // RequiredKeys in their property keys.
  vtkInformation *GetRequiredKeys() const;

  // Description:
  // Set the required property keys for the props. See GetRequiredKeys().
  // \post is_set: GetRequiredKeys()==keys
  void SetRequiredKeys(vtkInformation *keys);

 protected:
  // Description:
  // The renderer in which the render pass is performed.
  // It gives access to the RenderWindow, to the props.
  vtkRenderer *Renderer;

  // Description:
  // The framebuffer in use. NULL means the FrameBuffer provided by
  // the RenderWindow (it can actually be an FBO in case the RenderWindow
  // is in off screen mode).
  vtkFrameBufferObject *FrameBuffer;

  // Description:
  // Subset of props to render. A renderpass might ignore this filtered list
  // and access to all the props of the vtkRenderer object directly.
  // For example, a render pass may filter props that are visible and
  // not culled by the frustum, but a sub render pass building a shadow map may
  // need all the visible props.
  vtkProp **PropArray;
  int PropArrayCount;

  // Description:
  // It tells that the current render pass it supposed to render only props
  // that have all the RequiredKeys in their property keys.
  vtkInformation *RequiredKeys;

private:
  vtkRenderState(); // no default constructor.
  vtkRenderState(const vtkRenderState &);  // Not implemented.
  void operator=(const vtkRenderState &);  // Not implemented.
};

#endif
