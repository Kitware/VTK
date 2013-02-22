/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderState.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRenderState.h"
#include <assert.h>
#include "vtkRenderer.h"
#include "vtkFrameBufferObject.h"

// ----------------------------------------------------------------------------
// Description:
// Constructor. All values are initialized to 0 or NULL.
// \pre renderer_exists: renderer!=0
// \post renderer_is_set: GetRenderer()==renderer.
// \post valid_state: IsValid()
vtkRenderState::vtkRenderState(vtkRenderer *renderer)
{
  assert("pre: renderer_exists" && renderer!=0);
  this->Renderer = renderer;
  this->FrameBuffer = 0;
  this->PropArray = 0;
  this->PropArrayCount = 0;
  this->RequiredKeys = 0;

  assert("post: renderer_is_set" && this->GetRenderer() == renderer);
  assert("post: is_valid" && this->IsValid());
}

// ----------------------------------------------------------------------------
// Description:
// Destructor. As a vtkRenderState does not own any of its variables,
// the destructor does nothing.
vtkRenderState::~vtkRenderState()
{
}

// ----------------------------------------------------------------------------
// Description:
// Tells if the RenderState is a valid one (Renderer is not null).
bool vtkRenderState::IsValid() const
{
  return this->Renderer != 0;
}

// ----------------------------------------------------------------------------
// Description:
// Return the Renderer.
// \post result_exists: result!=0
vtkRenderer *vtkRenderState::GetRenderer() const
{
  assert("post: valid_result" && this->Renderer != 0);
  return this->Renderer;
}

// ----------------------------------------------------------------------------
// Description:
// Return the FrameBuffer.
vtkFrameBufferObject *vtkRenderState::GetFrameBuffer() const
{
  return this->FrameBuffer;
}

// ----------------------------------------------------------------------------
// Description:
// Set the FrameBuffer.
// \post is_set: GetFrameBuffer()==fbo
void vtkRenderState::SetFrameBuffer(vtkFrameBufferObject *fbo)
{
  this->FrameBuffer = fbo;
  assert("post: is_set" && this->GetFrameBuffer() == fbo);
}

// ----------------------------------------------------------------------------
// Description:
// Get the window size of the state.
void vtkRenderState::GetWindowSize(int size[2]) const
{
  if (this->FrameBuffer==0)
    {
    this->Renderer->GetTiledSize(&size[0], &size[1]);
    }
  else
    {
    this->FrameBuffer->GetLastSize(size);
    }
}

// ----------------------------------------------------------------------------
// Description:
// Return the array of filtered props
vtkProp **vtkRenderState::GetPropArray() const
{
  return this->PropArray;
}

// ----------------------------------------------------------------------------
// Description:
// Return the size of the array of filtered props.
// \post positive_result: result>=0
int vtkRenderState::GetPropArrayCount() const
{
  assert("post: positive_result"  && this->PropArrayCount >= 0);
  return this->PropArrayCount;
}

// ----------------------------------------------------------------------------
// Description:
// Set the array of of filtered props and its size.
// \pre positive_size: propArrayCount>=0
// \pre valid_null_array: propArray!=0 || propArrayCount==0
// \post is_set: GetPropArray()==propArray && GetPropArrayCount()==propArrayCount
void vtkRenderState::SetPropArrayAndCount(vtkProp **propArray,
                                          int propArrayCount)
{
  assert("pre: positive_size" && propArrayCount >= 0);
  assert("pre: valid_null_array" && (propArray != 0 || propArrayCount == 0));

  this->PropArray = propArray;
  this->PropArrayCount = propArrayCount;

  assert("post: is_set" && this->GetPropArray() == propArray
         && this->GetPropArrayCount() == propArrayCount);
}

// ----------------------------------------------------------------------------
// Description:
// Return the required property keys for the props.
vtkInformation *vtkRenderState::GetRequiredKeys() const
{
  return this->RequiredKeys;
}

// ----------------------------------------------------------------------------
// Description:
// Set the required property keys for the props.
// \post is_set: GetRequiredKeys()==keys
void vtkRenderState::SetRequiredKeys(vtkInformation *keys)
{
  this->RequiredKeys = keys;
  assert("post: is_set" && this->GetRequiredKeys() == keys);
}
