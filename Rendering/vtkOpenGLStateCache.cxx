/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLStateCache.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    to John Schalf who developed this class

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
//*************************************************************************
/*
  GlStateCache_Cache============================================
  This simply checks for redundancies in state-change requests and
  only calls the real OpenGL call if there has in fact been a change.
  This cannot, however, fix problems with the ordering of calls.
*/

#include "vtkWin32Header.h"
#include "vtkSystemIncludes.h"
#include "vtkOpenGLStateCache.h"

// only one of these defined, so lets define it here
vtkOpenGLStateCache *vtkOpenGLStateCache::CurrentGLCache = 0;

vtkOpenGLStateCache::vtkOpenGLStateCache()
{
  this->Initialize();
}

vtkOpenGLStateCache::~vtkOpenGLStateCache()
{
}

void vtkOpenGLStateCache::Initialize()
{ // when the context gets destroyed
  int i;
  for(i=0;i<8;i++)
    {
    Enable_GL_LIGHT_buckets[i]=-1;
    Enable_GL_CLIP_PLANE_buckets[i]=-1;
    }

  for(i=0;i<(0xDE1-0xB10);i++) 
    {
    Enable_buckets[i]=-1;
    }

  AlphaFunc_bucket=-1.0;
  BlendFunc_bucket=(GLenum)0;
  DepthFunc_bucket=(GLenum)0;
  TexEnvf_MODE_bucket=-1.0; 
  LightModeli_LIGHT_MODEL_TWO_SIDE_bucket=-1;
  LightModelfv_LIGHT_MODEL_AMBIENT_bucket[0]=-1.0f;

  for(i=0;i<8*4*8;i++) 
    {
    Lightfv_buckets[i]=-1.0f;
    }
  
  for(i=0;i<8*8;i++) 
    {
    Lightf_buckets[i]=-1.0f;
    }
  
  for(i=0;i<8;i++) 
    {
    Lighti_SPOT_CUTOFF_buckets[i]=-1;
    }
  
  for(i=0;i<8*8*4;i++) 
    {
    Materialfv_buckets[i]=-1.0f;
    }
  
  ShadeModel_bucket=(GLenum)0;
  for(i=0;i<4;i++) 
    {
    ClearColor_buckets[i]=-1.0f;
    }
  ClearDepth_bucket=-1.0;
  DepthMask_bucket=-1.0f;
  CullFace_bucket=(GLenum)0;
  DrawBuffer_bucket=(GLenum)0;
  MatrixMode_bucket=(GLenum)0;

  for(i=0;i<4;i++) 
    {
    Viewport_bucket[i]=-1;
    Scissor_bucket[i]=-1;
    }
  for(i=0;i<4*GL_MAX_CLIP_PLANES;i++) 
    {
    ClipPlane_bucket[i]=-1.0;
    }
  
  for(i=0;i<8;i++) 
    {
    ColorMaterial_bucket[i]=(GLenum)0;
    }
  
  PointSize_bucket=-1.0;
  LineWidth_bucket=-1.0;
  LineStipple_FACTOR_bucket=-1;
  LineStipple_PATTERN_bucket=0;
  DepthRange_NEAR_bucket=-1.0;
  DepthRange_FAR_bucket=-1.0;
  compile_and_exec=0;
  listnum=0;
}

